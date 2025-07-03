#ifndef MYNES_C_BUS_H
#define MYNES_C_BUS_H

#include "common/types.h"

// Forward declare all components that connect to the bus
typedef struct Cartridge Cartridge;
typedef struct CPU CPU;
typedef struct PPU PPU;

#define RAM_SIZE 2048

typedef struct Bus {
    u8 ram[RAM_SIZE];
    Cartridge* cart;
    CPU* cpu;
    PPU* ppu;
} Bus;

// --- Function Prototypes ---
void bus_init(Bus* bus);
void bus_connect_cartridge(Bus* bus, Cartridge* cart);
void bus_connect_cpu(Bus* bus, CPU* cpu);
void bus_connect_ppu(Bus* bus, PPU* ppu);
u8 bus_read(Bus* bus, u16 address);
void bus_write(Bus* bus, u16 address, u8 data);

#endif // MYNES_C_BUS_H