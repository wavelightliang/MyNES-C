// The Golden Rule: Include your own header first.
#include "cpu/cpu.h"

#include "bus/bus.h"
#include <string.h>
#include <stdio.h>

// --- Helper Functions (all now take Bus*) ---
static u8 cpu_fetch_byte(CPU* cpu, Bus* bus) { u8 d = bus_read(bus, cpu->pc); cpu->pc++; return d; }
static u16 cpu_fetch_word(CPU* cpu, Bus* bus) { u8 lo = cpu_fetch_byte(cpu, bus); u8 hi = cpu_fetch_byte(cpu, bus); return (u16)hi << 8 | (u16)lo; }
static void cpu_set_flag(CPU* cpu, CpuFlags flag, bool value) { if (value) cpu->status |= flag; else cpu->status &= ~flag; }
static u8 cpu_get_flag(CPU* cpu, CpuFlags flag) { return (cpu->status & flag) > 0; }
static void cpu_update_zn_flags(CPU* cpu, u8 v) { cpu_set_flag(cpu, Z_FLAG, v == 0); cpu_set_flag(cpu, N_FLAG, v & 0x80); }
static void cpu_stack_push(CPU* cpu, Bus* bus, u8 d) { bus_write(bus, 0x0100 | cpu->sp, d); cpu->sp--; }
static u8 cpu_stack_pop(CPU* cpu, Bus* bus) { cpu->sp++; return bus_read(bus, 0x0100 | cpu->sp); }
static void cpu_stack_push_word(CPU* cpu, Bus* bus, u16 d) { cpu_stack_push(cpu, bus, (u8)(d >> 8)); cpu_stack_push(cpu, bus, (u8)(d & 0xFF)); }
static u16 cpu_stack_pop_word(CPU* cpu, Bus* bus) { u8 lo = cpu_stack_pop(cpu, bus); u8 hi = cpu_stack_pop(cpu, bus); return (u16)hi << 8 | (u16)lo; }

// --- Addressing Modes (all now take Bus*) ---
static u16 get_operand_address(CPU* cpu, Bus* bus, AddressingMode mode) {
    switch (mode) {
        case AM_IMM: return cpu->pc++;
        case AM_ZP0: return cpu_fetch_byte(cpu, bus);
        case AM_ZPX: return (cpu_fetch_byte(cpu, bus) + cpu->x) & 0xFF;
        case AM_ZPY: return (cpu_fetch_byte(cpu, bus) + cpu->y) & 0xFF;
        case AM_ABS: return cpu_fetch_word(cpu, bus);
        case AM_ABX: return cpu_fetch_word(cpu, bus) + cpu->x;
        case AM_ABY: return cpu_fetch_word(cpu, bus) + cpu->y;
        case AM_IND: { u16 ptr = cpu_fetch_word(cpu, bus); u16 lo = bus_read(bus, ptr); u16 hi = bus_read(bus, (ptr & 0xFF00) | ((ptr + 1) & 0x00FF)); return (u16)hi << 8 | (u16)lo; }
        case AM_IZX: { u8 ptr = cpu_fetch_byte(cpu, bus) + cpu->x; u16 lo = bus_read(bus, ptr & 0xFF); u16 hi = bus_read(bus, (ptr + 1) & 0xFF); return (u16)hi << 8 | (u16)lo; }
        case AM_IZY: { u8 ptr = cpu_fetch_byte(cpu, bus); u16 lo = bus_read(bus, ptr); u16 hi = bus_read(bus, (ptr + 1) & 0xFF); return ((u16)hi << 8 | (u16)lo) + cpu->y; }
        case AM_ACC: case AM_IMP: case AM_REL: default: return 0;
    }
}

