/*
 * scheduler.c
 * Cross-Platform Task Scheduler with Time-Based Execution
 * 
 * Usage: scheduler.exe --program hello.exe --interval 5s --repeat 10
 * Runs hello.exe every 5 seconds, 10 times
 * 
 * OBINexus Constitutional Computing Framework
 * Time allocation and scheduling for executable programs
 * 
 * Author: Nnamdi Michael Okpala
 * Date: 22 May 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Platform detection */
#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
    #define SLEEP(ms) Sleep(ms)
    #define SPAWN_PROCESS(prog) _spawnl(_P_NOWAIT, prog, prog, NULL)
#else
    #include <unistd.h>
    #include <sys/wait.h>
    #include <sys/types.h>
    #define SLEEP(ms) usleep((ms) * 1000)
    #define SPAWN_PROCESS(prog) fork_and_exec(prog)
#endif

/* Time measurement macros */
#if !defined(_GNU_SOURCE)
    #define _GNU_SOURCE
#endif
#if !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200809L
#endif

#ifdef _WIN32
    #include <windows.h>
#endif

/* ============================================================================
 * TIME UTILITIES (Cross-Platform)
 * ============================================================================ */

/**
 * Get current time in milliseconds
 * Uses platform-specific timer for accuracy
 */
uint64_t scheduler_now_ms(void) {
#ifdef _WIN32
    static LARGE_INTEGER frequency = {0};
    static LARGE_INTEGER start_time = {0};
    if (frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start_time);
    }
    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    return (uint64_t)(((current_time.QuadPart - start_time.QuadPart) * 1000) / frequency.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

/**
 * Parse interval string (e.g., "5s", "500ms", "2m")
 * Returns milliseconds
 */
uint64_t parse_interval(const char *interval_str) {
    uint64_t value = 0;
    char unit[10];
    
    if (sscanf(interval_str, "%lu%s", &value, unit) != 2) {
        fprintf(stderr, "ERROR: Invalid interval format. Use: 5s, 500ms, 2m\n");
        return 0;
    }
    
    if (strcmp(unit, "ms") == 0) {
        return value;
    } else if (strcmp(unit, "s") == 0) {
        return value * 1000;
    } else if (strcmp(unit, "m") == 0) {
        return value * 60000;
    } else {
        fprintf(stderr, "ERROR: Unknown time unit: %s\n", unit);
        return 0;
    }
}

/* ============================================================================
 * PROCESS SPAWNING (Platform-Specific)
 * ============================================================================ */

#ifndef _WIN32
/**
 * Linux/macOS: Fork and execute program
 */
int fork_and_exec(const char *program) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    }
    
    if (pid == 0) {
        /* Child process */
        execlp(program, program, NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }
    
    /* Parent process */
    return (int)pid;
}
#endif

/**
 * Wait for process to complete
 */
