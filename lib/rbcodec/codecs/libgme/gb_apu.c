// Gb_Snd_Emu 0.1.4. http://www.slack.net/~ant/

#include "gb_apu.h"

//#include "gb_apu_logger.h"

/* Copyright (C) 2003-2008 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include "blargg_source.h"

int const vol_reg    = 0xFF24;
int const stereo_reg = 0xFF25;
int const status_reg = 0xFF26;
int const wave_ram   = 0xFF30;

int const power_mask = 0x80;

static inline int calc_output( struct Gb_Apu* this, int osc )
{
	int bits = this->regs [stereo_reg - io_addr] >> osc;
	return (bits >> 3 & 2) | (bits & 1);
}

void Apu_set_output( struct Gb_Apu* this, int i, struct Blip_Buffer* center, struct Blip_Buffer* left, struct Blip_Buffer* right )
{
	// Must be silent (all NULL), mono (left and right NULL), or stereo (none NULL)
	require( !center || (center && !left && !right) || (center && left && right) );
	require( (unsigned) i < osc_count ); // fails if you pass invalid osc index
	
	if ( !center || !left || !right )
	{
		left  = center;
		right = center;
	}
	
	struct Gb_Osc* o = this->oscs [i];
	o->outputs [1] = right;
	o->outputs [2] = left;
	o->outputs [3] = center;
	o->output = o->outputs [calc_output( this, i )];
}

static void synth_volume( struct Gb_Apu* this, int iv )
{
	int v = (this->volume_ * 6) / 10 / osc_count / 15 /*steps*/ / 8 /*master vol range*/ * iv;
	Synth_volume( &this->synth, v );
}

static void apply_volume( struct Gb_Apu* this )
{
	// TODO: Doesn't handle differing left and right volumes (panning).
	// Not worth the complexity.
	int data  = this->regs [vol_reg - io_addr];
	int left  = data >> 4 & 7;
	int right = data & 7;
	//if ( data & 0x88 ) dprintf( "Vin: %02X\n", data & 0x88 );
	//if ( left != right ) dprintf( "l: %d r: %d\n", left, right );
	synth_volume( this, max( left, right ) + 1 );
}

void Apu_volume( struct Gb_Apu* this, int v )
{
	if ( this->volume_ != v )
	{
		this->volume_ = v;
		apply_volume( this );
	}
}

static void reset_regs( struct Gb_Apu* this )
{
	int i;
	for ( i = 0; i < 0x20; i++ )
		this->regs [i] = 0;
	
	Sweep_reset ( &this->square1 );
	Square_reset( &this->square2 );
	Wave_reset  ( &this->wave );
	Noise_reset ( &this->noise );
	
	apply_volume( this );
}

static void reset_lengths( struct Gb_Apu* this )
{
	this->square1.osc.length_ctr = 64;
	this->square2.osc.length_ctr = 64;
	this->wave   .osc.length_ctr = 256;
	this->noise  .osc.length_ctr = 64;
}

void Apu_reduce_clicks( struct Gb_Apu* this, bool reduce )
{
	this->reduce_clicks_ = reduce;
	
	// Click reduction makes DAC off generate same output as volume 0
	int dac_off_amp = 0;
	if ( reduce && this->wave.osc.mode != mode_agb ) // AGB already eliminates clicks
		dac_off_amp = -dac_bias;

	int i;
	for ( i = 0; i < osc_count; i++ )
		this->oscs [i]->dac_off_amp = dac_off_amp;
	
	// AGB always eliminates clicks on wave channel using same method
	if ( this->wave.osc.mode == mode_agb )
		this->wave.osc.dac_off_amp = -dac_bias;
}

