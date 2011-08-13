// Konami VRC7 sound chip emulator

#ifndef NES_VRC7_APU_H
#define NES_VRC7_APU_H

#include "blargg_common.h"
#include "blip_buffer.h"

#include "emu2413.h"

enum { vrc7_osc_count = 6 };

struct vrc7_osc_t {
	struct Blip_Buffer* output;
	int last_amp;
};

struct Nes_Vrc7_Apu {
	OPLL opll;
	int addr;
	blip_time_t next_time;
	struct vrc7_osc_t osc;
	struct Blip_Synth synth;
	e_uint32 mask;
};

// See Nes_Apu.h for reference
void Vrc7_init( struct Nes_Vrc7_Apu* this );
void Vrc7_reset( struct Nes_Vrc7_Apu* this );
void Vrc7_set_rate( struct Nes_Vrc7_Apu* this, int r );
void Vrc7_end_frame( struct Nes_Vrc7_Apu* this, blip_time_t );

void Vrc7_write_reg( struct Nes_Vrc7_Apu* this, int reg );
void Vrc7_write_data( struct Nes_Vrc7_Apu* this, blip_time_t, int data );

void output_changed( struct Nes_Vrc7_Apu* this );
static inline void Vrc7_set_output( struct Nes_Vrc7_Apu* this, int i, struct Blip_Buffer* buf )
{
	assert( (unsigned) i < vrc7_osc_count );
	this->mask |= 1 << i;

	// Will use OPLL_setMask to mute voices
	if ( buf ) {
		this->mask ^= 1 << i;
		this->osc.output = buf;
	}
}

// DB2LIN_AMP_BITS == 11, * 2
static inline void Vrc7_volume( struct Nes_Vrc7_Apu* this, int v ) { Synth_volume( &this->synth, v / 3 / 4096 ); }

#endif
