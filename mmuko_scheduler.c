// ============================================================================
// MMUKO_SCHEDULER.C — Polar Priority Heap Scheduler
// Project:    github.com/obinexus/mmuko-os
// Framework:  OBINexus Constitutional Computing / Dimensional Game Theory
// Author:     Nnamdi Michael Okpala
// Date:       22 May 2026
// Version:    0.2-polar-scheduler
// ============================================================================
//
// Implements the three-mode MMUKO scheduler:
//   1. PREEMPTIVE     — high-rights process interrupts running process
//   2. NON-PREEMPTIVE — process runs to completion or I/O block
//   3. INEMPTIVE      — memory-preemptive: staged allocation via buffer PID
//                        without full context switch (time-nonpreemptive)
//
// Core Algorithms:
//   - Polar Priority:     priority = D × need_factor
//                         where D = a + b + d + e (rights dimensions)
//   - Node Complexity:    N = a × b (product of active base dimensions)
//   - Heap Height:        H = p^n (exponential priority scaling)
//   - Bayesian Resolver:  P(A|B) for conflict resolution between competing
//                         processes claiming the same tower slot
//   - Dimensional Rebalance: detects strategic imbalance via minimax and
//                             restores Nash equilibrium across towers
//
// Tower of Hanoi Model:
//   - 3 Towers = Stack, Heap, Data memory domains
//   - Disks    = processes with token_memory size
//   - Rule     = larger process cannot be placed on smaller process
//                (memory hierarchy protection)
//   - Optimal  = 2^n - 1 moves for n processes
//
// ============================================================================

#include "mmuko.h"

// ─────────────────────────────────────────────────────────────────────────────
// TOWER LIFECYCLE (Hanoi Memory Domains)
// ─────────────────────────────────────────────────────────────────────────────

mmuko_tower_t* mmuko_tower_create(mmuko_segment_t seg, uint32_t capacity,
                                   uint64_t domain_size, const char* name) {
    mmuko_tower_t* tower = (mmuko_tower_t*)calloc(1, sizeof(mmuko_tower_t));
    if (!tower) return NULL;

    tower->disks = (mmuko_pcb_t**)calloc(capacity, sizeof(mmuko_pcb_t*));
    if (!tower->disks) {
        free(tower);
        return NULL;
    }

    tower->capacity = capacity;
    tower->domain_size = domain_size;
    tower->used = 0;
    tower->count = 0;
    tower->segment = seg;
    strncpy(tower->name, name, 15);
    tower->name[15] = '\0';

    printf("[TOWER] Created %s domain: cap=%u, size=%lu bytes\n",
           name, capacity, domain_size);
    return tower;
}

void mmuko_tower_destroy(mmuko_tower_t* tower) {
    if (!tower) return;
    // Note: we do NOT destroy the PCB objects themselves here;
    // they are owned by the system proc_table.
    if (tower->disks) free(tower->disks);
    free(tower);
}

// Hanoi constraint: larger disk (process) cannot be placed on smaller disk
// In MMUKO: this protects memory hierarchy — large allocations cannot
// overlay smaller ones in stack domains (FILO integrity)
int mmuko_can_stack(mmuko_tower_t* tower, mmuko_pcb_t* proc) {
    if (!tower || !proc) return 0;
    if (tower->count == 0) return 1; // empty tower always accepts
    if (tower->count >= tower->capacity) return 0; // full

    mmuko_pcb_t* top = tower->disks[tower->count - 1];
    if (!top) return 1;

    // Hanoi rule: proc must be <= top in memory size
    // (smaller or equal can sit on larger; larger cannot sit on smaller)
    return proc->token.token_memory <= top->token.token_memory;
}

