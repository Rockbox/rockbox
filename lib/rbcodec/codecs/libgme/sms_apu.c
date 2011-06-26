// Sms_Snd_Emu 0.1.1. http://www.slack.net/~ant/

#include "sms_apu.h"

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

int const noise_osc = 3;

void Sms_apu_volume( struct Sms_Apu* this, int vol )
{
	vol = (vol - (vol*3)/20) / sms_osc_count / 64;
	Synth_volume( &this->synth, vol );
}

static inline int calc_output( struct Sms_Apu* this, int i )
{
	int flags = this->ggstereo >> i;
	return (flags >> 3 & 2) | (flags & 1);
}

void Sms_apu_set_output( struct Sms_Apu* this, int i, struct Blip_Buffer* center, struct Blip_Buffer* left, struct Blip_Buffer* right )
{
#if defined(ROCKBOX)
	(void) left;
	(void) right;
#endif
	
	// Must be silent (all NULL), mono (left and right NULL), or stereo (none NULL)
	require( !center || (center && !left && !right) || (center && left && right) );
	require( (unsigned) i < sms_osc_count ); // fails if you pass invalid osc index
	
	if ( center )
	{
		unsigned const divisor = 16384 * 16 * 2;
		this->min_tone_period = ((unsigned) Blip_clock_rate( center ) + divisor/2) / divisor;
	}
	
	if ( !center || !left || !right )
	{
		left  = center;
		right = center;
	}
	
	struct Osc* o = &this->oscs [i];
	o->outputs [0] = NULL;
	o->outputs [1] = right;
	o->outputs [2] = left;
	o->outputs [3] = center;
	o->output = o->outputs [calc_output( this, i )];
}

static inline unsigned fibonacci_to_galois_lfsr( unsigned fibonacci, int width )
{
	unsigned galois = 0;
	while ( --width >= 0 )
	{
		galois = (galois << 1) | (fibonacci & 1);
		fibonacci >>= 1;
	}
	return galois;
}

void Sms_apu_reset( struct Sms_Apu* this, unsigned feedback, int noise_width )
{
	this->last_time = 0;
	this->latch     = 0;
	this->ggstereo  = 0;
	
	// Calculate noise feedback values
	if ( !feedback || !noise_width )
	{
		feedback    = 0x0009;
		noise_width = 16;
	}
	this->looped_feedback = 1 << (noise_width - 1);
	this->noise_feedback  = fibonacci_to_galois_lfsr( feedback, noise_width );
	
	// Reset oscs
	int i;
	for ( i = sms_osc_count; --i >= 0; )
	{
		struct Osc* o = &this->oscs [i];
		o->output   =  NULL;
		o->last_amp =  0;
		o->delay    =  0;
		o->phase    =  0;
		o->period   =  0;
		o->volume   = 15; // silent
	}
	
	this->oscs [noise_osc].phase = 0x8000;
	Sms_apu_write_ggstereo( this, 0, 0xFF );
}

void Sms_apu_init( struct Sms_Apu* this )
{
	this->min_tone_period = 7;
	
	Synth_init( &this->synth );
	
	// Clear outputs to NULL FIRST
	this->ggstereo = 0;
	
	int i;
	for ( i = sms_osc_count; --i >= 0; )
		Sms_apu_set_output( this, i, NULL, NULL, NULL );
	
	Sms_apu_volume( this, (int)FP_ONE_VOLUME );
	Sms_apu_reset( this, 0, 0 );
}

