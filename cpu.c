// cpu.c
#include "cpu.h"
#include <stdio.h>
#include <string.h>

typedef void (*InstructionFunc)(CPU *cpu, uint16_t address, AddressingMode mode);

// One table entry describes exactly one opcode byte. For example, LDA has
// several entries because immediate, zero page, and absolute all have different
// opcode values and cycle counts.
typedef struct {
    const char *name;
    InstructionFunc execute;
    AddressingMode mode;
    uint8_t cycles;
    bool page_cross_penalty;
} Opcode;

static Opcode opcodes[256];
static bool opcodes_ready = false;

// The handlers are private to this file. cpu_step only needs the opcode table,
// and the rest of the emulator should not call individual instructions directly.
static void op_adc(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_sbc(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_lda(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_ldx(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_ldy(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_sta(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_stx(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_sty(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_inx(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_iny(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_dex(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_dey(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_inc(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_dec(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_asl(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_lsr(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_rol(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_ror(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_cmp(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_cpx(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_cpy(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_bcc(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_bcs(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_beq(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_bne(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_bmi(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_bpl(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_bvc(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_bvs(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_jmp(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_jsr(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_rts(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_rti(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_pha(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_pla(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_php(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_plp(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_nop(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_brk(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_ora(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_and(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_eor(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_bit(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_tax(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_tay(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_tsx(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_txa(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_txs(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_tya(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_clc(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_cld(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_cli(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_clv(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_sec(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_sed(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_sei(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_lax(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_sax(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_dcp(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_isc(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_slo(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_rla(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_sre(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_rra(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_anc(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_alr(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_arr(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_axs(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_las(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_ahx(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_shx(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_shy(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_tas(CPU *cpu, uint16_t addr, AddressingMode mode);
static void op_xaa(CPU *cpu, uint16_t addr, AddressingMode mode);

// Immediate mode also returns an address, not a raw value, so instruction
// handlers can read their operand the same way for every mode.
static uint8_t cpu_read_operand(CPU *cpu, uint16_t addr) {
    return bus_read(cpu->bus, addr);
}

// Small helper to keep the big opcode table readable.
static void set_opcode(uint8_t code, const char *name, InstructionFunc execute,
                       AddressingMode mode, uint8_t cycles, bool page_cross_penalty) {
    opcodes[code] = (Opcode){name, execute, mode, cycles, page_cross_penalty};
}

static void cpu_init_opcodes(void) {
    memset(opcodes, 0, sizeof(opcodes));

    // page_cross_penalty is only for read-style addressing modes where the
    // 6502 adds one cycle after crossing a 256-byte page. Stores and read-write
    // instructions have fixed cycle counts, so they keep this false.

    // Arithmetic
    set_opcode(0x69, "ADC", op_adc, ADDR_IMMEDIATE,        2, false);
    set_opcode(0x65, "ADC", op_adc, ADDR_ZERO_PAGE,        3, false);
    set_opcode(0x75, "ADC", op_adc, ADDR_ZERO_PAGE_X,      4, false);
    set_opcode(0x6D, "ADC", op_adc, ADDR_ABSOLUTE,         4, false);
    set_opcode(0x7D, "ADC", op_adc, ADDR_ABSOLUTE_X,       4, true);
    set_opcode(0x79, "ADC", op_adc, ADDR_ABSOLUTE_Y,       4, true);
    set_opcode(0x61, "ADC", op_adc, ADDR_INDEXED_INDIRECT, 6, false);
    set_opcode(0x71, "ADC", op_adc, ADDR_INDIRECT_INDEXED, 5, true);

    set_opcode(0xE9, "SBC", op_sbc, ADDR_IMMEDIATE,        2, false);
    set_opcode(0xEB, "SBC", op_sbc, ADDR_IMMEDIATE,        2, false);
    set_opcode(0xE5, "SBC", op_sbc, ADDR_ZERO_PAGE,        3, false);
    set_opcode(0xF5, "SBC", op_sbc, ADDR_ZERO_PAGE_X,      4, false);
    set_opcode(0xED, "SBC", op_sbc, ADDR_ABSOLUTE,         4, false);
    set_opcode(0xFD, "SBC", op_sbc, ADDR_ABSOLUTE_X,       4, true);
    set_opcode(0xF9, "SBC", op_sbc, ADDR_ABSOLUTE_Y,       4, true);
    set_opcode(0xE1, "SBC", op_sbc, ADDR_INDEXED_INDIRECT, 6, false);
    set_opcode(0xF1, "SBC", op_sbc, ADDR_INDIRECT_INDEXED, 5, true);

    // Logical
    set_opcode(0x09, "ORA", op_ora, ADDR_IMMEDIATE,        2, false);
    set_opcode(0x05, "ORA", op_ora, ADDR_ZERO_PAGE,        3, false);
    set_opcode(0x15, "ORA", op_ora, ADDR_ZERO_PAGE_X,      4, false);
    set_opcode(0x0D, "ORA", op_ora, ADDR_ABSOLUTE,         4, false);
    set_opcode(0x1D, "ORA", op_ora, ADDR_ABSOLUTE_X,       4, true);
    set_opcode(0x19, "ORA", op_ora, ADDR_ABSOLUTE_Y,       4, true);
    set_opcode(0x01, "ORA", op_ora, ADDR_INDEXED_INDIRECT, 6, false);
    set_opcode(0x11, "ORA", op_ora, ADDR_INDIRECT_INDEXED, 5, true);

    set_opcode(0x29, "AND", op_and, ADDR_IMMEDIATE,        2, false);
    set_opcode(0x25, "AND", op_and, ADDR_ZERO_PAGE,        3, false);
    set_opcode(0x35, "AND", op_and, ADDR_ZERO_PAGE_X,      4, false);
    set_opcode(0x2D, "AND", op_and, ADDR_ABSOLUTE,         4, false);
    set_opcode(0x3D, "AND", op_and, ADDR_ABSOLUTE_X,       4, true);
    set_opcode(0x39, "AND", op_and, ADDR_ABSOLUTE_Y,       4, true);
    set_opcode(0x21, "AND", op_and, ADDR_INDEXED_INDIRECT, 6, false);
    set_opcode(0x31, "AND", op_and, ADDR_INDIRECT_INDEXED, 5, true);

    set_opcode(0x49, "EOR", op_eor, ADDR_IMMEDIATE,        2, false);
    set_opcode(0x45, "EOR", op_eor, ADDR_ZERO_PAGE,        3, false);
    set_opcode(0x55, "EOR", op_eor, ADDR_ZERO_PAGE_X,      4, false);
    set_opcode(0x4D, "EOR", op_eor, ADDR_ABSOLUTE,         4, false);
    set_opcode(0x5D, "EOR", op_eor, ADDR_ABSOLUTE_X,       4, true);
    set_opcode(0x59, "EOR", op_eor, ADDR_ABSOLUTE_Y,       4, true);
    set_opcode(0x41, "EOR", op_eor, ADDR_INDEXED_INDIRECT, 6, false);
    set_opcode(0x51, "EOR", op_eor, ADDR_INDIRECT_INDEXED, 5, true);

    set_opcode(0x24, "BIT", op_bit, ADDR_ZERO_PAGE, 3, false);
    set_opcode(0x2C, "BIT", op_bit, ADDR_ABSOLUTE,  4, false);

    // Load/store
    set_opcode(0xA9, "LDA", op_lda, ADDR_IMMEDIATE,        2, false);
    set_opcode(0xA5, "LDA", op_lda, ADDR_ZERO_PAGE,        3, false);
    set_opcode(0xB5, "LDA", op_lda, ADDR_ZERO_PAGE_X,      4, false);
    set_opcode(0xAD, "LDA", op_lda, ADDR_ABSOLUTE,         4, false);
    set_opcode(0xBD, "LDA", op_lda, ADDR_ABSOLUTE_X,       4, true);
    set_opcode(0xB9, "LDA", op_lda, ADDR_ABSOLUTE_Y,       4, true);
    set_opcode(0xA1, "LDA", op_lda, ADDR_INDEXED_INDIRECT, 6, false);
    set_opcode(0xB1, "LDA", op_lda, ADDR_INDIRECT_INDEXED, 5, true);

    set_opcode(0xA2, "LDX", op_ldx, ADDR_IMMEDIATE,   2, false);
    set_opcode(0xA6, "LDX", op_ldx, ADDR_ZERO_PAGE,   3, false);
    set_opcode(0xB6, "LDX", op_ldx, ADDR_ZERO_PAGE_Y, 4, false);
    set_opcode(0xAE, "LDX", op_ldx, ADDR_ABSOLUTE,    4, false);
    set_opcode(0xBE, "LDX", op_ldx, ADDR_ABSOLUTE_Y,  4, true);

    set_opcode(0xA0, "LDY", op_ldy, ADDR_IMMEDIATE,   2, false);
    set_opcode(0xA4, "LDY", op_ldy, ADDR_ZERO_PAGE,   3, false);
    set_opcode(0xB4, "LDY", op_ldy, ADDR_ZERO_PAGE_X, 4, false);
    set_opcode(0xAC, "LDY", op_ldy, ADDR_ABSOLUTE,    4, false);
    set_opcode(0xBC, "LDY", op_ldy, ADDR_ABSOLUTE_X,  4, true);

    set_opcode(0x85, "STA", op_sta, ADDR_ZERO_PAGE,        3, false);
    set_opcode(0x95, "STA", op_sta, ADDR_ZERO_PAGE_X,      4, false);
    set_opcode(0x8D, "STA", op_sta, ADDR_ABSOLUTE,         4, false);
    set_opcode(0x9D, "STA", op_sta, ADDR_ABSOLUTE_X,       5, false);
    set_opcode(0x99, "STA", op_sta, ADDR_ABSOLUTE_Y,       5, false);
    set_opcode(0x81, "STA", op_sta, ADDR_INDEXED_INDIRECT, 6, false);
    set_opcode(0x91, "STA", op_sta, ADDR_INDIRECT_INDEXED, 6, false);

    set_opcode(0x86, "STX", op_stx, ADDR_ZERO_PAGE,   3, false);
    set_opcode(0x96, "STX", op_stx, ADDR_ZERO_PAGE_Y, 4, false);
    set_opcode(0x8E, "STX", op_stx, ADDR_ABSOLUTE,    4, false);
    set_opcode(0x84, "STY", op_sty, ADDR_ZERO_PAGE,   3, false);
    set_opcode(0x94, "STY", op_sty, ADDR_ZERO_PAGE_X, 4, false);
    set_opcode(0x8C, "STY", op_sty, ADDR_ABSOLUTE,    4, false);

    // Transfers
    set_opcode(0xAA, "TAX", op_tax, ADDR_IMPLICIT, 2, false);
    set_opcode(0xA8, "TAY", op_tay, ADDR_IMPLICIT, 2, false);
    set_opcode(0xBA, "TSX", op_tsx, ADDR_IMPLICIT, 2, false);
    set_opcode(0x8A, "TXA", op_txa, ADDR_IMPLICIT, 2, false);
    set_opcode(0x9A, "TXS", op_txs, ADDR_IMPLICIT, 2, false);
    set_opcode(0x98, "TYA", op_tya, ADDR_IMPLICIT, 2, false);

    // Increment/decrement
    set_opcode(0xE8, "INX", op_inx, ADDR_IMPLICIT, 2, false);
    set_opcode(0xC8, "INY", op_iny, ADDR_IMPLICIT, 2, false);
    set_opcode(0xCA, "DEX", op_dex, ADDR_IMPLICIT, 2, false);
    set_opcode(0x88, "DEY", op_dey, ADDR_IMPLICIT, 2, false);
    set_opcode(0xE6, "INC", op_inc, ADDR_ZERO_PAGE,   5, false);
    set_opcode(0xF6, "INC", op_inc, ADDR_ZERO_PAGE_X, 6, false);
    set_opcode(0xEE, "INC", op_inc, ADDR_ABSOLUTE,    6, false);
    set_opcode(0xFE, "INC", op_inc, ADDR_ABSOLUTE_X,  7, false);
    set_opcode(0xC6, "DEC", op_dec, ADDR_ZERO_PAGE,   5, false);
    set_opcode(0xD6, "DEC", op_dec, ADDR_ZERO_PAGE_X, 6, false);
    set_opcode(0xCE, "DEC", op_dec, ADDR_ABSOLUTE,    6, false);
    set_opcode(0xDE, "DEC", op_dec, ADDR_ABSOLUTE_X,  7, false);

    // Shifts/rotates
    set_opcode(0x0A, "ASL", op_asl, ADDR_ACCUMULATOR, 2, false);
    set_opcode(0x06, "ASL", op_asl, ADDR_ZERO_PAGE,   5, false);
    set_opcode(0x16, "ASL", op_asl, ADDR_ZERO_PAGE_X, 6, false);
    set_opcode(0x0E, "ASL", op_asl, ADDR_ABSOLUTE,    6, false);
    set_opcode(0x1E, "ASL", op_asl, ADDR_ABSOLUTE_X,  7, false);
    set_opcode(0x4A, "LSR", op_lsr, ADDR_ACCUMULATOR, 2, false);
    set_opcode(0x46, "LSR", op_lsr, ADDR_ZERO_PAGE,   5, false);
    set_opcode(0x56, "LSR", op_lsr, ADDR_ZERO_PAGE_X, 6, false);
    set_opcode(0x4E, "LSR", op_lsr, ADDR_ABSOLUTE,    6, false);
    set_opcode(0x5E, "LSR", op_lsr, ADDR_ABSOLUTE_X,  7, false);
    set_opcode(0x2A, "ROL", op_rol, ADDR_ACCUMULATOR, 2, false);
    set_opcode(0x26, "ROL", op_rol, ADDR_ZERO_PAGE,   5, false);
    set_opcode(0x36, "ROL", op_rol, ADDR_ZERO_PAGE_X, 6, false);
    set_opcode(0x2E, "ROL", op_rol, ADDR_ABSOLUTE,    6, false);
    set_opcode(0x3E, "ROL", op_rol, ADDR_ABSOLUTE_X,  7, false);
    set_opcode(0x6A, "ROR", op_ror, ADDR_ACCUMULATOR, 2, false);
    set_opcode(0x66, "ROR", op_ror, ADDR_ZERO_PAGE,   5, false);
    set_opcode(0x76, "ROR", op_ror, ADDR_ZERO_PAGE_X, 6, false);
    set_opcode(0x6E, "ROR", op_ror, ADDR_ABSOLUTE,    6, false);
    set_opcode(0x7E, "ROR", op_ror, ADDR_ABSOLUTE_X,  7, false);

    // Comparison
    set_opcode(0xC9, "CMP", op_cmp, ADDR_IMMEDIATE,        2, false);
    set_opcode(0xC5, "CMP", op_cmp, ADDR_ZERO_PAGE,        3, false);
    set_opcode(0xD5, "CMP", op_cmp, ADDR_ZERO_PAGE_X,      4, false);
    set_opcode(0xCD, "CMP", op_cmp, ADDR_ABSOLUTE,         4, false);
    set_opcode(0xDD, "CMP", op_cmp, ADDR_ABSOLUTE_X,       4, true);
    set_opcode(0xD9, "CMP", op_cmp, ADDR_ABSOLUTE_Y,       4, true);
    set_opcode(0xC1, "CMP", op_cmp, ADDR_INDEXED_INDIRECT, 6, false);
    set_opcode(0xD1, "CMP", op_cmp, ADDR_INDIRECT_INDEXED, 5, true);
    set_opcode(0xE0, "CPX", op_cpx, ADDR_IMMEDIATE, 2, false);
    set_opcode(0xE4, "CPX", op_cpx, ADDR_ZERO_PAGE, 3, false);
    set_opcode(0xEC, "CPX", op_cpx, ADDR_ABSOLUTE,  4, false);
    set_opcode(0xC0, "CPY", op_cpy, ADDR_IMMEDIATE, 2, false);
    set_opcode(0xC4, "CPY", op_cpy, ADDR_ZERO_PAGE, 3, false);
    set_opcode(0xCC, "CPY", op_cpy, ADDR_ABSOLUTE,  4, false);

    // Branches handle both taken and page-cross penalties internally.
    set_opcode(0x90, "BCC", op_bcc, ADDR_RELATIVE, 2, false);
    set_opcode(0xB0, "BCS", op_bcs, ADDR_RELATIVE, 2, false);
    set_opcode(0xF0, "BEQ", op_beq, ADDR_RELATIVE, 2, false);
    set_opcode(0xD0, "BNE", op_bne, ADDR_RELATIVE, 2, false);
    set_opcode(0x30, "BMI", op_bmi, ADDR_RELATIVE, 2, false);
    set_opcode(0x10, "BPL", op_bpl, ADDR_RELATIVE, 2, false);
    set_opcode(0x50, "BVC", op_bvc, ADDR_RELATIVE, 2, false);
    set_opcode(0x70, "BVS", op_bvs, ADDR_RELATIVE, 2, false);

    // Jumps, stack, and system
    set_opcode(0x4C, "JMP", op_jmp, ADDR_ABSOLUTE, 3, false);
    set_opcode(0x6C, "JMP", op_jmp, ADDR_INDIRECT, 5, false);
    set_opcode(0x20, "JSR", op_jsr, ADDR_ABSOLUTE, 6, false);
    set_opcode(0x60, "RTS", op_rts, ADDR_IMPLICIT, 6, false);
    set_opcode(0x40, "RTI", op_rti, ADDR_IMPLICIT, 6, false);
    set_opcode(0x48, "PHA", op_pha, ADDR_IMPLICIT, 3, false);
    set_opcode(0x68, "PLA", op_pla, ADDR_IMPLICIT, 4, false);
    set_opcode(0x08, "PHP", op_php, ADDR_IMPLICIT, 3, false);
    set_opcode(0x28, "PLP", op_plp, ADDR_IMPLICIT, 4, false);
    set_opcode(0x00, "BRK", op_brk, ADDR_IMPLICIT, 7, false);
    set_opcode(0xEA, "NOP", op_nop, ADDR_IMPLICIT, 2, false);

    // Flag instructions
    set_opcode(0x18, "CLC", op_clc, ADDR_IMPLICIT, 2, false);
    set_opcode(0xD8, "CLD", op_cld, ADDR_IMPLICIT, 2, false);
    set_opcode(0x58, "CLI", op_cli, ADDR_IMPLICIT, 2, false);
    set_opcode(0xB8, "CLV", op_clv, ADDR_IMPLICIT, 2, false);
    set_opcode(0x38, "SEC", op_sec, ADDR_IMPLICIT, 2, false);
    set_opcode(0xF8, "SED", op_sed, ADDR_IMPLICIT, 2, false);
    set_opcode(0x78, "SEI", op_sei, ADDR_IMPLICIT, 2, false);

    // Unofficial NOPs used by nestest.
    set_opcode(0x1A, "NOP", op_nop, ADDR_IMPLICIT, 2, false);
    set_opcode(0x3A, "NOP", op_nop, ADDR_IMPLICIT, 2, false);
    set_opcode(0x5A, "NOP", op_nop, ADDR_IMPLICIT, 2, false);
    set_opcode(0x7A, "NOP", op_nop, ADDR_IMPLICIT, 2, false);
    set_opcode(0xDA, "NOP", op_nop, ADDR_IMPLICIT, 2, false);
    set_opcode(0xFA, "NOP", op_nop, ADDR_IMPLICIT, 2, false);
    set_opcode(0x80, "NOP", op_nop, ADDR_IMMEDIATE, 2, false);
    set_opcode(0x82, "NOP", op_nop, ADDR_IMMEDIATE, 2, false);
    set_opcode(0x89, "NOP", op_nop, ADDR_IMMEDIATE, 2, false);
    set_opcode(0xC2, "NOP", op_nop, ADDR_IMMEDIATE, 2, false);
    set_opcode(0xE2, "NOP", op_nop, ADDR_IMMEDIATE, 2, false);
    set_opcode(0x04, "NOP", op_nop, ADDR_ZERO_PAGE, 3, false);
    set_opcode(0x44, "NOP", op_nop, ADDR_ZERO_PAGE, 3, false);
    set_opcode(0x64, "NOP", op_nop, ADDR_ZERO_PAGE, 3, false);
    set_opcode(0x14, "NOP", op_nop, ADDR_ZERO_PAGE_X, 4, false);
    set_opcode(0x34, "NOP", op_nop, ADDR_ZERO_PAGE_X, 4, false);
    set_opcode(0x54, "NOP", op_nop, ADDR_ZERO_PAGE_X, 4, false);
    set_opcode(0x74, "NOP", op_nop, ADDR_ZERO_PAGE_X, 4, false);
    set_opcode(0xD4, "NOP", op_nop, ADDR_ZERO_PAGE_X, 4, false);
    set_opcode(0xF4, "NOP", op_nop, ADDR_ZERO_PAGE_X, 4, false);
    set_opcode(0x0C, "NOP", op_nop, ADDR_ABSOLUTE, 4, false);
    set_opcode(0x1C, "NOP", op_nop, ADDR_ABSOLUTE_X, 4, true);
    set_opcode(0x3C, "NOP", op_nop, ADDR_ABSOLUTE_X, 4, true);
    set_opcode(0x5C, "NOP", op_nop, ADDR_ABSOLUTE_X, 4, true);
    set_opcode(0x7C, "NOP", op_nop, ADDR_ABSOLUTE_X, 4, true);
    set_opcode(0xDC, "NOP", op_nop, ADDR_ABSOLUTE_X, 4, true);
    set_opcode(0xFC, "NOP", op_nop, ADDR_ABSOLUTE_X, 4, true);

    // Common unofficial opcodes tested by nestest.
    set_opcode(0x03, "SLO", op_slo, ADDR_INDEXED_INDIRECT, 8, false);
    set_opcode(0x07, "SLO", op_slo, ADDR_ZERO_PAGE,        5, false);
    set_opcode(0x0F, "SLO", op_slo, ADDR_ABSOLUTE,         6, false);
    set_opcode(0x13, "SLO", op_slo, ADDR_INDIRECT_INDEXED, 8, false);
    set_opcode(0x17, "SLO", op_slo, ADDR_ZERO_PAGE_X,      6, false);
    set_opcode(0x1B, "SLO", op_slo, ADDR_ABSOLUTE_Y,       7, false);
    set_opcode(0x1F, "SLO", op_slo, ADDR_ABSOLUTE_X,       7, false);

    set_opcode(0x23, "RLA", op_rla, ADDR_INDEXED_INDIRECT, 8, false);
    set_opcode(0x27, "RLA", op_rla, ADDR_ZERO_PAGE,        5, false);
    set_opcode(0x2F, "RLA", op_rla, ADDR_ABSOLUTE,         6, false);
    set_opcode(0x33, "RLA", op_rla, ADDR_INDIRECT_INDEXED, 8, false);
    set_opcode(0x37, "RLA", op_rla, ADDR_ZERO_PAGE_X,      6, false);
    set_opcode(0x3B, "RLA", op_rla, ADDR_ABSOLUTE_Y,       7, false);
    set_opcode(0x3F, "RLA", op_rla, ADDR_ABSOLUTE_X,       7, false);

    set_opcode(0x43, "SRE", op_sre, ADDR_INDEXED_INDIRECT, 8, false);
    set_opcode(0x47, "SRE", op_sre, ADDR_ZERO_PAGE,        5, false);
    set_opcode(0x4F, "SRE", op_sre, ADDR_ABSOLUTE,         6, false);
    set_opcode(0x53, "SRE", op_sre, ADDR_INDIRECT_INDEXED, 8, false);
    set_opcode(0x57, "SRE", op_sre, ADDR_ZERO_PAGE_X,      6, false);
    set_opcode(0x5B, "SRE", op_sre, ADDR_ABSOLUTE_Y,       7, false);
    set_opcode(0x5F, "SRE", op_sre, ADDR_ABSOLUTE_X,       7, false);

    set_opcode(0x63, "RRA", op_rra, ADDR_INDEXED_INDIRECT, 8, false);
    set_opcode(0x67, "RRA", op_rra, ADDR_ZERO_PAGE,        5, false);
    set_opcode(0x6F, "RRA", op_rra, ADDR_ABSOLUTE,         6, false);
    set_opcode(0x73, "RRA", op_rra, ADDR_INDIRECT_INDEXED, 8, false);
    set_opcode(0x77, "RRA", op_rra, ADDR_ZERO_PAGE_X,      6, false);
    set_opcode(0x7B, "RRA", op_rra, ADDR_ABSOLUTE_Y,       7, false);
    set_opcode(0x7F, "RRA", op_rra, ADDR_ABSOLUTE_X,       7, false);

    set_opcode(0xA3, "LAX", op_lax, ADDR_INDEXED_INDIRECT, 6, false);
    set_opcode(0xA7, "LAX", op_lax, ADDR_ZERO_PAGE,        3, false);
    set_opcode(0xAB, "LAX", op_lax, ADDR_IMMEDIATE,        2, false);
    set_opcode(0xAF, "LAX", op_lax, ADDR_ABSOLUTE,         4, false);
    set_opcode(0xB3, "LAX", op_lax, ADDR_INDIRECT_INDEXED, 5, true);
    set_opcode(0xB7, "LAX", op_lax, ADDR_ZERO_PAGE_Y,      4, false);
    set_opcode(0xBF, "LAX", op_lax, ADDR_ABSOLUTE_Y,       4, true);

    set_opcode(0x83, "SAX", op_sax, ADDR_INDEXED_INDIRECT, 6, false);
    set_opcode(0x87, "SAX", op_sax, ADDR_ZERO_PAGE,        3, false);
    set_opcode(0x8F, "SAX", op_sax, ADDR_ABSOLUTE,         4, false);
    set_opcode(0x97, "SAX", op_sax, ADDR_ZERO_PAGE_Y,      4, false);

    set_opcode(0xC3, "DCP", op_dcp, ADDR_INDEXED_INDIRECT, 8, false);
    set_opcode(0xC7, "DCP", op_dcp, ADDR_ZERO_PAGE,        5, false);
    set_opcode(0xCF, "DCP", op_dcp, ADDR_ABSOLUTE,         6, false);
    set_opcode(0xD3, "DCP", op_dcp, ADDR_INDIRECT_INDEXED, 8, false);
    set_opcode(0xD7, "DCP", op_dcp, ADDR_ZERO_PAGE_X,      6, false);
    set_opcode(0xDB, "DCP", op_dcp, ADDR_ABSOLUTE_Y,       7, false);
    set_opcode(0xDF, "DCP", op_dcp, ADDR_ABSOLUTE_X,       7, false);

    set_opcode(0xE3, "ISC", op_isc, ADDR_INDEXED_INDIRECT, 8, false);
    set_opcode(0xE7, "ISC", op_isc, ADDR_ZERO_PAGE,        5, false);
    set_opcode(0xEF, "ISC", op_isc, ADDR_ABSOLUTE,         6, false);
    set_opcode(0xF3, "ISC", op_isc, ADDR_INDIRECT_INDEXED, 8, false);
    set_opcode(0xF7, "ISC", op_isc, ADDR_ZERO_PAGE_X,      6, false);
    set_opcode(0xFB, "ISC", op_isc, ADDR_ABSOLUTE_Y,       7, false);
    set_opcode(0xFF, "ISC", op_isc, ADDR_ABSOLUTE_X,       7, false);

    set_opcode(0x0B, "ANC", op_anc, ADDR_IMMEDIATE, 2, false);
    set_opcode(0x2B, "ANC", op_anc, ADDR_IMMEDIATE, 2, false);
    set_opcode(0x4B, "ALR", op_alr, ADDR_IMMEDIATE, 2, false);
    set_opcode(0x6B, "ARR", op_arr, ADDR_IMMEDIATE, 2, false);
    set_opcode(0x8B, "XAA", op_xaa, ADDR_IMMEDIATE, 2, false);
    set_opcode(0xCB, "AXS", op_axs, ADDR_IMMEDIATE, 2, false);
    set_opcode(0x93, "AHX", op_ahx, ADDR_INDIRECT_INDEXED, 6, false);
    set_opcode(0x9F, "AHX", op_ahx, ADDR_ABSOLUTE_Y,       5, false);
    set_opcode(0x9C, "SHY", op_shy, ADDR_ABSOLUTE_X,       5, false);
    set_opcode(0x9E, "SHX", op_shx, ADDR_ABSOLUTE_Y,       5, false);
    set_opcode(0x9B, "TAS", op_tas, ADDR_ABSOLUTE_Y,       5, false);
    set_opcode(0xBB, "LAS", op_las, ADDR_ABSOLUTE_Y,       4, true);

    opcodes_ready = true;
}

void cpu_init(CPU *cpu, Bus *bus) {
    memset(cpu, 0, sizeof(*cpu));
    cpu->bus = bus;
    if (!opcodes_ready) {
        cpu_init_opcodes();
    }
}

uint8_t cpu_get_flags(CPU *cpu) {
    // Bit 5 is always set when the status register is pushed or inspected.
    // Bit 4, the B flag, is added by PHP and BRK because it depends on why the
    // flags are being pushed.
    return (cpu->flag_c ? 0x01 : 0) |
           (cpu->flag_z ? 0x02 : 0) |
           (cpu->flag_i ? 0x04 : 0) |
           (cpu->flag_d ? 0x08 : 0) |
           0x20 |
           (cpu->flag_v ? 0x40 : 0) |
           (cpu->flag_n ? 0x80 : 0);
}

void cpu_set_flags(CPU *cpu, uint8_t value) {
    // Bit 4 and bit 5 are intentionally ignored here. They are stack/status
    // byte details, not normal flags the CPU logic needs to store.
    cpu->flag_c = (value & 0x01) != 0;
    cpu->flag_z = (value & 0x02) != 0;
    cpu->flag_i = (value & 0x04) != 0;
    cpu->flag_d = (value & 0x08) != 0;
    cpu->flag_v = (value & 0x40) != 0;
    cpu->flag_n = (value & 0x80) != 0;
}

void cpu_update_zero_and_negative(CPU *cpu, uint8_t value) {
    // Most load, math, shift, and transfer instructions end with these two
    // flags. Keeping it here avoids slightly different copies everywhere.
    cpu->flag_z = (value == 0);
    cpu->flag_n = (value & 0x80) != 0;
}

uint16_t cpu_read16(CPU *cpu, uint16_t address) {
    // 6502 operands are little-endian, so the low byte comes first.
    uint8_t lo = bus_read(cpu->bus, address);
    uint8_t hi = bus_read(cpu->bus, address + 1);
    return ((uint16_t)hi << 8) | lo;
}

void cpu_push(CPU *cpu, uint8_t value) {
    // Stack lives at $0100-$01FF. SP is only the low byte inside that page.
    // Push writes first, then moves downward.
    bus_write(cpu->bus, 0x0100 + cpu->sp, value);
    cpu->sp--;
}

uint8_t cpu_pop(CPU *cpu) {
    // Pop moves upward first because SP points to the next free stack slot.
    cpu->sp++;
    return bus_read(cpu->bus, 0x0100 + cpu->sp);
}

void cpu_push16(CPU *cpu, uint16_t value) {
    // Return addresses are pushed high byte first. cpu_pop16 reverses this.
    cpu_push(cpu, (value >> 8) & 0xFF);
    cpu_push(cpu, value & 0xFF);
}

uint16_t cpu_pop16(CPU *cpu) {
    uint8_t lo = cpu_pop(cpu);
    uint8_t hi = cpu_pop(cpu);
    return ((uint16_t)hi << 8) | lo;
}

uint16_t cpu_resolve_address(CPU *cpu, AddressingMode mode, bool *page_crossed) {
    // This function consumes the operand bytes after the opcode and returns the
    // final address the instruction should use. It also tells cpu_step when an
    // indexed read crossed a page and may need one extra cycle.
    *page_crossed = false;

    switch (mode) {
    case ADDR_IMPLICIT:
    case ADDR_ACCUMULATOR:
        return 0;

    case ADDR_IMMEDIATE: {
        uint16_t addr = cpu->pc;
        cpu->pc++;
        return addr;
    }

    case ADDR_ZERO_PAGE: {
        uint8_t addr = bus_read(cpu->bus, cpu->pc);
        cpu->pc++;
        return addr;
    }

    case ADDR_ZERO_PAGE_X: {
        uint8_t base = bus_read(cpu->bus, cpu->pc);
        cpu->pc++;
        return (base + cpu->x) & 0xFF;
    }

    case ADDR_ZERO_PAGE_Y: {
        uint8_t base = bus_read(cpu->bus, cpu->pc);
        cpu->pc++;
        return (base + cpu->y) & 0xFF;
    }

    case ADDR_ABSOLUTE: {
        uint16_t addr = cpu_read16(cpu, cpu->pc);
        cpu->pc += 2;
        return addr;
    }

    case ADDR_ABSOLUTE_X: {
        uint16_t base = cpu_read16(cpu, cpu->pc);
        cpu->pc += 2;
        uint16_t addr = base + cpu->x;
        *page_crossed = ((base & 0xFF00) != (addr & 0xFF00));
        return addr;
    }

    case ADDR_ABSOLUTE_Y: {
        uint16_t base = cpu_read16(cpu, cpu->pc);
        cpu->pc += 2;
        uint16_t addr = base + cpu->y;
        *page_crossed = ((base & 0xFF00) != (addr & 0xFF00));
        return addr;
    }

    case ADDR_INDIRECT: {
        uint16_t ptr = cpu_read16(cpu, cpu->pc);
        cpu->pc += 2;
        // Original 6502 JMP ($xxFF) bug. The high byte wraps to $xx00 instead
        // of reading from the next page. Some test ROMs check this exactly.
        if ((ptr & 0x00FF) == 0x00FF) {
            uint8_t lo = bus_read(cpu->bus, ptr);
            uint8_t hi = bus_read(cpu->bus, ptr & 0xFF00);
            return ((uint16_t)hi << 8) | lo;
        }
        return cpu_read16(cpu, ptr);
    }

    case ADDR_INDEXED_INDIRECT: {
        uint8_t base = bus_read(cpu->bus, cpu->pc);
        cpu->pc++;
        uint8_t ptr = (base + cpu->x) & 0xFF;
        uint8_t lo = bus_read(cpu->bus, ptr);
        uint8_t hi = bus_read(cpu->bus, (ptr + 1) & 0xFF);
        return ((uint16_t)hi << 8) | lo;
    }

    case ADDR_INDIRECT_INDEXED: {
        uint8_t ptr = bus_read(cpu->bus, cpu->pc);
        cpu->pc++;
        uint8_t lo = bus_read(cpu->bus, ptr);
        uint8_t hi = bus_read(cpu->bus, (ptr + 1) & 0xFF);
        uint16_t base = ((uint16_t)hi << 8) | lo;
        uint16_t addr = base + cpu->y;
        *page_crossed = ((base & 0xFF00) != (addr & 0xFF00));
        return addr;
    }

    case ADDR_RELATIVE: {
        uint8_t offset = bus_read(cpu->bus, cpu->pc);
        cpu->pc++;
        // Branch offsets are signed and are relative to the PC after the
        // operand byte has already been consumed.
        return cpu->pc + (int8_t)offset;
    }
    }

    return 0;
}

void cpu_reset(CPU *cpu) {
    cpu->a = 0;
    cpu->x = 0;
    cpu->y = 0;
    cpu->sp = 0xFD;
    cpu_set_flags(cpu, 0x24);
    cpu->pc = cpu_read16(cpu, 0xFFFC);
    cpu->cycles = 7;
    cpu->extra_cycles = 0;
}

void cpu_interrupt_nmi(CPU *cpu) {
    cpu_push16(cpu, cpu->pc);
    cpu_push(cpu, cpu_get_flags(cpu) & ~0x10);
    cpu->flag_i = true;
    cpu->pc = cpu_read16(cpu, 0xFFFA);
    cpu->cycles += 7;
}

void cpu_interrupt_irq(CPU *cpu) {
    if (cpu->flag_i) return;
    cpu_push16(cpu, cpu->pc);
    cpu_push(cpu, cpu_get_flags(cpu) & ~0x10);
    cpu->flag_i = true;
    cpu->pc = cpu_read16(cpu, 0xFFFE);
    cpu->cycles += 7;
}

uint8_t cpu_step(CPU *cpu) {
    // One call runs exactly one CPU instruction.
    uint8_t opcode_byte = bus_read(cpu->bus, cpu->pc);
    cpu->pc++;

    Opcode *op = &opcodes[opcode_byte];
    if (op->execute == NULL) {
        fprintf(stderr, "Invalid opcode: $%02X at $%04X\n", opcode_byte, cpu->pc - 1);
        return 0;
    }

    bool page_crossed = false;
    uint16_t address = cpu_resolve_address(cpu, op->mode, &page_crossed);

    // Some handlers add branch cycles themselves. The generic page-cross
    // penalty is added after execution because the addressing resolver detects it.
    cpu->extra_cycles = 0;
    op->execute(cpu, address, op->mode);

    uint8_t total_cycles = op->cycles + cpu->extra_cycles;
    if (page_crossed && op->page_cross_penalty) {
        total_cycles++;
    }
    cpu->cycles += total_cycles;

    return total_cycles;
}

static void adc_value(CPU *cpu, uint8_t val) {
    uint8_t old_a = cpu->a;
    uint16_t result = old_a + val + (cpu->flag_c ? 1 : 0);

    cpu->a = result & 0xFF;
    cpu_update_zero_and_negative(cpu, cpu->a);
    // Carry is unsigned overflow. Overflow is signed overflow.
    cpu->flag_c = (result > 0xFF);
    cpu->flag_v = ((~(old_a ^ val) & (old_a ^ cpu->a)) & 0x80) != 0;
}

static void sbc_value(CPU *cpu, uint8_t val) {
    uint8_t old_a = cpu->a;
    // The 6502 performs subtraction as A + bitwise-not(value) + carry.
    uint16_t result = old_a + (val ^ 0xFF) + (cpu->flag_c ? 1 : 0);

    cpu->a = result & 0xFF;
    cpu_update_zero_and_negative(cpu, cpu->a);
    cpu->flag_c = (result > 0xFF);
    cpu->flag_v = ((~(old_a ^ (val ^ 0xFF)) & (old_a ^ cpu->a)) & 0x80) != 0;
}

static void compare_value(CPU *cpu, uint8_t reg, uint8_t val) {
    uint8_t result = reg - val;
    cpu->flag_c = (reg >= val);
    cpu_update_zero_and_negative(cpu, result);
}

static void op_adc(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    adc_value(cpu, cpu_read_operand(cpu, addr));
}

static void op_sbc(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    sbc_value(cpu, cpu_read_operand(cpu, addr));
}

static void op_ora(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    cpu->a |= cpu_read_operand(cpu, addr);
    cpu_update_zero_and_negative(cpu, cpu->a);
}

static void op_and(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    cpu->a &= cpu_read_operand(cpu, addr);
    cpu_update_zero_and_negative(cpu, cpu->a);
}

static void op_eor(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    cpu->a ^= cpu_read_operand(cpu, addr);
    cpu_update_zero_and_negative(cpu, cpu->a);
}

static void op_bit(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    uint8_t val = bus_read(cpu->bus, addr);
    // BIT is unusual. Z comes from A & memory, but N and V are copied from
    // bits 7 and 6 of the memory value.
    cpu->flag_z = ((cpu->a & val) == 0);
    cpu->flag_v = (val & 0x40) != 0;
    cpu->flag_n = (val & 0x80) != 0;
}

static void op_lda(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    cpu->a = cpu_read_operand(cpu, addr);
    cpu_update_zero_and_negative(cpu, cpu->a);
}

static void op_ldx(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    cpu->x = cpu_read_operand(cpu, addr);
    cpu_update_zero_and_negative(cpu, cpu->x);
}

static void op_ldy(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    cpu->y = cpu_read_operand(cpu, addr);
    cpu_update_zero_and_negative(cpu, cpu->y);
}

static void op_sta(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    bus_write(cpu->bus, addr, cpu->a);
}

static void op_stx(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    bus_write(cpu->bus, addr, cpu->x);
}

static void op_sty(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    bus_write(cpu->bus, addr, cpu->y);
}

static void op_tax(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->x = cpu->a;
    cpu_update_zero_and_negative(cpu, cpu->x);
}

static void op_tay(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->y = cpu->a;
    cpu_update_zero_and_negative(cpu, cpu->y);
}

static void op_tsx(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->x = cpu->sp;
    cpu_update_zero_and_negative(cpu, cpu->x);
}

static void op_txa(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->a = cpu->x;
    cpu_update_zero_and_negative(cpu, cpu->a);
}

static void op_txs(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->sp = cpu->x;
}

static void op_tya(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->a = cpu->y;
    cpu_update_zero_and_negative(cpu, cpu->a);
}

static void op_inx(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->x++;
    cpu_update_zero_and_negative(cpu, cpu->x);
}

static void op_iny(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->y++;
    cpu_update_zero_and_negative(cpu, cpu->y);
}

static void op_dex(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->x--;
    cpu_update_zero_and_negative(cpu, cpu->x);
}

static void op_dey(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->y--;
    cpu_update_zero_and_negative(cpu, cpu->y);
}

static void op_inc(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    uint8_t val = bus_read(cpu->bus, addr) + 1;
    bus_write(cpu->bus, addr, val);
    cpu_update_zero_and_negative(cpu, val);
}

static void op_dec(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    uint8_t val = bus_read(cpu->bus, addr) - 1;
    bus_write(cpu->bus, addr, val);
    cpu_update_zero_and_negative(cpu, val);
}

static void op_asl(CPU *cpu, uint16_t addr, AddressingMode mode) {
    uint8_t val = (mode == ADDR_ACCUMULATOR) ? cpu->a : bus_read(cpu->bus, addr);
    cpu->flag_c = (val & 0x80) != 0;
    val <<= 1;
    if (mode == ADDR_ACCUMULATOR) cpu->a = val;
    else bus_write(cpu->bus, addr, val);
    cpu_update_zero_and_negative(cpu, val);
}

static void op_lsr(CPU *cpu, uint16_t addr, AddressingMode mode) {
    uint8_t val = (mode == ADDR_ACCUMULATOR) ? cpu->a : bus_read(cpu->bus, addr);
    cpu->flag_c = (val & 0x01) != 0;
    val >>= 1;
    if (mode == ADDR_ACCUMULATOR) cpu->a = val;
    else bus_write(cpu->bus, addr, val);
    cpu_update_zero_and_negative(cpu, val);
}

static void op_rol(CPU *cpu, uint16_t addr, AddressingMode mode) {
    uint8_t val = (mode == ADDR_ACCUMULATOR) ? cpu->a : bus_read(cpu->bus, addr);
    bool old_carry = cpu->flag_c;
    cpu->flag_c = (val & 0x80) != 0;
    val = (val << 1) | (old_carry ? 1 : 0);
    if (mode == ADDR_ACCUMULATOR) cpu->a = val;
    else bus_write(cpu->bus, addr, val);
    cpu_update_zero_and_negative(cpu, val);
}

static void op_ror(CPU *cpu, uint16_t addr, AddressingMode mode) {
    uint8_t val = (mode == ADDR_ACCUMULATOR) ? cpu->a : bus_read(cpu->bus, addr);
    bool old_carry = cpu->flag_c;
    cpu->flag_c = (val & 0x01) != 0;
    val = (val >> 1) | (old_carry ? 0x80 : 0);
    if (mode == ADDR_ACCUMULATOR) cpu->a = val;
    else bus_write(cpu->bus, addr, val);
    cpu_update_zero_and_negative(cpu, val);
}

static void op_cmp(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    compare_value(cpu, cpu->a, cpu_read_operand(cpu, addr));
}

static void op_cpx(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    compare_value(cpu, cpu->x, cpu_read_operand(cpu, addr));
}

static void op_cpy(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    compare_value(cpu, cpu->y, cpu_read_operand(cpu, addr));
}

static void branch_if(CPU *cpu, uint16_t addr, bool condition) {
    if (condition) {
        // Branch timing is special: 1 cycle if taken, plus 1 more if the
        // destination crosses a page.
        cpu->extra_cycles++;
        if ((cpu->pc & 0xFF00) != (addr & 0xFF00)) {
            cpu->extra_cycles++;
        }
        cpu->pc = addr;
    }
}

static void op_bcc(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    branch_if(cpu, addr, !cpu->flag_c);
}

static void op_bcs(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    branch_if(cpu, addr, cpu->flag_c);
}

static void op_beq(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    branch_if(cpu, addr, cpu->flag_z);
}

static void op_bne(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    branch_if(cpu, addr, !cpu->flag_z);
}

static void op_bmi(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    branch_if(cpu, addr, cpu->flag_n);
}

static void op_bpl(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    branch_if(cpu, addr, !cpu->flag_n);
}

static void op_bvc(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    branch_if(cpu, addr, !cpu->flag_v);
}

static void op_bvs(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    branch_if(cpu, addr, cpu->flag_v);
}

static void op_jmp(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    cpu->pc = addr;
}

static void op_jsr(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    cpu_push16(cpu, cpu->pc - 1);
    cpu->pc = addr;
}

static void op_rts(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->pc = cpu_pop16(cpu) + 1;
}

static void op_rti(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu_set_flags(cpu, cpu_pop(cpu));
    cpu->pc = cpu_pop16(cpu);
}

static void op_pha(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu_push(cpu, cpu->a);
}

static void op_pla(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->a = cpu_pop(cpu);
    cpu_update_zero_and_negative(cpu, cpu->a);
}

static void op_php(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu_push(cpu, cpu_get_flags(cpu) | 0x10);
}

static void op_plp(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu_set_flags(cpu, cpu_pop(cpu));
}

static void op_clc(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->flag_c = false;
}

static void op_cld(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->flag_d = false;
}

static void op_cli(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->flag_i = false;
}

static void op_clv(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->flag_v = false;
}

static void op_sec(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->flag_c = true;
}

static void op_sed(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->flag_d = true;
}

static void op_sei(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->flag_i = true;
}

static void op_lax(CPU *cpu, uint16_t addr, AddressingMode mode) {
    // Unofficial opcode. LAX is basically LDA and LDX using the same value.
    (void)mode;
    uint8_t val = bus_read(cpu->bus, addr);
    cpu->a = val;
    cpu->x = val;
    cpu_update_zero_and_negative(cpu, val);
}

static void op_sax(CPU *cpu, uint16_t addr, AddressingMode mode) {
    // Unofficial opcode. Stores A & X without changing flags.
    (void)mode;
    bus_write(cpu->bus, addr, cpu->a & cpu->x);
}

static void op_dcp(CPU *cpu, uint16_t addr, AddressingMode mode) {
    // Unofficial opcode. DEC memory, then compare the result with A.
    (void)mode;
    uint8_t val = bus_read(cpu->bus, addr) - 1;
    bus_write(cpu->bus, addr, val);
    compare_value(cpu, cpu->a, val);
}

static void op_isc(CPU *cpu, uint16_t addr, AddressingMode mode) {
    // Unofficial opcode. INC memory, then SBC the result.
    (void)mode;
    uint8_t val = bus_read(cpu->bus, addr) + 1;
    bus_write(cpu->bus, addr, val);
    sbc_value(cpu, val);
}

static void op_slo(CPU *cpu, uint16_t addr, AddressingMode mode) {
    // Unofficial opcode. ASL memory, then ORA the result into A.
    (void)mode;
    uint8_t val = bus_read(cpu->bus, addr);
    cpu->flag_c = (val & 0x80) != 0;
    val <<= 1;
    bus_write(cpu->bus, addr, val);
    cpu->a |= val;
    cpu_update_zero_and_negative(cpu, cpu->a);
}

static void op_rla(CPU *cpu, uint16_t addr, AddressingMode mode) {
    // Unofficial opcode. ROL memory, then AND the result into A.
    (void)mode;
    uint8_t val = bus_read(cpu->bus, addr);
    bool old_carry = cpu->flag_c;
    cpu->flag_c = (val & 0x80) != 0;
    val = (val << 1) | (old_carry ? 1 : 0);
    bus_write(cpu->bus, addr, val);
    cpu->a &= val;
    cpu_update_zero_and_negative(cpu, cpu->a);
}

static void op_sre(CPU *cpu, uint16_t addr, AddressingMode mode) {
    // Unofficial opcode. LSR memory, then EOR the result into A.
    (void)mode;
    uint8_t val = bus_read(cpu->bus, addr);
    cpu->flag_c = (val & 0x01) != 0;
    val >>= 1;
    bus_write(cpu->bus, addr, val);
    cpu->a ^= val;
    cpu_update_zero_and_negative(cpu, cpu->a);
}

static void op_rra(CPU *cpu, uint16_t addr, AddressingMode mode) {
    // Unofficial opcode. ROR memory, then ADC the result.
    (void)mode;
    uint8_t val = bus_read(cpu->bus, addr);
    bool old_carry = cpu->flag_c;
    cpu->flag_c = (val & 0x01) != 0;
    val = (val >> 1) | (old_carry ? 0x80 : 0);
    bus_write(cpu->bus, addr, val);
    adc_value(cpu, val);
}

static void op_anc(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    cpu->a &= bus_read(cpu->bus, addr);
    cpu_update_zero_and_negative(cpu, cpu->a);
    cpu->flag_c = cpu->flag_n;
}

static void op_alr(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    cpu->a &= bus_read(cpu->bus, addr);
    cpu->flag_c = (cpu->a & 0x01) != 0;
    cpu->a >>= 1;
    cpu_update_zero_and_negative(cpu, cpu->a);
}

static void op_arr(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    cpu->a &= bus_read(cpu->bus, addr);
    cpu->a = (cpu->a >> 1) | (cpu->flag_c ? 0x80 : 0);
    cpu_update_zero_and_negative(cpu, cpu->a);
    cpu->flag_c = (cpu->a & 0x40) != 0;
    cpu->flag_v = ((cpu->a >> 6) ^ (cpu->a >> 5)) & 0x01;
}

static void op_axs(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    uint8_t val = bus_read(cpu->bus, addr);
    uint8_t ax = cpu->a & cpu->x;
    uint8_t result = ax - val;
    cpu->flag_c = (ax >= val);
    cpu->x = result;
    cpu_update_zero_and_negative(cpu, cpu->x);
}

static void op_las(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    uint8_t val = bus_read(cpu->bus, addr) & cpu->sp;
    cpu->a = val;
    cpu->x = val;
    cpu->sp = val;
    cpu_update_zero_and_negative(cpu, val);
}

static void op_ahx(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    bus_write(cpu->bus, addr, cpu->a & cpu->x & (((addr >> 8) + 1) & 0xFF));
}

static void op_shx(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    bus_write(cpu->bus, addr, cpu->x & (((addr >> 8) + 1) & 0xFF));
}

static void op_shy(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    bus_write(cpu->bus, addr, cpu->y & (((addr >> 8) + 1) & 0xFF));
}

static void op_tas(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    cpu->sp = cpu->a & cpu->x;
    bus_write(cpu->bus, addr, cpu->sp & (((addr >> 8) + 1) & 0xFF));
}

static void op_xaa(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)mode;
    cpu->a = cpu->x & bus_read(cpu->bus, addr);
    cpu_update_zero_and_negative(cpu, cpu->a);
}

static void op_nop(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)cpu;
    (void)addr;
    (void)mode;
}

static void op_brk(CPU *cpu, uint16_t addr, AddressingMode mode) {
    (void)addr;
    (void)mode;
    cpu->pc++;
    cpu_push16(cpu, cpu->pc);
    cpu_push(cpu, cpu_get_flags(cpu) | 0x10);
    cpu->flag_i = true;
    cpu->pc = cpu_read16(cpu, 0xFFFE);
}
