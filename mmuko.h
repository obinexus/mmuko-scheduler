#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// ============================================================================
// MMUKO.H — MMUKO OS Kernel Header
// Project:    github.com/obinexus/mmuko-os
// Framework:  OBINexus Constitutional Computing / NSIGII Codec
// Author:     Nnamdi Michael Okpala
// Date:       22 May 2026
// Version:    0.2-polar-scheduler
// ============================================================================
//
// MMUKO (Muco & Muko) — Physical Operating System
// Core Axioms:
//   1. Every process is a disk; every memory domain is a tower.
//   2. Rights are architecture: Civil + Human + Disability + Stability = D.
//   3. Scheduling is dimensional game theory — Nash equilibrium via polar heap.
//   4. Memory is governance contract, not storage (RiftLang principle).
//   5. Boot = resolving superposition into coherent frame without lock.
//
// ============================================================================

#ifndef MMUKO_H
#define MMUKO_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 0: FUNDAMENTAL CONSTANTS
// ─────────────────────────────────────────────────────────────────────────────

#define MMUKO_VERSION           "0.2-polar-scheduler"
#define MMUKO_BUILD_DATE        "2026-05-22"
#define MMUKO_PI                3.14159265358979323846

// Memory unit bit-shifts (from transcript: left-shift formalization)
#define MMUKO_BIT_SHIFT_BYTE    3
#define MMUKO_BIT_SHIFT_KB      10
#define MMUKO_BIT_SHIFT_MB      20
#define MMUKO_BIT_SHIFT_GB      30

#define MMUKO_BYTE(x)           ((uint64_t)(x) << MMUKO_BIT_SHIFT_BYTE)
#define MMUKO_KB(x)             ((uint64_t)(x) << MMUKO_BIT_SHIFT_KB)
#define MMUKO_MB(x)             ((uint64_t)(x) << MMUKO_BIT_SHIFT_MB)
#define MMUKO_GB(x)             ((uint64_t)(x) << MMUKO_BIT_SHIFT_GB)

// Alignment constraints (RiftLang token architecture)
#define MMUKO_CLASSICAL_ALIGN   512     // 4096-bit = 512-byte
#define MMUKO_QUANTUM_ALIGN     8       // 8-qubit alignment

// Hanoi optimal moves: 2^n - 1
#define MMUKO_OPTIMAL_MOVES(n)  (((uint64_t)1 << (n)) - 1)

// Gravity medium constants (from mmuko-boot)
#define G_VACUUM                9.8
#define G_LEPTON                (G_VACUUM / 10.0)
#define G_MUON                  (G_LEPTON / 10.0)
#define G_DEEP                  (G_MUON / 10.0)

// Compass spin values (radians)
#define SPIN_NORTH              (MMUKO_PI / 4.0)
#define SPIN_NORTHEAST          (MMUKO_PI / 3.0)
#define SPIN_EAST               (MMUKO_PI / 2.0)
#define SPIN_SOUTHEAST          MMUKO_PI
#define SPIN_SOUTH              (MMUKO_PI * 2.0)
#define SPIN_SOUTHWEST          (MMUKO_PI / 2.0)
#define SPIN_WEST               (MMUKO_PI / 3.0)
#define SPIN_NORTHWEST          (MMUKO_PI / 4.0)

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 1: RIGHTS DIMENSIONS (D = a + b + d + e)
// ─────────────────────────────────────────────────────────────────────────────

typedef enum {
    MMUKO_RIGHTS_NONE       = 0x00,
    MMUKO_RIGHTS_CIVIL      = 0x01,   // a = 1
    MMUKO_RIGHTS_HUMAN      = 0x02,   // b = 2
    MMUKO_RIGHTS_DISABILITY = 0x04,   // d = 4
    MMUKO_RIGHTS_STABILITY  = 0x08,   // e = 8
    MMUKO_RIGHTS_ALL        = 0x0F
} mmuko_rights_t;

// Dimensional sum: D = a + b + d + e
#define MMUKO_DIM_D(p) \
    (((p)->rights & MMUKO_RIGHTS_CIVIL)      ? 1 : 0) + \
    (((p)->rights & MMUKO_RIGHTS_HUMAN)      ? 2 : 0) + \
    (((p)->rights & MMUKO_RIGHTS_DISABILITY) ? 4 : 0) + \
    (((p)->rights & MMUKO_RIGHTS_STABILITY)  ? 8 : 0)

// Node complexity: N = a × b (product of active base dimensions)
#define MMUKO_DIM_N(p) \
    ((((p)->rights & MMUKO_RIGHTS_CIVIL)  ? 1 : 0) * \
     (((p)->rights & MMUKO_RIGHTS_HUMAN)  ? 2 : 0))

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 2: MEMORY SEGMENTS & TOKEN ARCHITECTURE
// ─────────────────────────────────────────────────────────────────────────────

