// Turbo Grafx 16 (PC Engine) ADPCM sound chip emulator

// Game_Music_Emu 0.6-pre
#ifndef HES_APU_ADPCM_H
#define HES_APU_ADPCM_H

#include "blargg_source.h"
#include "blargg_common.h"
#include "blip_buffer.h"

enum { adpcm_amp_range = 2048 };
enum { adpcm_osc_count = 1 }; // 0 <= chan < osc_count

// Registers are at io_addr to io_addr+io_size-1
enum { adpcm_io_addr = 0x1800 };
enum { adpcm_io_size = 0x400 };

struct State
{
	byte           pcmbuf [0x10000];
	byte           port [0x10];
	int            ad_sample;
	int            ad_ref_index;
	bool           ad_low_nibble;
	int            freq;
	unsigned short addr;
	unsigned short writeptr;
	unsigned short readptr;
	unsigned short playptr;
	byte           playflag;
	byte           repeatflag;
	int            length;
	int            playlength;
	int            playedsamplecount;
	int            volume;
	int            fadetimer;
	int            fadecount;
};

struct Hes_Apu_Adpcm {
	struct State state;
	struct Blip_Synth synth;

	struct Blip_Buffer* output;
	blip_time_t  last_time;
	int          next_timer;
	int          last_amp;
};

// Init HES adpcm sound chip
void Adpcm_init( struct Hes_Apu_Adpcm* this );

// Rest HES adpcm sound chip
void Adpcm_reset( struct Hes_Apu_Adpcm* this );

// Sets buffer(s) to generate sound into, or 0 to mute. If only center is not 0,
// output is mono.
static inline void Adpcm_set_output( struct Hes_Apu_Adpcm* this, int chan, struct Blip_Buffer* center, struct Blip_Buffer* left, struct Blip_Buffer* right )
{
	// Must be silent (all NULL), mono (left and right NULL), or stereo (none NULL)
	require( !center || (center && !left && !right) || (center && left && right) );
	require( (unsigned) chan < adpcm_osc_count ); // fails if you pass invalid osc index

#if defined(ROCKBOX)
    (void) chan;
#endif

	if ( !center || !left || !right )
	{
		left  = center;
		right = center;
	}
	
	this->output = center;
}

// Emulates to time t, then writes data to addr
void Adpcm_write_data( struct Hes_Apu_Adpcm* this, blip_time_t t, int addr, int data );
	
// Emulates to time t, then reads from addr
int Adpcm_read_data( struct Hes_Apu_Adpcm* this, blip_time_t t, int addr );

// Emulates to time t, then subtracts t from the current time.
// OK if previous write call had time slightly after t.
void Adpcm_end_frame( struct Hes_Apu_Adpcm* this,blip_time_t t );

// Sets overall volume, where 1.0 is normal
static inline void Adpcm_volume( struct Hes_Apu_Adpcm* this, int v )	{ Synth_volume( &this->synth, (v*3)/5 / adpcm_osc_count / adpcm_amp_range ); }
#endif