void Apu_reset( struct Gb_Apu* this, enum gb_mode_t mode, bool agb_wave )
{
	// Hardware mode
	if ( agb_wave )
		mode = mode_agb; // using AGB wave features implies AGB hardware
	this->wave.agb_mask = agb_wave ? 0xFF : 0;
	int i;
	for ( i = 0; i < osc_count; i++ )
		this->oscs [i]->mode = mode;
	Apu_reduce_clicks( this, this->reduce_clicks_ );
	
	// Reset state
	this->frame_time  = 0;
	this->last_time   = 0;
	this->frame_phase = 0;
	
	reset_regs( this );
	reset_lengths( this );
	
	// Load initial wave RAM
	static byte const initial_wave [2] [16] = {
		{0x84,0x40,0x43,0xAA,0x2D,0x78,0x92,0x3C,0x60,0x59,0x59,0xB0,0x34,0xB8,0x2E,0xDA},
		{0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF},
	};
	int b;
	for ( b = 2; --b >= 0; )
	{
		// Init both banks (does nothing if not in AGB mode)
		// TODO: verify that this works
		Apu_write_register( this, 0, 0xFF1A, b * 0x40 );
		unsigned i;
		for ( i = 0; i < sizeof initial_wave [0]; i++ )
			Apu_write_register( this, 0, i + wave_ram, initial_wave [(mode != mode_dmg)] [i] );
	}
}

void Apu_set_tempo( struct Gb_Apu* this, int t )
{
	this->frame_period = 4194304 / 512; // 512 Hz
	if ( t != (int)FP_ONE_TEMPO )
		this->frame_period = t ? (blip_time_t) ((this->frame_period * FP_ONE_TEMPO) / t) : (blip_time_t) (0);
}

void Apu_init( struct Gb_Apu* this )
{
	this->wave.wave_ram = &this->regs [wave_ram - io_addr];
	
    Synth_init( &this->synth );
	
	this->oscs [0] = &this->square1.osc;
	this->oscs [1] = &this->square2.osc;
	this->oscs [2] = &this->wave.osc;
	this->oscs [3] = &this->noise.osc;
	
	int i;
	for ( i = osc_count; --i >= 0; )
	{
		struct Gb_Osc* o = this->oscs [i];
		o->regs        = &this->regs [i * 5];
		o->output      = NULL;
		o->outputs [0] = NULL;
		o->outputs [1] = NULL;
		o->outputs [2] = NULL;
		o->outputs [3] = NULL;
		o->synth       = &this->synth;       
	}
	
	this->reduce_clicks_ = false;
	Apu_set_tempo( this, (int)FP_ONE_TEMPO );
	this->volume_ = (int)FP_ONE_VOLUME;
	Apu_reset( this, mode_cgb, false );
}

static void run_until_( struct Gb_Apu* this, blip_time_t end_time )
{
	if ( !this->frame_period )
		this->frame_time += end_time - this->last_time;
	
	while ( true )
	{
		// run oscillators
		blip_time_t time = end_time;
		if ( time > this->frame_time )
			time = this->frame_time;
		
		Square_run( &this->square1, this->last_time, time );
		Square_run( &this->square2, this->last_time, time );
		Wave_run  ( &this->wave, this->last_time, time );
		Noise_run ( &this->noise, this->last_time, time );
		this->last_time = time;
		
		if ( time == end_time )
			break;
		
		// run frame sequencer
		assert( this->frame_period );
		this->frame_time += this->frame_period * clk_mul;
		switch ( this->frame_phase++ )
		{
		case 2:
		case 6:
			// 128 Hz
			clock_sweep( &this->square1 );
		case 0:
		case 4:
			// 256 Hz
			Osc_clock_length( &this->square1.osc );
			Osc_clock_length( &this->square2.osc);
			Osc_clock_length( &this->wave.osc);
			Osc_clock_length( &this->noise.osc);
			break;
		
		case 7:
			// 64 Hz
			this->frame_phase = 0;
			Square_clock_envelope( &this->square1 );
			Square_clock_envelope( &this->square2 );
			Noise_clock_envelope( &this->noise );
		}
	}
}

static inline void run_until( struct Gb_Apu* this, blip_time_t time )
{
	require( time >= this->last_time ); // end_time must not be before previous time
	if ( time > this->last_time )
		run_until_( this, time );
}

void Apu_end_frame( struct Gb_Apu* this, blip_time_t end_time )
{
	#ifdef LOG_FRAME
		LOG_FRAME( end_time );
	#endif
	
	if ( end_time > this->last_time )
		run_until( this, end_time );
	
	this->frame_time -= end_time;
	assert( this->frame_time >= 0 );
	
	this->last_time -= end_time;
	assert( this->last_time >= 0 );
}