// --- Instruction Implementations (all now take Bus*) ---
static void adc(CPU* cpu, Bus* bus, u16 a) { u8 v = bus_read(bus, a); u16 sum = cpu->a + v + cpu_get_flag(cpu, C_FLAG); cpu_set_flag(cpu, C_FLAG, sum > 0xFF); cpu_set_flag(cpu, V_FLAG, (~(cpu->a ^ v) & (cpu->a ^ sum) & 0x80) != 0); cpu->a = (u8)sum; cpu_update_zn_flags(cpu, cpu->a); }
static void sbc(CPU* cpu, Bus* bus, u16 a) { u8 v = bus_read(bus, a); u16 diff = cpu->a - v - (1 - cpu_get_flag(cpu, C_FLAG)); cpu_set_flag(cpu, C_FLAG, diff < 0x100); cpu_set_flag(cpu, V_FLAG, ((cpu->a ^ v) & (cpu->a ^ diff) & 0x80) != 0); cpu->a = (u8)diff; cpu_update_zn_flags(cpu, cpu->a); }
static void compare(CPU* cpu, Bus* bus, u8 reg, u16 a) { u8 v = bus_read(bus, a); u8 res = reg - v; cpu_set_flag(cpu, C_FLAG, reg >= v); cpu_update_zn_flags(cpu, res); }
static void asl(CPU* cpu, Bus* bus, u16 a, AddressingMode m) { u8 v = (m == AM_ACC) ? cpu->a : bus_read(bus, a); cpu_set_flag(cpu, C_FLAG, v & 0x80); v <<= 1; if (m == AM_ACC) cpu->a = v; else bus_write(bus, a, v); cpu_update_zn_flags(cpu, v); }
static void lsr(CPU* cpu, Bus* bus, u16 a, AddressingMode m) { u8 v = (m == AM_ACC) ? cpu->a : bus_read(bus, a); cpu_set_flag(cpu, C_FLAG, v & 1); v >>= 1; if (m == AM_ACC) cpu->a = v; else bus_write(bus, a, v); cpu_update_zn_flags(cpu, v); }
static void rol(CPU* cpu, Bus* bus, u16 a, AddressingMode m) { u8 v = (m == AM_ACC) ? cpu->a : bus_read(bus, a); bool old_c = cpu_get_flag(cpu, C_FLAG); cpu_set_flag(cpu, C_FLAG, v & 0x80); v = (v << 1) | old_c; if (m == AM_ACC) cpu->a = v; else bus_write(bus, a, v); cpu_update_zn_flags(cpu, v); }
static void ror(CPU* cpu, Bus* bus, u16 a, AddressingMode m) { u8 v = (m == AM_ACC) ? cpu->a : bus_read(bus, a); bool old_c = cpu_get_flag(cpu, C_FLAG); cpu_set_flag(cpu, C_FLAG, v & 1); v = (v >> 1) | (old_c << 7); if (m == AM_ACC) cpu->a = v; else bus_write(bus, a, v); cpu_update_zn_flags(cpu, v); }
static void inc(CPU* cpu, Bus* bus, u16 a) { u8 v = bus_read(bus, a) + 1; bus_write(bus, a, v); cpu_update_zn_flags(cpu, v); }
static void dec(CPU* cpu, Bus* bus, u16 a) { u8 v = bus_read(bus, a) - 1; bus_write(bus, a, v); cpu_update_zn_flags(cpu, v); }
static void lda(CPU* cpu, Bus* bus, u16 a) { cpu->a = bus_read(bus, a); cpu_update_zn_flags(cpu, cpu->a); }
static void ldx(CPU* cpu, Bus* bus, u16 a) { cpu->x = bus_read(bus, a); cpu_update_zn_flags(cpu, cpu->x); }
static void ldy(CPU* cpu, Bus* bus, u16 a) { cpu->y = bus_read(bus, a); cpu_update_zn_flags(cpu, cpu->y); }
static void sta(CPU* cpu, Bus* bus, u16 a) { bus_write(bus, a, cpu->a); }
static void stx(CPU* cpu, Bus* bus, u16 a) { bus_write(bus, a, cpu->x); }
static void sty(CPU* cpu, Bus* bus, u16 a) { bus_write(bus, a, cpu->y); }
static void tax(CPU* cpu) { cpu->x = cpu->a; cpu_update_zn_flags(cpu, cpu->x); }
static void tay(CPU* cpu) { cpu->y = cpu->a; cpu_update_zn_flags(cpu, cpu->y); }
static void tsx(CPU* cpu) { cpu->x = cpu->sp; cpu_update_zn_flags(cpu, cpu->x); }
static void txa(CPU* cpu) { cpu->a = cpu->x; cpu_update_zn_flags(cpu, cpu->a); }
static void txs(CPU* cpu) { cpu->sp = cpu->x; }
static void tya(CPU* cpu) { cpu->a = cpu->y; cpu_update_zn_flags(cpu, cpu->a); }
static void inx(CPU* cpu) { cpu->x++; cpu_update_zn_flags(cpu, cpu->x); }
static void iny(CPU* cpu) { cpu->y++; cpu_update_zn_flags(cpu, cpu->y); }
static void dex(CPU* cpu) { cpu->x--; cpu_update_zn_flags(cpu, cpu->x); }
static void dey(CPU* cpu) { cpu->y--; cpu_update_zn_flags(cpu, cpu->y); }
static void generic_and(CPU* cpu, Bus* bus, u16 a) { cpu->a &= bus_read(bus, a); cpu_update_zn_flags(cpu, cpu->a); }
static void generic_eor(CPU* cpu, Bus* bus, u16 a) { cpu->a ^= bus_read(bus, a); cpu_update_zn_flags(cpu, cpu->a); }
static void generic_ora(CPU* cpu, Bus* bus, u16 a) { cpu->a |= bus_read(bus, a); cpu_update_zn_flags(cpu, cpu->a); }
static void bit(CPU* cpu, Bus* bus, u16 a) { u8 v = bus_read(bus, a); cpu_set_flag(cpu, Z_FLAG, (v & cpu->a) == 0); cpu_set_flag(cpu, N_FLAG, v & 0x80); cpu_set_flag(cpu, V_FLAG, v & 0x40); }
static void jmp(CPU* cpu, u16 a) { cpu->pc = a; }
static void jsr(CPU* cpu, Bus* bus, u16 a) { cpu_stack_push_word(cpu, bus, cpu->pc - 1); cpu->pc = a; }
static void rts(CPU* cpu, Bus* bus) { cpu->pc = cpu_stack_pop_word(cpu, bus) + 1; }
static void rti(CPU* cpu, Bus* bus) { cpu->status = cpu_stack_pop(cpu, bus) | U_FLAG; cpu->pc = cpu_stack_pop_word(cpu, bus); }
static void pha(CPU* cpu, Bus* bus) { cpu_stack_push(cpu, bus, cpu->a); }
static void pla(CPU* cpu, Bus* bus) { cpu->a = cpu_stack_pop(cpu, bus); cpu_update_zn_flags(cpu, cpu->a); }
static void php(CPU* cpu, Bus* bus) { cpu_stack_push(cpu, bus, cpu->status | B_FLAG); }
static void plp(CPU* cpu, Bus* bus) { cpu->status = cpu_stack_pop(cpu, bus) | U_FLAG; }
static void nop(void) {}
static void brk(CPU* cpu, Bus* bus) { cpu_stack_push_word(cpu, bus, cpu->pc); cpu_stack_push(cpu, bus, cpu->status | B_FLAG); cpu_set_flag(cpu, I_FLAG, true); u8 lo = bus_read(bus, 0xFFFE); u8 hi = bus_read(bus, 0xFFFF); cpu->pc = (u16)hi << 8 | (u16)lo; }
static void clc(CPU* cpu) { cpu_set_flag(cpu, C_FLAG, false); }
static void cld(CPU* cpu) { cpu_set_flag(cpu, D_FLAG, false); }
static void cli(CPU* cpu) { cpu_set_flag(cpu, I_FLAG, false); }
static void clv(CPU* cpu) { cpu_set_flag(cpu, V_FLAG, false); }
static void sec(CPU* cpu) { cpu_set_flag(cpu, C_FLAG, true); }
static void sed(CPU* cpu) { cpu_set_flag(cpu, D_FLAG, true); }
static void sei(CPU* cpu) { cpu_set_flag(cpu, I_FLAG, true); }
static void cmp(CPU* cpu, Bus* bus, u16 a) { compare(cpu, bus, cpu->a, a); }
static void cpx(CPU* cpu, Bus* bus, u16 a) { compare(cpu, bus, cpu->x, a); }
static void cpy(CPU* cpu, Bus* bus, u16 a) { compare(cpu, bus, cpu->y, a); }
static void branch(CPU* cpu, Bus* bus, bool condition) { if (condition) { s8 offset = (s8)cpu_fetch_byte(cpu, bus); cpu->pc += offset; } else { cpu_fetch_byte(cpu, bus); } }
static void bcc(CPU* cpu, Bus* bus) { branch(cpu, bus, !cpu_get_flag(cpu, C_FLAG)); }
static void bcs(CPU* cpu, Bus* bus) { branch(cpu, bus, cpu_get_flag(cpu, C_FLAG)); }
static void beq(CPU* cpu, Bus* bus) { branch(cpu, bus, cpu_get_flag(cpu, Z_FLAG)); }
static void bmi(CPU* cpu, Bus* bus) { branch(cpu, bus, cpu_get_flag(cpu, N_FLAG)); }
static void bne(CPU* cpu, Bus* bus) { branch(cpu, bus, !cpu_get_flag(cpu, Z_FLAG)); }
static void bpl(CPU* cpu, Bus* bus) { branch(cpu, bus, !cpu_get_flag(cpu, N_FLAG)); }
static void bvc(CPU* cpu, Bus* bus) { branch(cpu, bus, !cpu_get_flag(cpu, V_FLAG)); }
static void bvs(CPU* cpu, Bus* bus) { branch(cpu, bus, cpu_get_flag(cpu, V_FLAG)); }
static void xxx(void) {}