typedef enum {
    MMUKO_SEG_TEXT  = 0,   // Read-only code segment
    MMUKO_SEG_DATA  = 1,   // Global variables
    MMUKO_SEG_STACK = 2,   // FILO: local vars, return addresses
    MMUKO_SEG_HEAP  = 3,   // Dynamic allocation, O(1) after alloc
} mmuko_segment_t;

// RiftLang Token Architecture: memory-type-value triplet
typedef struct {
    uint8_t  token_type;      // segment_id (classical) or quantum mode
    uint64_t token_value;     // priority weight / deferred binding value
    uint64_t token_memory;    // bytes allocated (disk size in Hanoi model)
} mmuko_token_t;

// Token type flags (quantum modes from RiftLang)
#define TOKEN_CLASSICAL_FIXED       0x10
#define TOKEN_CLASSICAL_ROW         0x11
#define TOKEN_CLASSICAL_CONTINUOUS  0x12
#define TOKEN_QUANTUM_SUPERPOSED    0x20
#define TOKEN_QUANTUM_ENTANGLED     0x21
#define TOKEN_MODE_MASK             0xF0

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 3: PROCESS CONTROL BLOCK (PCB)
// ─────────────────────────────────────────────────────────────────────────────

typedef enum {
    MMUKO_PROC_NEW       = 0,
    MMUKO_PROC_READY     = 1,
    MMUKO_PROC_RUNNING   = 2,
    MMUKO_PROC_BLOCKED   = 3,
    MMUKO_PROC_ZOMBIE    = 4,
    MMUKO_PROC_TERMINATED= 5
} mmuko_proc_state_t;

typedef struct mmuko_pcb {
    uint32_t            pid;              // Process ID (the "disk")
    mmuko_token_t       token;            // Memory contract (RiftLang)
    mmuko_rights_t      rights;           // Rights dimensions
    mmuko_proc_state_t  state;            // Process state
    uint64_t            arrival_time;     // When process entered system
    uint64_t            burst_time;       // CPU execution duration
    uint64_t            remaining_time;     // For preemptive accounting
    uint64_t            completion_time;  // When finished
    uint64_t            waiting_time;     // Accumulated wait
    uint64_t            turnaround_time;  // Completion - Arrival

    // Dimensional Game Theory fields
    uint32_t            dimension_count;  // N = a × b
    uint64_t            strategic_value;  // Computed polar priority
    double              bayesian_conf;    // P(A|B) confidence score

    // Scheduling linkage
    struct mmuko_pcb   *next;             // Queue / heap linkage
    struct mmuko_pcb   *prev;             // Doubly-linked for O(1) removal

    // NSIGII Calibration linkage
    uint8_t             cal_fingerprint[16]; // SHA-256 truncated
    bool                calibrated;         // Passed tripartite verification
} mmuko_pcb_t;

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 4: TOWER OF HANOI MEMORY DOMAINS
// ─────────────────────────────────────────────────────────────────────────────

// Tower = Memory domain. Disks = processes with token_memory size.
// Constraint: larger disk cannot be placed on smaller (memory hierarchy protection)
typedef struct {
    mmuko_pcb_t     **disks;            // Process stack (LIFO array)
    uint32_t          count;            // Current tower height (H)
    uint32_t          capacity;         // Max height
    uint64_t          domain_size;      // Total memory in bytes
    uint64_t          used;             // Allocated memory
    mmuko_segment_t   segment;          // Which segment this tower represents
    char              name[16];         // Human-readable domain name
} mmuko_tower_t;

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 5: SCHEDULER MODES
// ─────────────────────────────────────────────────────────────────────────────

typedef enum {
    MMUKO_SCHED_PREEMPTIVE,      // High-priority interrupts running
    MMUKO_SCHED_NON_PREEMPTIVE,  // Runs to completion or I/O block
    MMUKO_SCHED_INEMPTIVE,       // Memory-preemptive, time-nonpreemptive
} mmuko_sched_mode_t;

// Inemptive buffer: staged memory allocation without full context switch
typedef struct {
    uint32_t      buffer_pid;       // Auxiliary staging process
    uint64_t      overlap_size;     // Bytes to pre-allocate
    mmuko_pcb_t  *target_proc;      // Process receiving allocation
    mmuko_tower_t *staging_tower;   // Heap tower used for staging
    bool          resolved;         // Buffer committed?
} mmuko_inemptive_buffer_t;

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 6: CUBIT / SUPERPOSITION / BOOT (from mmuko-boot)
// ─────────────────────────────────────────────────────────────────────────────

