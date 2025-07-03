// The Golden Rule: Include your own header first.
#include "bus/bus.h"

#include <string.h>
// The bus needs the full definitions to call component functions.
#include "cartridge/cartridge.h"
#include "cpu/cpu.h"
#include "ppu/ppu.h"

void bus_init(Bus* bus) {
    memset(bus->ram, 0, RAM_SIZE);
    bus->cart = NULL;
    bus->cpu = NULL;
    bus->ppu = NULL;
}

void bus_connect_cartridge(Bus* bus, Cartridge* cart) { bus->cart = cart; }
void bus_connect_cpu(Bus* bus, CPU* cpu) { bus->cpu = cpu; }
void bus_connect_ppu(Bus* bus, PPU* ppu) { bus->ppu = ppu; }

u8 bus_read(Bus* bus, u16 address) {
    if (address <= 0x1FFF) {
        return bus->ram[address & 0x07FF];
    }
    if (address >= 0x2000 && address <= 0x3FFF) {
        return ppu_read(bus->ppu, address);
    }
    if (address >= 0x8000) {
        if (bus->cart) {
            u16 mapped_addr = (address - 0x8000);
            if (bus->cart->prg_rom_len == 16384) {
                mapped_addr %= 16384;
            }
            return bus->cart->prg_rom[mapped_addr];
        }
    }
    return 0;
}

void bus_write(Bus* bus, u16 address, u8 data) {
    if (address <= 0x1FFF) {
        bus->ram[address & 0x07FF] = data;
    } else if (address >= 0x2000 && address <= 0x3FFF) {
        ppu_write(bus->ppu, address, data);
    }
}