// bus.c
#include "bus.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void bus_init(Bus *bus) {
    memset(bus->ram, 0, sizeof(bus->ram));
    bus->cart = NULL;
    bus->ctrl[0] = malloc(sizeof(Controller));
    bus->ctrl[1] = malloc(sizeof(Controller));
    controller_init(bus->ctrl[0]);
    controller_init(bus->ctrl[1]);
}

void bus_load_cartridge(Bus *bus, Cartridge *cart) {
    bus->cart = cart;
}

uint8_t bus_read(Bus *bus, uint16_t address) {
    if (address <= 0x1FFF) {
        return bus->ram[address & 0x07FF];

    } else if (address <= 0x3FFF) {
        // uint16_t reg = address & 0x0007;
        // return ppu_read(bus->ppu, reg);
        return 0;

    } else if (address == 0x4016) {
        return controller_read(bus->ctrl[0]);

    } else if (address == 0x4017) {
        return controller_read(bus->ctrl[1]);

    } else if (address >= 0x8000) {
        if (bus->cart) {
            return bus->cart->prg_rom[(address - 0x8000) % bus->cart->prg_rom_size];
        }
        return 0;

    } else if (address >= 0x4020) {
        return 0;
    }

    return 0;
}

void bus_write(Bus *bus, uint16_t address, uint8_t value) {
    if (address <= 0x1FFF) {
        bus->ram[address & 0x07FF] = value;

    } else if (address <= 0x3FFF) {
        // uint16_t reg = address & 0x0007;
        // ppu_write(bus->ppu, reg, value);

    } else if (address == 0x4016) {
        controller_write(bus->ctrl[0], value);
        controller_write(bus->ctrl[1], value);

    } else if (address >= 0x4020) {
        // mapper_write(bus->cart, address, value);
    }
}
