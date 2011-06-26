// Nintendo Game Boy CPU emulator

// Game_Music_Emu 0.6-pre
#ifndef GB_CPU_H
#define GB_CPU_H

#include "blargg_common.h"
#include "blargg_source.h"

typedef int addr_t;

// Emulator reads this many bytes past end of a page
enum { cpu_padding = 8 };
enum { mem_size = 0x10000 };
enum { page_bits = 13 };
enum { page_size = 1 << page_bits };
enum { page_count = mem_size >> page_bits };

// Game Boy Z-80 registers. NOT kept updated during emulation.
struct core_regs_t {
	uint16_t bc, de, hl, fa;
};
	
struct registers_t {
	int pc; // more than 16 bits to allow overflow detection
	uint16_t sp;

	struct core_regs_t rp;
};

struct cpu_state_t {
	byte* code_map [page_count + 1];
	int time;
};

struct Gb_Cpu {	
	// Base address for RST vectors, to simplify GBS player (normally 0)
	addr_t rst_base;
	
	struct registers_t r;
	struct cpu_state_t* cpu_state; // points to state_ or a local copy within run()
	struct cpu_state_t cpu_state_;
};

// Initializes Gb cpu
static inline void Cpu_init( struct Gb_Cpu* this )
{
	this->rst_base = 0;
	this->cpu_state = &this->cpu_state_;
}

// Clears registers and map all pages to unmapped
void Cpu_reset( struct Gb_Cpu* this, void* unmapped );
	
// Maps code memory (memory accessed via the program counter). Start and size
// must be multiple of page_size.
void Cpu_map_code( struct Gb_Cpu* this, addr_t start, int size, void* code );
			
// Current time.
static inline int Cpu_time( struct Gb_Cpu* this ) { return this->cpu_state->time; }
	
// Changes time. Must not be called during emulation.
// Should be negative, because emulation stops once it becomes >= 0.
static inline void Cpu_set_time( struct Gb_Cpu* this, int t ) { this->cpu_state->time = t; }

#define GB_CPU_PAGE( addr ) ((unsigned) (addr) >> page_bits)

#ifdef BLARGG_NONPORTABLE
	#define GB_CPU_OFFSET( addr ) (addr)
#else
	#define GB_CPU_OFFSET( addr ) ((addr) & (page_size - 1))
#endif

// Accesses emulated memory as CPU does
static inline uint8_t* Cpu_get_code( struct Gb_Cpu* this, addr_t addr )
{
	return this->cpu_state_.code_map [GB_CPU_PAGE( addr )] + GB_CPU_OFFSET( addr );
}

#endif
