// cartridge.c
#include "cartridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t INES_MAGIC[] = { 0x4E, 0x45, 0x53, 0x1A };

Cartridge *cartridge_load(const char *filepath) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        fprintf(stderr, "Error: could not open %s\n", filepath);
        return NULL;
    }

    // Read the 16-byte header
    uint8_t header[16];
    if (fread(header, 1, 16, fp) != 16) {
        fprintf(stderr, "Error: file too small for iNES header\n");
        fclose(fp);
        return NULL;
    }

    // Validate magic constant
    if (memcmp(header, INES_MAGIC, 4) != 0) {
        fprintf(stderr, "Error: invalid ROM (magic constant mismatch)\n");
        fclose(fp);
        return NULL;
    }

    Cartridge *cart = calloc(1, sizeof(Cartridge));

    // Parse header fields
    cart->prg_rom_pages = header[4];
    cart->chr_rom_pages = header[5];
    cart->uses_chr_ram  = (header[5] == 0);
    cart->mirroring     = header[6] & 0x01;
    cart->has_battery   = (header[6] >> 1) & 0x01;
    cart->has_trainer   = (header[6] >> 2) & 0x01;
    cart->mapper_id     = (header[7] & 0xF0) | (header[6] >> 4);

    // Skip trainer if present
    if (cart->has_trainer) {
        fseek(fp, 512, SEEK_CUR);
    }

    // Read PRG-ROM
    cart->prg_rom_size = cart->prg_rom_pages * 16384;
    cart->prg_rom = malloc(cart->prg_rom_size);
    if (fread(cart->prg_rom, 1, cart->prg_rom_size, fp) != cart->prg_rom_size) {
        fprintf(stderr, "Error: unexpected end of PRG-ROM data\n");
        cartridge_free(cart);
        fclose(fp);
        return NULL;
    }

    // Read CHR-ROM (or allocate CHR-RAM)
    if (cart->uses_chr_ram) {
        cart->chr_rom_size = 8192;  // 8 KB of CHR-RAM
        cart->chr_rom = calloc(1, cart->chr_rom_size);
    } else {
        cart->chr_rom_size = cart->chr_rom_pages * 8192;
        cart->chr_rom = malloc(cart->chr_rom_size);
        if (fread(cart->chr_rom, 1, cart->chr_rom_size, fp) != cart->chr_rom_size) {
            fprintf(stderr, "Error: unexpected end of CHR-ROM data\n");
            cartridge_free(cart);
            fclose(fp);
            return NULL;
        }
    }

    fclose(fp);
    return cart;
}

void cartridge_free(Cartridge *cart) {
    if (cart) {
        free(cart->prg_rom);
        free(cart->chr_rom);
        free(cart);
    }
}
