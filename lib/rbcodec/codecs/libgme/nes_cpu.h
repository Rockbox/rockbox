// NES cpu emulator

// Game_Music_Emu 0.6-pre
#ifndef NES_CPU_H
#define NES_CPU_H

#include "blargg_common.h"
#include "blargg_source.h"

typedef int nes_time_t;
typedef int addr_t;

enum { page_bits = 11 };
enum { page_size = 1 << page_bits };
enum { page_count = 0x10000 >> page_bits };

// Unmapped page should be filled with this
enum { halt_opcode = 0x22 };

enum { future_time = INT_MAX/2 + 1 };
enum { irq_inhibit_mask = 0x04 };
	
// Can read this many bytes past end of a page
enum { cpu_padding = 8 };

struct registers_t {
	uint16_t pc;
	uint8_t a;
	uint8_t x;
	uint8_t y;
	uint8_t flags;
	uint8_t sp;
};

struct cpu_state_t {
	uint8_t const* code_map [page_count + 1];
	nes_time_t base;
	int time;
};

struct Nes_Cpu {
	// NES 6502 registers. NOT kept updated during emulation.
	struct registers_t r;
	nes_time_t irq_time;
	nes_time_t end_time;
	
	struct cpu_state_t* cpu_state; // points to cpu_state_ or a local copy
	struct cpu_state_t cpu_state_;
};

static inline void Cpu_init( struct Nes_Cpu* this )
{
	this->cpu_state = &this->cpu_state_;
}

// Clears registers and maps all pages to unmapped_page
void Cpu_reset( struct Nes_Cpu* this, void const* unmapped_page );

// Maps code memory (memory accessed via the program counter). Start and size
// must be multiple of page_size. If mirror_size is non-zero, the first
// mirror_size bytes are repeated over the range. mirror_size must be a
// multiple of page_size.
void Cpu_map_code( struct Nes_Cpu* this, addr_t start, int size, void const* code, int mirror_size );
	
// Time of beginning of next instruction to be executed
static inline nes_time_t Cpu_time( struct Nes_Cpu* this ) { return this->cpu_state->time + this->cpu_state->base; }
static inline void Cpu_set_time( struct Nes_Cpu* this, nes_time_t t ) { this->cpu_state->time = t - this->cpu_state->base; }
static inline void Cpu_adjust_time( struct Nes_Cpu* this, int delta ) { this->cpu_state->time += delta; }
	
// Clocks past end (negative if before)
static inline int Cpu_time_past_end( struct Nes_Cpu* this ) { return this->cpu_state->time; }
			
#define NES_CPU_PAGE( addr ) ((unsigned) (addr) >> page_bits)

#ifdef BLARGG_NONPORTABLE
	#define NES_CPU_OFFSET( addr ) (addr)
#else
	#define NES_CPU_OFFSET( addr ) ((addr) & (page_size - 1))
#endif

// Accesses emulated memory as cpu does
static inline uint8_t const* Cpu_get_code( struct Nes_Cpu* this, addr_t addr )
{
	return this->cpu_state_.code_map [NES_CPU_PAGE( addr )] + NES_CPU_OFFSET( addr );
}

static inline void Cpu_update_end_time( struct Nes_Cpu* this, nes_time_t end, nes_time_t irq )
{
	if ( end > irq && !(this->r.flags & irq_inhibit_mask) )
		end = irq;
	
	this->cpu_state->time += this->cpu_state->base - end;
	this->cpu_state->base = end;
}

// Time of next IRQ
static inline void Cpu_set_irq_time( struct Nes_Cpu* this, nes_time_t t )
{
	this->irq_time = t;
	Cpu_update_end_time( this, this->end_time, t );
}

static inline void Cpu_set_end_time( struct Nes_Cpu* this, nes_time_t t )
{
	this->end_time = t;
	Cpu_update_end_time( this, t, this->irq_time );
}

#endif
