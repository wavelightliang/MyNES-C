#ifndef MYNES_C_CARTRIDGE_H
#define MYNES_C_CARTRIDGE_H

#include "common/types.h"
#include <stdio.h>

// --- iNES File Header Structure ---
// This struct directly maps to the 16-byte header of an iNES file.
#pragma pack(push, 1) // Ensure no padding is added by the compiler
typedef struct {
    char magic[4];      // Should be "NES\x1A"
    u8 prg_rom_size;    // Size of PRG ROM in 16 KB units
    u8 chr_rom_size;    // Size of CHR ROM in 8 KB units (0 means CHR RAM)
    u8 flags6;
    u8 flags7;
    u8 flags8_prg_ram;
    u8 flags9;
    u8 flags10;
    u8 reserved[5];
} iNESHeader;
#pragma pack(pop)

// --- Cartridge Structure ---
// This holds the loaded game data.
typedef struct {
    u8* prg_rom; // Pointer to the start of PRG ROM data
    u32 prg_rom_len;

    u8* chr_rom; // Pointer to the start of CHR ROM data
    u32 chr_rom_len;

    u8 mapper_id;
    // We will add more mapper-specific state here later.
} Cartridge;

// --- Function Prototypes ---

/**
 * @brief Loads a .nes file into the cartridge structure.
 * This function reads the header, allocates memory, and loads the ROM data.
 * @param cart A pointer to the Cartridge structure to fill.
 * @param path The file path to the .nes ROM file.
 * @return true if loading was successful, false otherwise.
 */
bool cartridge_load(Cartridge* cart, const char* path);

/**
 * @brief Frees the memory allocated for the cartridge's ROM data.
 * @param cart A pointer to the Cartridge structure.
 */
void cartridge_free(Cartridge* cart);

#endif // MYNES_C_CARTRIDGE_H