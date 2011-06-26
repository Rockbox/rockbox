// YM2413 FM sound chip emulator interface

// Game_Music_Emu 0.6-pre
#ifndef YM2413_EMU_H
#define YM2413_EMU_H

#include "blargg_common.h"
#include "emu2413.h"

enum { out_chan_count = 2 }; // stereo
enum { channel_count = 14 };
enum { disabled_time = -1 };

struct Ym2413_Emu  {
	OPLL opll;
		
	// Impl
	int last_time;
	short* out;
};

void Ym2413_init( struct Ym2413_Emu* this );

static inline bool Ym2413_supported( void ) { return true; }
	
// Sets output sample rate and chip clock rates, in Hz. Returns non-zero
// if error.
int Ym2413_set_rate( struct Ym2413_Emu* this, int sample_rate, int clock_rate );
	
// Resets to power-up state
void Ym2413_reset( struct Ym2413_Emu* this );
	
// Mutes voice n if bit n (1 << n) of mask is set
void Ym2413_mute_voices( struct Ym2413_Emu* this, int mask );
	
// Writes data to addr
void Ym2413_write( struct Ym2413_Emu* this, int addr, int data );
	
// Runs and writes pair_count*2 samples to output
void Ym2413_run( struct Ym2413_Emu* this, int pair_count, short* out );

static inline void Ym2413_enable( struct Ym2413_Emu* this, bool b ) { this->last_time = b ? 0 : disabled_time; }
static inline bool Ym2413_enabled( struct Ym2413_Emu* this ) { return this->last_time != disabled_time; }
static inline void Ym2413_begin_frame( struct Ym2413_Emu* this, short* buf ) { this->out = buf; this->last_time = 0; }
	
static inline int Ym2413_run_until( struct Ym2413_Emu* this, int time )
{
	int count = time - this->last_time;
	if ( count > 0 )
	{
		if ( this->last_time < 0 )
			return false;
		this->last_time = time;
		short* p = this->out;
		this->out += count * out_chan_count;
		Ym2413_run( this, count, p );
	}
	return true;
}

#endif
