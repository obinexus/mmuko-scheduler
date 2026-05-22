# Task Scheduler Documentation

**Program:** scheduler.exe / scheduler  
**Version:** 1.0  
**Platform:** Windows (MinGW), Linux, macOS  
**Author:** OBINexus Constitutional Computing  
**Date:** 22 May 2026

---

## Overview

The **Task Scheduler** is a cross-platform time-based program execution scheduler. It runs any executable program on a fixed time interval, a specified number of times.

**Core Feature:** Allocate time slots to programs and execute them according to a schedule.

---

## Quick Start

### Build

**Windows (PowerShell/CMD):**
```bash
cd scheduler
make clean && make
```

**Linux/macOS:**
```bash
cd scheduler
make clean && make
./scheduler --help
```

### Basic Usage

**Run hello.exe every 5 seconds, 10 times:**
```bash
scheduler --program hello.exe --interval 5s --repeat 10
```

**Run a Linux command every 2 seconds, 5 times:**
```bash
./scheduler --program /bin/echo --interval 2s --repeat 5
```

**Run a Python script every 500ms, 20 times:**
```bash
scheduler --program python script.py --interval 500ms --repeat 20
```

---

## Command-Line Arguments

### Required Arguments

| Argument | Format | Description |
|----------|--------|-------------|
| `--program` | `<path>` | Path to executable program |
| `--interval` | `<time>` | Time between executions |
| `--repeat` | `<count>` | Number of times to repeat |

### Time Format

Supported time units:
- `ms` — milliseconds (e.g., `500ms`)
- `s` — seconds (e.g., `5s`, `2.5s`)
- `m` — minutes (e.g., `1m`, `30m`)

### Examples

```bash
# Every 5 seconds, 10 times
scheduler --program hello.exe --interval 5s --repeat 10

# Every 500 milliseconds, 20 times
scheduler --program task.exe --interval 500ms --repeat 20

# Every 2 minutes, 3 times
scheduler --program backup.exe --interval 2m --repeat 3

# Every 1 second, 60 times (1 minute total)
scheduler --program test.exe --interval 1s --repeat 60

# Every 100ms, 50 times
scheduler --program fast.exe --interval 100ms --repeat 50
```

---

## Output Format

The scheduler produces detailed timing information:

```
========================================
  TASK SCHEDULER
  OBINexus Constitutional Computing
========================================
Program:      hello.exe
Interval:     5000 ms
Repeat count: 10
Start time:   t=0 ms
========================================

[EXEC] Starting: hello.exe at t=0 ms
[EXEC] Completed: hello.exe (exit code: 0, duration: 25 ms)

[SCHEDULER] Waiting 4975 ms until next execution...
[EXEC] Starting: hello.exe at t=5000 ms
[EXEC] Completed: hello.exe (exit code: 0, duration: 26 ms)

...

========================================
  SCHEDULER SUMMARY
========================================
Total executions: 10/10
Total time:       50025 ms

Execution #1:
  Scheduled: t=0 ms
  Started:   t=0 ms
  Completed: t=25 ms
  Duration:  25 ms
  Exit code: 0

...

Execution Statistics:
  Average duration: 25 ms
  Min duration:     24 ms
  Max duration:     27 ms
  Total work time:  250 ms
========================================
```

---

## Key Features

### 1. **Time-Based Scheduling**
- Precise interval timing using platform-specific high-resolution timers
- Windows: QueryPerformanceCounter (nanosecond precision)
- Linux/macOS: clock_gettime(CLOCK_MONOTONIC)

### 2. **Cross-Platform Support**
- Windows (MinGW/MSVC compatible)
- Linux (glibc)
- macOS (Darwin)
- Auto-detects platform; no configuration needed

### 3. **Accurate Execution Tracking**
- Records scheduled time vs. actual execution time
- Measures program duration
- Tracks exit codes
- Calculates statistics (min, max, average)

### 4. **Execution Records**
For each execution, scheduler records:
- `scheduled_time` — When it should have run
- `actual_start_time` — When it actually started
- `actual_end_time` — When it completed
- `exit_code` — Process return value
- `duration` — Execution time (ms)

### 5. **Summary Statistics**
After all executions:
- Total time elapsed
- Average program duration
- Min/max execution times
- Total work time vs. waiting time

---

## Platform-Specific Behavior

### Windows (MinGW)
```bash
# Build
gcc -o scheduler.exe scheduler.c -lm

# Run
scheduler.exe --program hello.exe --interval 5s --repeat 10
```

Uses `CreateProcess` + `WaitForSingleObject` for process management.

### Linux/macOS
```bash
# Build
gcc -o scheduler scheduler.c -lm -lrt

# Run
./scheduler --program ./hello --interval 5s --repeat 10
```