// --- Lookup Table & Core Functions ---
typedef struct { void (*execute)(); AddressingMode mode; } OpcodeHandler;
static OpcodeHandler instruction_table[256];

void cpu_power_up(void) {
    OpcodeHandler* table = instruction_table;
    for(int i=0; i<256; ++i) table[i] = (OpcodeHandler){xxx, AM_IMP};
    #define OP(op, ex, am) instruction_table[op] = (OpcodeHandler){(void (*)())ex, am}
    OP(0x69, adc, AM_IMM); OP(0x65, adc, AM_ZP0); OP(0x75, adc, AM_ZPX); OP(0x6D, adc, AM_ABS); OP(0x7D, adc, AM_ABX); OP(0x79, adc, AM_ABY); OP(0x61, adc, AM_IZX); OP(0x71, adc, AM_IZY);
    OP(0x29, generic_and, AM_IMM); OP(0x25, generic_and, AM_ZP0); OP(0x35, generic_and, AM_ZPX); OP(0x2D, generic_and, AM_ABS); OP(0x3D, generic_and, AM_ABX); OP(0x39, generic_and, AM_ABY); OP(0x21, generic_and, AM_IZX); OP(0x31, generic_and, AM_IZY);
    OP(0x0A, asl, AM_ACC); OP(0x06, asl, AM_ZP0); OP(0x16, asl, AM_ZPX); OP(0x0E, asl, AM_ABS); OP(0x1E, asl, AM_ABX);
    OP(0x90, bcc, AM_REL); OP(0xB0, bcs, AM_REL); OP(0xF0, beq, AM_REL);
    OP(0x24, bit, AM_ZP0); OP(0x2C, bit, AM_ABS);
    OP(0x30, bmi, AM_REL); OP(0xD0, bne, AM_REL); OP(0x10, bpl, AM_REL);
    OP(0x00, brk, AM_IMP);
    OP(0x50, bvc, AM_REL); OP(0x70, bvs, AM_REL);
    OP(0x18, clc, AM_IMP); OP(0xD8, cld, AM_IMP); OP(0x58, cli, AM_IMP); OP(0xB8, clv, AM_IMP);
    OP(0xC9, cmp, AM_IMM); OP(0xC5, cmp, AM_ZP0); OP(0xD5, cmp, AM_ZPX); OP(0xCD, cmp, AM_ABS); OP(0xDD, cmp, AM_ABX); OP(0xD9, cmp, AM_ABY); OP(0xC1, cmp, AM_IZX); OP(0xD1, cmp, AM_IZY);
    OP(0xE0, cpx, AM_IMM); OP(0xE4, cpx, AM_ZP0); OP(0xEC, cpx, AM_ABS);
    OP(0xC0, cpy, AM_IMM); OP(0xC4, cpy, AM_ZP0); OP(0xCC, cpy, AM_ABS);
    OP(0xC6, dec, AM_ZP0); OP(0xD6, dec, AM_ZPX); OP(0xCE, dec, AM_ABS); OP(0xDE, dec, AM_ABX);
    OP(0xCA, dex, AM_IMP); OP(0x88, dey, AM_IMP);
    OP(0x49, generic_eor, AM_IMM); OP(0x45, generic_eor, AM_ZP0); OP(0x55, generic_eor, AM_ZPX); OP(0x4D, generic_eor, AM_ABS); OP(0x5D, generic_eor, AM_ABX); OP(0x59, generic_eor, AM_ABY); OP(0x41, generic_eor, AM_IZX); OP(0x51, generic_eor, AM_IZY);
    OP(0xE6, inc, AM_ZP0); OP(0xF6, inc, AM_ZPX); OP(0xEE, inc, AM_ABS); OP(0xFE, inc, AM_ABX);
    OP(0xE8, inx, AM_IMP); OP(0xC8, iny, AM_IMP);
    OP(0x4C, jmp, AM_ABS); OP(0x6C, jmp, AM_IND);
    OP(0x20, jsr, AM_ABS);
    OP(0xA9, lda, AM_IMM); OP(0xA5, lda, AM_ZP0); OP(0xB5, lda, AM_ZPX); OP(0xAD, lda, AM_ABS); OP(0xBD, lda, AM_ABX); OP(0xB9, lda, AM_ABY); OP(0xA1, lda, AM_IZX); OP(0xB1, lda, AM_IZY);
    OP(0xA2, ldx, AM_IMM); OP(0xA6, ldx, AM_ZP0); OP(0xB6, ldx, AM_ZPY); OP(0xAE, ldx, AM_ABS); OP(0xBE, ldx, AM_ABY);
    OP(0xA0, ldy, AM_IMM); OP(0xA4, ldy, AM_ZP0); OP(0xB4, ldy, AM_ZPY); OP(0xAC, ldy, AM_ABS); OP(0xBC, ldy, AM_ABY);
    OP(0x4A, lsr, AM_ACC); OP(0x46, lsr, AM_ZP0); OP(0x56, lsr, AM_ZPX); OP(0x4E, lsr, AM_ABS); OP(0x5E, lsr, AM_ABX);
    OP(0xEA, nop, AM_IMP);
    OP(0x09, generic_ora, AM_IMM); OP(0x05, generic_ora, AM_ZP0); OP(0x15, generic_ora, AM_ZPX); OP(0x0D, generic_ora, AM_ABS); OP(0x1D, generic_ora, AM_ABX); OP(0x19, generic_ora, AM_ABY); OP(0x01, generic_ora, AM_IZX); OP(0x11, generic_ora, AM_IZY);
    OP(0x48, pha, AM_IMP); OP(0x08, php, AM_IMP); OP(0x68, pla, AM_IMP); OP(0x28, plp, AM_IMP);
    OP(0x2A, rol, AM_ACC); OP(0x26, rol, AM_ZP0); OP(0x36, rol, AM_ZPX); OP(0x2E, rol, AM_ABS); OP(0x3E, rol, AM_ABX);
    OP(0x6A, ror, AM_ACC); OP(0x66, ror, AM_ZP0); OP(0x76, ror, AM_ZPX); OP(0x6E, ror, AM_ABS); OP(0x7E, ror, AM_ABX);
    OP(0x40, rti, AM_IMP); OP(0x60, rts, AM_IMP);
    OP(0xE9, sbc, AM_IMM); OP(0xE5, sbc, AM_ZP0); OP(0xF5, sbc, AM_ZPX); OP(0xED, sbc, AM_ABS); OP(0xFD, sbc, AM_ABX); OP(0xF9, sbc, AM_ABY); OP(0xE1, sbc, AM_IZX); OP(0xF1, sbc, AM_IZY);
    OP(0x38, sec, AM_IMP); OP(0xF8, sed, AM_IMP); OP(0x78, sei, AM_IMP);
    OP(0x85, sta, AM_ZP0); OP(0x95, sta, AM_ZPX); OP(0x8D, sta, AM_ABS); OP(0x9D, sta, AM_ABX); OP(0x99, sta, AM_ABY); OP(0x81, sta, AM_IZX); OP(0x91, sta, AM_IZY);
    OP(0x86, stx, AM_ZP0); OP(0x96, stx, AM_ZPY); OP(0x8E, stx, AM_ABS);
    OP(0x84, sty, AM_ZP0); OP(0x94, sty, AM_ZPX); OP(0x8C, sty, AM_ABS);
    OP(0xAA, tax, AM_IMP); OP(0xA8, tay, AM_IMP); OP(0xBA, tsx, AM_IMP); OP(0x8A, txa, AM_IMP); OP(0x9A, txs, AM_IMP); OP(0x98, tya, AM_IMP);
}

