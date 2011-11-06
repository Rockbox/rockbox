// Game_Music_Emu 0.6-pre. http://www.slack.net/~ant/

#include "sgc_emu.h"

/* Copyright (C) 2009 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License aint with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include "blargg_source.h"

int const osc_count = sms_osc_count + fm_apu_osc_count;

const char gme_wrong_file_type [] = "Wrong file type for this emulator";

static void clear_track_vars( struct Sgc_Emu* this )
{
	this->current_track    = -1;
	track_stop( &this->track_filter );
}

void Sgc_init( struct Sgc_Emu* this )
{
	assert( offsetof (struct header_t,copyright [32]) == header_size );
	
	this->sample_rate = 0;
	this->mute_mask_  = 0;
	this->tempo       = (int)FP_ONE_TEMPO;
	this->gain        = (int)FP_ONE_GAIN;
	
	// defaults
	this->tfilter = *track_get_setup( &this->track_filter );
	this->tfilter.max_initial = 2;
	this->tfilter.lookahead   = 6;
	this->track_filter.silence_ignored_ = false;

	Sms_apu_init( &this->apu );
	Fm_apu_create( &this->fm_apu );

	Rom_init( &this->rom, 0x4000 );
	Z80_init( &this->cpu );

	Sound_set_gain( this, (int)(FP_ONE_GAIN*1.2) );
	
	// Unload
	this->voice_count = 0;
	this->voice_types = 0;
	clear_track_vars( this );
}

// Setup

blargg_err_t Sgc_load_mem( struct Sgc_Emu* this, const void* data, long size )
{
	RETURN_ERR( Rom_load( &this->rom, data, size, header_size, &this->header, 0 ) );
	
	if ( !valid_tag( &this->header ) )
		return gme_wrong_file_type;
	
	/* if ( header.vers != 1 )
		warning( "Unknown file version" ); */
	
	/* if ( header.system > 2 )
		warning( "Unknown system" ); */
	
	addr_t load_addr = get_le16( this->header.load_addr );
	/* if ( load_addr < 0x400 )
		set_warning( "Invalid load address" ); */
	
	Rom_set_addr( &this->rom, load_addr );
	this->play_period = clock_rate( this ) / 60;
	
	if ( sega_mapping( this ) && Fm_apu_supported() )
		RETURN_ERR( Fm_apu_init( &this->fm_apu, clock_rate( this ), clock_rate( this ) / 72 ) );
	
	this->m3u.size = 0;
	this->track_count = this->header.song_count;
	this->voice_count =  sega_mapping( this ) ? osc_count : sms_osc_count;
	static int const types [sms_osc_count + fm_apu_osc_count] = {
		wave_type+1, wave_type+2, wave_type+3, mixed_type+1, mixed_type+2
	};
	this->voice_types = types;
	
	Sms_apu_volume( &this->apu, this->gain );
	Fm_apu_volume( &this->fm_apu, this->gain );

	// Setup buffer
	this->clock_rate_ = clock_rate( this );
	Buffer_clock_rate( &this->stereo_buf, clock_rate( this ) );
	RETURN_ERR( Buffer_set_channel_count( &this->stereo_buf, this->voice_count, this->voice_types ) );
	this->buf_changed_count = Buffer_channels_changed_count( &this->stereo_buf );

	Sound_set_tempo( this, this->tempo );
	Sound_mute_voices( this, this->mute_mask_ );
	return 0;
}

static void Sound_set_voice( struct Sgc_Emu* this, int i, struct Blip_Buffer* c, struct Blip_Buffer* l, struct Blip_Buffer* r )
{
	if ( i < sms_osc_count )
		Sms_apu_set_output( &this->apu, i, c, l, r );
	else
		Fm_apu_set_output( &this->fm_apu, c );
}

