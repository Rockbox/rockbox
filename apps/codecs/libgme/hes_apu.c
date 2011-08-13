// Game_Music_Emu 0.5.2. http://www.slack.net/~ant/

#include "hes_apu.h"
#include <string.h>

/* Copyright (C) 2006 Shay Green. This module is free software; you
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

enum { center_waves = 1 }; // reduces asymmetry and clamping when starting notes

static void Apu_balance_changed( struct Hes_Apu* this, struct Hes_Osc* osc );
static void Apu_balance_changed( struct Hes_Apu* this, struct Hes_Osc* osc )
{
	static short const log_table [32] = { // ~1.5 db per step
		#define ENTRY( factor ) (short) (factor * amp_range / 31.0 + 0.5)
		ENTRY( 0.000000 ),ENTRY( 0.005524 ),ENTRY( 0.006570 ),ENTRY( 0.007813 ),
		ENTRY( 0.009291 ),ENTRY( 0.011049 ),ENTRY( 0.013139 ),ENTRY( 0.015625 ),
		ENTRY( 0.018581 ),ENTRY( 0.022097 ),ENTRY( 0.026278 ),ENTRY( 0.031250 ),
		ENTRY( 0.037163 ),ENTRY( 0.044194 ),ENTRY( 0.052556 ),ENTRY( 0.062500 ),
		ENTRY( 0.074325 ),ENTRY( 0.088388 ),ENTRY( 0.105112 ),ENTRY( 0.125000 ),
		ENTRY( 0.148651 ),ENTRY( 0.176777 ),ENTRY( 0.210224 ),ENTRY( 0.250000 ),
		ENTRY( 0.297302 ),ENTRY( 0.353553 ),ENTRY( 0.420448 ),ENTRY( 0.500000 ),
		ENTRY( 0.594604 ),ENTRY( 0.707107 ),ENTRY( 0.840896 ),ENTRY( 1.000000 ),
		#undef ENTRY
	};
	
	int vol = (osc->control & 0x1F) - 0x1E * 2;
	
	int left  = vol + (osc->balance >> 3 & 0x1E) + (this->balance >> 3 & 0x1E);
	if ( left  < 0 ) left  = 0;
	
	int right = vol + (osc->balance << 1 & 0x1E) + (this->balance << 1 & 0x1E);
	if ( right < 0 ) right = 0;
	
	left  = log_table [left ];
	right = log_table [right];
	
	// optimizing for the common case of being centered also allows easy
	// panning using Effects_Buffer
	osc->outputs [0] = osc->chans [0]; // center
	osc->outputs [1] = 0;
	if ( left != right )
	{
		osc->outputs [0] = osc->chans [1]; // left
		osc->outputs [1] = osc->chans [2]; // right
	}
	
	if ( center_waves )
	{
		osc->last_amp [0] += (left  - osc->volume [0]) * 16;
		osc->last_amp [1] += (right - osc->volume [1]) * 16;
	}
	
	osc->volume [0] = left;
	osc->volume [1] = right;
}

void Apu_init( struct Hes_Apu* this )
{
	struct Hes_Osc* osc = &this->oscs [osc_count];
	do
	{
		osc--;
		osc->outputs [0] = 0;
		osc->outputs [1] = 0;
		osc->chans [0] = 0;
		osc->chans [1] = 0;
		osc->chans [2] = 0;
	}
	while ( osc != this->oscs );
	
	Apu_reset( this );
}

void Apu_reset( struct Hes_Apu* this )
{
	this->latch   = 0;
	this->balance = 0xFF;
	
	struct Hes_Osc* osc = &this->oscs [osc_count];
	do
	{
		osc--;
		memset( osc, 0, offsetof (struct Hes_Osc,outputs) );
		osc->noise_lfsr = 1;
		osc->control    = 0x40;
		osc->balance    = 0xFF;
	}
	while ( osc != this->oscs );
}

void Apu_osc_output( struct Hes_Apu* this, int index, struct Blip_Buffer* center, struct Blip_Buffer* left, struct Blip_Buffer* right )
{
	require( (unsigned) index < osc_count );
	this->oscs [index].chans [0] = center;
	this->oscs [index].chans [1] = left;
	this->oscs [index].chans [2] = right;
	
	struct Hes_Osc* osc = &this->oscs [osc_count];
	do
	{
		osc--;
		Apu_balance_changed( this, osc );
	}
	while ( osc != this->oscs );
}

void Osc_run_until( struct Hes_Osc* this, struct Blip_Synth* synth_, blip_time_t end_time )
{
	struct Blip_Buffer* const osc_outputs_0 = this->outputs [0]; // cache often-used values
	if ( osc_outputs_0 && this->control & 0x80 )
	{
		int dac = this->dac;
		
		int const volume_0 = this->volume [0];
		{
			int delta = dac * volume_0 - this->last_amp [0];
			if ( delta )
				Synth_offset( synth_, this->last_time, delta, osc_outputs_0 );
			Blip_set_modified( osc_outputs_0 );
		}
		
		struct Blip_Buffer* const osc_outputs_1 = this->outputs [1];
		int const volume_1 = this->volume [1];
		if ( osc_outputs_1 )
		{
			int delta = dac * volume_1 - this->last_amp [1];
			if ( delta )
				Synth_offset( synth_, this->last_time, delta, osc_outputs_1 );
			Blip_set_modified( osc_outputs_1 );
		}
		
		blip_time_t time = this->last_time + this->delay;
		if ( time < end_time )
		{
			if ( this->noise & 0x80 )
			{
				if ( volume_0 | volume_1 )
				{
					// noise
					int const period = (32 - (this->noise & 0x1F)) * 64; // TODO: correct?
					unsigned noise_lfsr = this->noise_lfsr;
					do
					{
						int new_dac = 0x1F & -(noise_lfsr >> 1 & 1);
						// Implemented using "Galios configuration"
						// TODO: find correct LFSR algorithm
						noise_lfsr = (noise_lfsr >> 1) ^ (0xE008 & -(noise_lfsr & 1));
						//noise_lfsr = (noise_lfsr >> 1) ^ (0x6000 & -(noise_lfsr & 1));
						int delta = new_dac - dac;
						if ( delta )
						{
							dac = new_dac;
							Synth_offset( synth_, time, delta * volume_0, osc_outputs_0 );
							if ( osc_outputs_1 )
								Synth_offset( synth_, time, delta * volume_1, osc_outputs_1 );
						}
						time += period;
					}
					while ( time < end_time );
					
					this->noise_lfsr = noise_lfsr;
					assert( noise_lfsr );
				}
			}
			else if ( !(this->control & 0x40) )
			{
				// wave
				int phase = (this->phase + 1) & 0x1F; // pre-advance for optimal inner loop
				int period = this->period * 2;
				if ( period >= 14 && (volume_0 | volume_1) )
				{
					do
					{
						int new_dac = this->wave [phase];
						phase = (phase + 1) & 0x1F;
						int delta = new_dac - dac;
						if ( delta )
						{
							dac = new_dac;
							Synth_offset( synth_, time, delta * volume_0, osc_outputs_0 );
							if ( osc_outputs_1 )
								Synth_offset( synth_, time, delta * volume_1, osc_outputs_1 );
						}
						time += period;
					}
					while ( time < end_time );
				}
				else
				{
					if ( !period )
					{
						// TODO: Gekisha Boy assumes that period = 0 silences wave
						//period = 0x1000 * 2;
						period = 1;
						//if ( !(volume_0 | volume_1) )
						//  dprintf( "Used period 0\n" );
					}
					
					// maintain phase when silent
					blargg_long count = (end_time - time + period - 1) / period;
					phase += count; // phase will be masked below
					time += count * period;
				}
				this->phase = (phase - 1) & 0x1F; // undo pre-advance
			}
		}
		time -= end_time;
		if ( time < 0 )
			time = 0;
		this->delay = time;
		
		this->dac = dac;
		this->last_amp [0] = dac * volume_0;
		this->last_amp [1] = dac * volume_1;
	}
	this->last_time = end_time;
}

void Apu_write_data( struct Hes_Apu* this, blip_time_t time, int addr, int data )
{
	if ( addr == 0x800 )
	{
		this->latch = data & 7;
	}
	else if ( addr == 0x801 )
	{
		if ( this->balance != data )
		{
			this->balance = data;
			
			struct Hes_Osc* osc = &this->oscs [osc_count];
			do
			{
				osc--;
				Osc_run_until( osc, &this->synth, time );
				Apu_balance_changed( this, this->oscs );
			}
			while ( osc != this->oscs );
		}
	}
	else if ( this->latch < osc_count )
	{
		struct Hes_Osc* osc = &this->oscs [this->latch];
		Osc_run_until( osc, &this->synth, time );
		switch ( addr )
		{
		case 0x802:
			osc->period = (osc->period & 0xF00) | data;
			break;
		
		case 0x803:
			osc->period = (osc->period & 0x0FF) | ((data & 0x0F) << 8);
			break;
		
		case 0x804:
			if ( osc->control & 0x40 & ~data )
				osc->phase = 0;
			osc->control = data;
			Apu_balance_changed( this, osc );
			break;
		
		case 0x805:
			osc->balance = data;
			Apu_balance_changed( this, osc );
			break;
		
		case 0x806:
			data &= 0x1F;
			if ( !(osc->control & 0x40) )
			{
				osc->wave [osc->phase] = data;
				osc->phase = (osc->phase + 1) & 0x1F;
			}
			else if ( osc->control & 0x80 )
			{
				osc->dac = data;
			}
			break;
		
		 case 0x807:
		 	if ( osc >= &this->oscs [4] )
		 		osc->noise = data;
		 	break;
		 case 0x809:
		 	if ( !(data & 0x80) && (data & 0x03) != 0 ) {
		 		dprintf( "HES LFO not supported\n" );
			}
		}
	}
}

void Apu_end_frame( struct Hes_Apu* this, blip_time_t end_time )
{
	struct Hes_Osc* osc = &this->oscs [osc_count];
	do
	{
		osc--;
		if ( end_time > osc->last_time )
			Osc_run_until( osc, &this->synth, end_time );
		assert( osc->last_time >= end_time );
		osc->last_time -= end_time;
	}
	while ( osc != this->oscs );
}
