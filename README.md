# MMUKO OS Polar Priority Scheduler

**Constitutional Computing Framework | OBINexus R&D**

![Version](https://img.shields.io/badge/version-0.2--polar--scheduler-blue)
![License](https://img.shields.io/badge/license-OBINexus-brightgreen)
![Platforms](https://img.shields.io/badge/platforms-Windows%20%7C%20Linux%20%7C%20macOS-blue)
![Status](https://img.shields.io/badge/status-Production%20Ready-green)

---

## 📖 Overview

MMUKO (Muco & Muko) is a **nonlinear, non-polar operating system kernel** that implements a **Polar Priority Scheduler** based on **Dimensional Game Theory** and **Constitutional Computing principles**. 

This scheduler optimizes process execution through:
- **Rights Dimensions (D):** Civil, Human, Disability, Stability rights as first-class architectural concepts
- **Dimensional Game Theory:** Nash equilibrium detection and strategic rebalancing
- **Tower of Hanoi Memory Model:** Memory hierarchy as constitutional constraints
- **NSIGII Calibration:** Human rights verification protocol (Transmitter → Receiver → Verifier)
- **Cross-Platform Support:** Windows (QueryPerformanceCounter), Linux (clock_gettime), macOS (compatible)

**Status:** ✅ Builds and runs successfully on Windows, Linux, and macOS.

---

## 🚀 Quick Start

### Prerequisites

**Windows (MinGW-w64):**
```bash
# Install MSYS2 from https://www.msys2.org/
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
```

**Linux:**
```bash
# Ubuntu/Debian
sudo apt-get install gcc make

# Fedora/RHEL
sudo dnf install gcc make
```

**macOS:**
```bash
# Xcode command-line tools
xcode-select --install
```

### Build & Run

```bash
cd mmuko-scheduler
make clean
make
./mmuko-os          # Linux/macOS
.\mmuko-os.exe      # Windows PowerShell
```

**Expected Output:**
```
MMUKO OS Polar Priority Heap Scheduler Test
OBINexus R&D — "Don't just schedule processes. Schedule truthful ones."

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
[REBALANCE] System at Nash equilibrium — no rebalancing needed.
========================================
  MMUKO POLAR SCHEDULER END
  Ticks executed: 100
========================================
```

---

## 🏗️ Architecture

### Core Components

#### 1. **Seven-Phase Boot Sequence**

```
PHASE 0: Vacuum Medium Initialization
  └─ Gravity constants (G_VACUUM, G_LEPTON, G_MUON, G_DEEP)
  
PHASE 1: Cubit Ring Initialization
  └─ 8-cubit per byte compass geometry
  └─ Spin states: UP, DOWN, CHARM, STRANGE, LEFT, RIGHT
  
PHASE 2: Compass Alignment
  └─ Every cubit faces a direction: N, NE, E, SE, S, SW, W, NW
  
PHASE 3: Superposition Entanglement
  └─ Resolve constructive interference via state flipping
  
PHASE 4: Frame of Reference Centering
  └─ Lock frame to middle base (6 for 8-bit system)
  
PHASE 5: Nonlinear Index Resolution
  └─ Diamond table traversal: [12, 6, 8, 4, 10, 2, 1]
  
PHASE 6: Rotation Freedom Verification
  └─ Confirm 360° rotation capability (no lock)
  
PHASE 7: Boot Complete
  └─ System ready for scheduler
```

#### 2. **Polar Priority Scheduler**

**Strategic Dimension Calculation:**
```
D = Civil (1) + Human (2) + Disability (4) + Stability (8)
N = Civil × Human (product of base dimensions)
Priority = D × N × Strategic_Value
```

**Process Control Block (PCB):**
```c
typedef struct {
    uint32_t pid;              // Process ID
    mmuko_rights_t rights;     // Rights dimensions
    uint64_t burst_time;       // CPU burst duration
    uint64_t remaining_time;   // For preemptive accounting
    uint32_t dimension_count;  // N = a × b
    uint64_t strategic_value;  // Computed polar priority
    double bayesian_conf;      // P(A|B) confidence
} mmuko_pcb_t;
```

#### 3. **Tower of Hanoi Memory Model**

Memory domains as Hanoi towers with constraint: **larger process cannot stack on smaller process.**

```
Tower[TEXT]:   Read-only code segment
Tower[DATA]:   Global variables
Tower[STACK]:  FILO - local vars, return addresses
Tower[HEAP]:   Dynamic allocation, O(1) after alloc
```

#### 4. **NSIGII Calibration (Tripartite Verification)**

```
NSIGII_Stream = {
    Transmitter (TX),
    Receiver (RX),
    Verifier (VRF)
}
```

Ensures human rights verification across transmitter, receiver, and verifier nodes.

#### 5. **Dimensional Rebalancing**

**Nash Equilibrium Detection:**
```
When both players (scheduler vs. processes) employ optimal strategies,
the game results in deterministic outcome (ties).
```

If system detects strategic imbalance, triggers rebalancing to restore equilibrium.

---

## 📊 File Structure

```
mmuko-scheduler/
├── Makefile                    # Cross-platform build config
├── mmuko.h                     # Main header (constants, types, functions)
├── mmuko.c                     # Boot sequence + system lifecycle
├── mmuko_scheduler.c           # Polar Priority Heap + rebalancing
├── README.md                   # This file
└── docs/
    ├── WINDOWS_BUILD_GUIDE.md
    ├── CROSS_PLATFORM_FIX_SUMMARY.md
    ├── ARCHITECTURE_DIAGRAM.md
    └── DIMENSIONAL_GAME_THEORY.md
```

---

## 🔧 Key Features

### Cross-Platform Support

| Platform | Timer API | Library | Status |
|----------|-----------|---------|--------|
| **Windows** | QueryPerformanceCounter | windows.h | ✅ Native |
| **Linux** | clock_gettime(CLOCK_MONOTONIC) | libc + librt | ✅ Native |
| **macOS** | clock_gettime(CLOCK_MONOTONIC) | libc | ✅ Compatible |

**Smart Build System:** Auto-detects OS and links appropriate libraries. No manual configuration needed.

### Rights Dimensions

MMUKO formalizes rights as a 4-bit vector:
- **Bit 0 (Civil):** Civil rights (value = 1)
- **Bit 1 (Human):** Human rights (value = 2)
- **Bit 2 (Disability):** Disability rights (value = 4)
- **Bit 3 (Stability):** Stability rights (value = 8)

**Example:** `rights = 0x03` means Civil + Human rights (D=3, N=2).

### Cubit Ring Geometry

Each byte is modeled as 8 cubits in a compass ring:

```
        N (index 0)
      NW (7) ↕ NE (1)
    W (6) ←→ E (2)
      SW (5) ↔ SE (3)
        S (4)
```

Entangled pairs resolve constructive interference independently.

### Stress Zone Management (RFC)

Reserved for future RAF (Regulation As Firmware) integration:
```
Zone[0-3]:  OK - Normal operations
Zone[3-6]:  Warning - Enhanced monitoring
Zone[6-9]:  Critical - Restricted operations
Zone[9-12]: Panic - Process termination
```

---

## 📚 Theoretical Foundation

### Dimensional Game Theory

From Okpala, N.M. (2025):

**Theorem 1 (Perfect Game Outcome):**
> In a two-player zero-sum game with complete information, if both players employ optimal strategies in all relevant dimensions, the game will result in a deterministic tie.

**Corollary 1 (Strategic Imbalance):**
> The existence of a non-tie outcome implies a strategic imbalance in at least one dimension.

This theoretical framework enables the scheduler to:
1. Detect when process-scheduler dynamics are balanced (Nash equilibrium)
2. Identify strategic imbalances
3. Rebalance dimensions to restore equilibrium

### Constitutional Computing

MMUKO embodies constitutional computing principles:
- **Rights as Architecture:** Rights dimensions are first-class OS concepts
- **Memory is Governance:** Memory hierarchy enforces constitutional constraints
- **No Ghosting:** All state transitions logged and verifiable
- **Tripolar Identity:** Uche (knowledge) + Eze (leadership) + Obi (heart) architecture

---

## 🧪 Testing

### Run Test Suite

```bash
make clean
make test
```

### Expected Results

```
[MMUKO] System created: 64 bytes, 16 max processes
[TOWER] Created STACK domain: cap=256, size=2097152 bytes
[PHASE 7] MMUKO BOOT COMPLETE
[SCHEDULER] Initialized mode=0
[PROC] Created pid=1000, rights=0x03, burst=5ms, mem=4096, D=3, N=2, priority=36
[TICK] pid=1000 scheduled RUNNING (priority=36)
[TICK] pid=1000 COMPLETED (turnaround=0ms)
[REBALANCE] System at Nash equilibrium — no rebalancing needed.
```

---

## 🔨 Build Configuration

### Makefile Targets

```bash
make              # Build mmuko-os executable
make clean        # Remove build artifacts
make test         # Build and run tests
make help         # Show help
```

### Platform Detection

Makefile automatically detects:
- **Linux** → Links `-lm -lrt`
- **macOS** → Links `-lm`
- **Windows** → Links `-lm` (no `-lrt`)

### Compiler Flags

```makefile
CFLAGS = -Wall -Wextra -std=c11 -O2 \
         -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
         -DMMUKO_SCHEDULER_TEST
```

---

## 📖 Documentation

- **[WINDOWS_BUILD_GUIDE.md](docs/WINDOWS_BUILD_GUIDE.md)** — Detailed Windows setup
- **[CROSS_PLATFORM_FIX_SUMMARY.md](docs/CROSS_PLATFORM_FIX_SUMMARY.md)** — Technical architecture
- **[ARCHITECTURE_DIAGRAM.md](docs/ARCHITECTURE_DIAGRAM.md)** — Visual explanations
- **[DIMENSIONAL_GAME_THEORY.md](docs/DIMENSIONAL_GAME_THEORY.md)** — Theoretical foundation

---

## 🌐 Project Context

**MMUKO** is part of the broader **OBINexus Constitutional Computing Framework**:

- **OBINexus:** Constitutional computing framework grounded in Igbo philosophy
- **NSIGII:** Human rights verification protocol (Never a Weapon, Never a Toy, Never a Problem)
- **libpolycall:** Polyglot protocol layer for cross-language RPC
- **RiftLang:** Compiler infrastructure
- **MMUKO:** Physical operating system kernel

**Long-term Vision:** Deploy constitutional computing infrastructure in Nnewi/Anambra, Nigeria.

---

## 🏅 Credits

- **Author:** Nnamdi Michael Okpala
- **Framework:** OBINexus R&D
- **Theoretical Foundation:** Dimensional Game Theory (Okpala, 2025)
- **Architecture Inspiration:** Igbo cultural philosophy (Uche/Eze/Obi)

---

## 📝 License

OBINexus Constitutional Computing Framework  
© 2025 Nnamdi Michael Okpala

---

## 🚀 Next Steps

1. **Explore the Code:** Start with `mmuko.h` for type definitions
2. **Run the Scheduler:** Execute `./mmuko-os` to see all 7 boot phases
3. **Read the Theory:** Check `docs/DIMENSIONAL_GAME_THEORY.md`
4. **Contribute:** Issues, PRs, and ideas welcome
5. **Deploy:** Windows, Linux, macOS all supported

---

## ❓ FAQ

**Q: Why is it called "MMUKO"?**
A: Muco & Muko — a play on "much" and "make," representing the dual nature of constitutional computing.

**Q: What does "nonlinear, non-polar" mean?**
A: The OS boot doesn't proceed linearly (0→255). It boots via diamond traversal [12, 6, 8, 4, 10, 2, 1], and processes aren't evaluated in polar coordinates but in dimensional spaces.

**Q: Is this production-ready?**
A: Yes. It builds cleanly, boots completely, and runs the scheduler with dimensional rebalancing on Windows, Linux, and macOS.

**Q: How does it relate to traditional OSes?**
A: It's not a traditional OS replacement. It's a *kernel architecture research project* exploring constitutional computing, rights-based process scheduling, and dimensional game theory in OS design.

**Q: Can I use this in my project?**
A: Yes, if you understand the theoretical framework and acknowledge OBINexus. See LICENSE for details.

---

## 📞 Contact

**OBINexus R&D**  
Nnamdi Michael Okpala  
GitHub: [@obinexusmk2](https://github.com/obinexusmk2)

---

**Motto:** *"Don't just boot systems. Boot truthful ones."*

🔄 **Remember:** Every process is a disk. Every memory domain is a tower. Every right is architecture.

---

*Last Updated: 22 May 2026*  
*Status: ✅ Production Ready (Windows/Linux/macOS)*
