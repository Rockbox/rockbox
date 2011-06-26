// Sunsoft FME-7 sound emulator

// Game_Music_Emu 0.6-pre
#ifndef NES_FME7_APU_H
#define NES_FME7_APU_H

#include "blargg_common.h"
#include "blip_buffer.h"

enum { fme7_reg_count = 14 };

// Mask and addresses of registers
enum { fme7_addr_mask = 0xE000 };
enum { fme7_data_addr = 0xE000 };
enum { fme7_latch_addr = 0xC000 };
enum { fme7_osc_count = 3 };

enum { amp_range = 192 }; // can be any value; this gives best error/quality tradeoff

struct osc_t {
	struct Blip_Buffer* output;
	int last_amp;
};

// static unsigned char const amp_table [16];

struct Nes_Fme7_Apu {
	// fme7 apu state
	uint8_t regs [fme7_reg_count];
	uint8_t phases [3]; // 0 or 1
	uint8_t latch;
	uint16_t delays [3]; // a, b, c
	
	struct osc_t oscs [fme7_osc_count];
	blip_time_t last_time;
	
	struct Blip_Synth synth;
};

// See Nes_Apu.h for reference
void Fme7_init( struct Nes_Fme7_Apu* this );
void Fme7_reset( struct Nes_Fme7_Apu* this );
	
static inline void Fme7_volume( struct Nes_Fme7_Apu* this, int v )
{
	Synth_volume( &this->synth, (v/2 - (v*3)/25) / amp_range ); // to do: fine-tune
}

static inline void Fme7_osc_output( struct Nes_Fme7_Apu* this, int i, struct Blip_Buffer* buf )
{
	assert( (unsigned) i < fme7_osc_count );
	this->oscs [i].output = buf;
}

static inline void Fme7_output( struct Nes_Fme7_Apu* this, struct Blip_Buffer* buf )
{
	int i;
	for ( i = 0; i < fme7_osc_count; i++ )
		Fme7_osc_output( this, i, buf );
}

// (addr & addr_mask) == latch_addr
static inline void Fme7_write_latch( struct Nes_Fme7_Apu* this, int data ) { this->latch = data; }

// (addr & addr_mask) == data_addr
void Fme7_run_until( struct Nes_Fme7_Apu* this, blip_time_t end_time );
static inline void Fme7_write_data( struct Nes_Fme7_Apu* this, blip_time_t time, int data )
{
	if ( (unsigned) this->latch >= fme7_reg_count )
	{
		#ifdef debug_printf
			debug_printf( "FME7 write to %02X (past end of sound registers)\n", (int) latch );
		#endif
		return;
	}
	
	Fme7_run_until( this, time );
	this->regs [this->latch] = data;
}

static inline void Fme7_end_frame( struct Nes_Fme7_Apu* this, blip_time_t time )
{
	if ( time > this->last_time )
		Fme7_run_until( this, time );
	
	assert( this->last_time >= time );
	this->last_time -= time;
}

#endif
