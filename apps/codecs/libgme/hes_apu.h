// Turbo Grafx 16 (PC Engine) PSG sound chip emulator

// Game_Music_Emu 0.5.2
#ifndef HES_APU_H
#define HES_APU_H

#include "blargg_common.h"
#include "blip_buffer.h"

enum { amp_range = 0x8000 };
enum { osc_count = 6 };
enum { start_addr = 0x0800 };
enum { end_addr   = 0x0809 };

struct Hes_Osc
{
	unsigned char wave [32];
	short volume [2];
	int last_amp [2];
	int delay;
	int period;
	unsigned char noise;
	unsigned char phase;
	unsigned char balance;
	unsigned char dac;
	blip_time_t last_time;
	
	struct Blip_Buffer* outputs [2];
	struct Blip_Buffer* chans [3];
	unsigned noise_lfsr;
	unsigned char control;
};

void Osc_run_until( struct Hes_Osc* this, struct Blip_Synth* synth, blip_time_t ) ICODE_ATTR;

struct Hes_Apu {
	struct Hes_Osc oscs [osc_count];
	
	int latch;
	int balance;
	struct Blip_Synth synth;
};

// Init HES apu sound chip
void Apu_init( struct Hes_Apu* this );

// Reset HES apu couns chip
void Apu_reset( struct Hes_Apu* this );

void Apu_osc_output( struct Hes_Apu* this, int index, struct Blip_Buffer* center, struct Blip_Buffer* left, struct Blip_Buffer* right ) ICODE_ATTR;
void Apu_write_data( struct Hes_Apu* this, blip_time_t, int addr, int data ) ICODE_ATTR;
void Apu_end_frame( struct Hes_Apu* this, blip_time_t ) ICODE_ATTR;

static inline void Apu_volume( struct Hes_Apu* this, double v ) { Synth_volume( &this->synth, 1.8 / osc_count / amp_range * v ); }
#endif
