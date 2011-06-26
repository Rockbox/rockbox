// PC Engine CPU emulator for use with HES music files

// Game_Music_Emu 0.6-pre
#ifndef HES_CPU_H
#define HES_CPU_H

#include "blargg_common.h"
#include "blargg_source.h"

typedef int hes_time_t; // clock cycle count
typedef int hes_addr_t; // 16-bit address

struct Hes_Emu;

enum { future_time = INT_MAX/2 + 1 };
enum { page_bits = 13 };
enum { page_size = 1 << page_bits };
enum { page_count = 0x10000 / page_size };

// Can read this many bytes past end of a page
enum { cpu_padding = 8 };
enum { irq_inhibit_mask = 0x04 };
enum { idle_addr = 0x1FFF };

// Cpu state
struct cpu_state_t {
	byte const* code_map [page_count + 1];
	hes_time_t base;
	int time;
};

// NOT kept updated during emulation.
struct registers_t {
	uint16_t pc;
	byte a;
	byte x;
	byte y;
	byte flags;
	byte sp;
};

struct Hes_Cpu {
	struct registers_t r;

	hes_time_t irq_time_;
	hes_time_t end_time_;
	
	struct cpu_state_t* cpu_state; // points to state_ or a local copy within run()
	struct cpu_state_t cpu_state_;
	
	// page mapping registers
	uint8_t mmr [page_count + 1];
	uint8_t ram [page_size];
};

// Init cpu state
static inline void Cpu_init( struct Hes_Cpu* this )
{
	this->cpu_state = &this->cpu_state_;
}

// Reset hes cpu
void Cpu_reset( struct Hes_Cpu* this );

// Set end_time and run CPU from current time. Returns true if any illegal
// instructions were encountered.
bool Cpu_run( struct Hes_Emu* this, hes_time_t end_time );

// Time of ning of next instruction to be executed
static inline hes_time_t Cpu_time( struct Hes_Cpu* this )
{
	return this->cpu_state->time + this->cpu_state->base;
}

static inline void Cpu_set_time( struct Hes_Cpu* this, hes_time_t t )    { this->cpu_state->time = t - this->cpu_state->base; }
static inline void Cpu_adjust_time( struct Hes_Cpu* this, int delta )    { this->cpu_state->time += delta; }

#define HES_CPU_PAGE( addr ) ((unsigned) (addr) >> page_bits)

#ifdef BLARGG_NONPORTABLE
	#define HES_CPU_OFFSET( addr ) (addr)
#else
	#define HES_CPU_OFFSET( addr ) ((addr) & (page_size - 1))
#endif

static inline uint8_t const* Cpu_get_code( struct Hes_Cpu* this, hes_addr_t addr )
{
	return this->cpu_state_.code_map [HES_CPU_PAGE( addr )] + HES_CPU_OFFSET( addr );
}

static inline void update_end_time( struct Hes_Cpu* this, hes_time_t end, hes_time_t irq )
{
	if ( end > irq && !(this->r.flags & irq_inhibit_mask) )
		end = irq;
	
	this->cpu_state->time += this->cpu_state->base - end;
	this->cpu_state->base = end;
}

static inline hes_time_t Cpu_end_time( struct Hes_Cpu* this ) { return this->end_time_; }

static inline void Cpu_set_irq_time( struct Hes_Cpu* this, hes_time_t t )
{
	this->irq_time_ = t;
	update_end_time( this, this->end_time_, t );
}

static inline void Cpu_set_end_time( struct Hes_Cpu* this, hes_time_t t )
{
	this->end_time_ = t;
	update_end_time( this, t, this->irq_time_ );
}

static inline void Cpu_end_frame( struct Hes_Cpu* this, hes_time_t t )
{
	assert( this->cpu_state == &this->cpu_state_ );
	this->cpu_state_.base -= t;
	if ( this->irq_time_ < future_time ) this->irq_time_ -= t;
	if ( this->end_time_ < future_time ) this->end_time_ -= t;
}

static inline void Cpu_set_mmr( struct Hes_Cpu* this, int reg, int bank, void const* code )
{
	assert( (unsigned) reg <= page_count ); // allow page past end to be set
	assert( (unsigned) bank < 0x100 );
	this->mmr [reg] = bank;
	byte const* p = STATIC_CAST(byte const*,code) - HES_CPU_OFFSET( reg << page_bits );
	this->cpu_state->code_map [reg] = p;
	this->cpu_state_.code_map [reg] = p;
}

#endif