typedef enum {
    N, NE, E, SE, S, SW, W, NW, UNDEFINED_DIR
} Direction;

typedef enum {
    UP, DOWN, CHARM, STRANGE, LEFT, RIGHT
} CubitState;

typedef enum {
    RSHIFT, LSHIFT, ROTATE
} ShiftOp;

typedef enum {
    BOOT_OK,
    BOOT_LOCK_DETECTED,
    BOOT_ROTATION_LOCK,
    BOOT_UNDEFINED_DIRECTION,
    BOOT_FAILED
} BootStatus;

typedef struct {
    int         index;          // 0–7
    uint8_t     value;          // 0 or 1
    double      spin;           // Derived from compass direction
    Direction   direction;      // Compass direction
    CubitState  state;          // Quantum-like state
    bool        superposed;     // Is this cubit in superposition?
    int         entangled_with; // Index of entangled partner (-1 if none)
} Cubit;

typedef struct {
    uint8_t     raw_value;
    Cubit       cubit_ring[8];
    int         base_index;
    Direction   primary_superposition;
    Direction   secondary_superposition;
} MMUKO_Byte;

typedef struct {
    double      gravity;
    double      air;
    double      water;
} VacuumMedium;

typedef struct {
    MMUKO_Byte     *memory_map;
    size_t          memory_size;
    VacuumMedium    medium;
    Direction       frame_of_reference;
    bool            boot_complete;
    mmuko_tower_t  *towers[4];      // Text, Data, Stack, Heap
    mmuko_pcb_t    *proc_table;     // Global process registry
    uint32_t        proc_count;
    uint32_t        proc_capacity;
    mmuko_sched_mode_t sched_mode;  // Current scheduler mode
} MMUKO_System;

// Superposition lookup entry
typedef struct {
    int         base;
    Direction   primary;
    Direction   secondary;
} SuperpositionEntry;

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 7: CALIBRATION / NSIGII (from MMUKO Calibration Sequence)
// ─────────────────────────────────────────────────────────────────────────────

typedef enum {
    BYTESTATE_NOISE    = 0,  // Real-world entropy
    BYTESTATE_NONOISE  = 1,  // Clean but silent channel
    BYTESTATE_SIGNAL   = 2,  // Intentional pattern
    BYTESTATE_NOSIGNAL = 3,  // Null / silence
} ByteState;

// Calibration tuple: C = (NOISE, NONOISE, SIGNAL, NOSIGNAL)
typedef struct {
    double      noise_threshold;      // P(entropy) above this → NOISE
    double      signal_threshold;     // P(intentional) above this → SIGNAL
    int         silence_window;       // Consecutive null bytes → NOSIGNAL
} CalibrationTuple;

// Tripartite stream: S = (TRANSMITTER, RECEIVER, VERIFIER)
typedef struct {
    char        node_id[16];
    uint8_t     preamble[2];        // 0xAA 0x55
    ByteState  *vector;             // Calibration vector
    size_t      vector_len;
    bool        connected;
} NSIGII_Node;

typedef struct {
    NSIGII_Node tx;                 // Transmitter
    NSIGII_Node rx;                 // Receiver
    NSIGII_Node vrf;                // Verifier
    char        fingerprint[17];    // Shared calibration hash (16 + null)
    bool        validated;
} NSIGII_Stream;

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 8: FUNCTION DECLARATIONS — BOOT & SYSTEM
// ─────────────────────────────────────────────────────────────────────────────

// System lifecycle
MMUKO_System*   mmuko_system_create(size_t memory_size, uint32_t max_procs);
void            mmuko_system_destroy(MMUKO_System* sys);
BootStatus      mmuko_boot(MMUKO_System* sys);

// Boot phases
BootStatus phase0_vacuum_init(MMUKO_System* sys);
BootStatus phase1_cubit_init(MMUKO_System* sys);
BootStatus phase2_compass_alignment(MMUKO_System* sys);
BootStatus phase3_superposition_entanglement(MMUKO_System* sys);
BootStatus phase4_frame_centering(MMUKO_System* sys);
BootStatus phase5_nonlinear_resolution(MMUKO_System* sys);
BootStatus phase6_rotation_verification(MMUKO_System* sys);

// Cubit helpers
void        init_cubit_ring(MMUKO_Byte* byte);
CubitState  resolve_cubit_state(int index, uint8_t byte_val);
void        lookup_superposition(int base, Direction* primary, Direction* secondary);
int         round_to_even_base(int base);
const char* direction_to_string(Direction dir);
const char* cubit_state_to_string(CubitState s);
uint8_t     rotate_bits(uint8_t value, int n);
uint8_t     bit_shift_semantic(uint8_t value, ShiftOp op, int n);
CubitState  flip_cubit_state(CubitState s);

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 9: FUNCTION DECLARATIONS — SCHEDULER
// ─────────────────────────────────────────────────────────────────────────────

