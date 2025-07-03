#include "cartridge/cartridge.h"
#include <stdlib.h> // For malloc, free
#include <string.h> // For memcmp

bool cartridge_load(Cartridge* cart, const char* path) {
    FILE* file = fopen(path, "rb"); // Open in binary read mode
    if (!file) {
        perror("Failed to open ROM file");
        return false;
    }

    // Read the header
    iNESHeader header;
    if (fread(&header, sizeof(iNESHeader), 1, file) != 1) {
        fprintf(stderr, "Failed to read iNES header.\n");
        fclose(file);
        return false;
    }

    // Verify the magic number
    const char magic[4] = {'N', 'E', 'S', 0x1A};
    if (memcmp(header.magic, magic, 4) != 0) {
        fprintf(stderr, "Invalid .nes file: incorrect magic number.\n");
        fclose(file);
        return false;
    }

    // Calculate ROM sizes
    cart->prg_rom_len = header.prg_rom_size * 16384; // 16 * 1024
    cart->chr_rom_len = header.chr_rom_size * 8192;  // 8 * 1024

    // Extract mapper ID
    cart->mapper_id = (header.flags7 & 0xF0) | (header.flags6 >> 4);

    printf("Cartridge Loaded:\n");
    printf("  PRG ROM size: %u KB\n", cart->prg_rom_len / 1024);
    printf("  CHR ROM size: %u KB\n", cart->chr_rom_len / 1024);
    printf("  Mapper ID: %u\n", cart->mapper_id);

    // Allocate memory for ROM data
    cart->prg_rom = (u8*)malloc(cart->prg_rom_len);
    if (!cart->prg_rom) {
        fprintf(stderr, "Failed to allocate memory for PRG ROM.\n");
        fclose(file);
        return false;
    }

    // Load PRG ROM
    if (fread(cart->prg_rom, cart->prg_rom_len, 1, file) != 1) {
        fprintf(stderr, "Failed to read PRG ROM data.\n");
        free(cart->prg_rom);
        fclose(file);
        return false;
    }

    // Load CHR ROM if it exists
    if (cart->chr_rom_len > 0) {
        cart->chr_rom = (u8*)malloc(cart->chr_rom_len);
        if (!cart->chr_rom) {
            fprintf(stderr, "Failed to allocate memory for CHR ROM.\n");
            free(cart->prg_rom);
            fclose(file);
            return false;
        }
        if (fread(cart->chr_rom, cart->chr_rom_len, 1, file) != 1) {
            fprintf(stderr, "Failed to read CHR ROM data.\n");
            free(cart->prg_rom);
            free(cart->chr_rom);
            fclose(file);
            return false;
        }
    } else {
        cart->chr_rom = NULL;
    }

    fclose(file);
    return true;
}

void cartridge_free(Cartridge* cart) {
    if (cart->prg_rom) {
        free(cart->prg_rom);
        cart->prg_rom = NULL;
    }
    if (cart->chr_rom) {
        free(cart->chr_rom);
        cart->chr_rom = NULL;
    }
}