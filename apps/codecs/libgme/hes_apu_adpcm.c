// Game_Music_Emu 0.6-pre. http://www.slack.net/~ant/

#include "hes_apu_adpcm.h"

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


void Adpcm_init( struct Hes_Apu_Adpcm* this )
{
	this->output = NULL;
	memset( &this->state, 0, sizeof( this->state ) );
	Adpcm_reset( this );
}

void Adpcm_reset( struct Hes_Apu_Adpcm* this )
{
	this->last_time = 0;
	this->next_timer = 0;
	this->last_amp = 0;

	memset( &this->state.pcmbuf, 0, sizeof(this->state.pcmbuf) );
	memset( &this->state.port, 0, sizeof(this->state.port) );

	this->state.ad_sample = 0;
	this->state.ad_ref_index = 0;

	this->state.addr = 0;
	this->state.freq = 0;
	this->state.writeptr = 0;
	this->state.readptr = 0;
	this->state.playflag = 0;
	this->state.repeatflag = 0;
	this->state.length = 0;
	this->state.volume = 0xFF;
	this->state.fadetimer = 0;
	this->state.fadecount = 0;
}

static short stepsize[49] = {
  16,  17,  19,  21,  23,  25,  28,
  31,  34,  37,  41,  45,  50,  55,
  60,  66,  73,  80,  88,  97, 107,
 118, 130, 143, 157, 173, 190, 209,
 230, 253, 279, 307, 337, 371, 408,
 449, 494, 544, 598, 658, 724, 796,
 876, 963,1060,1166,1282,1411,1552
};

static int Adpcm_decode( struct Hes_Apu_Adpcm* this,int code );
static int Adpcm_decode( struct Hes_Apu_Adpcm* this,int code )
{
	struct State* state = &this->state;
	int step = stepsize[state->ad_ref_index];
	int delta;
	int c = code & 7;
#if 1
	delta = 0;
	if ( c & 4 ) delta += step;
	step >>= 1;
	if ( c & 2 ) delta += step;
	step >>= 1;
	if ( c & 1 ) delta += step;
	step >>= 1;
	delta += step;
#else
	delta = ( ( c + c + 1 ) * step ) / 8; // maybe faster, but introduces rounding
#endif
	if ( c != code )
	{
		state->ad_sample -= delta;
		if ( state->ad_sample < -2048 )
			state->ad_sample = -2048;
	}
	else
	{
		state->ad_sample += delta;
		if ( state->ad_sample > 2047 )
			state->ad_sample = 2047;
	}

	static int const steps [8] = {
		-1, -1, -1, -1, 2, 4, 6, 8
	};
	state->ad_ref_index += steps [c];
	if ( state->ad_ref_index < 0 )
		state->ad_ref_index = 0;
	else if ( state->ad_ref_index > 48 )
		state->ad_ref_index = 48;

	return state->ad_sample;
}

static void Adpcm_run_until( struct Hes_Apu_Adpcm* this, blip_time_t end_time );
static void Adpcm_run_until( struct Hes_Apu_Adpcm* this, blip_time_t end_time )
{
	struct State* state = &this->state;
	int volume = state->volume;
	int fadetimer = state->fadetimer;
	int fadecount = state->fadecount;
	int last_time = this->last_time;
	int next_timer = this->next_timer;
	int last_amp = this->last_amp;
	
	struct Blip_Buffer* output = this->output; // cache often-used values

	while ( state->playflag && last_time < end_time )
	{
		while ( last_time >= next_timer )
		{
			if ( fadetimer )
			{
				if ( fadecount > 0 )
				{
					fadecount--;
					volume = 0xFF * fadecount / fadetimer;
				}
				else if ( fadecount < 0 )
				{
					fadecount++;
					volume = 0xFF - ( 0xFF * fadecount / fadetimer );
				}
			}
			next_timer += 7159; // 7159091/1000;
		}
		int amp;
		if ( state->ad_low_nibble )
		{
			amp = Adpcm_decode( this, state->pcmbuf[ state->playptr ] & 0x0F );
			state->ad_low_nibble = false;
			state->playptr++;
			state->playedsamplecount++;
			if ( state->playedsamplecount == state->playlength )
			{
				state->playflag = 0;
			}
		}
		else
		{
			amp = Adpcm_decode( this, state->pcmbuf[ state->playptr ] >> 4 );
			state->ad_low_nibble = true;
		}
		amp = amp * volume / 0xFF;
		int delta = amp - last_amp;
		if ( output && delta )
		{
			last_amp = amp;
			Synth_offset_inline( &this->synth, last_time, delta, output );
		}
		last_time += state->freq;
	}

	if ( !state->playflag )
	{
		while ( next_timer <= end_time ) next_timer += 7159; // 7159091/1000
		last_time = end_time;
	}
	
	this->last_time  = last_time;
	this->next_timer = next_timer;
	this->last_amp   = last_amp;
	state->volume = volume;
	state->fadetimer = fadetimer;
	state->fadecount = fadecount;
}