// Tower / Hanoi operations
mmuko_tower_t*  mmuko_tower_create(mmuko_segment_t seg, uint32_t capacity, uint64_t domain_size, const char* name);
void            mmuko_tower_destroy(mmuko_tower_t* tower);
int             mmuko_can_stack(mmuko_tower_t* tower, mmuko_pcb_t* proc);
int             mmuko_migrate(mmuko_tower_t* src, mmuko_tower_t* dst, mmuko_pcb_t* proc);

// Process lifecycle
mmuko_pcb_t*    mmuko_proc_create(uint32_t pid, mmuko_rights_t rights, uint64_t burst_time, uint64_t memory_need);
void            mmuko_proc_destroy(mmuko_pcb_t* proc);
int             mmuko_proc_allocate(mmuko_pcb_t* proc, mmuko_tower_t* tower);

// Polar Priority Heap (core algorithm)
uint64_t        mmuko_polar_priority(mmuko_pcb_t* p);
double          mmuko_bayesian_priority(mmuko_pcb_t* candidate, mmuko_tower_t* tower);
int             mmuko_compare_priority(mmuko_pcb_t* a, mmuko_pcb_t* b);

// Scheduling modes
int mmuko_schedule_preemptive(mmuko_tower_t* ready, mmuko_tower_t* running, mmuko_pcb_t* incoming);
int mmuko_schedule_non_preemptive(mmuko_tower_t* ready, mmuko_tower_t* running, mmuko_pcb_t* incoming);
int mmuko_schedule_inemptive(mmuko_tower_t* ready, mmuko_tower_t* running,
                              mmuko_inemptive_buffer_t* buffer, mmuko_pcb_t* incoming);

// Dimensional Game Theory rebalancer
int mmuko_rebalance_dimensions(mmuko_tower_t* towers[], uint32_t num_towers, uint32_t proc_count);
int mmuko_detect_strategic_imbalance(mmuko_tower_t* towers[], uint32_t num_towers);

// Scheduler main loop
int mmuko_scheduler_init(MMUKO_System* sys, mmuko_sched_mode_t mode);
int mmuko_scheduler_tick(MMUKO_System* sys);  // One scheduling quantum
int mmuko_scheduler_run(MMUKO_System* sys, uint64_t max_ticks);  // Full run

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 10: FUNCTION DECLARATIONS — CALIBRATION
// ─────────────────────────────────────────────────────────────────────────────

CalibrationTuple*   cal_tuple_create(double noise_thr, double signal_thr, int silence_win);
void                cal_tuple_destroy(CalibrationTuple* ct);
ByteState           cal_classify(CalibrationTuple* ct, const uint8_t* window, size_t win_len);
ByteState*          cal_classify_stream(CalibrationTuple* ct, const uint8_t* stream, size_t len, size_t window_size, size_t* out_count);
double              cal_entropy_score(const uint8_t* window, size_t len);
double              cal_structure_score(const uint8_t* window, size_t len);

NSIGII_Stream*      nsigii_stream_create(const char* tx_id, const char* rx_id, const char* vrf_id);
void                nsigii_stream_destroy(NSIGII_Stream* stream);
int                 nsigii_transmit(NSIGII_Stream* stream, const uint8_t* payload, size_t len);
int                 nsigii_verify(NSIGII_Stream* stream, CalibrationTuple* ct);

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 11: GOVERNANCE CHANNELS (CH_0 / CH_1 / CH_2 from RiftLang)
// ─────────────────────────────────────────────────────────────────────────────

typedef enum {
    CH_0_OBSERVE,       // YES: immediate execution
    CH_1_DEFER,         // MAYBE: conditional with retry
    CH_2_COLLAPSE       // NO: forced immediate collapse / abort
} mmuko_channel_t;

// Governance enforcement for memory allocation
mmuko_channel_t mmuko_governance_check(mmuko_pcb_t* proc, mmuko_tower_t* target);
int             mmuko_governance_enforce(mmuko_pcb_t* proc, mmuko_channel_t ch);

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 12: UTILITY / DEBUG
// ─────────────────────────────────────────────────────────────────────────────

void mmuko_print_pcb(const mmuko_pcb_t* p);
void mmuko_print_tower(const mmuko_tower_t* t);
void mmuko_print_system_status(const MMUKO_System* sys);
void mmuko_hexdump(const uint8_t* data, size_t len);
uint64_t mmuko_now_ms(void);

#ifdef __cplusplus
}
#endif

#endif // MMUKO_H
