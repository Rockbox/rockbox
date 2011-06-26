    // Nes_Snd_Emu 0.1.8. http://www.slack.net/~ant/

#include "nes_vrc6_apu.h"

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

void Vrc6_init( struct Nes_Vrc6_Apu* this )
{
	Synth_init( &this->saw_synth );
	Synth_init( &this->square_synth );
	
	Vrc6_output( this, NULL );
	Vrc6_volume( this, (int)FP_ONE_VOLUME );
	Vrc6_reset( this );
}

void Vrc6_reset( struct Nes_Vrc6_Apu* this )
{
	this->last_time = 0;
	int i;
	for ( i = 0; i < vrc6_osc_count; i++ )
	{
		struct Vrc6_Osc* osc = &this->oscs [i];
		int j;
		for ( j = 0; j < vrc6_reg_count; j++ )
			osc->regs [j] = 0;
		osc->delay = 0;
		osc->last_amp = 0;
		osc->phase = 1;
		osc->amp = 0;
	}
}

void Vrc6_output( struct Nes_Vrc6_Apu* this, struct Blip_Buffer* buf )
{
	int i;
	for ( i = 0; i < vrc6_osc_count; i++ )
		Vrc6_osc_output( this, i, buf );
}

void run_square( struct Nes_Vrc6_Apu* this, struct Vrc6_Osc* osc, blip_time_t end_time );
void run_saw( struct Nes_Vrc6_Apu* this, blip_time_t end_time );
static void Vrc6_run_until( struct Nes_Vrc6_Apu* this, blip_time_t time )
{
	require( time >= this->last_time );
	run_square( this, &this->oscs [0], time );
	run_square( this, &this->oscs [1], time );
	run_saw( this, time );
	this->last_time = time;
}

void Vrc6_write_osc( struct Nes_Vrc6_Apu* this, blip_time_t time, int osc_index, int reg, int data )
{
	require( (unsigned) osc_index < vrc6_osc_count );
	require( (unsigned) reg < vrc6_reg_count );
	
	Vrc6_run_until( this, time );
	this->oscs [osc_index].regs [reg] = data;
}

void Vrc6_end_frame( struct Nes_Vrc6_Apu* this, blip_time_t time )
{
	if ( time > this->last_time )
		Vrc6_run_until( this, time );
	
	assert( this->last_time >= time );
	this->last_time -= time;
}

void run_square( struct Nes_Vrc6_Apu* this, struct Vrc6_Osc* osc, blip_time_t end_time )
{
	struct Blip_Buffer* output = osc->output;
	if ( !output )
		return;
	
	int volume = osc->regs [0] & 15;
	if ( !(osc->regs [2] & 0x80) )
		volume = 0;
	
	int gate = osc->regs [0] & 0x80;
	int duty = ((osc->regs [0] >> 4) & 7) + 1;
	int delta = ((gate || osc->phase < duty) ? volume : 0) - osc->last_amp;
	blip_time_t time = this->last_time;
	if ( delta )
	{
		osc->last_amp += delta;
		Blip_set_modified( output );
		Synth_offset( &this->square_synth, time, delta, output );
	}
	
	time += osc->delay;
	osc->delay = 0;
	int period = Vrc6_osc_period( osc );
	if ( volume && !gate && period > 4 )
	{
		if ( time < end_time )
		{
			int phase = osc->phase;
			Blip_set_modified( output );
			
			do
			{
				phase++;
				if ( phase == 16 )
				{
					phase = 0;
					osc->last_amp = volume;
					Synth_offset( &this->square_synth, time, volume, output );
				}
				if ( phase == duty )
				{
					osc->last_amp = 0;
					Synth_offset( &this->square_synth, time, -volume, output );
				}
				time += period;
			}
			while ( time < end_time );
			
			osc->phase = phase;
		}
		osc->delay = time - end_time;
	}
}

void run_saw( struct Nes_Vrc6_Apu* this, blip_time_t end_time )
{
	struct Vrc6_Osc* osc = &this->oscs [2];
	struct Blip_Buffer* output = osc->output;
	if ( !output )
		return;
	Blip_set_modified( output );
	
	int amp = osc->amp;
	int amp_step = osc->regs [0] & 0x3F;
	blip_time_t time = this->last_time;
	int last_amp = osc->last_amp;
	if ( !(osc->regs [2] & 0x80) || !(amp_step | amp) )
	{
		osc->delay = 0;
		int delta = (amp >> 3) - last_amp;
		last_amp = amp >> 3;
		Synth_offset( &this->saw_synth, time, delta, output );
	}
	else
	{
		time += osc->delay;
		if ( time < end_time )
		{
			int period = Vrc6_osc_period( osc ) * 2;
			int phase = osc->phase;
			
			do
			{
				if ( --phase == 0 )
				{
					phase = 7;
					amp = 0;
				}
				
				int delta = (amp >> 3) - last_amp;
				if ( delta )
				{
					last_amp = amp >> 3;
					Synth_offset( &this->saw_synth, time, delta, output );
				}
				
				time += period;
				amp = (amp + amp_step) & 0xFF;
			}
			while ( time < end_time );
			
			osc->phase = phase;
			osc->amp = amp;
		}
		
		osc->delay = time - end_time;
	}
	
	osc->last_amp = last_amp;
}

