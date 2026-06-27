// nestest_smoke.c
#include <stdio.h>
#include <stdlib.h>
#include "bus.h"
#include "cartridge.h"
#include "cpu.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s nestest.nes\n", argv[0]);
        return 1;
    }

    Cartridge *cart = cartridge_load(argv[1]);
    if (!cart) return 1;

    Bus bus;
    bus_init(&bus);
    bus_load_cartridge(&bus, cart);

    CPU cpu;
    cpu_init(&cpu, &bus);
    cpu.pc = 0xC000;
    cpu.sp = 0xFD;
    cpu_set_flags(&cpu, 0x24);
    cpu.cycles = 7;

    int max_steps = 9000;
    if (argc >= 3) {
        max_steps = atoi(argv[2]);
    }

    for (int i = 0; i < max_steps; i++) {
        uint16_t pc_before = cpu.pc;
        uint8_t op0 = bus_read(&bus, pc_before);
        uint8_t op1 = bus_read(&bus, pc_before + 1);
        uint8_t op2 = bus_read(&bus, pc_before + 2);

        printf("%04X  %02X %02X %02X  A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%u\n",
               pc_before,
               op0,
               op1,
               op2,
               cpu.a,
               cpu.x,
               cpu.y,
               cpu_get_flags(&cpu),
               cpu.sp,
               cpu.cycles);

        uint8_t used_cycles = cpu_step(&cpu);
        if (used_cycles == 0) {
            printf("Stopped at unsupported opcode $%02X at $%04X\n", op0, pc_before);
            break;
        }
    }

    cartridge_free(cart);
    return 0;
}
