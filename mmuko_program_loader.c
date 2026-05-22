// ============================================================================
// MMUKO_PROGRAM_LOADER.C — Time-Payload Program Scheduler
// Project:    github.com/obinexus/mmuko-os
// Author:     Nnamdi Michael Okpala
// Date:       22 May 2026
// Version:    0.3-time-payload
// ============================================================================
//
// Implements the Laplace Transform Drift Theorem for scheduling external
// programs (like hello.exe) as time-payloads in the MMUKO framework.
//
// Core Principle:
//   L{f(t)} = F(s) = integral of f(t)*e^(-st) dt
//   In MMUKO: time_payload(tick) = burst_time * exp(-s * tick)
//   where s = strategic_value / 1000.0 (the "space operator")
//
// This means: as a process ages in the scheduler, its remaining time
// "decays" exponentially. High-priority processes decay slower.
//
// ============================================================================

#include "mmuko.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// ─────────────────────────────────────────────────────────────────────────────
// DISCRETE LAPLACE TRANSFORM: time-domain -> space-domain
// ─────────────────────────────────────────────────────────────────────────────

// L{f(t)} = F(s) = f(t) * e^(-s*t)
// In discrete form: remaining_time(tick) = burst_time * exp(-s * tick)
// where s = strategic_value / 1000.0 (normalized priority as frequency)

double mmuko_laplace_decay(mmuko_pcb_t* proc, uint64_t tick) {
    if (!proc || proc->burst_time == 0) return 0.0;

    // s = space operator = strategic priority normalized
    double s = (double)proc->strategic_value / 1000.0;
    if (s < 0.001) s = 0.001; // minimum decay rate

    // e^(-s*t) — exponential decay of remaining time
    double decay = exp(-s * (double)tick);

    // Remaining time = burst_time * decay
    double remaining = (double)proc->burst_time * decay;

    return remaining;
}

// Inverse: given remaining time, compute which tick we're at
// F(s) -> f(t): tick = -ln(remaining/burst) / s
uint64_t mmuko_inverse_laplace_tick(mmuko_pcb_t* proc, double remaining) {
    if (!proc || proc->burst_time == 0 || remaining <= 0) return 0;

    double s = (double)proc->strategic_value / 1000.0;
    if (s < 0.001) s = 0.001;

    double ratio = remaining / (double)proc->burst_time;
    if (ratio >= 1.0) return 0;
    if (ratio <= 0.0) return proc->burst_time;

    double tick = -log(ratio) / s;
    return (uint64_t)tick;
}

// ─────────────────────────────────────────────────────────────────────────────
// TIME PAYLOAD STRUCTURE
// ─────────────────────────────────────────────────────────────────────────────

typedef struct {
    char        program_path[256];  // Path to executable (e.g., "./hello.exe")
    char**      argv;               // Arguments
    uint64_t    time_interval;      // ΔT: how long to allocate (ticks)
    double      drift_factor;       // Lorenz factor γ = 1/sqrt(1 - v^2/c^2)
    mmuko_rights_t rights;          // Rights dimensions for this payload
    uint64_t    memory_need;        // token_memory for this program
} mmuko_time_payload_t;

// Lorenz factor for process velocity
// v = process speed = priority / max_priority
// c = speed of light = maximum possible priority
double mmuko_lorenz_factor(mmuko_pcb_t* proc, uint64_t max_priority) {
    if (!proc || max_priority == 0) return 1.0;

    double v = (double)proc->strategic_value / (double)max_priority;
    if (v >= 1.0) v = 0.999; // cap below lightspeed

    double gamma = 1.0 / sqrt(1.0 - v * v);
    return gamma;
}

// ─────────────────────────────────────────────────────────────────────────────
// PROGRAM LOADER: fork/exec external program as MMUKO process
// ─────────────────────────────────────────────────────────────────────────────

