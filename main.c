// main.c
#include <stdio.h>
#include "bus.h"
#include "cartridge.h"
#include "cpu.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <rom.nes>\n", argv[0]);
        return 1;
    }

    Cartridge *cart = cartridge_load(argv[1]);
    if (!cart) return 1;

    printf("ROM loaded successfully!\n");
    printf("  PRG-ROM: %d KB (%d pages)\n", cart->prg_rom_size / 1024, cart->prg_rom_pages);
    printf("  CHR-%s: %d KB (%d pages)\n",
           cart->uses_chr_ram ? "RAM" : "ROM",
           cart->chr_rom_size / 1024,
           cart->chr_rom_pages);
    printf("  Mapper:     %d\n", cart->mapper_id);
    printf("  Mirroring:  %s\n", cart->mirroring ? "vertical" : "horizontal");
    printf("  Battery:    %s\n\n", cart->has_battery ? "yes" : "no");

    Bus bus;
    bus_init(&bus);
    bus_load_cartridge(&bus, cart);

    // --- Test RAM mirroring ---
    printf("RAM Mirroring Test\n");
    bus_write(&bus, 0x0000, 0x42);
    printf("Write $42 to $0000\n");
    printf("Read $0000: $%02X\n", bus_read(&bus, 0x0000));
    printf("Read $0800: $%02X\n", bus_read(&bus, 0x0800));
    printf("Read $1000: $%02X\n", bus_read(&bus, 0x1000));
    printf("Read $1800: $%02X\n", bus_read(&bus, 0x1800));

    // --- Test cartridge read ---
    printf("\nCartridge Test\n");
    printf("First PRG-ROM byte at $8000: $%02X\n", bus_read(&bus, 0x8000));

    // --- Test controller ---
    printf("\nController Test\n");

    bus.ctrl[0]->buttons[0] = true;  // A
    bus.ctrl[0]->buttons[3] = true;  // Start

    bus_write(&bus, 0x4016, 1);
    bus_write(&bus, 0x4016, 0);

    const char *btn_names[] = {"A", "B", "Select", "Start", "Up", "Down", "Left", "Right"};
    for (int i = 0; i < 8; i++) {
        uint8_t state = bus_read(&bus, 0x4016);
        printf("Button %s: %d\n", btn_names[i], state);
    }

    // --- Test CPU ---
    printf("\nCPU Test\n");

    CPU cpu;
    cpu_init(&cpu, &bus);
    cpu.pc = 0x0000;
    cpu.sp = 0xFD;
    cpu_set_flags(&cpu, 0x24);

    // LDA #$10
    // STA $20
    // INX
    // ADC #$05
    // BNE +2
    // LDA #$00   (skipped)
    bus_write(&bus, 0x0000, 0xA9);
    bus_write(&bus, 0x0001, 0x10);
    bus_write(&bus, 0x0002, 0x85);
    bus_write(&bus, 0x0003, 0x20);
    bus_write(&bus, 0x0004, 0xE8);
    bus_write(&bus, 0x0005, 0x69);
    bus_write(&bus, 0x0006, 0x05);
    bus_write(&bus, 0x0007, 0xD0);
    bus_write(&bus, 0x0008, 0x02);
    bus_write(&bus, 0x0009, 0xA9);
    bus_write(&bus, 0x000A, 0x00);

    for (int i = 0; i < 5; i++) {
        uint8_t used_cycles = cpu_step(&cpu);
        printf("Step %d: PC=$%04X A=$%02X X=$%02X Y=$%02X P=$%02X SP=$%02X cycles=%u\n",
               i + 1,
               cpu.pc,
               cpu.a,
               cpu.x,
               cpu.y,
               cpu_get_flags(&cpu),
               cpu.sp,
               used_cycles);
    }
    printf("RAM[$20]: $%02X\n", bus_read(&bus, 0x0020));

    cartridge_free(cart);
    return 0;
}
