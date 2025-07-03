// The Golden Rule: Include your own header first.
#include "ppu/ppu.h"

#include "bus/bus.h"
#include <string.h>

void ppu_init(PPU* ppu) {
    memset(ppu, 0, sizeof(PPU));
}

void ppu_reset(PPU* ppu) {
    ppu->ppuctrl = 0;
    ppu->ppumask = 0;
    ppu->ppustatus = 0;
    ppu->oamaddr = 0;
}

void ppu_step(PPU* ppu, Bus* bus) {
    (void)ppu;
    (void)bus;
}

u8 ppu_read(PPU* ppu, u16 address) {
    u16 mirrored_addr = 0x2000 | (address & 0x0007);
    switch (mirrored_addr) {
        case 0x2002: {
            u8 data = (ppu->ppustatus & 0xE0);
            ppu->ppustatus &= ~0x80; // Clear VBlank flag after reading
            return data;
        }
    }
    return 0;
}

void ppu_write(PPU* ppu, u16 address, u8 data) {
    u16 mirrored_addr = 0x2000 | (address & 0x0007);
    switch (mirrored_addr) {
        case 0x2000: ppu->ppuctrl = data; break;
        case 0x2001: ppu->ppumask = data; break;
    }
}