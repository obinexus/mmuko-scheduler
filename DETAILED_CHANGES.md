# MMUKO Build Fix — Detailed Changes

## Change Summary

| Component | Issue | Fix |
|-----------|-------|-----|
| `mmuko.c` | Feature macros only in header; not honored in compilation unit | Add `_GNU_SOURCE` and `_POSIX_C_SOURCE=200809L` **before** `#include "mmuko.h"` |
| `Makefile` | `-lm` in CFLAGS; `-lrt` missing; feature macros not in build flags | Separate CFLAGS and LDFLAGS; add feature macros to CFLAGS; add `-lrt` to LDFLAGS |

---

## File: mmuko.c

### BEFORE

```c
// ============================================================================
// MMUKO.C — MMUKO OS Boot, System Lifecycle & Calibration
// ... header comments ...
// ============================================================================

#include "mmuko.h"

// ─────────────────────────────────────────────────────────────────────────────
// GLOBAL LOOKUP TABLES
// ...
```

### AFTER

```c
// ============================================================================
// MMUKO.C — MMUKO OS Boot, System Lifecycle & Calibration
// ... header comments ...
// ============================================================================

// POSIX feature test macros — MUST come before any includes
#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L  // Ensures clock_gettime() is visible
#endif

#include "mmuko.h"

// ─────────────────────────────────────────────────────────────────────────────
// GLOBAL LOOKUP TABLES
// ...
```

**Impact:** Enables `CLOCK_MONOTONIC` and `clock_gettime()` declarations in `<time.h>`.

---

## File: Makefile

### BEFORE

```makefile
# MMUKO OS Makefile
# github.com/obinexus/mmuko-os

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -lm -DMMUKO_SCHEDULER_TEST
TARGET = mmuko-os

SRCS = mmuko.c mmuko_scheduler.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c mmuko.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

test: $(TARGET)
	./$(TARGET)
```

### AFTER

```makefile
# MMUKO OS Makefile
# github.com/obinexus/mmuko-os

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -DMMUKO_SCHEDULER_TEST
LDFLAGS = -lm -lrt
TARGET = mmuko-os

SRCS = mmuko.c mmuko_scheduler.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c mmuko.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

test: $(TARGET)
	./$(TARGET)
```

**Key Changes:**

| Line | Change | Reason |
|------|--------|--------|
| 5 | `CFLAGS = ... -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L ...` | Unlock POSIX declarations |
| 6 | `LDFLAGS = -lm -lrt` | New variable: separate linker flags |
| 17 | `$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)` | Use LDFLAGS in link rule |

---

## Impact on Compilation Pipeline

### Before (Broken)

```bash
$ gcc -Wall -Wextra -std=c11 -O2 -lm -DMMUKO_SCHEDULER_TEST -c mmuko.c -o mmuko.o
# ✗ Compiler sees -lm (linker flag, not compiler flag)
# ✗ No POSIX feature macros → <time.h> doesn't expose CLOCK_MONOTONIC
# ✗ clock_gettime() not declared
```

### After (Fixed)

```bash
$ gcc -Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -DMMUKO_SCHEDULER_TEST -c mmuko.c -o mmuko.o
# ✓ Compiler flags clean and separated
# ✓ POSIX macros enable CLOCK_MONOTONIC in <time.h>
# ✓ clock_gettime() properly declared
# ✓ Compilation succeeds

$ gcc -Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -DMMUKO_SCHEDULER_TEST -c mmuko_scheduler.c -o mmuko_scheduler.o
# ✓ Same setup for scheduler compilation

$ gcc -Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -DMMUKO_SCHEDULER_TEST -o mmuko-os mmuko.o mmuko_scheduler.o -lm -lrt
# ✓ Linker flags now in proper place
# ✓ -lrt enables POSIX timer symbols
# ✓ Link succeeds
```

---

## Code Location: mmuko_now_ms()

The function that was failing:

```c
// File: mmuko.c, lines 74-78
uint64_t mmuko_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);  // ← This line required the fix
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
```

**Dependencies:**
- `struct timespec` — from `<time.h>` (included via `mmuko.h`)
- `clock_gettime()` — POSIX function, visible only with `_POSIX_C_SOURCE`
- `CLOCK_MONOTONIC` — POSIX constant, visible only with `_POSIX_C_SOURCE`

**Usage in MMUKO Scheduler:**
- Called during process scheduling to measure **burst times** and **turnaround times**
- Used in **Polar Priority Heap** calculation to enforce fairness
- Essential for **MMUKO_SCHEDULER_TEST** mode validation

---

## Verification Output

### Compilation (Clean)

```
$ make clean && make
rm -f mmuko.o mmuko_scheduler.o mmuko-os
gcc -Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -DMMUKO_SCHEDULER_TEST -c mmuko.c -o mmuko.o
gcc -Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -DMMUKO_SCHEDULER_TEST -c mmuko_scheduler.c -o mmuko_scheduler.o
gcc -Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -DMMUKO_SCHEDULER_TEST -o mmuko-os mmuko.o mmuko_scheduler.o -lm -lrt
$ echo $?
0  # Success!
```

### Runtime (Excerpt)

```
$ ./mmuko-os
MMUKO OS Polar Priority Heap Scheduler Test
OBINexus R&D — "Don't just schedule processes. Schedule truthful ones."

[TOWER] Created DATA domain: cap=64, size=1048576 bytes
[TOWER] Created STACK domain: cap=256, size=2097152 bytes
[TOWER] Created HEAP domain: cap=512, size=4194304 bytes
[MMUKO] System created: 64 bytes, 16 max processes

========================================
  MMUKO BOOT SEQUENCE v0.2-polar-scheduler
  OBINexus Constitutional Computing
========================================

[PHASE 0] Vacuum medium initialization...
[PHASE 1] Initializing cubit rings...
[PHASE 2] Compass alignment...
[PHASE 3] Entangling superposition pairs...
[PHASE 4] Frame of reference centering...
[PHASE 5] Nonlinear index resolution (diamond table)...
[PHASE 6] Rotation freedom check...

[PHASE 7] MMUKO BOOT COMPLETE — All cubits aligned, no lock detected.

[SCHEDULER] Initialized mode=0
[PROC] Created pid=1000, rights=0x03, burst=5ms, mem=4096, D=3, N=2, priority=36
...
```

✅ **All 7 boot phases complete. Scheduler initializes. Processes created and allocated.**

---

## References

- **POSIX Standard:** IEEE 1003.1-2008 (includes `clock_gettime()`)
- **GCC Feature Macros:** https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
- **Linux clock_gettime(2):** `man 2 clock_gettime`
- **OBINexus:** github.com/obinexus/mmuko-os
