// cartridge.h
#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t *prg_rom;       // program code
    uint8_t *chr_rom;       // graphics data (or CHR-RAM)
    uint32_t prg_rom_size;  // in bytes
    uint32_t chr_rom_size;  // in bytes

    // Header info
    uint8_t  prg_rom_pages; // in 16 KB units
    uint8_t  chr_rom_pages; // in 8 KB units
    uint8_t  mapper_id;
    bool     uses_chr_ram;
    bool     has_battery;
    bool     has_trainer;
    uint8_t  mirroring;     // 0 = horizontal, 1 = vertical
} Cartridge;

Cartridge *cartridge_load(const char *filepath);
void       cartridge_free(Cartridge *cart);

#endif
