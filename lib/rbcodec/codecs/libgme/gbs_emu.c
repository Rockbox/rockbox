// Game_Music_Emu 0.5.2. http://www.slack.net/~ant/

#include "gbs_emu.h"

#include "blargg_endian.h"

/* Copyright (C) 2003-2006 Shay Green. this module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. this
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include "blargg_source.h"

const char gme_wrong_file_type [] = "Wrong file type for this emulator";

int const idle_addr = 0xF00D;
int const tempo_unit = 16;

static void clear_track_vars( struct Gbs_Emu* this )
{
	this->current_track_    = -1;
	track_stop( &this->track_filter );
}

void Gbs_init( struct Gbs_Emu* this )
{	
	this->sample_rate_ = 0;
	this->mute_mask_   = 0;
	this->tempo_       = (int)(FP_ONE_TEMPO);
	
	// Unload
	this->header.timer_mode = 0;

	// defaults
	this->tfilter = *track_get_setup( &this->track_filter );
	this->tfilter.max_initial = 21;
	this->tfilter.lookahead   = 6;
	this->track_filter.silence_ignored_ = false;

	Sound_set_gain( this, (int)(FP_ONE_GAIN*1.2) );
	
	Rom_init( &this->rom, 0x4000 );
	
	Apu_init( &this->apu );
	Cpu_init( &this->cpu );
	
	this->tempo = tempo_unit;
	this->sound_hardware = sound_gbs;
	
	// Reduce apu sound clicks?
	Apu_reduce_clicks( &this->apu, true );
	
	// clears fields
	this->voice_count_ = 0;
	this->voice_types_ = 0;
	clear_track_vars( this );
}

static blargg_err_t check_gbs_header( void const* header )
{
	if ( memcmp( header, "GBS", 3 ) )
		return gme_wrong_file_type;
	return 0;
}

// Setup

blargg_err_t Gbs_load_mem( struct Gbs_Emu* this, void* data, long size )
{
	// Unload
	this->header.timer_mode = 0;
	this->voice_count_ = 0;
	this->track_count = 0;
	this->m3u.size = 0;
	clear_track_vars( this );

	assert( offsetof (struct header_t,copyright [32]) == header_size );
	RETURN_ERR( Rom_load( &this->rom, data, size, header_size, &this->header, 0 ) );
	
	RETURN_ERR( check_gbs_header( &this->header ) );
	
	/* Ignore warnings? */
	/*if ( header_.vers != 1 )
		warning( "Unknown file version" );

	if ( header_.timer_mode & 0x78 )
		warning( "Invalid timer mode" ); */

	/* unsigned load_addr = get_le16( this->header.load_addr ); */
	/* if ( (header_.load_addr [1] | header_.init_addr [1] | header_.play_addr [1]) > 0x7F ||
			load_addr < 0x400 )
		warning( "Invalid load/init/play address" ); */

	unsigned load_addr = get_le16( this->header.load_addr );
	/* if ( (this->header.load_addr [1] | this->header.init_addr [1] | this->header.play_addr [1]) > 0x7F ||
			load_addr < 0x400 )
		warning( "Invalid load/init/play address" ); */

	this->cpu.rst_base = load_addr;
	Rom_set_addr( &this->rom, load_addr );

	this->voice_count_ = osc_count;
	static int const types [osc_count] = {
		wave_type+1, wave_type+2, wave_type+3, mixed_type+1
	};
	this->voice_types_ = types;
	
	Apu_volume( &this->apu, this->gain_ );

	// Change clock rate & setup buffer
	this->clock_rate_ = 4194304;
	Buffer_clock_rate( &this->stereo_buf, 4194304 );
	RETURN_ERR( Buffer_set_channel_count( &this->stereo_buf, this->voice_count_, this->voice_types_ ) );
	this->buf_changed_count = Buffer_channels_changed_count( &this->stereo_buf );

	// Post load
	Sound_set_tempo( this, this->tempo_ );
	Sound_mute_voices( this, this->mute_mask_ );

	// Set track count
	this->track_count = this->header.track_count;
	return 0;
}

// Emulation

// see gb_cpu_io.h for read/write functions

void set_bank( struct Gbs_Emu* this, int n )
{
	addr_t addr = mask_addr( n * this->rom.bank_size, this->rom.mask );
	if ( addr == 0 && this->rom.size > this->rom.bank_size )
		addr = this->rom.bank_size; // MBC1&2 behavior, bank 0 acts like bank 1
	Cpu_map_code( &this->cpu, this->rom.bank_size, this->rom.bank_size, Rom_at_addr( &this->rom, addr ) );
}

void update_timer( struct Gbs_Emu* this )
{
	this->play_period = 70224 / tempo_unit; /// 59.73 Hz
	
	if ( this->header.timer_mode & 0x04 ) 
	{ 
		// Using custom rate
		static byte const rates [4] = { 6, 0, 2, 4 };
		// TODO: emulate double speed CPU mode rather than halving timer rate
		int double_speed = this->header.timer_mode >> 7;
		int shift = rates [this->ram [hi_page + 7] & 3] - double_speed;
		this->play_period = (256 - this->ram [hi_page + 6]) << shift;
	}
	
	this->play_period *= this->tempo;
}

