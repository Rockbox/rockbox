#include <stdio.h>
#include <stdint.h>
#include "core/registers.h"

void exAFAF(z80registers_t *r) {
	uint16_t temp = r->_AF;
	r->_AF = r->AF;
	r->AF = temp;
}

void exDEHL(z80registers_t *r) {
	uint16_t temp = r->HL;
	r->HL = r->DE;
	r->DE = temp;
}

void exx(z80registers_t *r) {
	uint16_t temp;
	temp = r->_HL; r->_HL = r->HL; r->HL = temp;
	temp = r->_DE; r->_DE = r->DE; r->DE = temp;
	temp = r->_BC; r->_BC = r->BC; r->BC = temp;
}

const uint64_t m1  = 0x5555555555555555;
const uint64_t m2  = 0x3333333333333333;
const uint64_t m4  = 0x0f0f0f0f0f0f0f0f;
const uint64_t h01 = 0x0101010101010101;
int popcount(uint64_t x) {
	x -= (x >> 1) & m1;
	x = (x & m2) + ((x >> 2) & m2);
	x = (x + (x >> 4)) & m4;
	return (x * h01) >> 56;
}

void print_state(z80registers_t *r) {
	printf("   AF: 0x%04X   BC: 0x%04X   DE: 0x%04X  HL: 0x%04X\n", r->AF, r->BC, r->DE, r->HL);
	printf("  'AF: 0x%04X  'BC: 0x%04X  'DE: 0x%04X 'HL: 0x%04X\n", r->_AF, r->_BC, r->_DE, r->_HL);
	printf("   PC: 0x%04X   SP: 0x%04X   IX: 0x%04X  IY: 0x%04X\n", r->PC, r->SP, r->IX, r->IY);
	printf("Flags: ");
	if (r->flags.S) printf("S ");
	if (r->flags.Z) printf("Z ");
	if (r->flags.H) printf("H ");
	if (r->flags._3) printf("5 ");
	if (r->flags.PV) printf("P/V ");
	if (r->flags._5) printf("3 ");
	if (r->flags.N) printf("N ");
	if (r->flags.C) printf("C ");
	if (r->F == 0) printf("None set");
	printf("\n");
}