static blargg_err_t run_clocks( struct Sgc_Emu* this, blip_time_t* duration, int msec )
{
#if defined(ROCKBOX)
	(void) msec;
#endif

	cpu_time_t t = *duration;
	while ( Z80_time( &this->cpu ) < t )
	{
		cpu_time_t next = min( t, this->next_play );
		if ( run_cpu( this, next ) )
		{
			/* warning( "Unsupported CPU instruction" ); */
			Z80_set_time( &this->cpu, next );
		}
		
		if ( this->cpu.r.pc == this->idle_addr )
			Z80_set_time( &this->cpu, next );
		
		if ( Z80_time( &this->cpu ) >= this->next_play )
		{
			this->next_play += this->play_period;
			if ( this->cpu.r.pc == this->idle_addr )
				jsr( this, this->header.play_addr );
		}
	}
	
	this->next_play -= t;
	check( this->next_play >= 0 );
	Z80_adjust_time( &this->cpu, -t );
	
	Sms_apu_end_frame( &this->apu, t );
	if ( sega_mapping( this ) && this->fm_accessed )
	{
		if ( Fm_apu_supported() )
			Fm_apu_end_frame( &this->fm_apu, t );
		/* else
			warning( "FM sound not supported" ); */
	}

	return 0;
}

// Emulation

void cpu_out( struct Sgc_Emu* this, cpu_time_t time, addr_t addr, int data )
{
	int port = addr & 0xFF;
	
	if ( sega_mapping( this ) )
	{
		switch ( port )
		{
		case 0x06:
			Sms_apu_write_ggstereo( &this->apu, time, data );
			return;
		
		case 0x7E:
		case 0x7F:
			Sms_apu_write_data( &this->apu, time, data ); /* dprintf( "$7E<-%02X\n", data ); */
			return;
		
		case 0xF0:
			this->fm_accessed = true;
			if ( Fm_apu_supported() )
				Fm_apu_write_addr( &this->fm_apu, data );//, dprintf( "$F0<-%02X\n", data );
			return;
		
		case 0xF1:
			this->fm_accessed = true;
			if ( Fm_apu_supported() )
				Fm_apu_write_data( &this->fm_apu, time, data );//, dprintf( "$F1<-%02X\n", data );
			return;
		}
	}
	else if ( port >= 0xE0 )
	{
		Sms_apu_write_data( &this->apu, time, data );
		return;
	}
}

void jsr( struct Sgc_Emu* this, byte addr [2] )
{
	*Z80_write( &this->cpu, --this->cpu.r.sp ) = this->idle_addr >> 8;
	*Z80_write( &this->cpu, --this->cpu.r.sp ) = this->idle_addr & 0xFF;
	this->cpu.r.pc = get_le16( addr );
}

static void set_bank( struct Sgc_Emu* this, int bank, void const* data )
{
	//dprintf( "map bank %d to %p\n", bank, (byte*) data - rom.at_addr( 0 ) );
	Z80_map_mem( &this->cpu, bank * this->rom.bank_size, this->rom.bank_size, this->unmapped_write, data );
}

void cpu_write( struct Sgc_Emu* this, addr_t addr, int data )
{
	if ( (addr ^ 0xFFFC) > 3 || !sega_mapping( this ) )
	{
		*Z80_write( &this->cpu, addr ) = data;
		return;
	}
	
	switch ( addr )
	{
	case 0xFFFC:
		Z80_map_mem_rw( &this->cpu, 2 * this->rom.bank_size, this->rom.bank_size, this->ram2 );
		if ( data & 0x08 )
			break;
		
		this->bank2 = this->ram2;
		// FALL THROUGH
	
	case 0xFFFF: {
		bool rom_mapped = (Z80_read( &this->cpu, 2 * this->rom.bank_size ) == this->bank2);
		this->bank2 = Rom_at_addr( &this->rom, data * this->rom.bank_size );
		if ( rom_mapped )
			set_bank( this, 2, this->bank2 );
		break;
	}
		
	case 0xFFFD:
		set_bank( this, 0, Rom_at_addr( &this->rom, data * this->rom.bank_size ) );
		break;
	
	case 0xFFFE:
		set_bank( this, 1, Rom_at_addr( &this->rom, data * this->rom.bank_size ) );
		break;
	}
}

blargg_err_t Sgc_set_sample_rate( struct Sgc_Emu* this, int rate )
{
	require( !this->sample_rate ); // sample rate can't be changed once set
	Buffer_init( &this->stereo_buf );
	Buffer_set_sample_rate( &this->stereo_buf, rate, 1000 / 20 );

	// Set buffer bass
	Buffer_bass_freq( &this->stereo_buf, 80 );

	this->sample_rate = rate;
	RETURN_ERR( track_init( &this->track_filter, this ) );
	this->tfilter.max_silence = 6 * stereo * this->sample_rate;
	return 0;
}