// Jumps to routine, given pointer to address in file header. Pushes idle_addr
// as return address, NOT old PC.
void jsr_then_stop( struct Gbs_Emu* this, byte const addr [] )
{
	check( this->cpu.r.sp == get_le16( this->header.stack_ptr ) );
	this->cpu.r.pc = get_le16( addr );
	write_mem( this, --this->cpu.r.sp, idle_addr >> 8 );
	write_mem( this, --this->cpu.r.sp, idle_addr      );
}

static blargg_err_t run_until( struct Gbs_Emu* this, int end )
{
	this->end_time = end;
	Cpu_set_time( &this->cpu, Cpu_time( &this->cpu ) - end );
	while ( true )
	{
		run_cpu( this );
		if ( Cpu_time( &this->cpu ) >= 0 )
			break;
		
		if ( this->cpu.r.pc == idle_addr )
		{
			if ( this->next_play > this->end_time )
			{
				Cpu_set_time( &this->cpu, 0 );
				break;
			}
			
			if ( Cpu_time( &this->cpu ) < this->next_play - this->end_time )
				Cpu_set_time( &this->cpu, this->next_play - this->end_time );
			this->next_play += this->play_period;
			jsr_then_stop( this, this->header.play_addr );
		}
		else if ( this->cpu.r.pc > 0xFFFF )
		{
			/* warning( "PC wrapped around\n" ); */
			this->cpu.r.pc &= 0xFFFF;
		}
		else
		{
			/* warning( "Emulation error (illegal/unsupported instruction)" ); */
			this->cpu.r.pc = (this->cpu.r.pc + 1) & 0xFFFF;
			Cpu_set_time( &this->cpu, Cpu_time( &this->cpu ) + 6 );
		}
	}
	
	return 0;
}

static blargg_err_t end_frame( struct Gbs_Emu* this, int end )
{
	RETURN_ERR( run_until( this, end ) );
	
	this->next_play -= end;
	if ( this->next_play < 0 ) // happens when play routine takes too long
	{
		#if !defined(GBS_IGNORE_STARVED_PLAY)
			check( false );
		#endif
		this->next_play = 0;
	}
	
	Apu_end_frame( &this->apu, end );
	
	return 0;
}

blargg_err_t run_clocks( struct Gbs_Emu* this, blip_time_t duration )
{
	return end_frame( this, duration );
}

blargg_err_t play_( void* emu, int count, sample_t* out )
{
	struct Gbs_Emu* this = (struct Gbs_Emu*) emu;
	
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
			RETURN_ERR( run_clocks( this, clocks_emulated ) );
			assert( clocks_emulated );
			Buffer_end_frame( &this->stereo_buf, clocks_emulated );
		}
	}
	return 0;
}

blargg_err_t Gbs_set_sample_rate( struct Gbs_Emu* this, int rate )
{
	require( !this->sample_rate_ ); // sample rate can't be changed once set
	Buffer_init( &this->stereo_buf );
	RETURN_ERR( Buffer_set_sample_rate( &this->stereo_buf, rate, 1000 / 20 ) );
	
	// Set bass frequency
	Buffer_bass_freq( &this->stereo_buf, 300 );
	
	this->sample_rate_ = rate;
	RETURN_ERR( track_init( &this->track_filter, this ) );
	this->tfilter.max_silence = 6 * stereo * this->sample_rate_;
	return 0;
}


// Sound

void Sound_mute_voice( struct Gbs_Emu* this, int index, bool mute )
{
	require( (unsigned) index < (unsigned) this->voice_count_ );
	int bit = 1 << index;
	int mask = this->mute_mask_ | bit;
	if ( !mute )
		mask ^= bit;
	Sound_mute_voices( this, mask );
}

void Sound_mute_voices( struct Gbs_Emu* this, int mask )
{
	require( this->sample_rate_ ); // sample rate must be set first
	this->mute_mask_ = mask;
	
	int i;
	for ( i = this->voice_count_; i--; )
	{
		if ( mask & (1 << i) )
		{
			Apu_set_output( &this->apu, i, 0, 0, 0 );
		}
		else
		{
			struct channel_t ch = Buffer_channel( &this->stereo_buf, i );
			assert( (ch.center && ch.left && ch.right) ||
					(!ch.center && !ch.left && !ch.right) ); // all or nothing
			Apu_set_output( &this->apu, i, ch.center, ch.left, ch.right );
		}
	}
}

void Sound_set_tempo( struct Gbs_Emu* this, int t )
{
	require( this->sample_rate_ ); // sample rate must be set first
	int const min = (int)(FP_ONE_TEMPO*0.02);
	int const max = (int)(FP_ONE_TEMPO*4.00);
	if ( t < min ) t = min;
	if ( t > max ) t = max;
	this->tempo_ = t;
		
	this->tempo = (int) ((tempo_unit * FP_ONE_TEMPO) / t);
	Apu_set_tempo( &this->apu, t );
	update_timer( this );
}

