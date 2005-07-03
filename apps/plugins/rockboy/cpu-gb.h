

#ifndef __CPU_GB_H__
#define __CPU_GB_H__



#include "defs.h"


union reg
{
	byte b[2][2];
	word w[2];
	un32 d; /* padding for alignment, carry */
};

struct cpu
{
#ifdef DYNAREC
	union reg a,b,c,d,e,hl,f,sp,pc;
#else
	union reg pc, sp, bc, de, hl, af;
#endif
	int ime, ima;
	int speed;
	int halt;
	int div, tim;
	int lcdc;
	int snd;
};

extern struct cpu cpu;

#ifdef DYNAREC
struct dynarec_block {
    union reg address;
    void *block;
    int length;
    struct dynarec_block *next;
};

#define HASH_SIGNIFICANT_LOWER_BITS 8
#define HASH_BITMASK ((1<<HASH_SIGNIFICANT_LOWER_BITS)-1)

extern struct dynarec_block *address_map[1<<HASH_SIGNIFICANT_LOWER_BITS];
extern int blockclen;
#endif

void cpu_reset(void);
void div_advance(int cnt);
void timer_advance(int cnt);
void lcdc_advance(int cnt);
void sound_advance(int cnt);
void cpu_timers(int cnt);
int cpu_emulate(int cycles) ICODE_ATTR;
int cpu_step(int max) ICODE_ATTR;

#endif
