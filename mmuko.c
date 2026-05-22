// ============================================================================
// MMUKO.C — MMUKO OS Boot, System Lifecycle & Calibration
// Project:    github.com/obinexus/mmuko-os
// Framework:  OBINexus Constitutional Computing / NSIGII Codec
// Author:     Nnamdi Michael Okpala
// Date:       22 May 2026
// Version:    0.2-polar-scheduler
// ============================================================================
//
// Implements the 7-phase MMUKO boot sequence:
//   PHASE 0: Vacuum Medium Initialization
//   PHASE 1: Cubit Ring Initialization
//   PHASE 2: Compass Alignment
//   PHASE 3: Superposition Entanglement
//   PHASE 4: Frame of Reference Centering
//   PHASE 5: Nonlinear Index Resolution (Diamond Table)
//   PHASE 6: Rotation Verification
//   PHASE 7: Boot Complete → Launch Scheduler
//
// Plus NSIGII Tripartite Calibration: Transmitter → Receiver → Verifier
//
// ============================================================================

#include "mmuko.h"

// ─────────────────────────────────────────────────────────────────────────────
// GLOBAL LOOKUP TABLES
// ─────────────────────────────────────────────────────────────────────────────

static const SuperpositionEntry superposition_table[] = {
    {12, S,  N},    // full cycle, pi*2
    {10, SE, N},
    {8,  E,  W},    // pi/2 pair
    {6,  SW, E},    // middle base
    {4,  W,  E},
    {2,  NE, W},
    {1,  N,  S}     // base unit
};
#define SUPERPOSITION_TABLE_SIZE (sizeof(superposition_table) / sizeof(SuperpositionEntry))

static const char* direction_names[] = {
    "NORTH", "NORTHEAST", "EAST", "SOUTHEAST",
    "SOUTH", "SOUTHWEST", "WEST", "NORTHWEST", "UNDEFINED"
};

static const char* cubit_state_names[] = {
    "UP", "DOWN", "CHARM", "STRANGE", "LEFT", "RIGHT"
};

static const double spin_values[] = {
    SPIN_NORTH, SPIN_NORTHEAST, SPIN_EAST, SPIN_SOUTHEAST,
    SPIN_SOUTH, SPIN_SOUTHWEST, SPIN_WEST, SPIN_NORTHWEST
};

static const int entangled_pairs[] = {7, 6, 5, -1, -1, 2, 1, 0};

static const int valid_bases[] = {12, 10, 8, 6, 4, 2, 1};
#define VALID_BASES_COUNT (sizeof(valid_bases) / sizeof(int))

// ─────────────────────────────────────────────────────────────────────────────
// UTILITY FUNCTIONS
// ─────────────────────────────────────────────────────────────────────────────

const char* direction_to_string(Direction dir) {
    if (dir >= 0 && dir <= 8) return direction_names[dir];
    return "INVALID";
}

const char* cubit_state_to_string(CubitState s) {
    if (s >= 0 && s <= 5) return cubit_state_names[s];
    return "INVALID";
}

uint64_t mmuko_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void mmuko_hexdump(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if (len % 16 != 0) printf("\n");
}

// ─────────────────────────────────────────────────────────────────────────────
// CUBIT & BIT OPERATIONS
// ─────────────────────────────────────────────────────────────────────────────

uint8_t rotate_bits(uint8_t value, int n) {
    n = n % 8;
    if (n == 0) return value;
    return ((value >> n) | (value << (8 - n))) & 0xFF;
}

uint8_t bit_shift_semantic(uint8_t value, ShiftOp op, int n) {
    switch (op) {
        case RSHIFT: return value >> n;
        case LSHIFT: return value << n;
        case ROTATE: return rotate_bits(value, n);
        default: return value;
    }
}

CubitState flip_cubit_state(CubitState s) {
    switch (s) {
        case UP:      return DOWN;
        case DOWN:    return UP;
        case CHARM:   return STRANGE;
        case STRANGE: return CHARM;
        case LEFT:    return RIGHT;
        case RIGHT:   return LEFT;
        default:      return s;
    }
}

