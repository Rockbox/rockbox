// Konami SCC sound chip emulator

// Game_Music_Emu 0.6-pre
#ifndef KSS_SCC_APU_H
#define KSS_SCC_APU_H

#include "blargg_common.h"
#include "blip_buffer.h"

enum { scc_reg_count = 0xB0 }; // 0 <= reg < reg_count
enum { scc_osc_count = 5 };
enum { scc_amp_range = 0x8000 };

struct scc_osc_t
{
	int delay;
	int phase;
	int last_amp;
	struct Blip_Buffer* output;
};

struct Scc_Apu {
	struct scc_osc_t oscs [scc_osc_count];
	blip_time_t last_time;
	unsigned char regs [scc_reg_count];

	struct Blip_Synth synth;
};

void Scc_init( struct Scc_Apu* this );

// Resets sound chip
void Scc_reset( struct Scc_Apu* this );

// Set overall volume, where 1.0 is normal
void Scc_volume( struct Scc_Apu* this, int v );

static inline void Scc_set_output( struct Scc_Apu* this, int index, struct Blip_Buffer* b )
{
	assert( (unsigned) index < scc_osc_count );
	this->oscs [index].output = b;
}

// Emulates to time t, then writes data to reg
void Scc_write( struct Scc_Apu* this, blip_time_t time, int addr, int data );

// Emulates to time t, then subtracts t from the current time.
// OK if previous write call had time slightly after t.
void Scc_end_frame( struct Scc_Apu* this, blip_time_t end_time );

#endif