// Migrate process from source tower to destination tower
// Returns: 1 on success, -1 on violation, 0 on error
int mmuko_migrate(mmuko_tower_t* src, mmuko_tower_t* dst, mmuko_pcb_t* proc) {
    if (!src || !dst || !proc) return 0;

    // Verify Hanoi constraint at destination
    if (!mmuko_can_stack(dst, proc)) {
        printf("[MIGRATE] VIOLATION: pid=%u (mem=%lu) cannot stack on %s top (mem=%lu)\n",
               proc->pid, proc->token.token_memory, dst->name,
               dst->count > 0 ? dst->disks[dst->count - 1]->token.token_memory : 0);
        return -1;
    }

    // Find and remove from source
    int found = -1;
    for (uint32_t i = 0; i < src->count; i++) {
        if (src->disks[i] == proc) { found = (int)i; break; }
    }

    if (found >= 0) {
        // Remove from source (shift down)
        for (uint32_t i = (uint32_t)found; i < src->count - 1; i++) {
            src->disks[i] = src->disks[i + 1];
        }
        src->disks[src->count - 1] = NULL;
        src->count--;
        if (proc->token.token_memory <= src->used)
            src->used -= proc->token.token_memory;
    }

    // Push to destination
    dst->disks[dst->count] = proc;
    dst->count++;
    dst->used += proc->token.token_memory;
    proc->token.token_type = dst->segment;

    printf("[MIGRATE] pid=%u %s -> %s (mem=%lu, used=%lu/%lu)\n",
           proc->pid, src->name, dst->name, proc->token.token_memory,
           dst->used, dst->domain_size);
    return 1;
}

// ─────────────────────────────────────────────────────────────────────────────
// PROCESS LIFECYCLE
// ─────────────────────────────────────────────────────────────────────────────

static uint32_t next_pid = 1000;

mmuko_pcb_t* mmuko_proc_create(uint32_t pid, mmuko_rights_t rights,
                                uint64_t burst_time, uint64_t memory_need) {
    mmuko_pcb_t* proc = (mmuko_pcb_t*)calloc(1, sizeof(mmuko_pcb_t));
    if (!proc) return NULL;

    proc->pid = (pid == 0) ? next_pid++ : pid;
    proc->rights = rights;
    proc->burst_time = burst_time;
    proc->remaining_time = burst_time;
    proc->arrival_time = mmuko_now_ms();
    proc->state = MMUKO_PROC_NEW;
    proc->token.token_type = TOKEN_CLASSICAL_FIXED;
    proc->token.token_value = 0;
    proc->token.token_memory = memory_need;
    proc->dimension_count = MMUKO_DIM_N(proc);
    proc->strategic_value = mmuko_polar_priority(proc);
    proc->bayesian_conf = 0.5; // uniform prior
    proc->calibrated = false;
    proc->next = NULL;
    proc->prev = NULL;

    printf("[PROC] Created pid=%u, rights=0x%02X, burst=%lums, mem=%lu, D=%lu, N=%u, priority=%lu\n",
           proc->pid, proc->rights, proc->burst_time, proc->token.token_memory,
           (uint64_t)MMUKO_DIM_D(proc), proc->dimension_count, proc->strategic_value);
    return proc;
}

void mmuko_proc_destroy(mmuko_pcb_t* proc) {
    free(proc);
}

