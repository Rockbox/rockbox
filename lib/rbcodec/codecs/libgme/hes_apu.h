// Turbo Grafx 16 (PC Engine) PSG sound chip emulator

// Game_Music_Emu 0.5.2
#ifndef HES_APU_H
#define HES_APU_H

#include "blargg_common.h"
#include "blargg_source.h"
#include "blip_buffer.h"

enum { amp_range = 0x8000 };
enum { osc_count = 6 }; // 0 <= chan < osc_count

// Registers are at io_addr to io_addr+io_size-1
enum { apu_io_addr = 0x0800 };
enum { apu_io_size = 10 };

struct Hes_Osc
{
	byte wave [32];
	int  delay;
	int  period;
	int  phase;
	
	int  noise_delay;
	byte noise;
	unsigned lfsr;
	
	byte control;
	byte balance;
	byte dac;
	short volume [2];
	int last_amp [2];
	
	blip_time_t last_time;
	struct Blip_Buffer* output [2];
	struct Blip_Buffer* outputs [3];
};

void Osc_run_until( struct Hes_Osc* this, struct Blip_Synth* synth, blip_time_t );

struct Hes_Apu {
	
	int latch;
	int balance;
	struct Blip_Synth synth;
	struct Hes_Osc oscs [osc_count];
};

// Init HES apu sound chip
void Apu_init( struct Hes_Apu* this );

// Reset HES apu couns chip
void Apu_reset( struct Hes_Apu* this );

void Apu_osc_output( struct Hes_Apu* this, int index, struct Blip_Buffer* center, struct Blip_Buffer* left, struct Blip_Buffer* right );

// Emulates to time t, then writes data to addr
void Apu_write_data( struct Hes_Apu* this, blip_time_t, int addr, int data );

// Emulates to time t, then subtracts t from the current time.
// OK if previous write call had time slightly after t.
void Apu_end_frame( struct Hes_Apu* this, blip_time_t );

static inline void Apu_volume( struct Hes_Apu* this, int v ) { Synth_volume( &this->synth, (v*9)/5 / osc_count / amp_range ); }
#endif
