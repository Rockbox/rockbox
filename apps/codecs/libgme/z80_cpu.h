// Z80 CPU emulator

// Game_Music_Emu 0.6-pre
#ifndef Z80_CPU_H
#define Z80_CPU_H

#include "blargg_source.h"
#include "blargg_endian.h"

typedef int cpu_time_t;
typedef int addr_t;

enum { page_bits = 10 };
enum { page_size = 1 << page_bits };
enum { page_count = 0x10000 / page_size };

// Can read this far past end of memory
enum { cpu_padding = 0x100 };

// Can read this many bytes past end of a page
enum { page_padding = 4 };

#ifdef BLARGG_BIG_ENDIAN
	struct regs_t { byte b,c, d,e, h,l, flags,a; };
#else
	struct regs_t { byte c,b, e,d, l,h, a,flags; };
#endif
// BOOST_STATIC_ASSERT( sizeof (regs_t) == 8 );
	
struct pairs_t { uint16_t bc, de, hl, fa; };
	
// Registers are not updated until run() returns
struct registers_t {
	uint16_t pc;
	uint16_t sp;
	uint16_t ix;
	uint16_t iy;
	union {
		struct regs_t b; //  b.b, b.c, b.d, b.e, b.h, b.l, b.flags, b.a
		struct pairs_t w; // w.bc, w.de, w.hl. w.fa
	};
	union {
		struct regs_t b;
		struct pairs_t w;
	} alt;
	byte iff1;
	byte iff2;
	byte r;
	byte i;
	byte im;
};

struct cpu_state_t {
	byte const* read  [page_count + 1];
	byte      * write [page_count + 1];
	cpu_time_t base;
	cpu_time_t time;
};

struct Z80_Cpu {
	byte szpc [0x200];
	cpu_time_t end_time_;
	
	struct cpu_state_t* cpu_state; // points to cpu_state_ or a local copy within run()
	struct cpu_state_t cpu_state_;

	struct registers_t r;
};

void Z80_init( struct Z80_Cpu* this );

// Clears registers and maps all pages to unmapped
void Z80_reset( struct Z80_Cpu* this, void* unmapped_write, void const* unmapped_read );
	
// TODO: split mapping out of CPU
	
// Maps memory. Start and size must be multiple of page_size.
void Z80_map_mem( struct Z80_Cpu* this, addr_t addr, int size, void* write, void const* read );
	
// Time of beginning of next instruction
static inline cpu_time_t Z80_time( struct Z80_Cpu* this ) { return this->cpu_state->time + this->cpu_state->base; }
	
// Alter current time
static inline void Z80_set_time( struct Z80_Cpu* this, cpu_time_t t ) { this->cpu_state->time = t - this->cpu_state->base; }
static inline void Z80_adjust_time( struct Z80_Cpu* this, int delta ) { this->cpu_state->time += delta; }

#ifdef BLARGG_NONPORTABLE
	#define Z80_CPU_OFFSET( addr ) (addr)
#else
	#define Z80_CPU_OFFSET( addr ) ((addr) & (page_size - 1))
#endif

// Maps address to pointer to that byte
static inline byte* Z80_write( struct Z80_Cpu* this, addr_t addr )
{
	return this->cpu_state->write [(unsigned) addr >> page_bits] + Z80_CPU_OFFSET( addr );
}

static inline byte const* Z80_read( struct Z80_Cpu* this, addr_t addr )
{
	return this->cpu_state->read [(unsigned) addr >> page_bits] + Z80_CPU_OFFSET( addr );
}

static inline void Z80_map_mem_rw( struct Z80_Cpu* this, addr_t addr, int size, void* p )
{
	Z80_map_mem( this, addr, size, p, p );
}

static inline void Z80_set_end_time( struct Z80_Cpu* this, cpu_time_t t )
{
	cpu_time_t delta = this->cpu_state->base - t;
	this->cpu_state->base = t;
	this->cpu_state->time += delta;
}

#endif
