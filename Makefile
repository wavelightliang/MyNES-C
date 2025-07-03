# =============================================================================
# Makefile for MyNES-C Project
# Version: 1.2
# Author: Your AI Professor
# Change: Re-enabled SDL2 linking for graphical output.
# =============================================================================

# --- 1. Compiler and Tools ---
CC = gcc

# --- 2. Build Directories ---
SRC_DIR := src
OBJ_DIR := obj
INC_DIR := include

# --- 3. Build Executable ---
EXECUTABLE := MyNES-C.exe

# --- 4. Compiler Flags ---
CFLAGS := -std=c11 -Wall -Wextra -g -I$(SRC_DIR)

# --- 5. Linker Flags and Libraries ---
LDFLAGS :=
# For SDL2 on MinGW, we need to link both SDL2main and SDL2.
LIBS := -lSDL2main -lSDL2

# --- 6. Source File Discovery (Automatic) ---
SOURCES := $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))

# --- 7. Build Targets (The Recipes) ---

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	@echo "Linking..."
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)
	@echo "Build finished: $(EXECUTABLE)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	@echo "Running application..."
	./$(EXECUTABLE)

clean:
	@echo "Cleaning up..."
	@rm -rf $(OBJ_DIR) $(EXECUTABLE) mynes.log

.PHONY: all run clean