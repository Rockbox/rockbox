// PC Engine CPU emulator for use with HES music files

// Game_Music_Emu 0.5.2
#ifndef HES_CPU_H
#define HES_CPU_H

#include "blargg_common.h"

typedef blargg_long hes_time_t; // clock cycle count
typedef unsigned hes_addr_t; // 16-bit address

struct Hes_Emu;

enum { future_hes_time = LONG_MAX / 2 + 1 };
enum { page_size = 0x2000 };
enum { page_shift = 13 };
enum { page_count = 8 };

// Attempt to execute instruction here results in CPU advancing time to
// lesser of irq_time() and end_time() (or end_time() if IRQs are
// disabled)
enum { idle_addr = 0x1FFF };
	
// Can read this many bytes past end of a page
enum { cpu_padding = 8 };
enum { irq_inhibit = 0x04 };


// Cpu state
struct state_t {
	uint8_t const* code_map [page_count + 1];
	hes_time_t base;
	blargg_long time;
};

// Cpu registers
struct registers_t {
	uint16_t pc;
	uint8_t a;
	uint8_t x;
	uint8_t y;
	uint8_t status;
	uint8_t sp;
};

struct Hes_Cpu {
	struct registers_t r;

	hes_time_t irq_time;
	hes_time_t end_time;
	
	struct state_t* state; // points to state_ or a local copy within run()
	struct state_t state_;
	
	// page mapping registers
	uint8_t mmr [page_count + 1];
	uint8_t ram [page_size];
};

// Init cpu state
void Cpu_init( struct Hes_Cpu* this );

// Reset hes cpu
void Cpu_reset( struct Hes_Cpu* this );

// Set end_time and run CPU from current time. Returns true if any illegal
// instructions were encountered.
bool Cpu_run( struct Hes_Emu* this, hes_time_t end_time ) ICODE_ATTR;

void Cpu_set_mmr( struct Hes_Emu* this, int reg, int bank ) ICODE_ATTR;

// Time of ning of next instruction to be executed
static inline hes_time_t Cpu_time( struct Hes_Cpu* this )
{
	return this->state->time + this->state->base;
}

static inline uint8_t const* Cpu_get_code( struct Hes_Cpu* this, hes_addr_t addr )
{
	return this->state->code_map [addr >> page_shift] + addr
	#if !defined (BLARGG_NONPORTABLE)
		% (unsigned) page_size
	#endif
	;
}

static inline int Cpu_update_end_time( struct Hes_Cpu* this, uint8_t reg_status, hes_time_t t, hes_time_t irq )
{
	if ( irq < t && !(reg_status & irq_inhibit) ) t = irq;
	int delta = this->state->base - t;
	this->state->base = t;
	return delta;
}

#endif
