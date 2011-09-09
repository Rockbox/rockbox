// Nes_Snd_Emu 0.1.8. http://www.slack.net/~ant/

#include "nes_apu.h"

/* Copyright (C) 2003-2006 Shay Green. This module is free software; you
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

int const amp_range = 15;

void Apu_init( struct Nes_Apu* this )
{
	this->tempo_ = (int)(FP_ONE_TEMPO);
	this->dmc.apu = this;
	this->dmc.prg_reader = NULL;
	this->irq_notifier_ = NULL;
	
	Synth_init( &this->square_synth );
	Synth_init( &this->triangle.synth );
	Synth_init( &this->noise.synth );
	Synth_init( &this->dmc.synth );
	
	Square_set_synth( &this->square1, &this->square_synth );
	Square_set_synth( &this->square2, &this->square_synth );
	
	this->oscs [0] = &this->square1.osc;
	this->oscs [1] = &this->square2.osc;
	this->oscs [2] = &this->triangle.osc;
	this->oscs [3] = &this->noise.osc;
	this->oscs [4] = &this->dmc.osc;
	
	Apu_output( this, NULL );
	this->dmc.nonlinear = false;
	Apu_volume( this, (int)FP_ONE_VOLUME );
	Apu_reset( this, false, 0 );
}

#if 0
// sq and tnd must use a fixed point frac where 1.0 = FP_ONE_VOLUME
void Apu_enable_nonlinear_( struct Nes_Apu* this, int sq, int tnd )
{
	this->dmc.nonlinear = true;
	Synth_volume( &this->square_synth, sq );
	
	Synth_volume( &this->triangle.synth, (int)((long long)(FP_ONE_VOLUME * 2.752) * tnd / FP_ONE_VOLUME) );
	Synth_volume( &this->noise.synth   , (int)((long long)(FP_ONE_VOLUME * 1.849) * tnd / FP_ONE_VOLUME) );
	Synth_volume( &this->dmc.synth     , tnd );
	
	this->square1 .osc.last_amp = 0;
	this->square2 .osc.last_amp = 0;
	this->triangle.osc.last_amp = 0;
	this->noise   .osc.last_amp = 0;
	this->dmc     .osc.last_amp = 0;
}
#endif

void Apu_volume( struct Nes_Apu* this, int v )
{
	if ( !this->dmc.nonlinear )
	{
		Synth_volume( &this->square_synth,  (int)((long long)((0.125 / 1.11) * FP_ONE_VOLUME) * v / amp_range / FP_ONE_VOLUME) ); // was 0.1128   1.108
		Synth_volume( &this->triangle.synth,(int)((long long)((0.150 / 1.11) * FP_ONE_VOLUME) * v / amp_range / FP_ONE_VOLUME) ); // was 0.12765  1.175
		Synth_volume( &this->noise.synth,   (int)((long long)((0.095 / 1.11) * FP_ONE_VOLUME) * v / amp_range / FP_ONE_VOLUME) ); // was 0.0741   1.282
		Synth_volume( &this->dmc.synth,     (int)((long long)((0.450 / 1.11) * FP_ONE_VOLUME) * v / 2048      / FP_ONE_VOLUME) ); // was 0.42545  1.058
	}
}

void Apu_output( struct Nes_Apu* this, struct Blip_Buffer* buffer )
{
	int i;
	for ( i = 0; i < apu_osc_count; i++ )
		Apu_osc_output( this, i, buffer );
}

void Apu_set_tempo( struct Nes_Apu* this, int t )
{
	this->tempo_ = t;
	this->frame_period = (this->dmc.pal_mode ? 8314 : 7458);
	if ( t != (int)FP_ONE_TEMPO )
		this->frame_period = (int) ((this->frame_period * FP_ONE_TEMPO) / t) & ~1; // must be even
}

void Apu_reset( struct Nes_Apu* this, bool pal_mode, int initial_dmc_dac )
{
	this->dmc.pal_mode = pal_mode;
	Apu_set_tempo( this, this->tempo_ );
	
	Square_reset( &this->square1 );
	Square_reset( &this->square2 );
	Triangle_reset( &this->triangle );
	Noise_reset( &this->noise );
	Dmc_reset( &this->dmc );
	
	this->last_time = 0;
	this->last_dmc_time = 0;
	this->osc_enables = 0;
	this->irq_flag = false;
	this->earliest_irq_ = apu_no_irq;
	this->frame_delay = 1;
	Apu_write_register( this, 0, 0x4017, 0x00 );
	Apu_write_register( this, 0, 0x4015, 0x00 );
	
	addr_t addr;
	for ( addr = apu_io_addr; addr <= 0x4013; addr++ )
		Apu_write_register( this, 0, addr, (addr & 3) ? 0x00 : 0x10 );
	
	this->dmc.dac = initial_dmc_dac;
	if ( !this->dmc.nonlinear )
		this->triangle.osc.last_amp = 15;
	if ( !this->dmc.nonlinear ) // TODO: remove?
		this->dmc.osc.last_amp = initial_dmc_dac; // prevent output transition
}

void Apu_irq_changed( struct Nes_Apu* this )
{
	nes_time_t new_irq = this->dmc.next_irq;
	if ( this->dmc.irq_flag | this->irq_flag ) {
		new_irq = 0;
	}
	else if ( new_irq > this->next_irq ) {
		new_irq = this->next_irq;
	}
	
	if ( new_irq != this->earliest_irq_ ) {
		this->earliest_irq_ = new_irq;
		if ( this->irq_notifier_ )
			this->irq_notifier_( this->irq_data );
	}
}

// frames

void Apu_run_until( struct Nes_Apu* this, nes_time_t end_time )
{
	require( end_time >= this->last_dmc_time );
	if ( end_time > Apu_next_dmc_read_time( this ) )
	{
		nes_time_t start = this->last_dmc_time;
		this->last_dmc_time = end_time;
		Dmc_run( &this->dmc, start, end_time );
	}
}

static void run_until_( struct Nes_Apu* this, nes_time_t end_time )
{
	require( end_time >= this->last_time );
	
	if ( end_time == this->last_time )
		return;
	
	if ( this->last_dmc_time < end_time )
	{
		nes_time_t start = this->last_dmc_time;
		this->last_dmc_time = end_time;
		Dmc_run( &this->dmc, start, end_time );
	}
	
	while ( true )
	{
		// earlier of next frame time or end time
		nes_time_t time = this->last_time + this->frame_delay;
		if ( time > end_time )
			time = end_time;
		this->frame_delay -= time - this->last_time;
		
		// run oscs to present
		Square_run( &this->square1, this->last_time, time );
		Square_run( &this->square2, this->last_time, time );
		Triangle_run( &this->triangle, this->last_time, time );
		Noise_run( &this->noise, this->last_time, time );
		this->last_time = time;
		
		if ( time == end_time )
			break; // no more frames to run
		
		// take frame-specific actions
		this->frame_delay = this->frame_period;
		switch ( this->frame++ )
		{
			case 0:
				if ( !(this->frame_mode & 0xC0) ) {
		 			this->next_irq = time + this->frame_period * 4 + 2;
		 			this->irq_flag = true;
		 		}
		 		// fall through
		 	case 2:
		 		// clock length and sweep on frames 0 and 2
				Osc_clock_length( &this->square1.osc, 0x20 );
				Osc_clock_length( &this->square2.osc, 0x20 );
				Osc_clock_length( &this->noise.osc, 0x20 );
				Osc_clock_length( &this->triangle.osc, 0x80 ); // different bit for halt flag on triangle
				
				Square_clock_sweep( &this->square1, -1 );
				Square_clock_sweep( &this->square2, 0 );
				
				// frame 2 is slightly shorter in mode 1
				if ( this->dmc.pal_mode && this->frame == 3 )
					this->frame_delay -= 2;
		 		break;
		 	
			case 1:
				// frame 1 is slightly shorter in mode 0
				if ( !this->dmc.pal_mode )
					this->frame_delay -= 2;
				break;
			
		 	case 3:
		 		this->frame = 0;
		 		
		 		// frame 3 is almost twice as long in mode 1
		 		if ( this->frame_mode & 0x80 )
					this->frame_delay += this->frame_period - (this->dmc.pal_mode ? 2 : 6);
				break;
		}
		
		// clock envelopes and linear counter every frame
		Triangle_clock_linear_counter( &this->triangle );
		Square_clock_envelope( &this->square1 );
		Square_clock_envelope( &this->square2 );
		Noise_clock_envelope( &this->noise );
	}
}

static inline void zero_apu_osc( struct Nes_Osc* osc, struct Blip_Synth* synth, nes_time_t time )
{
	struct Blip_Buffer* output = osc->output;
	int last_amp = osc->last_amp;
	osc->last_amp = 0;
	if ( output && last_amp )
		Synth_offset( synth, time, -osc->last_amp, output );
}

void Apu_end_frame( struct Nes_Apu* this, nes_time_t end_time )
{
	if ( end_time > this->last_time )
		run_until_( this, end_time );
	
	if ( this->dmc.nonlinear )
	{
		zero_apu_osc( &this->square1.osc, this->square1.synth, this->last_time );
		zero_apu_osc( &this->square2.osc, this->square2.synth, this->last_time );
		zero_apu_osc( &this->triangle.osc, &this->triangle.synth, this->last_time );
		zero_apu_osc( &this->noise.osc, &this->noise.synth, this->last_time );
		zero_apu_osc( &this->dmc.osc, &this->dmc.synth, this->last_time );
	}
	
	// make times relative to new frame
	this->last_time -= end_time;
	require( this->last_time >= 0 );
	
	this->last_dmc_time -= end_time;
	require( this->last_dmc_time >= 0 );
	
	if ( this->next_irq != apu_no_irq ) {
		this->next_irq -= end_time;
		check( this->next_irq >= 0 );
	}
	if ( this->dmc.next_irq != apu_no_irq ) {
		this->dmc.next_irq -= end_time;
		check( this->dmc.next_irq >= 0 );
	}
	if ( this->earliest_irq_ != apu_no_irq ) {
		this->earliest_irq_ -= end_time;
		if ( this->earliest_irq_ < 0 )
			this->earliest_irq_ = 0;
	}
}

// registers

static const unsigned char length_table [0x20] = {
	0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06,
	0xA0, 0x08, 0x3C, 0x0A, 0x0E, 0x0C, 0x1A, 0x0E, 
	0x0C, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
	0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E
};

void Apu_write_register( struct Nes_Apu* this, nes_time_t time, addr_t addr, int data )
{
	require( addr > 0x20 ); // addr must be actual address (i.e. 0x40xx)
	require( (unsigned) data <= 0xFF );
	
	// Ignore addresses outside range
	if ( (unsigned) (addr - apu_io_addr) >= apu_io_size )
		return;
	
	run_until_( this, time );
	
	if ( addr < 0x4014 )
	{
		// Write to channel
		int osc_index = (addr - apu_io_addr) >> 2;
		struct Nes_Osc* osc = this->oscs [osc_index];
		
		int reg = addr & 3;
		osc->regs [reg] = data;
		osc->reg_written [reg] = true;
		
		if ( osc_index == 4 )
		{
			// handle DMC specially
			Dmc_write_register( &this->dmc, reg, data );
		}
		else if ( reg == 3 )
		{
			// load length counter
			if ( (this->osc_enables >> osc_index) & 1 )
				osc->length_counter = length_table [(data >> 3) & 0x1F];
			
			// reset square phase
			if ( osc_index == 0 ) this->square1.phase = square_phase_range - 1;
			else if ( osc_index == 1 )	this->square2.phase = square_phase_range - 1;
		}
	}
	else if ( addr == 0x4015 )
	{
		// Channel enables
		int i;
		for ( i = apu_osc_count; i--; )
			if ( !((data >> i) & 1) )
				this->oscs [i]->length_counter = 0;
		
		bool recalc_irq = this->dmc.irq_flag;
		this->dmc.irq_flag = false;
		
		int old_enables = this->osc_enables;
		this->osc_enables = data;
		if ( !(data & 0x10) ) {
			this->dmc.next_irq = apu_no_irq;
			recalc_irq = true;
		}
		else if ( !(old_enables & 0x10) ) {
			Dmc_start( &this->dmc ); // dmc just enabled
		}
		
		if ( recalc_irq )
			Apu_irq_changed( this );
	}
	else if ( addr == 0x4017 )
	{
		// Frame mode
		this->frame_mode = data;
		
		bool irq_enabled = !(data & 0x40);
		this->irq_flag &= irq_enabled;
		this->next_irq = apu_no_irq;
		
		// mode 1
		this->frame_delay = (this->frame_delay & 1);
		this->frame = 0;
		
		if ( !(data & 0x80) )
		{
			// mode 0
			this->frame = 1;
			this->frame_delay += this->frame_period;
			if ( irq_enabled )
				this->next_irq = time + this->frame_delay + this->frame_period * 3 + 1;
		}
		
		Apu_irq_changed( this );
	}
}

int Apu_read_status( struct Nes_Apu* this, nes_time_t time )
{
	run_until_( this, time - 1 );
	
	int result = (this->dmc.irq_flag << 7) | (this->irq_flag << 6);
	
	int i;
	for ( i = 0; i < apu_osc_count; i++ )
		if ( this->oscs [i]->length_counter )
			result |= 1 << i;
	
	run_until_( this, time );
	
	if ( this->irq_flag )
	{
		result |= 0x40;
		this->irq_flag = false;
		Apu_irq_changed( this );
	}
	
	//debug_printf( "%6d/%d Read $4015->$%02X\n", frame_delay, frame, result );
	
	return result;
}