void Adpcm_write_data( struct Hes_Apu_Adpcm* this, blip_time_t time, int addr, int data )
{
	if ( time > this->last_time ) Adpcm_run_until( this, time );
	struct State* state = &this->state;

	data &= 0xFF;
	state->port[ addr & 15 ] = data;
	switch ( addr & 15 )
	{
	case 8:
		state->addr &= 0xFF00;
		state->addr |= data;
		break;
	case 9:
		state->addr &= 0xFF;
		state->addr |= data << 8;
		break;
	case 10:
		state->pcmbuf[ state->writeptr++ ] = data;
		state->playlength ++;
		break;
	case 11:
		dprintf("ADPCM DMA 0x%02X", data);
		break;
	case 13:
		if ( data & 0x80 )
		{
			state->addr = 0;
			state->freq = 0;
			state->writeptr = 0;
			state->readptr = 0;
			state->playflag = 0;
			state->repeatflag = 0;
			state->length = 0;
			state->volume = 0xFF;
		}
		if ( ( data & 3 ) == 3 )
		{
			state->writeptr = state->addr;
		}
		if ( data & 8 )
		{
			state->readptr = state->addr ? state->addr - 1 : state->addr;
		}
		if ( data & 0x10 )
		{
			state->length = state->addr;
		}
		state->repeatflag = data & 0x20;
		state->playflag = data & 0x40;
		if ( state->playflag )
		{
			state->playptr = state->readptr;
			state->playlength = state->length + 1;
			state->playedsamplecount = 0;
			state->ad_sample = 0;
			state->ad_low_nibble = false;
		}
		break;
	case 14:
		state->freq = 7159091 / ( 32000 / ( 16 - ( data & 15 ) ) );
		break;
	case 15:
		switch ( data & 15 )
		{
		case 0:
		case 8:
		case 12:
			state->fadetimer = -100;
			state->fadecount = state->fadetimer;
			break;
		case 10:
			state->fadetimer = 5000;
			state->fadecount = state->fadetimer;
			break;
		case 14:
			state->fadetimer = 1500;
			state->fadecount = state->fadetimer;
			break;
		}
		break;
	}
}

int Adpcm_read_data( struct Hes_Apu_Adpcm* this, blip_time_t time, int addr )
{
	if ( time > this->last_time ) Adpcm_run_until( this, time );

	struct State* state = &this->state;
	switch ( addr & 15 )
	{
	case 10:
		return state->pcmbuf [state->readptr++];
	case 11:
		return state->port [11] & ~1;
	case 12:
		if (!state->playflag)
		{
			state->port [12] |= 1;
			state->port [12] &= ~8;
		}
		else
		{
			state->port [12] &= ~1;
			state->port [12] |= 8;
		}
		return state->port [12];
	case 13:
		return state->port [13];
	}

	return 0xFF;
}

void Adpcm_end_frame( struct Hes_Apu_Adpcm* this, blip_time_t end_time )
{
	Adpcm_run_until( this, end_time );
	this->last_time -= end_time;
	this->next_timer -= end_time;
	check( last_time >= 0 );
	if ( this->output )
		Blip_set_modified( this->output );
}