void cpu_init(CPU* cpu) { memset(cpu, 0, sizeof(CPU)); }
void cpu_reset(CPU* cpu, Bus* bus) { cpu->a = 0; cpu->x = 0; cpu->y = 0; cpu->sp = 0xFD; cpu->status = U_FLAG | I_FLAG; u8 lo = bus_read(bus, 0xFFFC); u8 hi = bus_read(bus, 0xFFFF); cpu->pc = (u16)hi << 8 | (u16)lo; cpu->cycles = 0; }

void cpu_step(CPU* cpu, Bus* bus) {
    u8 opcode = cpu_fetch_byte(cpu, bus);
    OpcodeHandler handler = instruction_table[opcode];
    u16 address = 0;
    if (handler.mode != AM_IMP && handler.mode != AM_ACC) {
        address = get_operand_address(cpu, bus, handler.mode);
    }

    // This is a simplified way to call functions with different signatures.
    // A more robust system might use a union or more complex wrappers.
    // This part is complex and relies on knowing the function signatures.
    // We will simplify this in the next step.
    switch (opcode) {
        // Cases for functions with specific signatures
        case 0x90: case 0xB0: case 0xF0: case 0x30: case 0xD0: case 0x10: case 0x50: case 0x70:
            ((void (*)(CPU*, Bus*))handler.execute)(cpu, bus);
            break;
        case 0x18: case 0xD8: case 0x58: case 0xB8: case 0x38: case 0xF8: case 0x78:
        case 0xCA: case 0x88: case 0xE8: case 0xC8: case 0xAA: case 0xA8: case 0xBA:
        case 0x8A: case 0x9A: case 0x98: case 0x48: case 0x08: case 0x68: case 0x28:
        case 0x40: case 0x60: case 0x00: case 0xEA:
            ((void (*)(CPU*))handler.execute)(cpu);
            break;
        default:
            ((void (*)(CPU*, Bus*, u16))handler.execute)(cpu, bus, address);
            break;
    }
}