#ifndef MYNES_C_CPU_H
#define MYNES_C_CPU_H

#include "common/types.h"

// Forward declare Bus. CPU functions will receive it as a parameter.
typedef struct Bus Bus;

// --- Addressing Modes ---
typedef enum {
    AM_IMP, AM_ACC, AM_IMM, AM_ZP0, AM_ZPX, AM_ZPY,
    AM_REL, AM_ABS, AM_ABX, AM_ABY, AM_IND, AM_IZX, AM_IZY,
} AddressingMode;

// --- 6502 CPU Structure ---
typedef struct CPU {
    u8 a, x, y;
    u16 pc;
    u8 sp;
    u8 status;
    u32 cycles;
    // No Bus* pointer here anymore!
} CPU;

// --- Processor Status Flags ---
typedef enum {
    C_FLAG = (1 << 0), Z_FLAG = (1 << 1), I_FLAG = (1 << 2), D_FLAG = (1 << 3),
    B_FLAG = (1 << 4), U_FLAG = (1 << 5), V_FLAG = (1 << 6), N_FLAG = (1 << 7),
} CpuFlags;

// --- Function Prototypes ---
void cpu_power_up(void);
void cpu_init(CPU* cpu);
void cpu_reset(CPU* cpu, Bus* bus);
void cpu_step(CPU* cpu, Bus* bus);

#endif // MYNES_C_CPU_H