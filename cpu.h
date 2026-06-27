// cpu.h
#ifndef CPU_H
#define CPU_H

#include <stdbool.h>
#include <stdint.h>
#include "bus.h"

typedef enum {
    ADDR_IMPLICIT,          // No operand. Example: INX, RTS, NOP.
    ADDR_ACCUMULATOR,       // The instruction works directly on A. Example: ASL A.
    ADDR_IMMEDIATE,         // Operand byte is the value. Example: LDA #$10.
    ADDR_ZERO_PAGE,         // One byte address in $0000 to $00FF.
    ADDR_ZERO_PAGE_X,       // Zero page address plus X, wrapped to one byte.
    ADDR_ZERO_PAGE_Y,       // Zero page address plus Y, wrapped to one byte.
    ADDR_ABSOLUTE,          // Full 16-bit address from the next two bytes.
    ADDR_ABSOLUTE_X,        // Absolute address plus X. Some reads cost 1 extra cycle on page cross.
    ADDR_ABSOLUTE_Y,        // Absolute address plus Y. Some reads cost 1 extra cycle on page cross.
    ADDR_INDIRECT,          // Used by JMP only. Includes the original 6502 page wrap bug.
    ADDR_INDEXED_INDIRECT,  // ($ZP,X). Add X first, then read a 16-bit pointer from zero page.
    ADDR_INDIRECT_INDEXED,  // ($ZP),Y. Read pointer first, then add Y.
    ADDR_RELATIVE,          // Signed branch offset from the PC after the operand byte.
} AddressingMode;

typedef struct {
    // CPU registers. These are intentionally plain integers because unsigned
    // C types already wrap the same way 8-bit and 16-bit CPU registers do.
    uint8_t  a;      // Accumulator
    uint8_t  x;      // Index X
    uint8_t  y;      // Index Y
    uint8_t  sp;     // Stack Pointer
    uint16_t pc;     // Program Counter

    // Processor status flags. Bit 5 is not stored here because it is always
    // forced on when the flags are packed into the status byte.
    bool flag_c;     // Carry
    bool flag_z;     // Zero
    bool flag_i;     // Interrupt Disable
    bool flag_d;     // Decimal (unused on NES, but tracked)
    bool flag_v;     // Overflow
    bool flag_n;     // Negative

    // Total CPU cycles executed so far. extra_cycles is reset each instruction
    // and is used for branches and a few page-crossing cases.
    uint32_t cycles;
    uint8_t  extra_cycles;

    // The CPU never talks to RAM or ROM directly. Every memory access goes
    // through the bus so the address map stays in one place.
    Bus *bus;
} CPU;

void     cpu_init(CPU *cpu, Bus *bus);
void     cpu_reset(CPU *cpu);
uint8_t  cpu_step(CPU *cpu);
void     cpu_interrupt_nmi(CPU *cpu);
void     cpu_interrupt_irq(CPU *cpu);

uint8_t  cpu_get_flags(CPU *cpu);
void     cpu_set_flags(CPU *cpu, uint8_t value);
void     cpu_update_zero_and_negative(CPU *cpu, uint8_t value);
uint16_t cpu_read16(CPU *cpu, uint16_t address);
void     cpu_push(CPU *cpu, uint8_t value);
uint8_t  cpu_pop(CPU *cpu);
void     cpu_push16(CPU *cpu, uint16_t value);
uint16_t cpu_pop16(CPU *cpu);
uint16_t cpu_resolve_address(CPU *cpu, AddressingMode mode, bool *page_crossed);

#endif