void Sound_mute_voice( struct Sgc_Emu* this, int index, bool mute )
{
	require( (unsigned) index < (unsigned) this->voice_count );
	int bit = 1 << index;
	int mask = this->mute_mask_ | bit;
	if ( !mute )
		mask ^= bit;
	Sound_mute_voices( this, mask );
}

void Sound_mute_voices( struct Sgc_Emu* this, int mask )
{
	require( this->sample_rate ); // sample rate must be set first
	this->mute_mask_ = mask;
	
	int i;
	for ( i = this->voice_count; i--; )
	{
		if ( mask & (1 << i) )
		{
			Sound_set_voice( this, i, 0, 0, 0 );
		}
		else
		{
			struct channel_t ch = Buffer_channel( &this->stereo_buf, i );
			assert( (ch.center && ch.left && ch.right) ||
					(!ch.center && !ch.left && !ch.right) ); // all or nothing
			Sound_set_voice( this, i, ch.center, ch.left, ch.right );
		}
	}
}

void Sound_set_tempo( struct Sgc_Emu* this, int t )
{
	require( this->sample_rate ); // sample rate must be set first
	int const min = (int)(FP_ONE_TEMPO*0.02);
	int const max = (int)(FP_ONE_TEMPO*4.00);
	if ( t < min ) t = min;
	if ( t > max ) t = max;
	this->tempo = t;

	this->play_period = (int) ((clock_rate( this ) * FP_ONE_TEMPO) / (this->header.rate ? 50 : 60) / t);
}

blargg_err_t Sgc_start_track( struct Sgc_Emu* this, int track )
{
	clear_track_vars( this );
	
	// Remap track if playlist available
	if ( this->m3u.size > 0 ) {
		struct entry_t* e = &this->m3u.entries[track];
		track = e->track;
	}
	
	this->current_track = track;
	
	if ( sega_mapping( this ) )
	{
		Sms_apu_reset( &this->apu, 0, 0 );
		Fm_apu_reset( &this->fm_apu );
		this->fm_accessed = false;
	}
	else
	{
		Sms_apu_reset( &this->apu, 0x0003, 15 );
	}
	
	memset( this->ram , 0, sizeof this->ram );
	memset( this->ram2, 0, sizeof this->ram2 );
	memset( this->vectors, 0xFF, sizeof this->vectors );
	Z80_reset( &this->cpu, this->unmapped_write, this->rom.unmapped );
	
	if ( sega_mapping( this ) )
	{
		this->vectors_addr = 0x10000 - page_size;
		this->idle_addr = this->vectors_addr;
		int i;
		for ( i = 1; i < 8; ++i )
		{
			this->vectors [i*8 + 0] = 0xC3; // JP addr
			this->vectors [i*8 + 1] = this->header.rst_addrs [i - 1] & 0xff;
			this->vectors [i*8 + 2] = this->header.rst_addrs [i - 1] >> 8;
		}
		
		Z80_map_mem_rw( &this->cpu, 0xC000, 0x2000, this->ram );
		Z80_map_mem( &this->cpu, this->vectors_addr, page_size, this->unmapped_write, this->vectors );
		
		this->bank2 = NULL;
		for ( i = 0; i < 4; ++i )
			cpu_write( this, 0xFFFC + i, this->header.mapping [i] );
	}
	else
	{
		if ( !this->coleco_bios )
			return "Coleco BIOS not set"; /* BLARGG_ERR( BLARGG_ERR_CALLER, "Coleco BIOS not set" ); */
		
		this->vectors_addr = 0;
		Z80_map_mem( &this->cpu, 0, 0x2000, this->unmapped_write, this->coleco_bios );
		int i;
		for ( i = 0; i < 8; ++i )
			Z80_map_mem_rw( &this->cpu, 0x6000 + i*0x400, 0x400, this->ram );
		
		this->idle_addr = 0x2000;
		Z80_map_mem( &this->cpu, 0x2000, page_size, this->unmapped_write, this->vectors );
		
		for ( i = 0; i < 0x8000 / this->rom.bank_size; ++i )
		{
			int addr = 0x8000 + i*this->rom.bank_size;
			Z80_map_mem( &this->cpu, addr, this->rom.bank_size, this->unmapped_write, Rom_at_addr( &this->rom, addr ) );
		}
	}
	
	this->cpu.r.sp  = get_le16( this->header.stack_ptr );
	this->cpu.r.b.a = track;
	this->next_play = this->play_period;

	jsr( this, this->header.init_addr );

	Buffer_clear( &this->stereo_buf );

	// convert filter times to samples
	struct setup_t s = this->tfilter;
	s.max_initial *= this->sample_rate * stereo;
	#ifdef GME_DISABLE_SILENCE_LOOKAHEAD
		s.lookahead = 1;
	#endif
	track_setup( &this->track_filter, &s );
	
	return track_start( &this->track_filter );
}

