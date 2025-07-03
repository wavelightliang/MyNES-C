#ifndef MYNES_C_PPU_H
#define MYNES_C_PPU_H

#include "common/types.h"

// Forward declare Bus. PPU functions will receive it as a parameter.
typedef struct Bus Bus;

typedef struct {
    u8 vram[2048];
    u8 palette_ram[32];
    u8 oam[256]; // Object Attribute Memory

    // Registers
    u8 ppuctrl;
    u8 ppumask;
    u8 ppustatus;
    u8 oamaddr;
    // No Bus* pointer here anymore!
} PPU;

// --- Function Prototypes ---
void ppu_init(PPU* ppu);
void ppu_reset(PPU* ppu);
void ppu_step(PPU* ppu, Bus* bus);

// PPU I/O functions are now called BY the bus.
u8 ppu_read(PPU* ppu, u16 address);
void ppu_write(PPU* ppu, u16 address, u8 data);

#endif // MYNES_C_PPU_H