blargg_err_t Gbs_start_track( struct Gbs_Emu* this, int track )
{
	clear_track_vars( this );

	// Remap track if playlist available
	if ( this->m3u.size > 0 ) {
		struct entry_t* e = &this->m3u.entries[track];
		track = e->track;
	}

	this->current_track_ = track;
	Buffer_clear( &this->stereo_buf );
	
	// Reset APU to state expected by most rips
	static byte const sound_data [] = {
		0x80, 0xBF, 0x00, 0x00, 0xB8, // square 1 DAC disabled
		0x00, 0x3F, 0x00, 0x00, 0xB8, // square 2 DAC disabled
		0x7F, 0xFF, 0x9F, 0x00, 0xB8, // wave     DAC disabled
		0x00, 0xFF, 0x00, 0x00, 0xB8, // noise    DAC disabled
		0x77, 0xFF, 0x80, // max volume, all chans in center, power on
	};
	
	enum sound_t mode = this->sound_hardware;
	if ( mode == sound_gbs )
		mode = (this->header.timer_mode & 0x80) ? sound_cgb : sound_dmg;
		
	Apu_reset( &this->apu, (enum gb_mode_t) mode, false );
	Apu_write_register( &this->apu, 0, 0xFF26, 0x80 ); // power on
	int i;
	for ( i = 0; i < (int) sizeof sound_data; i++ )
		Apu_write_register( &this->apu, 0, i + io_addr, sound_data [i] );
	Apu_end_frame( &this->apu, 1 ); // necessary to get click out of the way */
	
	memset( this->ram, 0, 0x4000 );
	memset( this->ram + 0x4000, 0xFF, 0x1F80 );
	memset( this->ram + 0x5F80, 0, sizeof this->ram - 0x5F80 );
	this->ram [hi_page] = 0; // joypad reads back as 0
	this->ram [idle_addr - ram_addr] = 0xED; // illegal instruction
	this->ram [hi_page + 6] = this->header.timer_modulo;
	this->ram [hi_page + 7] = this->header.timer_mode;
	
	Cpu_reset( &this->cpu, this->rom.unmapped );
	Cpu_map_code( &this->cpu, ram_addr, 0x10000 - ram_addr, this->ram );
	Cpu_map_code( &this->cpu, 0, this->rom.bank_size, Rom_at_addr( &this->rom, 0 ) );
	set_bank( this, this->rom.size > this->rom.bank_size );
	
	update_timer( this );
	this->next_play = this->play_period;
	this->cpu.r.rp.fa  = track;
	this->cpu.r.sp = get_le16( this->header.stack_ptr );
	this->cpu_time  = 0;
	jsr_then_stop( this, this->header.init_addr );
	
	// convert filter times to samples
	struct setup_t s = this->tfilter;
	s.max_initial *= this->sample_rate_ * stereo;
	#ifdef GME_DISABLE_SILENCE_LOOKAHEAD
		s.lookahead = 1;
	#endif
	track_setup( &this->track_filter, &s );
	
	return track_start( &this->track_filter );
}


// Track

static int msec_to_samples( int msec, int sample_rate )
{
	int sec = msec / 1000;
	msec -= sec * 1000;
	return (sec * sample_rate + msec * sample_rate / 1000) * stereo;
}

int Track_tell( struct Gbs_Emu* this ) 
{
	int rate = this->sample_rate_ * stereo;
	int sec = track_sample_count( &this->track_filter ) / rate;
	return sec * 1000 + (track_sample_count( &this->track_filter ) - sec * rate) * 1000 / rate;
}

blargg_err_t Track_seek( struct Gbs_Emu* this, int msec )
{
	int time = msec_to_samples( msec, this->sample_rate_ );
	if ( time < track_sample_count( &this->track_filter ) )
	RETURN_ERR( Gbs_start_track( this, this->current_track_ ) );
	return Track_skip( this, time - track_sample_count( &this->track_filter ) );
}

blargg_err_t skip_( void* emu, int count )
{
	struct Gbs_Emu* this = (struct Gbs_Emu*) emu;
	
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

blargg_err_t Track_skip( struct Gbs_Emu* this, int count )
{
	require( this->current_track_ >= 0 ); // start_track() must have been called already
	return track_skip( &this->track_filter, count );
}

void Track_set_fade( struct Gbs_Emu* this, int start_msec, int length_msec )
{
	track_set_fade( &this->track_filter, msec_to_samples( start_msec, this->sample_rate_ ),
		length_msec * this->sample_rate_ / (1000 / stereo) );
}

blargg_err_t Gbs_play( struct Gbs_Emu* this, int out_count, sample_t* out )
{
	require( this->current_track_ >= 0 );
	require( out_count % stereo == 0 );
	return track_play( &this->track_filter, out_count, out );
}