Uses `fork()` + `execl()` + `waitpid()` for process management.

---

## Use Cases

### 1. **Periodic Task Execution**
Run backups every hour:
```bash
scheduler --program backup.sh --interval 60m --repeat 24
# Runs every hour for 24 hours
```

### 2. **Test Automation**
Run test suite repeatedly:
```bash
scheduler --program run_tests.exe --interval 30s --repeat 100
# Runs test every 30 seconds, 100 times
```

### 3. **Monitoring**
Check system status periodically:
```bash
scheduler --program check_health.exe --interval 5s --repeat 720
# Checks every 5 seconds for 1 hour (720 × 5s = 3600s)
```

### 4. **Stress Testing**
Execute program with high frequency:
```bash
scheduler --program stress_test.exe --interval 100ms --repeat 1000
# Runs 1000 times every 100ms = 100 seconds total
```

### 5. **Scheduled Deployment**
Run deployment script at intervals:
```bash
scheduler --program deploy.exe --interval 1m --repeat 5
# Runs deployment 5 times, every minute
```

---

## How It Works

### Execution Flow

```
1. Parse command-line arguments
   ├── Extract program path
   ├── Parse interval string (e.g., "5s" → 5000ms)
   └── Get repeat count

2. Initialize scheduler
   ├── Record start time
   ├── Allocate execution records
   └── Print configuration

3. Main loop (for each repetition):
   ├── Calculate scheduled time
   ├── Get current time
   ├── Sleep until scheduled time
   ├── Spawn process
   ├── Record start time
   ├── Wait for process completion
   ├── Record end time & exit code
   └── Repeat

4. Print summary
   ├── Statistics (min/max/avg duration)
   ├── Execution records
   ├── Total time
   └── Completion status
```

### Time Synchronization

The scheduler maintains synchronization by:

```c
// Execution N is scheduled for:
scheduled_time = start_time + (N × interval_ms)

// Current time when ready to execute:
current_time = scheduler_now_ms()

// If behind schedule, wait this long:
wait_time = scheduled_time - current_time

// If ahead of schedule (program was fast):
wait_time = 0 (no wait needed)

// Execute immediately, next execution waits accordingly
```

---

## Philosophy: Time as Resource

The scheduler implements the **OBINexus Constitutional Computing** principle:

**Time is an allocable resource.**

Just as memory is allocated in bytes (addresses), time is allocated in milliseconds (intervals).

```
Memory allocation:    "Process gets 4KB at address 0x1000"
Time allocation:      "Process gets 5s every 30s interval"

Both are:
- Exclusive (one process at a time)
- Measurable (bytes, milliseconds)
- Verifiable (address range, timestamp range)
- Governable (fairness policy)
```

---

## Technical Details

### Cross-Platform Timer

**Windows:**
```c
QueryPerformanceCounter()  // CPU cycle counter
QueryPerformanceFrequency() // CPU frequency
// Conversion: milliseconds = (cycles / frequency) × 1000
```

**Linux/macOS:**
```c
clock_gettime(CLOCK_MONOTONIC, &ts)
// Returns: seconds + nanoseconds
// Conversion: milliseconds = (sec × 1000) + (nsec / 1000000)
```

### Process Spawning

**Windows:**
```c
_spawnl(_P_NOWAIT, program, program, NULL)
// Non-blocking spawn, returns process ID
OpenProcess() + WaitForSingleObject() // Wait for completion
```

**Linux/macOS:**
```c
fork()      // Create child process
execlp()    // Execute program in child
waitpid()   // Wait for child completion
```

---

## Error Handling

The scheduler handles these errors:

| Error | Message | Recovery |
|-------|---------|----------|
| Invalid time format | `Invalid interval format. Use: 5s, 500ms, 2m` | Exit with usage |
| Unknown time unit | `Unknown time unit: <unit>` | Exit with usage |
| Missing argument | `--program requires an argument` | Exit with usage |
| Program not found | `Failed to spawn process` | Continue to next execution |
| Process wait failed | `Failed to open process handle` | Return -1 exit code |
| Memory allocation | `Memory allocation failed` | Exit gracefully |

---

## Performance Characteristics

### Timing Accuracy

The scheduler's timing accuracy depends on:
1. **OS scheduler precision** (typically 1-10ms on modern systems)
2. **Program startup overhead** (typically 10-50ms)
3. **System load** (other processes competing for CPU)

**Typical behavior:**
```
Requested interval: 5000ms
Actual interval:    5010-5050ms (5-50ms variance)
Overhead:           10-50ms per execution
```

### Memory Usage

