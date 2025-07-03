#define SDL_MAIN_HANDLED
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "common/types.h"
#include "cartridge/cartridge.h"
#include "bus/bus.h"
#include "cpu/cpu.h"
#include "ppu/ppu.h"

const int SCREEN_SCALE = 3;
const int SCREEN_WIDTH = 256 * SCREEN_SCALE;
const int SCREEN_HEIGHT = 240 * SCREEN_SCALE;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path_to_rom.nes>\n", argv[0]);
        return 1;
    }

    Cartridge cart;
    if (!cartridge_load(&cart, argv[1])) { return 1; }

    cpu_power_up();
    Bus nes_bus;
    CPU nes_cpu;
    PPU nes_ppu;

    // Initialize individual components
    bus_init(&nes_bus);
    cpu_init(&nes_cpu);
    ppu_init(&nes_ppu);

    // --- The Great Connection ---
    // Connect all components to the bus, establishing the final hardware layout.
    bus_connect_cartridge(&nes_bus, &cart);
    bus_connect_cpu(&nes_bus, &nes_cpu);
    bus_connect_ppu(&nes_bus, &nes_ppu);

    // Now that the bus is fully connected, we can reset the components.
    cpu_reset(&nes_cpu, &nes_bus);
    ppu_reset(&nes_ppu);

    printf("Core components initialized and interconnected.\n");

    // --- SDL Setup ---
    if (SDL_Init(SDL_INIT_VIDEO) < 0) { fprintf(stderr, "SDL Error: %s\n", SDL_GetError()); return 1; }
    SDL_Window* window = SDL_CreateWindow("MyNES-C", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) { fprintf(stderr, "Window Error: %s\n", SDL_GetError()); return 1; }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) { fprintf(stderr, "Renderer Error: %s\n", SDL_GetError()); return 1; }
    printf("SDL setup successful.\n");

    // --- Main Emulation Loop ---
    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // The step functions now take the bus as an argument
        cpu_step(&nes_cpu, &nes_bus);
        ppu_step(&nes_ppu, &nes_bus);

        SDL_SetRenderDrawColor(renderer, 0x1D, 0x2B, 0x53, 0xFF);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    // --- Cleanup ---
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    cartridge_free(&cart);

    printf("Emulation finished. Shutting down.\n");
    return 0;
}