int wait_for_process(int pid) {
#ifdef _WIN32
    HANDLE proc_handle = OpenProcess(SYNCHRONIZE, FALSE, (DWORD)pid);
    if (proc_handle == NULL) {
        fprintf(stderr, "ERROR: Failed to open process handle\n");
        return -1;
    }
    WaitForSingleObject(proc_handle, INFINITE);
    CloseHandle(proc_handle);
    return 0;
#else
    int status;
    waitpid((pid_t)pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
}

/* ============================================================================
 * SCHEDULER DATA STRUCTURES
 * ============================================================================ */

typedef struct {
    char program_path[256];        /* Path to executable */
    uint64_t interval_ms;          /* Time between executions (ms) */
    uint32_t repeat_count;         /* Number of times to run */
    uint32_t executions_completed; /* Counter */
    uint64_t start_time;           /* When scheduler started */
} scheduler_task_t;

typedef struct {
    uint64_t scheduled_time;       /* When execution was scheduled */
    uint64_t actual_start_time;    /* When execution actually started */
    uint64_t actual_end_time;      /* When execution completed */
    int exit_code;                 /* Process exit code */
} execution_record_t;

/* ============================================================================
 * SCHEDULER CORE
 * ============================================================================ */

/**
 * Execute a single instance of the program
 */
int execute_program(const char *program, execution_record_t *record) {
    uint64_t start = scheduler_now_ms();
    record->actual_start_time = start;
    
    printf("[EXEC] Starting: %s at t=%llu ms\n", program, start);
    
    /* Spawn process */
    int pid = SPAWN_PROCESS(program);
    if (pid < 0) {
        fprintf(stderr, "[ERROR] Failed to spawn process\n");
        return -1;
    }
    
    /* Wait for completion */
    int exit_code = wait_for_process(pid);
    
    uint64_t end = scheduler_now_ms();
    record->actual_end_time = end;
    record->exit_code = exit_code;
    
    printf("[EXEC] Completed: %s (exit code: %d, duration: %llu ms)\n", 
           program, exit_code, end - start);
    
    return exit_code;
}

/**
 * Main scheduler loop
 */
void run_scheduler(scheduler_task_t *task) {
    execution_record_t *records = malloc(sizeof(execution_record_t) * task->repeat_count);
    if (records == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed\n");
        return;
    }
    
    memset(records, 0, sizeof(execution_record_t) * task->repeat_count);
    
    task->start_time = scheduler_now_ms();
    
    printf("\n");
    printf("========================================\n");
    printf("  TASK SCHEDULER\n");
    printf("  OBINexus Constitutional Computing\n");
    printf("========================================\n");
    printf("Program:      %s\n", task->program_path);
    printf("Interval:     %llu ms\n", task->interval_ms);
    printf("Repeat count: %u\n", task->repeat_count);
    printf("Start time:   t=%llu ms\n", task->start_time);
    printf("========================================\n\n");
    
    /* Main execution loop */
    for (uint32_t i = 0; i < task->repeat_count; i++) {
        uint64_t scheduled_time = task->start_time + (i * task->interval_ms);
        uint64_t current_time = scheduler_now_ms();
        uint64_t wait_ms = (current_time >= scheduled_time) ? 0 : (scheduled_time - current_time);
        
        if (i > 0 && wait_ms > 0) {
            printf("[SCHEDULER] Waiting %llu ms until next execution...\n", wait_ms);
            SLEEP(wait_ms);
        }
        
        records[i].scheduled_time = scheduled_time;
        
        /* Execute program */
        execute_program(task->program_path, &records[i]);
        
        task->executions_completed++;
        
        if (i < task->repeat_count - 1) {
            printf("\n");
        }
    }
    
    /* Print summary */
    printf("\n");
    printf("========================================\n");
    printf("  SCHEDULER SUMMARY\n");
    printf("========================================\n");
    printf("Total executions: %u/%u\n", task->executions_completed, task->repeat_count);
    printf("Total time:       %llu ms\n\n", scheduler_now_ms() - task->start_time);
    
    /* Execution statistics */
    uint64_t total_duration = 0;
    uint64_t max_duration = 0;
    uint64_t min_duration = UINT64_MAX;
    
    for (uint32_t i = 0; i < task->repeat_count; i++) {
        uint64_t duration = records[i].actual_end_time - records[i].actual_start_time;
        total_duration += duration;
        if (duration > max_duration) max_duration = duration;
        if (duration < min_duration) min_duration = duration;
        
        printf("Execution #%u:\n", i + 1);
        printf("  Scheduled: t=%llu ms\n", records[i].scheduled_time);
        printf("  Started:   t=%llu ms\n", records[i].actual_start_time);
        printf("  Completed: t=%llu ms\n", records[i].actual_end_time);
        printf("  Duration:  %llu ms\n", duration);
        printf("  Exit code: %d\n", records[i].exit_code);
        printf("\n");
    }
    
    printf("Execution Statistics:\n");
    printf("  Average duration: %llu ms\n", total_duration / task->repeat_count);
    printf("  Min duration:     %llu ms\n", min_duration);
    printf("  Max duration:     %llu ms\n", max_duration);
    printf("  Total work time:  %llu ms\n", total_duration);
    printf("========================================\n\n");
    
    free(records);
}

/* ============================================================================
 * COMMAND-LINE PARSING
 * ============================================================================ */

void print_usage(const char *program_name) {
    printf("\n");
    printf("USAGE:\n");
    printf("  %s --program <executable> --interval <time> --repeat <count>\n\n", program_name);
    printf("ARGUMENTS:\n");
    printf("  --program <executable>   Path to program to execute\n");
    printf("  --interval <time>        Time between executions (e.g., 5s, 500ms, 2m)\n");
    printf("  --repeat <count>         Number of times to repeat\n\n");
    printf("EXAMPLES:\n");
    printf("  %s --program hello.exe --interval 5s --repeat 10\n", program_name);
    printf("  %s --program ./test --interval 1s --repeat 5\n", program_name);
    printf("  %s --program /bin/ls --interval 2s --repeat 3\n\n", program_name);
}

int main(int argc, char *argv[]) {
    scheduler_task_t task;
    memset(&task, 0, sizeof(task));
    
    /* Default values */
    task.interval_ms = 5000;      /* 5 seconds */
    task.repeat_count = 10;       /* 10 times */
    
    /* Parse command-line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--program") == 0) {
            if (i + 1 < argc) {
                strncpy(task.program_path, argv[i + 1], sizeof(task.program_path) - 1);
                i++;
            } else {
                fprintf(stderr, "ERROR: --program requires an argument\n");
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
        }
        else if (strcmp(argv[i], "--interval") == 0) {
            if (i + 1 < argc) {
                task.interval_ms = parse_interval(argv[i + 1]);
                if (task.interval_ms == 0) {
                    print_usage(argv[0]);
                    return EXIT_FAILURE;
                }
                i++;
            } else {
                fprintf(stderr, "ERROR: --interval requires an argument\n");
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
        }
        else if (strcmp(argv[i], "--repeat") == 0) {
            if (i + 1 < argc) {
                task.repeat_count = (uint32_t)atoi(argv[i + 1]);
                if (task.repeat_count == 0) {
                    fprintf(stderr, "ERROR: --repeat must be > 0\n");
                    return EXIT_FAILURE;
                }
                i++;
            } else {
                fprintf(stderr, "ERROR: --repeat requires an argument\n");
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        }
        else {
            fprintf(stderr, "ERROR: Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }
    
    /* Validate arguments */
    if (strlen(task.program_path) == 0) {
        fprintf(stderr, "ERROR: --program is required\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    if (task.repeat_count == 0) {
        fprintf(stderr, "ERROR: --repeat must be > 0\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    if (task.interval_ms == 0) {
        fprintf(stderr, "ERROR: --interval is required\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    /* Run the scheduler */
    run_scheduler(&task);
    
    return EXIT_SUCCESS;
}