static void run_until( struct Sms_Apu* this, blip_time_t end_time )
{
	require( end_time >= this->last_time );
	if ( end_time <= this->last_time )
		return;
	
	// Synthesize each oscillator
	int idx;
	for ( idx = sms_osc_count; --idx >= 0; )
	{
		struct Osc* osc = &this->oscs [idx];
		int vol  = 0;
		int amp  = 0;
		
		// Determine what will be generated
		struct Blip_Buffer* const out = osc->output;
		if ( out )
		{
			// volumes [i] ~= 64 * pow( 1.26, 15 - i ) / pow( 1.26, 15 )
			static unsigned char const volumes [16] = {
				64, 50, 40, 32, 25, 20, 16, 13, 10, 8, 6, 5, 4, 3, 2, 0
			};
			
			vol = volumes [osc->volume];
			amp = (osc->phase & 1) * vol;
			
			// Square freq above 16 kHz yields constant amplitude at half volume
			if ( idx != noise_osc && osc->period < this->min_tone_period )
			{
				amp = vol >> 1;
				vol = 0;
			}
			
			// Update amplitude
			int delta = amp - osc->last_amp;
			if ( delta )
			{
				osc->last_amp = amp;
				Synth_offset( &this->synth, this->last_time, delta, out );
				Blip_set_modified( out );
			}
		}
		
		// Generate wave
		blip_time_t time = this->last_time + osc->delay;
		if ( time < end_time )
		{
			// Calculate actual period
			int period = osc->period;
			if ( idx == noise_osc )
			{
				period = 0x20 << (period & 3);
				if ( period == 0x100 )
					period = this->oscs [2].period * 2;
			}
			period *= 0x10;
			if ( !period )
				period = 0x10;
			
			// Maintain phase when silent
			int phase = osc->phase;
			if ( !vol )
			{
				int count = (end_time - time + period - 1) / period;
				time += count * period;
				if ( idx != noise_osc ) // TODO: maintain noise LFSR phase?
					phase ^= count & 1;
			}
			else
			{
				int delta = amp * 2 - vol;
				
				if ( idx != noise_osc )
				{
					// Square
					do
					{
						delta = -delta;
						Synth_offset( &this->synth, time, delta, out );
						time += period;
					}
					while ( time < end_time );
					phase = (delta >= 0);
				}
				else
				{
					// Noise
					unsigned const feedback = (osc->period & 4 ? this->noise_feedback : this->looped_feedback);
					do
					{
						unsigned changed = phase + 1;
						phase = ((phase & 1) * feedback) ^ (phase >> 1);
						if ( changed & 2 ) // true if bits 0 and 1 differ
						{
							delta = -delta;
							Synth_offset_inline( &this->synth, time, delta, out );
						}
						time += period;
					}
					while ( time < end_time );
					check( phase );
				}
				osc->last_amp = (phase & 1) * vol;
				Blip_set_modified( out );
			}
			osc->phase = phase;
		}
		osc->delay = time - end_time;
	}
	this->last_time = end_time;
}

void Sms_apu_write_ggstereo( struct Sms_Apu* this, blip_time_t time, int data )
{
	require( (unsigned) data <= 0xFF );
	
	run_until( this, time );
	this->ggstereo = data;
	
	int i;
	for ( i = sms_osc_count; --i >= 0; )
	{
		struct Osc* osc = &this->oscs [i];
		
		struct Blip_Buffer* old = osc->output;
		osc->output = osc->outputs [calc_output( this, i )];
		if ( osc->output != old )
		{
			int delta = -osc->last_amp;
			if ( delta )
			{
				osc->last_amp = 0;
				if ( old )
				{
					Blip_set_modified( old );
					Synth_offset( &this->synth, this->last_time, delta, old );
				}
			}
		}
	}
}

void Sms_apu_write_data( struct Sms_Apu* this, blip_time_t time, int data )
{
	require( (unsigned) data <= 0xFF );
	
	run_until( this, time );
	
	if ( data & 0x80 )
		this->latch = data;
	
	// We want the raw values written so our save state format can be
	// as close to hardware as possible and unspecific to any emulator.
	int idx = this->latch >> 5 & 3;
	struct Osc* osc = &this->oscs [idx];
	if ( this->latch & 0x10 )
	{
		osc->volume = data & 0x0F;
	}
	else
	{
		if ( idx == noise_osc )
			osc->phase = 0x8000; // reset noise LFSR
		
		// Replace high 6 bits/low 4 bits of register with data
		int lo = osc->period;
		int hi = data << 4;
		if ( idx == noise_osc || (data & 0x80) )
		{
			hi = lo;
			lo = data;
		}
		osc->period = (hi & 0x3F0) | (lo & 0x00F);
	}
}

void Sms_apu_end_frame( struct Sms_Apu* this, blip_time_t end_time )
{
	if ( end_time > this->last_time )
		run_until( this, end_time );
	
	this->last_time -= end_time;
	assert( this->last_time >= 0 );
}
