// Konami VRC6 sound chip emulator

// Nes_Snd_Emu 0.2.0-pre
#ifndef NES_VRC6_APU_H
#define NES_VRC6_APU_H

#include "blargg_common.h"
#include "blip_buffer.h"

enum { vrc6_osc_count = 3 };
enum { vrc6_reg_count = 3 };
enum { vrc6_base_addr = 0x9000 };
enum { vrc6_addr_step = 0x1000 };

struct Vrc6_Osc
{
	uint8_t regs [3];
	struct Blip_Buffer* output;
	int delay;
	int last_amp;
	int phase;
	int amp; // only used by saw
};

static inline int Vrc6_osc_period( struct Vrc6_Osc* this )
{
	return (this->regs [2] & 0x0F) * 0x100 + this->regs [1] + 1;
}

struct Nes_Vrc6_Apu {
	struct Vrc6_Osc oscs [vrc6_osc_count];
	blip_time_t last_time;
	
	struct Blip_Synth saw_synth;
	struct Blip_Synth square_synth;
};

// See Nes_Apu.h for reference
void Vrc6_init( struct Nes_Vrc6_Apu* this );
void Vrc6_reset( struct Nes_Vrc6_Apu* this );
void Vrc6_output( struct Nes_Vrc6_Apu* this, struct Blip_Buffer* );
void Vrc6_end_frame( struct Nes_Vrc6_Apu* this, blip_time_t );
	
// Oscillator 0 write-only registers are at $9000-$9002
// Oscillator 1 write-only registers are at $A000-$A002
// Oscillator 2 write-only registers are at $B000-$B002
void Vrc6_write_osc( struct Nes_Vrc6_Apu* this, blip_time_t, int osc, int reg, int data );

static inline void Vrc6_osc_output( struct Nes_Vrc6_Apu* this, int i, struct Blip_Buffer* buf )
{
	assert( (unsigned) i < vrc6_osc_count );
	this->oscs [i].output = buf;
}

static inline void Vrc6_volume( struct Nes_Vrc6_Apu* this, int v )
{
	long long const factor = (long long)(FP_ONE_VOLUME * 0.0967 * 2);
	Synth_volume( &this->saw_synth,    (int)(v * factor / 31 / FP_ONE_VOLUME) );
	Synth_volume( &this->square_synth, (int)(v * factor / 2 / 15 / FP_ONE_VOLUME) );
}

#endif