- **Base memory:** ~2KB for scheduler code
- **Per execution record:** ~40 bytes
- **For 10 executions:** ~400 bytes
- **For 1000 executions:** ~40KB

### CPU Usage

While waiting between executions:
- **Windows:** Sleep() = 0% CPU (OS-managed)
- **Linux/macOS:** usleep() = 0% CPU (OS-managed)

Only CPU used during program execution (outside scheduler control).

---

## Limitations and Notes

1. **Sequential Execution**
   - Programs run one at a time (not in parallel)
   - Next execution waits for previous to complete

2. **Blocking Wait**
   - Scheduler blocks while waiting between executions
   - Cannot handle dynamic scheduling during wait

3. **No Error Recovery**
   - If program fails (non-zero exit), next execution still happens
   - Exit codes are recorded but don't affect scheduling

4. **System Dependent**
   - Actual intervals may vary by 5-50ms due to OS scheduling
   - Not suitable for hard real-time requirements (<1ms precision)

5. **Single Program**
   - Scheduler runs one program on repeat
   - To run multiple different programs, run multiple scheduler instances

---

## Examples

### Example 1: Echo Every 2 Seconds

```bash
scheduler --program /bin/echo --interval 2s --repeat 5
```

**Output:**
```
========================================
  TASK SCHEDULER
  OBINexus Constitutional Computing
========================================
Program:      /bin/echo
Interval:     2000 ms
Repeat count: 5
Start time:   t=0 ms
========================================

[EXEC] Starting: /bin/echo at t=0 ms
[EXEC] Completed: /bin/echo (exit code: 0, duration: 2 ms)

[SCHEDULER] Waiting 1998 ms until next execution...
[EXEC] Starting: /bin/echo at t=2000 ms
[EXEC] Completed: /bin/echo (exit code: 0, duration: 1 ms)

...

Execution Statistics:
  Average duration: 1 ms
  Min duration:     1 ms
  Max duration:     2 ms
  Total work time:  5 ms
========================================
```

### Example 2: High-Frequency Execution

```bash
scheduler --program test.exe --interval 100ms --repeat 20
```

Runs test.exe 20 times with 100ms interval = 2 seconds total runtime.

### Example 3: Long-Running Program

```bash
scheduler --program backup.exe --interval 30s --repeat 10
```

If backup.exe takes 15 seconds to complete:
- Scheduled at: t=0, t=30, t=60, ...
- Executes from: t=0-15, t=30-45, t=60-75, ...
- Next waits for: 15s (30-15)

---

## Troubleshooting

### Program Not Found
```
ERROR: Failed to spawn process
```

**Solution:** Use full path to program:
```bash
scheduler --program /usr/bin/ls --interval 1s --repeat 5
# instead of
scheduler --program ls --interval 1s --repeat 5
```

### Wrong Interval Format
```
ERROR: Invalid interval format. Use: 5s, 500ms, 2m
```

**Solution:** Use correct format:
```bash
scheduler --program hello.exe --interval 5s --repeat 10  # ✓ Correct
# Not:
scheduler --program hello.exe --interval 5 --repeat 10    # ✗ Wrong
scheduler --program hello.exe --interval 5000 --repeat 10 # ✗ Wrong
```

### Timing Variance

If actual execution time varies from expected:
- This is normal (OS scheduling)
- Use `--interval 5s` not `--interval 4.99s`
- Variance is typically ±10-20ms

---

## Building from Source

### Windows (MinGW)
```bash
gcc -o scheduler.exe scheduler.c -lm
```

### Linux
```bash
gcc -o scheduler scheduler.c -lm -lrt
```

### macOS
```bash
gcc -o scheduler scheduler.c -lm
```

### With Makefile (All Platforms)
```bash
make clean && make
```

---

## Next Steps

### Future Enhancements

1. **Parallel Execution**
   - Run multiple programs simultaneously
   - Manage resource contention

2. **Dynamic Scheduling**
   - Adjust intervals based on program duration
   - Skip execution if still running

3. **Persistence**
   - Save execution logs to file
   - Resume interrupted schedules

4. **Integration with MMUKO**
   - Use as MMUKO process spawner
   - Integrate with dimensional scheduling

5. **Advanced Scheduling**
   - Cron-like syntax support
   - Multiple programs per schedule
   - Priority-based execution

---

## References

- **Dimensional Game Theory:** Okpala, N.M. (2025)
- **Philosophy of Time:** OBINexus PHILOSOPHY_OF_TIME.md
- **MMUKO OS:** OBINexus/mmuko-scheduler repository

---

**Status:** Production Ready ✅

*OBINexus Constitutional Computing Framework*
*22 May 2026*