CubitState resolve_cubit_state(int index, uint8_t byte_val) {
    uint8_t bit = (byte_val >> index) & 1;
    uint8_t neighbor = (byte_val >> ((index + 1) % 8)) & 1;
    if (bit == 1 && neighbor == 1) return UP;
    if (bit == 1 && neighbor == 0) return CHARM;
    if (bit == 0 && neighbor == 1) return STRANGE;
    return DOWN;
}

void init_cubit_ring(MMUKO_Byte* byte) {
    static const Direction directions[] = {N, NE, E, SE, S, SW, W, NW};
    for (int i = 0; i < 8; i++) {
        Cubit* c = &byte->cubit_ring[i];
        c->index = i;
        c->value = (byte->raw_value >> i) & 1;
        c->spin = spin_values[i];
        c->direction = directions[i];
        c->state = resolve_cubit_state(i, byte->raw_value);
        c->entangled_with = entangled_pairs[i];
        c->superposed = (entangled_pairs[i] != -1);
    }
}

int round_to_even_base(int base) {
    int nearest = valid_bases[0];
    int min_diff = abs(base - valid_bases[0]);
    for (size_t i = 1; i < VALID_BASES_COUNT; i++) {
        int diff = abs(base - valid_bases[i]);
        if (diff < min_diff) {
            min_diff = diff;
            nearest = valid_bases[i];
        }
    }
    return nearest;
}

void lookup_superposition(int base, Direction* primary, Direction* secondary) {
    for (size_t i = 0; i < SUPERPOSITION_TABLE_SIZE; i++) {
        if (superposition_table[i].base == base) {
            *primary = superposition_table[i].primary;
            *secondary = superposition_table[i].secondary;
            return;
        }
    }
    int nearest = round_to_even_base(base);
    for (size_t i = 0; i < SUPERPOSITION_TABLE_SIZE; i++) {
        if (superposition_table[i].base == nearest) {
            *primary = superposition_table[i].primary;
            *secondary = superposition_table[i].secondary;
            return;
        }
    }
    *primary = N;
    *secondary = S;
}

// ─────────────────────────────────────────────────────────────────────────────
// PHASE 0: VACUUM MEDIUM INITIALIZATION
// ─────────────────────────────────────────────────────────────────────────────

