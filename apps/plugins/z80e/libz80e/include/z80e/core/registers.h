#ifndef REGISTERS_H
#define REGISTERS_H
#include <stdint.h>

typedef struct {
	union {
		uint16_t AF;
		struct {
			union {
				uint8_t F;
				struct {
					uint8_t C  : 1;
					uint8_t N  : 1;
					uint8_t PV : 1;
					uint8_t _3 : 1;
					uint8_t H  : 1;
					uint8_t _5 : 1;
					uint8_t Z  : 1;
					uint8_t S  : 1;
				} flags;
			};
			uint8_t A;
		};
	};
	union {
		uint16_t BC;
		struct {
			uint8_t C;
			uint8_t B;
		};
	};
	union {
		uint16_t DE;
		struct {
			uint8_t E;
			uint8_t D;
		};
	};
	union {
		uint16_t HL;
		struct {
			uint8_t L;
			uint8_t H;
		};
	};
	uint16_t _AF, _BC, _DE, _HL;
	uint16_t PC, SP;
	union {
		uint16_t IX;
		struct {
			uint8_t IXL;
			uint8_t IXH;
		};
	};
	union {
		uint16_t IY;
		struct {
			uint8_t IYL;
			uint8_t IYH;
		};
	};
	uint8_t I, R;
} z80registers_t;

typedef enum {
	FLAG_S =  1 << 7,
	FLAG_Z =  1 << 6,
	FLAG_5 =  1 << 5,
	FLAG_H =  1 << 4,
	FLAG_3 =  1 << 3,
	FLAG_PV = 1 << 2,
	FLAG_N  = 1 << 1,
	FLAG_C  = 1 << 0,
	FLAG_NONE = 0
} z80flags;

typedef enum {
	A = (1 << 0), F = (1 << 1), AF = (1 << 2), _AF = (1 << 3),
	B = (1 << 4), C = (1 << 5), BC = (1 << 6), _BC = (1 << 7),
	D = (1 << 8), E = (1 << 9), DE = (1 << 10), _DE = (1 << 11),
	H = (1 << 12), L = (1 << 13), HL = (1 << 14), _HL = (1 << 15),
	PC = (1 << 16), SP = (1 << 17), I = (1 << 18), R = (1 << 19),
	IXH = (1 << 20), IXL = (1 << 21), IX = (1 << 22),
	IYH = (1 << 23), IYL = (1 << 24), IY = (1 << 25),
} registers;

void exAFAF(z80registers_t *r);
void exDEHL(z80registers_t *r);
void exx(z80registers_t *r);
void updateParity(z80registers_t *r, uint32_t newValue);
int popcount(uint64_t x);

// S Z 5 H 3 PV N C
#define __flag_s(a)  ((a) ? FLAG_S  : 0)
#define __flag_5(a)  ((a) ? FLAG_5  : 0)
#define __flag_h(a)  ((a) ? FLAG_H  : 0)
#define __flag_3(a)  ((a) ? FLAG_3  : 0)
#define __flag_pv(a) ((a) ? FLAG_PV : 0)
#define __flag_c(a)  ((a) ? FLAG_C  : 0)

#define _flag_carry_16(a) __flag_c((a) & 0x10000)
#define _flag_carry_8(a)  __flag_c((a) & 0x100)

#define _flag_sign_16(a)  __flag_s((a) & 0x8000)
#define _flag_sign_8(a)   __flag_s((a) & 0x80)

#define _flag_parity(a) __flag_pv(!(popcount(a) % 2))

#define _flag_undef_8(a) ({ uint8_t _res = (a); __flag_5(_res & 0x20) | __flag_3(_res & 0x8);})
#define _flag_undef_16(a) ({ uint16_t _res = (a); __flag_5(_res & 0x2000) | __flag_3(_res & 0x800);})

#define _flag_overflow_8_add(op1, op2, result) __flag_pv((op1 & 0x80) == (op2 & 0x80) && (op1 & 0x80) != (result & 0x80))
#define _flag_overflow_16_add(op1, op2, result) __flag_pv((op1 & 0x8000) == (op2 & 0x8000) && (op1 & 0x8000) != (result & 0x8000))

#define _flag_overflow_8_sub(op1, op2, result) \
	__flag_pv( \
		(op1 & 0x80) != (op2 & 0x80) && \
		(op1 & 0x80) != (result & 0x80))
#define _flag_overflow_16_sub(op1, op2, result) __flag_pv((op1 & 0x8000) != (op2 & 0x8000) && (op1 & 0x8000) != (result & 0x8000))

#define _flag_halfcarry_8_add(op1, op2) __flag_h(((op1 & 0xf) + (op2 & 0xf)) & 0x10)
#define _flag_halfcarry_8_sub(op1, op2) __flag_h(((op1 & 0xf) - (op2 & 0xf)) & 0x10)
#define _flag_halfcarry_16_add(op1, op2) __flag_h(((op1 & 0xfff) + (op2 & 0xfff)) & 0x1000)
#define _flag_halfcarry_16_sub(op1, op2) __flag_h(((op1 & 0xfff) - (op2 & 0xfff)) & 0x1000)

#define _flag_subtract(a)   ((a) ? FLAG_N : 0)
#define _flag_zero(a)       ((a) ? 0 : FLAG_Z)

void print_state(z80registers_t *);

#endif