static void silence_osc( struct Gb_Apu* this, struct Gb_Osc* o )
{
	int delta = -o->last_amp;
	if ( this->reduce_clicks_ )
		delta += o->dac_off_amp;
	
	if ( delta )
	{
		o->last_amp = o->dac_off_amp;
		if ( o->output )
		{
			Blip_set_modified( o->output );
			Synth_offset( &this->synth, this->last_time, delta, o->output );
		}
	}
}

static void apply_stereo( struct Gb_Apu* this )
{
	int i;
	for ( i = osc_count; --i >= 0; )
	{
		struct Gb_Osc* o = this->oscs [i];
		struct Blip_Buffer* out = o->outputs [calc_output( this, i )];
		if ( o->output != out )
		{
			silence_osc( this, o );
			o->output = out;
		}
	}
}

void Apu_write_register( struct Gb_Apu* this, blip_time_t time, int addr, int data )
{
	require( (unsigned) data < 0x100 );
	
	int reg = addr - io_addr;
	if ( (unsigned) reg >= io_size )
	{
		require( false );
		return;
	}
	
	#ifdef LOG_WRITE
		LOG_WRITE( time, addr, data );
	#endif
	
	if ( addr < status_reg && !(this->regs [status_reg - io_addr] & power_mask) )
	{
		// Power is off
		
		// length counters can only be written in DMG mode
		if ( this->wave.osc.mode != mode_dmg || (reg != 1 && reg != 5+1 && reg != 10+1 && reg != 15+1) )
			return;
		
		if ( reg < 10 )
			data &= 0x3F; // clear square duty
	}
	
	run_until( this, time );
	
	if ( addr >= wave_ram )
	{
		Wave_write( &this->wave, addr, data );
	}
	else
	{
		int old_data = this->regs [reg];
		this->regs [reg] = data;
		
		if ( addr < vol_reg )
		{
			// Oscillator
			write_osc( this, reg, old_data, data );
		}
		else if ( addr == vol_reg && data != old_data )
		{
			// Master volume
			int i;
			for ( i = osc_count; --i >= 0; )
				silence_osc( this, this->oscs [i] );
			
			apply_volume( this );
		}
		else if ( addr == stereo_reg )
		{
			// Stereo panning
			apply_stereo( this );
		}
		else if ( addr == status_reg && (data ^ old_data) & power_mask )
		{
			// Power control
			this->frame_phase = 0;
			int i;
			for ( i = osc_count; --i >= 0; )
				silence_osc( this, this->oscs [i] );
		
			reset_regs( this );
			if ( this->wave.osc.mode != mode_dmg )
				reset_lengths( this );
			
			this->regs [status_reg - io_addr] = data;
		}
	}
}

int Apu_read_register( struct Gb_Apu* this, blip_time_t time, int addr )
{
	if ( addr >= status_reg )
		run_until( this, time );
	
	int reg = addr - io_addr;
	if ( (unsigned) reg >= io_size )
	{
		require( false );
		return 0;
	}
	
	if ( addr >= wave_ram )
		return Wave_read( &this->wave, addr );
	
	// Value read back has some bits always set
	static byte const masks [] = {
		0x80,0x3F,0x00,0xFF,0xBF,
		0xFF,0x3F,0x00,0xFF,0xBF,
		0x7F,0xFF,0x9F,0xFF,0xBF,
		0xFF,0xFF,0x00,0x00,0xBF,
		0x00,0x00,0x70,
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
	};
	int mask = masks [reg];
	if ( this->wave.agb_mask && (reg == 10 || reg == 12) )
		mask = 0x1F; // extra implemented bits in wave regs on AGB
	int data = this->regs [reg] | mask;
	
	// Status register
	if ( addr == status_reg )
	{
		data &= 0xF0;
		data |= (int) this->square1.osc.enabled << 0;
		data |= (int) this->square2.osc.enabled << 1;
		data |= (int) this->wave   .osc.enabled << 2;
		data |= (int) this->noise  .osc.enabled << 3;
	}
	
	return data;
}