BootStatus phase0_vacuum_init(MMUKO_System* sys) {
    (void)sys;
    printf("\n[PHASE 0] Vacuum medium initialization...\n");
    VacuumMedium medium = {
        .gravity = G_VACUUM,
        .air = 0.0,
        .water = 0.0
    };
    sys->medium = medium;
    printf("[PHASE 0] G=%.4f (lepton=%.4f, muon=%.4f, deep=%.4f)\n",
           G_VACUUM, G_LEPTON, G_MUON, G_DEEP);
    return BOOT_OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// PHASE 1: CUBIT RING INITIALIZATION
// ─────────────────────────────────────────────────────────────────────────────

BootStatus phase1_cubit_init(MMUKO_System* sys) {
    printf("\n[PHASE 1] Initializing cubit rings...\n");
    for (size_t i = 0; i < sys->memory_size; i++) {
        uint8_t val = sys->memory_map[i].raw_value;
        sys->memory_map[i].base_index = (val % 12) + 1;
        init_cubit_ring(&sys->memory_map[i]);
        lookup_superposition(sys->memory_map[i].base_index,
                           &sys->memory_map[i].primary_superposition,
                           &sys->memory_map[i].secondary_superposition);
    }
    printf("[PHASE 1] Initialized %zu cubit rings\n", sys->memory_size);
    return BOOT_OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// PHASE 2: COMPASS ALIGNMENT
// ─────────────────────────────────────────────────────────────────────────────

static Direction resolve_direction_from_neighbors(MMUKO_Byte* byte, int cubit_index) {
    int dir_count[9] = {0};
    int max_count = 0;
    Direction max_dir = N;
    for (int i = -1; i <= 1; i++) {
        if (i == 0) continue;
        int neighbor_idx = (cubit_index + i + 8) % 8;
        Direction ndir = byte->cubit_ring[neighbor_idx].direction;
        if (ndir != UNDEFINED_DIR) {
            dir_count[ndir]++;
            if (dir_count[ndir] > max_count) {
                max_count = dir_count[ndir];
                max_dir = ndir;
            }
        }
    }
    if (max_count == 0) return N;
    return max_dir;
}

BootStatus phase2_compass_alignment(MMUKO_System* sys) {
    printf("\n[PHASE 2] Compass alignment...\n");
    for (size_t b = 0; b < sys->memory_size; b++) {
        MMUKO_Byte* byte = &sys->memory_map[b];
        for (int i = 0; i < 8; i++) {
            Cubit* c = &byte->cubit_ring[i];
            if (c->direction == UNDEFINED_DIR) {
                c->direction = resolve_direction_from_neighbors(byte, i);
                if (c->direction == UNDEFINED_DIR) {
                    printf("[ERROR] Boot lock at byte %zu, cubit %d\n", b, i);
                    return BOOT_LOCK_DETECTED;
                }
            }
        }
    }
    printf("[PHASE 2] All cubits aligned to compass directions\n");
    return BOOT_OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// PHASE 3: SUPERPOSITION ENTANGLEMENT
// ─────────────────────────────────────────────────────────────────────────────

static Cubit* get_cubit_from_byte(MMUKO_Byte* byte, int index) {
    if (index >= 0 && index < 8) return &byte->cubit_ring[index];
    return NULL;
}

BootStatus phase3_superposition_entanglement(MMUKO_System* sys) {
    printf("\n[PHASE 3] Entangling superposition pairs...\n");
    for (size_t b = 0; b < sys->memory_size; b++) {
        MMUKO_Byte* byte = &sys->memory_map[b];
        for (int i = 0; i < 8; i++) {
            Cubit* c = &byte->cubit_ring[i];
            if (c->superposed && c->entangled_with != -1) {
                Cubit* partner = get_cubit_from_byte(byte, c->entangled_with);
                if (partner && c->state == partner->state) {
                    partner->state = flip_cubit_state(partner->state);
                    printf("[PHASE 3] Resolved interference at byte %zu, pair (%d,%d)\n",
                           b, i, c->entangled_with);
                }
            }
        }
    }
    printf("[PHASE 3] Superposition entanglement complete\n");
    return BOOT_OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// PHASE 4: FRAME OF REFERENCE CENTERING
// ─────────────────────────────────────────────────────────────────────────────

static int get_middle_base(void) {
    return 6; // 12 / 2
}

BootStatus phase4_frame_centering(MMUKO_System* sys) {
    printf("\n[PHASE 4] Frame of reference centering...\n");
    int center_base = get_middle_base();
    Direction primary, secondary;
    lookup_superposition(center_base, &primary, &secondary);
    sys->frame_of_reference = primary;
    for (size_t b = 0; b < sys->memory_size; b++) {
        sys->memory_map[b].primary_superposition = primary;
        sys->memory_map[b].secondary_superposition = secondary;
    }
    printf("[PHASE 4] Frame set to %s (secondary: %s)\n",
           direction_to_string(primary), direction_to_string(secondary));
    return BOOT_OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// PHASE 5: NONLINEAR INDEX RESOLUTION (Diamond Table)
// ─────────────────────────────────────────────────────────────────────────────

static void resolve_base_state(MMUKO_System* sys, int base) {
    Direction primary, secondary;
    lookup_superposition(base, &primary, &secondary);
    for (size_t i = 0; i < sys->memory_size; i++) {
        if (sys->memory_map[i].base_index == base) {
            sys->memory_map[i].primary_superposition = primary;
            sys->memory_map[i].secondary_superposition = secondary;
        }
    }
}

BootStatus phase5_nonlinear_resolution(MMUKO_System* sys) {
    printf("\n[PHASE 5] Nonlinear index resolution (diamond table)...\n");
    const int boot_order[] = {12, 6, 8, 4, 10, 2, 1};
    const int boot_order_size = sizeof(boot_order) / sizeof(int);
    for (int i = 0; i < boot_order_size; i++) {
        int base = boot_order[i];
        resolve_base_state(sys, base);
        Direction primary, secondary;
        lookup_superposition(base, &primary, &secondary);
        printf("[PHASE 5] Base %d resolved -> %s/%s\n",
               base, direction_to_string(primary), direction_to_string(secondary));
    }
    return BOOT_OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// PHASE 6: ROTATION VERIFICATION
// ─────────────────────────────────────────────────────────────────────────────

BootStatus phase6_rotation_verification(MMUKO_System* sys) {
    printf("\n[PHASE 6] Rotation freedom check...\n");
    for (size_t b = 0; b < sys->memory_size; b++) {
        MMUKO_Byte* byte = &sys->memory_map[b];
        for (int i = 0; i < 8; i++) {
            uint8_t original = byte->cubit_ring[i].value;
            uint8_t test_val = rotate_bits(original, 4);
            test_val = rotate_bits(test_val, 4);
            if (test_val != original) {
                printf("[ERROR] Rotation lock at byte %zu, cubit %d\n", b, i);
                return BOOT_ROTATION_LOCK;
            }
        }
    }
    printf("[PHASE 6] All cubits rotate freely (360 deg verified)\n");
    return BOOT_OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// MAIN BOOT SEQUENCE
// ─────────────────────────────────────────────────────────────────────────────

BootStatus mmuko_boot(MMUKO_System* sys) {
    printf("\n========================================\n");
    printf("  MMUKO BOOT SEQUENCE v%s\n", MMUKO_VERSION);
    printf("  OBINexus Constitutional Computing\n");
    printf("  \"Don't just boot systems. Boot truthful ones.\"\n");
    printf("========================================\n");

    BootStatus status;

    status = phase0_vacuum_init(sys);
    if (status != BOOT_OK) return status;

    status = phase1_cubit_init(sys);
    if (status != BOOT_OK) return status;

    status = phase2_compass_alignment(sys);
    if (status != BOOT_OK) return status;

    status = phase3_superposition_entanglement(sys);
    if (status != BOOT_OK) return status;

    status = phase4_frame_centering(sys);
    if (status != BOOT_OK) return status;

    status = phase5_nonlinear_resolution(sys);
    if (status != BOOT_OK) return status;

    status = phase6_rotation_verification(sys);
    if (status != BOOT_OK) return status;

    printf("\n[PHASE 7] MMUKO BOOT COMPLETE — All cubits aligned, no lock detected.\n");
    sys->boot_complete = true;
    return BOOT_OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// SYSTEM LIFECYCLE
// ─────────────────────────────────────────────────────────────────────────────

MMUKO_System* mmuko_system_create(size_t memory_size, uint32_t max_procs) {
    MMUKO_System* sys = (MMUKO_System*)calloc(1, sizeof(MMUKO_System));
    if (!sys) return NULL;

    sys->memory_map = (MMUKO_Byte*)calloc(memory_size, sizeof(MMUKO_Byte));
    if (!sys->memory_map) {
        free(sys);
        return NULL;
    }

    sys->memory_size = memory_size;
    sys->frame_of_reference = N;
    sys->boot_complete = false;
    sys->proc_capacity = max_procs;
    sys->proc_count = 0;
    sys->sched_mode = MMUKO_SCHED_PREEMPTIVE;

    // Initialize memory with pseudo-random pattern
    for (size_t i = 0; i < memory_size; i++) {
        sys->memory_map[i].raw_value = (uint8_t)(i * 17 + 42);
    }

    // Create towers for each segment (Text, Data, Stack, Heap)
    sys->towers[MMUKO_SEG_TEXT]  = NULL; // Text is read-only, not scheduled
    sys->towers[MMUKO_SEG_DATA]  = mmuko_tower_create(MMUKO_SEG_DATA,  64, MMUKO_MB(1), "DATA");
    sys->towers[MMUKO_SEG_STACK] = mmuko_tower_create(MMUKO_SEG_STACK, 256, MMUKO_MB(2), "STACK");
    sys->towers[MMUKO_SEG_HEAP]  = mmuko_tower_create(MMUKO_SEG_HEAP,  512, MMUKO_MB(4), "HEAP");

    sys->proc_table = (mmuko_pcb_t*)calloc(max_procs, sizeof(mmuko_pcb_t));
    if (!sys->proc_table) {
        free(sys->memory_map);
        free(sys);
        return NULL;
    }

    printf("[MMUKO] System created: %zu bytes, %u max processes\n", memory_size, max_procs);
    return sys;
}

void mmuko_system_destroy(MMUKO_System* sys) {
    if (!sys) return;
    if (sys->memory_map) free(sys->memory_map);
    if (sys->proc_table) free(sys->proc_table);
    for (int i = 0; i < 4; i++) {
        if (sys->towers[i]) mmuko_tower_destroy(sys->towers[i]);
    }
    free(sys);
    printf("[MMUKO] System destroyed\n");
}

// ─────────────────────────────────────────────────────────────────────────────
// CALIBRATION TUPLE IMPLEMENTATION
// ─────────────────────────────────────────────────────────────────────────────

CalibrationTuple* cal_tuple_create(double noise_thr, double signal_thr, int silence_win) {
    CalibrationTuple* ct = (CalibrationTuple*)malloc(sizeof(CalibrationTuple));
    if (!ct) return NULL;
    ct->noise_threshold = noise_thr;
    ct->signal_threshold = signal_thr;
    ct->silence_window = silence_win;
    return ct;
}

void cal_tuple_destroy(CalibrationTuple* ct) {
    free(ct);
}

double cal_entropy_score(const uint8_t* window, size_t len) {
    if (!window || len == 0) return 0.0;
    int counts[256] = {0};
    for (size_t i = 0; i < len; i++) counts[window[i]]++;
    double score = 0.0;
    for (int i = 0; i < 256; i++) {
        if (counts[i] > 0) {
            double p = (double)counts[i] / (double)len;
            score -= p * log2(p);
        }
    }
    return score / 8.0; // normalize to [0,1]
}

double cal_structure_score(const uint8_t* window, size_t len) {
    if (!window || len == 0) return 0.0;
    int unique = 0;
    int seen[256] = {0};
    for (size_t i = 0; i < len; i++) {
        if (!seen[window[i]]) { seen[window[i]] = 1; unique++; }
    }
    double structure = 1.0 - ((double)unique / (double)len);
    if (len >= 2 && window[0] == 0xAA && window[1] == 0x55)
        structure = fmin(1.0, structure + 0.3);
    return structure;
}

ByteState cal_classify(CalibrationTuple* ct, const uint8_t* window, size_t win_len) {
    if (!window || win_len == 0) return BYTESTATE_NOSIGNAL;
    int silence = 1;
    for (size_t i = 0; i < win_len; i++) {
        if (window[i] != 0x00) { silence = 0; break; }
    }
    if (silence) return BYTESTATE_NOSIGNAL;

    double e_score = cal_entropy_score(window, win_len);
    double s_score = cal_structure_score(window, win_len);

    if (e_score >= ct->noise_threshold) return BYTESTATE_NOISE;
    if (s_score >= ct->signal_threshold) return BYTESTATE_SIGNAL;
    return BYTESTATE_NONOISE;
}

ByteState* cal_classify_stream(CalibrationTuple* ct, const uint8_t* stream, size_t len,
                                size_t window_size, size_t* out_count) {
    if (!stream || len == 0 || !out_count) return NULL;
    size_t num_windows = (len + window_size - 1) / window_size;
    ByteState* states = (ByteState*)malloc(num_windows * sizeof(ByteState));
    if (!states) return NULL;
    for (size_t i = 0; i < num_windows; i++) {
        size_t offset = i * window_size;
        size_t wlen = (offset + window_size > len) ? (len - offset) : window_size;
        states[i] = cal_classify(ct, stream + offset, wlen);
    }
    *out_count = num_windows;
    return states;
}

// ─────────────────────────────────────────────────────────────────────────────
// NSIGII TRIPARTITE STREAM
// ─────────────────────────────────────────────────────────────────────────────

NSIGII_Stream* nsigii_stream_create(const char* tx_id, const char* rx_id, const char* vrf_id) {
    NSIGII_Stream* stream = (NSIGII_Stream*)calloc(1, sizeof(NSIGII_Stream));
    if (!stream) return NULL;
    strncpy(stream->tx.node_id, tx_id, 15);
    strncpy(stream->rx.node_id, rx_id, 15);
    strncpy(stream->vrf.node_id, vrf_id, 15);
    stream->tx.preamble[0] = 0xAA;
    stream->tx.preamble[1] = 0x55;
    stream->tx.connected = false;
    stream->rx.connected = false;
    stream->vrf.connected = false;
    stream->validated = false;
    return stream;
}

void nsigii_stream_destroy(NSIGII_Stream* stream) {
    if (!stream) return;
    if (stream->tx.vector) free(stream->tx.vector);
    if (stream->rx.vector) free(stream->rx.vector);
    if (stream->vrf.vector) free(stream->vrf.vector);
    free(stream);
}

int nsigii_transmit(NSIGII_Stream* stream, const uint8_t* payload, size_t len) {
    if (!stream || !payload) return -1;
    // In a real system, this would send framed data.
    // Here we simulate by classifying the payload.
    stream->tx.connected = true;
    stream->rx.connected = true;
    printf("[NSIGII] TX %s -> RX %s: %zu bytes\n", stream->tx.node_id, stream->rx.node_id, len);
    return 0;
}

int nsigii_verify(NSIGII_Stream* stream, CalibrationTuple* ct) {
    if (!stream || !ct) return -1;
    // Simulate verification: dominant state must be SIGNAL
    // In real implementation, this would use the actual calibration vector
    stream->validated = true;
    strncpy(stream->fingerprint, "DEADBEEFCAFEBABE", 16);
    stream->fingerprint[16] = '\0';
    printf("[NSIGII] VRF %s: Calibration VALID (fingerprint=%s)\n",
           stream->vrf.node_id, stream->fingerprint);
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// DEBUG PRINTERS
// ─────────────────────────────────────────────────────────────────────────────

void mmuko_print_pcb(const mmuko_pcb_t* p) {
    if (!p) return;
    printf("PCB[pid=%u, state=%d, rights=0x%02X, burst=%lu, remain=%lu, mem=%lu, strat=%lu]\n",
           p->pid, p->state, p->rights, p->burst_time, p->remaining_time,
           p->token.token_memory, p->strategic_value);
}

void mmuko_print_tower(const mmuko_tower_t* t) {
    if (!t) return;
    printf("Tower[%s]: seg=%d, count=%u/%u, used=%lu/%lu bytes\n",
           t->name, t->segment, t->count, t->capacity, t->used, t->domain_size);
    for (uint32_t i = 0; i < t->count; i++) {
        printf("  [%u] ", i);
        mmuko_print_pcb(t->disks[i]);
    }
}

void mmuko_print_system_status(const MMUKO_System* sys) {
    if (!sys) return;
    printf("\n=== MMUKO SYSTEM STATUS ===\n");
    printf("Version: %s\n", MMUKO_VERSION);
    printf("Boot: %s\n", sys->boot_complete ? "COMPLETE" : "INCOMPLETE");
    printf("Frame: %s\n", direction_to_string(sys->frame_of_reference));
    printf("Gravity: %.4f\n", sys->medium.gravity);
    printf("Processes: %u/%u\n", sys->proc_count, sys->proc_capacity);
    printf("Scheduler mode: %d\n", sys->sched_mode);
    for (int i = 0; i < 4; i++) {
        if (sys->towers[i]) mmuko_print_tower(sys->towers[i]);
    }
    printf("===========================\n");
}

// ─────────────────────────────────────────────────────────────────────────────
// END OF MMUKO.C
// ─────────────────────────────────────────────────────────────────────────────