// Load an external program (like hello.exe) into the MMUKO scheduler
// as a time-payload with rights-based priority
mmuko_pcb_t* mmuko_load_program(MMUKO_System* sys, const char* path,
                                 mmuko_rights_t rights, uint64_t burst_ms,
                                 uint64_t memory_need) {
    if (!sys || !path) return NULL;

    printf("\n[LOADER] Loading program: %s\n", path);
    printf("[LOADER] Rights: 0x%02X, Burst: %lums, Memory: %lu bytes\n",
           rights, burst_ms, memory_need);

    // Create PCB for this program
    mmuko_pcb_t* proc = mmuko_proc_create(0, rights, burst_ms, memory_need);
    if (!proc) {
        printf("[LOADER] FAILED: cannot create PCB\n");
        return NULL;
    }

    // Store program path in token (using token_value as path hash)
    strncpy((char*)&proc->token.token_value, path, 8); // store first 8 chars

    // Compute Laplace decay profile
    double s = (double)proc->strategic_value / 1000.0;
    printf("[LOADER] Laplace operator s=%.4f, initial decay at tick 1: %.4f\n",
           s, exp(-s));

    // Allocate to ready tower (Stack domain)
    int r = mmuko_proc_allocate(proc, sys->towers[MMUKO_SEG_STACK]);
    if (r < 0) {
        printf("[LOADER] FAILED: cannot allocate to ready tower\n");
        mmuko_proc_destroy(proc);
        return NULL;
    }

    // Register in system process table
    if (sys->proc_count < sys->proc_capacity) {
        sys->proc_table[sys->proc_count++] = *proc;
    }

    printf("[LOADER] Program loaded as pid=%u (priority=%lu)\n",
           proc->pid, proc->strategic_value);
    return proc;
}

// Execute the program via fork/exec
// Returns child PID on success, -1 on failure
pid_t mmuko_exec_program(mmuko_pcb_t* proc, const char* path) {
    if (!proc || !path) return -1;

    printf("[EXEC] Forking pid=%u -> %s\n", proc->pid, path);

    pid_t child = fork();
    if (child < 0) {
        perror("[EXEC] fork failed");
        return -1;
    }

    if (child == 0) {
        // Child process: execute the program
        printf("[EXEC] Child executing: %s\n", path);
        execl(path, path, (char*)NULL);
        // If we get here, exec failed
        perror("[EXEC] execl failed");
        exit(1);
    }

    // Parent: update PCB with child PID
    printf("[EXEC] Child pid=%d started for MMUKO pid=%u\n", child, proc->pid);
    proc->state = MMUKO_PROC_RUNNING;
    return child;
}

// Wait for program completion with timeout (time-payload enforcement)
int mmuko_wait_program(mmuko_pcb_t* proc, pid_t child, uint64_t timeout_ms) {
    if (!proc || child < 0) return -1;

    printf("[WAIT] Waiting for pid=%d (timeout=%lums)...\n", child, timeout_ms);

    int status;
    pid_t result;
    uint64_t start = mmuko_now_ms();

    // Non-blocking wait with timeout
    do {
        result = waitpid(child, &status, WNOHANG);
        if (result == child) {
            // Child exited
            uint64_t elapsed = mmuko_now_ms() - start;
            proc->completion_time = mmuko_now_ms();
            proc->turnaround_time = elapsed;

            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                printf("[WAIT] Child exited with code %d after %lums\n",
                       exit_code, elapsed);
                proc->state = (exit_code == 0) ? MMUKO_PROC_TERMINATED : MMUKO_PROC_ZOMBIE;
                return exit_code;
            } else if (WIFSIGNALED(status)) {
                printf("[WAIT] Child killed by signal %d\n", WTERMSIG(status));
                proc->state = MMUKO_PROC_ZOMBIE;
                return -1;
            }
        } else if (result == 0) {
            // Still running
            usleep(100000); // 100ms
        } else {
            perror("[WAIT] waitpid error");
            return -1;
        }
    } while ((mmuko_now_ms() - start) < timeout_ms);

    // Timeout: kill the child
    printf("[WAIT] TIMEOUT after %lums — killing child pid=%d\n", timeout_ms, child);
    kill(child, SIGTERM);
    usleep(100000);
    kill(child, SIGKILL);
    waitpid(child, &status, 0);

    proc->state = MMUKO_PROC_ZOMBIE;
    return -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// TIME-PAYLOAD SCHEDULER: schedule hello.exe as a time-payload
// ─────────────────────────────────────────────────────────────────────────────