int mmuko_proc_allocate(mmuko_pcb_t* proc, mmuko_tower_t* tower) {
    if (!proc || !tower) return -1;
    if (!mmuko_can_stack(tower, proc)) return -1;

    tower->disks[tower->count] = proc;
    tower->count++;
    tower->used += proc->token.token_memory;
    proc->token.token_type = tower->segment;
    proc->state = MMUKO_PROC_READY;

    printf("[ALLOC] pid=%u -> %s (mem=%lu, tower_used=%lu/%lu)\n",
           proc->pid, tower->name, proc->token.token_memory,
           tower->used, tower->domain_size);
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// POLAR PRIORITY HEAP (Core Algorithm)
// ─────────────────────────────────────────────────────────────────────────────

// Polar Priority Formula:
//   D = a + b + d + e  (dimensional rights sum)
//   need = token_memory / 1KB  (memory need in kilobytes)
//   priority = D * need  (strategic value = rights × need)
//
// From transcript: "G to the power of N equals rights"
// We implement as: strategic_value = D * need_factor
// where need_factor scales exponentially with memory pressure.

uint64_t mmuko_polar_priority(mmuko_pcb_t* p) {
    if (!p) return 0;

    uint64_t D = MMUKO_DIM_D(p);
    uint64_t need_kb = p->token.token_memory / MMUKO_KB(1);
    if (need_kb == 0) need_kb = 1;

    // Exponential scaling under memory pressure: H = p^n
    // Base p = D, exponent n = log2(need_kb) + 1
    uint64_t n = 1;
    uint64_t temp = need_kb;
    while (temp > 1) { temp >>= 1; n++; }

    uint64_t priority = D * n * need_kb;
    p->strategic_value = priority;
    return priority;
}

// Bayesian Priority: P(A|B) for conflict resolution
// P(A)   = prior probability this process needs this resource
// P(B|A) = likelihood tower can accommodate given history
// P(B)   = marginal probability any process fits
// Posterior = P(A|B) = likelihood × prior / marginal

double mmuko_bayesian_priority(mmuko_pcb_t* candidate, mmuko_tower_t* tower) {
    if (!candidate || !tower) return 0.0;

    // P(A): prior = memory_need / tower_size
    double prior = (double)candidate->token.token_memory / (double)tower->domain_size;
    if (prior > 1.0) prior = 1.0;

    // P(B|A): likelihood = available capacity ratio
    double available = (double)(tower->domain_size - tower->used);
    double likelihood = available / (double)tower->domain_size;
    if (likelihood < 0.0) likelihood = 0.0;

    // P(B): marginal = 0.5 (uniform prior)
    double marginal = 0.5;

    double posterior = (likelihood * prior) / marginal;
    if (posterior > 1.0) posterior = 1.0;

    candidate->bayesian_conf = posterior;

    // Combined score: polar_priority × bayesian_confidence
    double combined = (double)mmuko_polar_priority(candidate) * posterior;
    return combined;
}

int mmuko_compare_priority(mmuko_pcb_t* a, mmuko_pcb_t* b) {
    if (!a || !b) return 0;
    uint64_t pa = mmuko_polar_priority(a);
    uint64_t pb = mmuko_polar_priority(b);
    if (pa > pb) return 1;
    if (pa < pb) return -1;
    // Tie-breaker: higher rights dimension wins
    uint64_t da = MMUKO_DIM_D(a);
    uint64_t db = MMUKO_DIM_D(b);
    if (da > db) return 1;
    if (da < db) return -1;
    // Final tie-breaker: lower PID wins (FIFO)
    return (a->pid < b->pid) ? 1 : -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// GOVERNANCE CHANNELS (CH_0 / CH_1 / CH_2)
// ─────────────────────────────────────────────────────────────────────────────
// From RiftLang: CH_0 observes, CH_1 defers with retry, CH_2 forces collapse

mmuko_channel_t mmuko_governance_check(mmuko_pcb_t* proc, mmuko_tower_t* target) {
    if (!proc || !target) return CH_2_COLLAPSE;

    // CH_0: YES — immediate execution if tower has space and rights match
    uint64_t available = target->domain_size - target->used;
    if (available >= proc->token.token_memory && mmuko_can_stack(target, proc)) {
        return CH_0_OBSERVE;
    }

    // CH_1: MAYBE — conditional if tower is near capacity but not full
    double capacity_ratio = (double)target->used / (double)target->domain_size;
    if (capacity_ratio < 0.9 && proc->dimension_count > 0) {
        return CH_1_DEFER;
    }

    // CH_2: NO — forced collapse / abort (tower full or rights mismatch)
    return CH_2_COLLAPSE;
}

int mmuko_governance_enforce(mmuko_pcb_t* proc, mmuko_channel_t ch) {
    switch (ch) {
        case CH_0_OBSERVE:
            proc->state = MMUKO_PROC_RUNNING;
            printf("[GOV] CH_0 OBSERVE: pid=%u executes immediately\n", proc->pid);
            return 0;
        case CH_1_DEFER:
            proc->state = MMUKO_PROC_BLOCKED;
            printf("[GOV] CH_1 DEFER: pid=%u blocked for retry (60s sleep)\n", proc->pid);
            // In real kernel, this would set a timer. Here we log.
            return 1;
        case CH_2_COLLAPSE:
            proc->state = MMUKO_PROC_ZOMBIE;
            printf("[GOV] CH_2 COLLAPSE: pid=%u aborted (measurement forced)\n", proc->pid);
            return -1;
    }
    return -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// SCHEDULING MODES
// ─────────────────────────────────────────────────────────────────────────────

// PREEMPTIVE: high-priority process interrupts running process
// If incoming has higher polar priority, running process is pushed to ready
int mmuko_schedule_preemptive(mmuko_tower_t* ready, mmuko_tower_t* running,
                               mmuko_pcb_t* incoming) {
    if (!ready || !running || !incoming) return -1;

    printf("\n[SCHED] PREEMPTIVE: incoming pid=%u (priority=%lu)\n",
           incoming->pid, mmuko_polar_priority(incoming));

    // Check governance
    mmuko_channel_t ch = mmuko_governance_check(incoming, running);
    if (ch == CH_2_COLLAPSE) {
        mmuko_governance_enforce(incoming, ch);
        return -1;
    }

    // If running tower has a process, compare priorities
    if (running->count > 0) {
        mmuko_pcb_t* current = running->disks[running->count - 1];
        int cmp = mmuko_compare_priority(incoming, current);

        if (cmp > 0) {
            // Incoming has higher priority — PREEMPT
            printf("[SCHED] PREEMPT: pid=%u (pri=%lu) > pid=%u (pri=%lu)\n",
                   incoming->pid, mmuko_polar_priority(incoming),
                   current->pid, mmuko_polar_priority(current));

            // Migrate current -> ready (save context)
            int r = mmuko_migrate(running, ready, current);
            if (r < 0) {
                printf("[SCHED] Preempt failed: cannot push current to ready\n");
                return -1;
            }
            current->state = MMUKO_PROC_READY;

            // Migrate incoming -> running
            r = mmuko_migrate(ready, running, incoming);
            if (r < 0) {
                printf("[SCHED] Preempt failed: cannot load incoming\n");
                return -1;
            }
            incoming->state = MMUKO_PROC_RUNNING;
            mmuko_governance_enforce(incoming, CH_0_OBSERVE);
            return 1; // Preemption occurred
        }
    }

    // No preemption needed — just queue in ready
    int r = mmuko_migrate(NULL, ready, incoming);
    if (r < 0) {
        // If no source tower (NULL), just allocate directly
        r = mmuko_proc_allocate(incoming, ready);
    }
    incoming->state = MMUKO_PROC_READY;
    return 0;
}

// NON-PREEMPTIVE: process runs to completion or until I/O block
// Only schedules if running tower is empty
int mmuko_schedule_non_preemptive(mmuko_tower_t* ready, mmuko_tower_t* running,
                                   mmuko_pcb_t* incoming) {
    if (!ready || !running || !incoming) return -1;

    printf("\n[SCHED] NON-PREEMPTIVE: incoming pid=%u\n", incoming->pid);

    // Check governance
    mmuko_channel_t ch = mmuko_governance_check(incoming, running);
    if (ch == CH_2_COLLAPSE) {
        mmuko_governance_enforce(incoming, ch);
        return -1;
    }

    if (running->count == 0) {
        // CPU idle — schedule immediately
        int r = mmuko_migrate(ready, running, incoming);
        if (r < 0) {
            r = mmuko_proc_allocate(incoming, running);
        }
        incoming->state = MMUKO_PROC_RUNNING;
        mmuko_governance_enforce(incoming, CH_0_OBSERVE);
        printf("[SCHED] NON-PREEMPTIVE: pid=%u scheduled to running (CPU was idle)\n",
               incoming->pid);
        return 1;
    } else {
        // CPU busy — must wait in ready queue
        int r = mmuko_proc_allocate(incoming, ready);
        if (r < 0) {
            printf("[SCHED] NON-PREEMPTIVE: pid=%u cannot fit in ready queue\n",
                   incoming->pid);
            return -1;
        }
        incoming->state = MMUKO_PROC_READY;
        printf("[SCHED] NON-PREEMPTIVE: pid=%u queued in ready (CPU busy)\n",
               incoming->pid);
        return 0;
    }
}

// INEMPTIVE: memory-preemptive, time-nonpreemptive
// Process needs memory; staged allocation via buffer PID without
// interrupting running process's time slice.
// From transcript: "inemptive permits a high priority of tasks to be
// allocated to memory" and "buffer process that needs memory and may
// need overlap"
int mmuko_schedule_inemptive(mmuko_tower_t* ready, mmuko_tower_t* running,
                              mmuko_inemptive_buffer_t* buffer, mmuko_pcb_t* incoming) {
    if (!ready || !running || !buffer || !incoming) return -1;

    printf("\n[SCHED] INEMPTIVE: incoming pid=%u (mem=%lu)\n",
           incoming->pid, incoming->token.token_memory);

    // Check if running process has enough memory for direct allocation
    uint64_t available = running->domain_size - running->used;

    if (incoming->token.token_memory <= available && mmuko_can_stack(running, incoming)) {
        // Direct allocation: no need for buffer staging
        int r = mmuko_migrate(ready, running, incoming);
        if (r < 0) r = mmuko_proc_allocate(incoming, running);
        if (r == 0) {
            incoming->state = MMUKO_PROC_RUNNING;
            printf("[SCHED] INEMPTIVE: pid=%u directly allocated to running\n",
                   incoming->pid);
            return 1;
        }
    }

    // Inemptive path: use buffer for staged memory allocation
    // Create auxiliary buffer process
    buffer->target_proc = incoming;
    buffer->overlap_size = incoming->token.token_memory;
    buffer->buffer_pid = next_pid++;
    buffer->resolved = false;

    // Stage in heap tower (index 3 in standard layout)
    // Find staging tower (heap) — in this implementation we use ready as fallback
    mmuko_tower_t* heap_tower = ready; // fallback

    // Check if we can stage in heap
    if (mmuko_can_stack(heap_tower, incoming)) {
        int r = mmuko_proc_allocate(incoming, heap_tower);
        if (r == 0) {
            incoming->state = MMUKO_PROC_BLOCKED;
            buffer->staging_tower = heap_tower;
            buffer->resolved = false;
            printf("[SCHED] INEMPTIVE: pid=%u staged in %s (buffer_pid=%u, overlap=%lu)\n",
                   incoming->pid, heap_tower->name, buffer->buffer_pid, buffer->overlap_size);
            return 0; // Buffered, awaiting resolution
        }
    }

    // If staging fails, governance collapse
    mmuko_governance_enforce(incoming, CH_2_COLLAPSE);
    return -1;
}

// Resolve an inemptive buffer: called when running process releases memory
int mmuko_inemptive_resolve(mmuko_inemptive_buffer_t* buffer, mmuko_tower_t* running) {
    if (!buffer || !buffer->target_proc || !running) return -1;
    if (buffer->resolved) return 0;

    mmuko_pcb_t* proc = buffer->target_proc;
    uint64_t available = running->domain_size - running->used;

    if (available >= buffer->overlap_size && mmuko_can_stack(running, proc)) {
        // Migrate from staging tower to running
        int r = mmuko_migrate(buffer->staging_tower, running, proc);
        if (r == 1 || r == 0) {
            proc->state = MMUKO_PROC_RUNNING;
            buffer->resolved = true;
            printf("[INEMPTIVE] Resolved: pid=%u migrated to running (buffer_pid=%u retired)\n",
                   proc->pid, buffer->buffer_pid);
            return 1;
        }
    }

    printf("[INEMPTIVE] Cannot resolve yet: pid=%u still staged\n", proc->pid);
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// DIMENSIONAL GAME THEORY REBALANCER
// ─────────────────────────────────────────────────────────────────────────────
// From PDF: "Perfect games result in deterministic outcomes; deviations
// indicate strategic imbalances that can be algorithmically detected."

int mmuko_detect_strategic_imbalance(mmuko_tower_t* towers[], uint32_t num_towers) {
    if (!towers || num_towers == 0) return -1;

    // Compute mean priority across all towers
    uint64_t total_priority = 0;
    uint32_t total_procs = 0;
    for (uint32_t i = 0; i < num_towers; i++) {
        if (!towers[i]) continue;
        for (uint32_t j = 0; j < towers[i]->count; j++) {
            total_priority += mmuko_polar_priority(towers[i]->disks[j]);
            total_procs++;
        }
    }
    if (total_procs == 0) return 0; // no imbalance if empty

    double mean_priority = (double)total_priority / (double)total_procs;

    // Detect deviation: any tower whose average priority deviates > 30% from mean
    int imbalance_count = 0;
    for (uint32_t i = 0; i < num_towers; i++) {
        if (!towers[i] || towers[i]->count == 0) continue;
        uint64_t tower_sum = 0;
        for (uint32_t j = 0; j < towers[i]->count; j++) {
            tower_sum += mmuko_polar_priority(towers[i]->disks[j]);
        }
        double tower_mean = (double)tower_sum / (double)towers[i]->count;
        double deviation = fabs(tower_mean - mean_priority) / mean_priority;

        if (deviation > 0.30) {
            printf("[REBALANCE] Imbalance in %s: mean=%.1f, global_mean=%.1f, dev=%.2f%%\n",
                   towers[i]->name, tower_mean, mean_priority, deviation * 100);
            imbalance_count++;
        }
    }

    return imbalance_count;
}

// Rebalance: migrate processes between towers to restore Nash equilibrium
// Strategy: move highest-priority processes from overloaded towers to
// underloaded towers, respecting Hanoi constraints.
int mmuko_rebalance_dimensions(mmuko_tower_t* towers[], uint32_t num_towers,
                                uint32_t proc_count) {
    (void)proc_count;
    if (!towers || num_towers < 2) return -1;

    printf("\n[REBALANCE] Dimensional rebalancing started...\n");

    int imbalances = mmuko_detect_strategic_imbalance(towers, num_towers);
    if (imbalances == 0) {
        printf("[REBALANCE] System at Nash equilibrium — no rebalancing needed.\n");
        return 0;
    }

    // Simple rebalancing: bubble-sort style priority redistribution
    // Find tower with highest average priority and lowest
    int max_idx = -1, min_idx = -1;
    double max_mean = 0.0, min_mean = 1e18;

    for (uint32_t i = 0; i < num_towers; i++) {
        if (!towers[i] || towers[i]->count == 0) continue;
        uint64_t tower_sum = 0;
        for (uint32_t j = 0; j < towers[i]->count; j++) {
            tower_sum += mmuko_polar_priority(towers[i]->disks[j]);
        }
        double tower_mean = (double)tower_sum / (double)towers[i]->count;
        if (tower_mean > max_mean) { max_mean = tower_mean; max_idx = (int)i; }
        if (tower_mean < min_mean) { min_mean = tower_mean; min_idx = (int)i; }
    }

    if (max_idx >= 0 && min_idx >= 0 && max_idx != min_idx) {
        // Migrate top process from max tower to min tower
        mmuko_tower_t* src = towers[max_idx];
        mmuko_tower_t* dst = towers[min_idx];
        if (src->count > 0) {
            mmuko_pcb_t* proc = src->disks[src->count - 1]; // top disk
            int r = mmuko_migrate(src, dst, proc);
            if (r == 1) {
                printf("[REBALANCE] Migrated pid=%u %s -> %s\n",
                       proc->pid, src->name, dst->name);
            } else {
                printf("[REBALANCE] Migration failed for pid=%u (Hanoi violation)\n",
                       proc->pid);
            }
        }
    }

    printf("[REBALANCE] Dimensional rebalancing complete.\n");
    return imbalances;
}

// ─────────────────────────────────────────────────────────────────────────────
// SCHEDULER MAIN LOOP
// ─────────────────────────────────────────────────────────────────────────────

int mmuko_scheduler_init(MMUKO_System* sys, mmuko_sched_mode_t mode) {
    if (!sys) return -1;
    sys->sched_mode = mode;
    printf("\n[SCHEDULER] Initialized mode=%d\n", mode);
    return 0;
}

// Execute one scheduling quantum (tick)
int mmuko_scheduler_tick(MMUKO_System* sys) {
    if (!sys || !sys->boot_complete) return -1;

    mmuko_tower_t* ready  = sys->towers[MMUKO_SEG_STACK]; // Stack as ready queue
    mmuko_tower_t* running= sys->towers[MMUKO_SEG_DATA];  // Data as running (CPU)
    mmuko_tower_t* heap   = sys->towers[MMUKO_SEG_HEAP];  // Heap for staging
    (void)heap;

    if (!ready || !running) return -1;

    // Decrement remaining time for running process
    if (running->count > 0) {
        mmuko_pcb_t* current = running->disks[running->count - 1];
        if (current->remaining_time > 0) {
            current->remaining_time--;
        }
        if (current->remaining_time == 0) {
            // Process completed
            current->state = MMUKO_PROC_TERMINATED;
            current->completion_time = mmuko_now_ms();
            current->turnaround_time = current->completion_time - current->arrival_time;
            printf("[TICK] pid=%u COMPLETED (turnaround=%lums)\n",
                   current->pid, current->turnaround_time);
            // Remove from running
            running->count--;
            running->disks[running->count] = NULL;
            running->used -= current->token.token_memory;
        }
    }

    // If running is empty and ready has processes, schedule next
    if (running->count == 0 && ready->count > 0) {
        // Find highest priority process in ready queue
        int best_idx = -1;
        uint64_t best_pri = 0;
        for (uint32_t i = 0; i < ready->count; i++) {
            uint64_t pri = mmuko_polar_priority(ready->disks[i]);
            if (best_idx < 0 || pri > best_pri) {
                best_pri = pri;
                best_idx = (int)i;
            }
        }
        if (best_idx >= 0) {
            mmuko_pcb_t* next = ready->disks[best_idx];
            // Remove from ready (shift)
            for (uint32_t i = (uint32_t)best_idx; i < ready->count - 1; i++) {
                ready->disks[i] = ready->disks[i + 1];
            }
            ready->count--;
            ready->used -= next->token.token_memory;

            // Push to running
            running->disks[running->count] = next;
            running->count++;
            running->used += next->token.token_memory;
            next->state = MMUKO_PROC_RUNNING;
            printf("[TICK] pid=%u scheduled RUNNING (priority=%lu)\n",
                   next->pid, best_pri);
        }
    }

    return 0;
}

// Full scheduler run: simulate until all processes complete or max ticks
int mmuko_scheduler_run(MMUKO_System* sys, uint64_t max_ticks) {
    if (!sys) return -1;
    printf("\n========================================\n");
    printf("  MMUKO POLAR SCHEDULER START\n");
    printf("  Mode: %s\n",
           sys->sched_mode == MMUKO_SCHED_PREEMPTIVE ? "PREEMPTIVE" :
           sys->sched_mode == MMUKO_SCHED_NON_PREEMPTIVE ? "NON-PREEMPTIVE" :
           "INEMPTIVE");
    printf("  Max ticks: %lu\n", max_ticks);
    printf("========================================\n");

    uint64_t tick = 0;
    int active = 1;

    while (active && tick < max_ticks) {
        mmuko_scheduler_tick(sys);

        // Periodic dimensional rebalancing every 10 ticks
        if (tick % 10 == 0 && tick > 0) {
            mmuko_tower_t* t[3] = { sys->towers[MMUKO_SEG_STACK],
                                    sys->towers[MMUKO_SEG_DATA],
                                    sys->towers[MMUKO_SEG_HEAP] };
            mmuko_rebalance_dimensions(t, 3, sys->proc_count);
        }

        // Check if all processes are terminated
        active = 0;
        for (uint32_t i = 0; i < sys->proc_count; i++) {
            if (sys->proc_table[i].state != MMUKO_PROC_TERMINATED &&
                sys->proc_table[i].state != MMUKO_PROC_ZOMBIE) {
                active = 1;
                break;
            }
        }
        tick++;
    }

    printf("\n========================================\n");
    printf("  MMUKO POLAR SCHEDULER END\n");
    printf("  Ticks executed: %lu\n", tick);
    printf("========================================\n");
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// DEMO / TEST ENTRY POINT (optional standalone test)
// ─────────────────────────────────────────────────────────────────────────────

#ifdef MMUKO_SCHEDULER_TEST

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    printf("MMUKO OS Polar Priority Heap Scheduler Test\n");
    printf("OBINexus R&D — \"Don't just schedule processes. Schedule truthful ones.\"\n\n");

    // Create system: 64 bytes MMUKO memory, 16 max processes
    MMUKO_System* sys = mmuko_system_create(64, 16);
    if (!sys) {
        fprintf(stderr, "Failed to create MMUKO system\n");
        return 1;
    }

    // Boot
    BootStatus status = mmuko_boot(sys);
    if (status != BOOT_OK) {
        printf("BOOT FAILED: %d\n", status);
        mmuko_system_destroy(sys);
        return 1;
    }

    // Initialize scheduler in PREEMPTIVE mode
    mmuko_scheduler_init(sys, MMUKO_SCHED_PREEMPTIVE);

    // Create sample processes with different rights dimensions
    // P0: Civil + Human (D=3), small memory, short burst
    mmuko_pcb_t* p0 = mmuko_proc_create(0,
        MMUKO_RIGHTS_CIVIL | MMUKO_RIGHTS_HUMAN,
        5, MMUKO_KB(4));
    mmuko_proc_allocate(p0, sys->towers[MMUKO_SEG_STACK]);
    sys->proc_table[sys->proc_count++] = *p0; // copy into table

    // P1: Civil + Human + Disability (D=7), medium memory, medium burst
    mmuko_pcb_t* p1 = mmuko_proc_create(0,
        MMUKO_RIGHTS_CIVIL | MMUKO_RIGHTS_HUMAN | MMUKO_RIGHTS_DISABILITY,
        8, MMUKO_KB(8));
    mmuko_proc_allocate(p1, sys->towers[MMUKO_SEG_STACK]);
    sys->proc_table[sys->proc_count++] = *p1;

    // P2: All rights (D=15), large memory, long burst
    mmuko_pcb_t* p2 = mmuko_proc_create(0,
        MMUKO_RIGHTS_ALL,
        12, MMUKO_KB(16));
    mmuko_proc_allocate(p2, sys->towers[MMUKO_SEG_STACK]);
    sys->proc_table[sys->proc_count++] = *p2;

    // P3: Human only (D=2), tiny memory, short burst
    mmuko_pcb_t* p3 = mmuko_proc_create(0,
        MMUKO_RIGHTS_HUMAN,
        3, MMUKO_KB(2));
    mmuko_proc_allocate(p3, sys->towers[MMUKO_SEG_STACK]);
    sys->proc_table[sys->proc_count++] = *p3;

    printf("\n--- Initial Tower States ---\n");
    mmuko_print_tower(sys->towers[MMUKO_SEG_STACK]);
    mmuko_print_tower(sys->towers[MMUKO_SEG_DATA]);
    mmuko_print_tower(sys->towers[MMUKO_SEG_HEAP]);

    // Run scheduler
    mmuko_scheduler_run(sys, 100);

    // Print final status
    mmuko_print_system_status(sys);

    // Cleanup
    mmuko_system_destroy(sys);
    free(p0); free(p1); free(p2); free(p3);

    printf("\nTest complete.\n");
    return 0;
}

#endif // MMUKO_SCHEDULER_TEST

// ─────────────────────────────────────────────────────────────────────────────
// END OF MMUKO_SCHEDULER.C
// ─────────────────────────────────────────────────────────────────────────────
