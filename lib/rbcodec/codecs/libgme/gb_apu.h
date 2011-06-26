// Nintendo Game Boy sound hardware emulator with save state support

// Gb_Snd_Emu 0.1.4
#ifndef GB_APU_H
#define GB_APU_H

#include "gb_oscs.h"

// Clock rate sound hardware runs at
enum { clock_rate = 4194304 * GB_APU_OVERCLOCK };
	
// Registers are at io_addr to io_addr+io_size-1
enum { io_addr = 0xFF10 };
enum { io_size = 0x30 };
enum { regs_size = io_size + 0x10 };
	
enum gb_mode_t {
	mode_dmg,   // Game Boy monochrome
	mode_cgb,   // Game Boy Color
	mode_agb    // Game Boy Advance
};

// 0: Square 1, 1: Square 2, 2: Wave, 3: Noise
enum { osc_count = 4 }; // 0 <= chan < osc_count

struct Gb_Apu {
	struct Gb_Osc*     oscs [osc_count];
	blip_time_t last_time;          // time sound emulator has been run to
	blip_time_t frame_period;       // clocks between each frame sequencer step
	int      volume_;
	bool        reduce_clicks_;
	
	struct Gb_Square square1;
	struct Gb_Square square2;
	struct Gb_Wave   wave;
	struct Gb_Noise  noise;
	blip_time_t     frame_time;     // time of next frame sequencer action
	int             frame_phase;    // phase of next frame sequencer step
	
	uint8_t  regs [regs_size];// last values written to registers
	
	// large objects after everything else
	struct Blip_Synth synth;
};

// Basics

// Initializes apu
void Apu_init( struct Gb_Apu* this );
	
// Emulates to time t, then writes data to addr
void Apu_write_register( struct Gb_Apu* this, blip_time_t t, int addr, int data );
	
// Emulates to time t, then subtracts t from the current time.
// OK if previous write call had time slightly after t.
void Apu_end_frame( struct Gb_Apu* this,blip_time_t t );
	
// More features
	
// Emulates to time t, then reads from addr
int Apu_read_register( struct Gb_Apu* this, blip_time_t t, int addr );

// Resets hardware to state after power, BEFORE boot ROM runs. Mode selects
// sound hardware. If agb_wave is true, enables AGB's extra wave features.
void Apu_reset( struct Gb_Apu* this, enum gb_mode_t mode, bool agb_wave );
	
// Same as set_output(), but for a particular channel
void Apu_set_output( struct Gb_Apu* this, int chan, struct Blip_Buffer* center,
		struct Blip_Buffer* left, struct Blip_Buffer* right );
	
// Sets overall volume, where 1.0 is normal
void Apu_volume( struct Gb_Apu* this, int v );
	
// If true, reduces clicking by disabling DAC biasing. Note that this reduces
// emulation accuracy, since the clicks are authentic.
void Apu_reduce_clicks( struct Gb_Apu* this, bool reduce );
	
// Sets frame sequencer rate, where 1.0 is normal. Meant for adjusting the
// tempo in a music player.
void Apu_set_tempo( struct Gb_Apu* this, int t );


void write_osc( struct Gb_Apu* this, int reg, int old_data, int data );

#endif