// Tell/Seek

static int msec_to_samples( int msec, int sample_rate )
{
	int sec = msec / 1000;
	msec -= sec * 1000;
	return (sec * sample_rate + msec * sample_rate / 1000) * stereo;
}

int Track_tell( struct Sgc_Emu* this )
{
	int rate = this->sample_rate * stereo;
	int sec = track_sample_count( &this->track_filter ) / rate;
	return sec * 1000 + (track_sample_count( &this->track_filter ) - sec * rate) * 1000 / rate;
}

blargg_err_t Track_seek( struct Sgc_Emu* this, int msec )
{
	int time = msec_to_samples( msec, this->sample_rate );
	if ( time < track_sample_count( &this->track_filter ) )
	RETURN_ERR( Sgc_start_track( this, this->current_track ) );
	return Track_skip( this, time - track_sample_count( &this->track_filter ) );
}

blargg_err_t Track_skip( struct Sgc_Emu* this, int count )
{
	require( this->current_track >= 0 ); // start_track() must have been called already
	return track_skip( &this->track_filter, count );
}

blargg_err_t skip_( void* emu, int count )
{
	struct Sgc_Emu* this = (struct Sgc_Emu*) emu;
	
	// for long skip, mute sound
	const int threshold = 32768;
	if ( count > threshold )
	{
		int saved_mute = this->mute_mask_;
		Sound_mute_voices( this, ~0 );

		int n = count - threshold/2;
		n &= ~(2048-1); // round to multiple of 2048
		count -= n;
		RETURN_ERR( skippy_( &this->track_filter, n ) );

		Sound_mute_voices( this, saved_mute );
	}

	return skippy_( &this->track_filter, count );
}

void Track_set_fade( struct Sgc_Emu* this, int start_msec, int length_msec )
{
	track_set_fade( &this->track_filter, msec_to_samples( start_msec, this->sample_rate ),
		length_msec * this->sample_rate / (1000 / stereo) );
}

blargg_err_t Sgc_play( struct Sgc_Emu* this, int out_count, sample_t* out )
{
	require( this->current_track >= 0 );
	require( out_count % stereo == 0 );
	return track_play( &this->track_filter, out_count, out );
}

blargg_err_t play_( void* emu, int count, sample_t out [] )
{
	struct Sgc_Emu* this = (struct Sgc_Emu*) emu;
	
	int remain = count;
	while ( remain )
	{
		Buffer_disable_immediate_removal( &this->stereo_buf );
		remain -= Buffer_read_samples( &this->stereo_buf, &out [count - remain], remain );
		if ( remain )
		{
			if ( this->buf_changed_count != Buffer_channels_changed_count( &this->stereo_buf ) )
			{
				this->buf_changed_count = Buffer_channels_changed_count( &this->stereo_buf );
				
				// Remute voices
				Sound_mute_voices( this, this->mute_mask_ );
			}
			int msec = Buffer_length( &this->stereo_buf );
			blip_time_t clocks_emulated = msec * this->clock_rate_ / 1000 - 100;
			RETURN_ERR( run_clocks( this, &clocks_emulated, msec ) );
			assert( clocks_emulated );
			Buffer_end_frame( &this->stereo_buf, clocks_emulated );
		}
	}
	return 0;
}
