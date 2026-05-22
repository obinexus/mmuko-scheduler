# Task Scheduler - Quick Start (2 Minutes)

**Goal:** Build and run `scheduler --program hello.exe --interval 5s --repeat 10`

---

## Build (30 seconds)

### Windows (PowerShell)
```powershell
# Copy Makefile
cp scheduler_Makefile Makefile

# Build
make clean
make
```

### Linux/macOS (Terminal)
```bash
# Copy Makefile
cp scheduler_Makefile Makefile

# Build
make clean
make
./scheduler --help
```

---

## Run (1 minute)

### Windows
```powershell
# Run hello.exe every 5 seconds, 10 times
.\scheduler.exe --program hello.exe --interval 5s --repeat 10
```

### Linux/macOS
```bash
# Run hello every 5 seconds, 10 times
./scheduler --program ./hello --interval 5s --repeat 10
```

---

## What It Does

1. **Spawns** hello.exe (or your program)
2. **Measures** how long it takes
3. **Waits** until the next 5-second interval
4. **Repeats** 10 times total
5. **Reports** timing statistics

---

## Example Output

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
[EXEC] Completed: hello.exe (exit code: 0, duration: 24 ms)

...

========================================
  SCHEDULER SUMMARY
========================================
Total executions: 10/10
Total time:       50100 ms

Execution Statistics:
  Average duration: 24 ms
  Min duration:     23 ms
  Max duration:     26 ms
  Total work time:  240 ms
========================================
```

---

## Common Commands

```bash
# Every 1 second, 60 times
scheduler --program test.exe --interval 1s --repeat 60

# Every 100ms, 20 times (2 seconds total)
scheduler --program quick.exe --interval 100ms --repeat 20

# Every 2 minutes, 5 times (10 minutes total)
scheduler --program slow.exe --interval 2m --repeat 5

# Help
scheduler --help
```

---

## Files

- `scheduler.c` — Source code (copy this)
- `scheduler_Makefile` — Build config (rename to `Makefile`)
- `SCHEDULER_DOCUMENTATION.md` — Full reference

---

**Total time: 2 minutes** ⏱️

*OBINexus Constitutional Computing*
