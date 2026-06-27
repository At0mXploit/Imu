// bus.h
#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include "cartridge.h"
#include "controller.h"

typedef struct {
    uint8_t ram[2048];       // 2 KB of internal RAM
    Cartridge *cart;          // loaded cartridge
    // PPU *ppu;              // we'll add these later
    // APU *apu;
    Controller *ctrl[2];
} Bus;

void     bus_init(Bus *bus);
void     bus_load_cartridge(Bus *bus, Cartridge *cart);
uint8_t  bus_read(Bus *bus, uint16_t address);
void     bus_write(Bus *bus, uint16_t address, uint8_t value);

#endif
