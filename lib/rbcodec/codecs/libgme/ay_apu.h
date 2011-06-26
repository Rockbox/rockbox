// AY-3-8910 sound chip ulator

// Game_Music_Emu 0.6-pre
#ifndef AY_APU_H
#define AY_APU_H

#include "blargg_common.h"
#include "blargg_source.h"
#include "blip_buffer.h"

// Number of registers
enum { ay_reg_count = 16 };
enum { ay_osc_count = 3 };
enum { ay_amp_range = 255 };

struct osc_t
{
	blip_time_t  period;
	blip_time_t  delay;
	short        last_amp;
	short        phase;
	struct Blip_Buffer* output;
};

struct Ay_Apu {
	struct osc_t oscs [ay_osc_count];
	
	blip_time_t last_time;
	byte        addr_;
	byte        regs [ay_reg_count];
	
	blip_time_t noise_delay;
	unsigned    noise_lfsr;
	
	blip_time_t env_delay;
	byte const* env_wave;
	int         env_pos;
	byte        env_modes [8] [48]; // values already passed through volume table
		
	struct Blip_Synth synth_; // used by Ay_Core for beeper sound
};
	
void Ay_apu_init( struct Ay_Apu* this );

// Writes to address register
static inline void Ay_apu_write_addr( struct Ay_Apu* this, int data ) { this->addr_ = data & 0x0F; }
	
// Emulates to time t, then writes to current data register
void run_until( struct Ay_Apu* this, blip_time_t final_end_time );;
void write_data_( struct Ay_Apu* this, int addr, int data );
static inline void Ay_apu_write_data( struct Ay_Apu* this, blip_time_t t, int data )  { run_until( this, t ); write_data_( this, this->addr_, data ); }
	
// Reads from current data register
int Ay_apu_read( struct Ay_Apu* this );
	
// Resets sound chip
void Ay_apu_reset( struct Ay_Apu* this );
		
// Sets overall volume, where 1.0 is normal
static inline void Ay_apu_volume( struct Ay_Apu* this, int v ) { Synth_volume( &this->synth_, (v*7)/10 /ay_osc_count/ay_amp_range ); }

static inline void Ay_apu_set_output( struct Ay_Apu* this, int i, struct Blip_Buffer* out )
{
	assert( (unsigned) i < ay_osc_count );
	this->oscs [i].output = out;
}

// Emulates to time t, then subtracts t from the current time.
// OK if previous write call had time slightly after t.
static inline void Ay_apu_end_frame( struct Ay_Apu* this, blip_time_t time )
{
	if ( time > this->last_time )
		run_until( this, time );
	
	this->last_time -= time;
	assert( this->last_time >= 0 );
}

#endif
