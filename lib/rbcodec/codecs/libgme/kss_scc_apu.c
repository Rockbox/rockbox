// Game_Music_Emu 0.6-pre. http://www.slack.net/~ant/

#include "kss_scc_apu.h"

/* Copyright (C) 2006-2008 Shay Green. This module is free software; you
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

// Tones above this frequency are treated as disabled tone at half volume.
// Power of two is more efficient (avoids division).
extern int const inaudible_freq;

int const wave_size = 0x20;

static void set_output( struct Scc_Apu* this, struct Blip_Buffer* buf )
{
	int i;
	for ( i = 0; i < scc_osc_count; ++i )
		Scc_set_output( this, i, buf );
}

void Scc_volume( struct Scc_Apu* this, int v )
{
	Synth_volume( &this->synth, (v/2 - (v*7)/100) / scc_osc_count / scc_amp_range );
}

void Scc_reset( struct Scc_Apu* this )
{
	this->last_time = 0;

	int i;
	for ( i = scc_osc_count; --i >= 0; )
		memset( &this->oscs [i], 0, offsetof (struct scc_osc_t,output) );

	memset( this->regs, 0, sizeof this->regs );
}

void Scc_init( struct Scc_Apu* this )
{
	Synth_init( &this->synth);
	
	set_output( this, NULL );
	Scc_volume( this, (int)FP_ONE_VOLUME );
	Scc_reset( this );
}

static void run_until( struct Scc_Apu* this, blip_time_t end_time )
{
	int index;
	for ( index = 0; index < scc_osc_count; index++ )
	{
		struct scc_osc_t* osc = &this->oscs [index];

		struct Blip_Buffer* const output = osc->output;
		if ( !output )
			continue;

		blip_time_t period = (this->regs [0xA0 + index * 2 + 1] & 0x0F) * 0x100 +
				this->regs [0xA0 + index * 2] + 1;
		int volume = 0;
		if ( this->regs [0xAF] & (1 << index) )
		{
			blip_time_t inaudible_period = (unsigned) (Blip_clock_rate( output ) +
					inaudible_freq * 32) / (unsigned) (inaudible_freq * 16);
			if ( period > inaudible_period )
				volume = (this->regs [0xAA + index] & 0x0F) * (scc_amp_range / 256 / 15);
		}

		int8_t const* wave = (int8_t*) this->regs + index * wave_size;
		/*if ( index == osc_count - 1 )
			wave -= wave_size; // last two oscs share same wave RAM*/

		{
			int delta = wave [osc->phase] * volume - osc->last_amp;
			if ( delta )
			{
				osc->last_amp += delta;
				Blip_set_modified( output );
				Synth_offset( &this->synth, this->last_time, delta, output );
			}
		}

		blip_time_t time = this->last_time + osc->delay;
		if ( time < end_time )
		{
			int phase = osc->phase;
			if ( !volume )
			{
				// maintain phase
				int count = (end_time - time + period - 1) / period;
				phase += count; // will be masked below
				time  += count * period;
			}
			else
			{
				int last_wave = wave [phase];
				phase = (phase + 1) & (wave_size - 1); // pre-advance for optimal inner loop
				do
				{
					int delta = wave [phase] - last_wave;
					phase = (phase + 1) & (wave_size - 1);
					if ( delta )
					{
						last_wave += delta;
						Synth_offset_inline( &this->synth, time, delta * volume, output );
					}
					time += period;
				}
				while ( time < end_time );

				osc->last_amp = last_wave * volume;
				Blip_set_modified( output );
				phase--; // undo pre-advance
			}
			osc->phase = phase & (wave_size - 1);
		}
		osc->delay = time - end_time;
	}
	this->last_time = end_time;
}

void Scc_write( struct Scc_Apu* this, blip_time_t time, int addr, int data )
{
	//assert( (unsigned) addr < reg_count );
	assert( ( addr >= 0x9800 && addr <= 0x988F ) || ( addr >= 0xB800 && addr <= 0xB8AF ) );
	run_until( this, time );

	addr -= 0x9800;
	if ( ( unsigned ) addr < 0x90 )
	{
	    if ( ( unsigned ) addr < 0x60 )
            this->regs [addr] = data;
        else if ( ( unsigned ) addr < 0x80 )
        {
            this->regs [addr] = this->regs[addr + 0x20] = data;
        }
        else if ( ( unsigned ) addr < 0x90 )
        {
            this->regs [addr + 0x20] = data;
        }
	}
	else
	{
	    addr -= 0xB800 - 0x9800;
	    if ( ( unsigned ) addr < 0xB0 )
            this->regs [addr] = data;
	}
}

void Scc_end_frame( struct Scc_Apu* this, blip_time_t end_time )
{
	if ( end_time > this->last_time )
		run_until( this, end_time );

	this->last_time -= end_time;
	assert( this->last_time >= 0 );
}
