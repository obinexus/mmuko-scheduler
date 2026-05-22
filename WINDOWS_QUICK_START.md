# MMUKO OS Windows Build — Quick Start
**Platform:** Windows PowerShell + MinGW-w64  
**Status:** ✅ Ready to Build

---

## Prerequisites (One-Time Setup)

### Option 1: MSYS2 + MinGW-w64 (Recommended)

```powershell
# 1. Download MSYS2 from https://www.msys2.org/
# 2. Run installer, complete setup
# 3. Open MSYS2 MinGW terminal and run:
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make

# 4. Verify installation:
gcc --version
make --version
```

### Option 2: Add MinGW to Windows PATH

```powershell
# If MinGW already installed elsewhere:
$env:PATH += ";C:\MinGW\bin"  # Adjust path as needed
# Or use Windows Settings > Environment Variables for permanent change
```

---

## Build (Every Time)

```powershell
cd C:\Users\Nnamdi\Downloads\mmuko-scheduler

# Clean previous build
make clean

# Build
make

# Run
.\mmuko-os.exe
```

**Expected Output:**
```
MMUKO OS Polar Priority Heap Scheduler Test
OBINexus R&D — "Don't just schedule processes. Schedule truthful ones."

[MMUKO BOOT SEQUENCE v0.2-polar-scheduler]
[PHASE 0] Vacuum medium initialization...
[PHASE 1] Initializing cubit rings...
[PHASE 2] Compass alignment...
[PHASE 3] Entangling superposition pairs...
[PHASE 4] Frame of reference centering...
[PHASE 5] Nonlinear index resolution (diamond table)...
[PHASE 6] Rotation freedom check...

[PHASE 7] MMUKO BOOT COMPLETE — All cubits aligned, no lock detected.
[SCHEDULER] Initialized mode=0
```

---

## What Changed?

| File | Change | Why |
|------|--------|-----|
| `mmuko.c` | Added `#ifdef _WIN32` + `#include <windows.h>` | Use Windows API instead of POSIX |
| `mmuko.c` | Replaced `mmuko_now_ms()` | Windows: `QueryPerformanceCounter()`, Linux: `clock_gettime()` |
| `Makefile` | Added platform detection | Auto-detect OS and link appropriate libraries |

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| `make: command not found` | Install make via MSYS2: `pacman -S make` |
| `gcc: command not found` | Ensure MinGW is in PATH; restart PowerShell after adding PATH |
| `mmuko-os: command not found` | Run as `.\mmuko-os.exe` (requires ./ prefix in PowerShell) |
| Still says "CLOCK_MONOTONIC undeclared" | You have **old** mmuko.c. Copy the new one from outputs. |

---

## Platform Support

| OS | Status | Command |
|----|--------|---------|
| Windows (PowerShell + MinGW) | ✅ **FIXED** | `make && .\mmuko-os.exe` |
| Linux (WSL2, Ubuntu, Debian) | ✅ Works | `make && ./mmuko-os` |
| macOS | ✅ Should work | `make && ./mmuko-os` |

---

## Key Points

✅ **Windows uses `QueryPerformanceCounter()`** — Faster, more precise than POSIX  
✅ **Automatic platform detection** — Makefile handles Windows/Linux/macOS  
✅ **Same behavior everywhere** — Identical scheduler behavior across all platforms  
✅ **No manual configuration** — Just `make` and it works  

---

**Need help?** See `WINDOWS_BUILD_GUIDE.md` for detailed instructions.
