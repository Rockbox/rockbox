// Nes_Snd_Emu 0.1.8. http://www.slack.net/~ant/

#include "nes_namco_apu.h"

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

void Namco_init( struct Nes_Namco_Apu* this )
{
	Synth_init( &this->synth );
	
	Namco_output( this, NULL );
	Namco_volume( this, (int)FP_ONE_VOLUME );
	Namco_reset( this );
}

void Namco_reset( struct Nes_Namco_Apu* this )
{
	this->last_time = 0;
	this->addr_reg = 0;
	
	int i;
	for ( i = 0; i < namco_reg_count; i++ )
		this->reg [i] = 0;
	
	for ( i = 0; i < namco_osc_count; i++ )
	{
		struct Namco_Osc* osc = &this->oscs [i];
		osc->delay = 0;
		osc->last_amp = 0;
		osc->wave_pos = 0;
	}
}

void Namco_output( struct Nes_Namco_Apu* this, struct Blip_Buffer* buf )
{
	int i;
	for ( i = 0; i < namco_osc_count; i++ )
		Namco_osc_output( this, i, buf );
}

void Namco_end_frame( struct Nes_Namco_Apu* this, blip_time_t time )
{
	if ( time > this->last_time )
		Namco_run_until( this, time );
	
	assert( this->last_time >= time );
	this->last_time -= time;
}

void Namco_run_until( struct Nes_Namco_Apu* this, blip_time_t nes_end_time )
{
	int active_oscs = (this->reg [0x7F] >> 4 & 7) + 1;
	int i;
	for ( i = namco_osc_count - active_oscs; i < namco_osc_count; i++ )
	{
		struct Namco_Osc* osc = &this->oscs [i];
		struct Blip_Buffer* output = osc->output;
		if ( !output )
			continue;
		
		blip_resampled_time_t time =
				Blip_resampled_time( output, this->last_time ) + osc->delay;
		blip_resampled_time_t end_time = Blip_resampled_time( output, nes_end_time );
		osc->delay = 0;
		if ( time < end_time )
		{
			const uint8_t* osc_reg = &this->reg [i * 8 + 0x40];
			if ( !(osc_reg [4] & 0xE0) )
				continue;
			
			int volume = osc_reg [7] & 15;
			if ( !volume )
				continue;
			
			int freq = (osc_reg [4] & 3) * 0x10000 + osc_reg [2] * 0x100L + osc_reg [0];
			if ( freq < 64 * active_oscs )
				continue; // prevent low frequencies from excessively delaying freq changes
			
			int const master_clock_divider = 12; // NES time derived via divider of master clock
			int const n106_divider = 45; // N106 then divides master clock by this
			int const max_freq = 0x3FFFF;
			int const lowest_freq_period = (max_freq + 1) * n106_divider / master_clock_divider;
			// divide by 8 to avoid overflow
			blip_resampled_time_t period =
					Blip_resampled_duration( output, lowest_freq_period / 8 ) / freq * 8 * active_oscs;
			
			int wave_size = 32 - (osc_reg [4] >> 2 & 7) * 4;
			if ( !wave_size )
				continue;
			
			int last_amp = osc->last_amp;
			int wave_pos = osc->wave_pos;
			
			Blip_set_modified( output );
			
			do
			{
				// read wave sample
				int addr = wave_pos + osc_reg [6];
				int sample = this->reg [addr >> 1] >> (addr << 2 & 4);
				wave_pos++;
				sample = (sample & 15) * volume;
				
				// output impulse if amplitude changed
				int delta = sample - last_amp;
				if ( delta )
				{
					last_amp = sample;
					Synth_offset_resampled( &this->synth, time, delta, output );
				}
				
				// next sample
				time += period;
				if ( wave_pos >= wave_size )
					wave_pos = 0;
			}
			while ( time < end_time );
			
			osc->wave_pos = wave_pos;
			osc->last_amp = last_amp;
		}
		osc->delay = time - end_time;
	}
	
	this->last_time = nes_end_time;
}