// This is the main entry point for scheduling an external program
// It combines: Laplace transform + Lorenz factor + rights-based priority
int mmuko_schedule_time_payload(MMUKO_System* sys, const char* program_path,
                                 mmuko_rights_t rights, uint64_t burst_ms,
                                 uint64_t memory_need) {
    if (!sys || !program_path) return -1;

    printf("\n========================================\n");
    printf("  TIME-PAYLOAD SCHEDULER\n");
    printf("  Program: %s\n", program_path);
    printf("========================================\n");

    // Step 1: Load program as MMUKO process
    mmuko_pcb_t* proc = mmuko_load_program(sys, program_path, rights, burst_ms, memory_need);
    if (!proc) return -1;

    // Step 2: Compute Lorenz factor (relativistic time dilation)
    uint64_t max_pri = 1000; // assumed max priority for normalization
    double gamma = mmuko_lorenz_factor(proc, max_pri);
    printf("[PAYLOAD] Lorenz factor γ=%.4f (time dilation)\n", gamma);

    // Step 3: Adjust burst time by Lorenz factor
    // High-priority processes get MORE time (time dilation)
    uint64_t adjusted_burst = (uint64_t)((double)burst_ms * gamma);
    printf("[PAYLOAD] Adjusted burst: %lums -> %lums (γ=%.4f)\n",
           burst_ms, adjusted_burst, gamma);
    proc->burst_time = adjusted_burst;
    proc->remaining_time = adjusted_burst;

    // Step 4: Execute the program
    pid_t child = mmuko_exec_program(proc, program_path);
    if (child < 0) {
        printf("[PAYLOAD] Execution failed\n");
        return -1;
    }

    // Step 5: Wait with time-payload enforcement (timeout = adjusted burst)
    int result = mmuko_wait_program(proc, child, adjusted_burst);

    // Step 6: Compute Laplace decay profile post-execution
    printf("\n[PAYLOAD] Laplace decay profile for pid=%u:\n", proc->pid);
    for (uint64_t tick = 0; tick <= 5; tick++) {
        double remaining = mmuko_laplace_decay(proc, tick);
        printf("  tick=%lu: remaining=%.2f ms (decay=%.4f)\n",
               tick, remaining, remaining / (double)proc->burst_time);
    }

    printf("\n[PAYLOAD] Time-payload complete for pid=%u\n", proc->pid);
    printf("  Turnaround time: %lums\n", proc->turnaround_time);
    printf("  Final state: %d\n", proc->state);

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// DEMO: Schedule hello.exe as a time-payload
// ─────────────────────────────────────────────────────────────────────────────

#ifdef MMUKO_TIME_PAYLOAD_TEST

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    printf("MMUKO OS Time-Payload Scheduler\n");
    printf("OBINexus R&D — \"Don't just schedule programs. Schedule them in time.\"\n\n");

    // Create system
    MMUKO_System* sys = mmuko_system_create(64, 16);
    if (!sys) {
        fprintf(stderr, "Failed to create MMUKO system\n");
        return 1;
    }

    // Boot
    BootStatus status = mmuko_boot(sys);
    if (status != BOOT_OK) {
        printf("BOOT FAILED\n");
        mmuko_system_destroy(sys);
        return 1;
    }

    // Schedule hello.exe as time-payload with Civil + Human rights
    printf("\n--- Scheduling hello.exe ---\n");
    int result = mmuko_schedule_time_payload(
        sys,
        "/tmp/hello.exe",              // program path
        MMUKO_RIGHTS_CIVIL | MMUKO_RIGHTS_HUMAN,  // rights = D=3
        5000,                          // burst time: 5 seconds max
        MMUKO_KB(1)                    // memory: 1 KB
    );

    printf("\n--- Result: %d ---\n", result);

    // Schedule again with higher rights (all rights = D=15)
    printf("\n--- Scheduling hello.exe with FULL RIGHTS ---\n");
    result = mmuko_schedule_time_payload(
        sys,
        "/tmp/hello.exe",
        MMUKO_RIGHTS_ALL,              // rights = D=15 (highest priority)
        5000,
        MMUKO_KB(2)
    );

    printf("\n--- Result: %d ---\n", result);

    // Print system status
    mmuko_print_system_status(sys);

    // Cleanup
    mmuko_system_destroy(sys);

    printf("\nTime-payload scheduler test complete.\n");
    return 0;
}

#endif // MMUKO_TIME_PAYLOAD_TEST
