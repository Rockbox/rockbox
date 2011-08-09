// Game_Music_Emu 0.5.2. http://www.slack.net/~ant/

#include "gbs_emu.h"

#include "blargg_endian.h"
#include "blargg_source.h"

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


const char gme_wrong_file_type [] ICONST_ATTR = "Wrong file type for this emulator";

int const idle_addr = 0xF00D;
int const tempo_unit = 16;

int const stereo = 2; // number of channels for stereo
int const silence_max = 6; // seconds
int const silence_threshold = 0x10;
long const fade_block_size = 512;
int const fade_shift = 8; // fade ends with gain at 1.0 / (1 << fade_shift)

void clear_track_vars( struct Gbs_Emu* this )
{
	this->current_track_   = -1;
	this->out_time         = 0;
	this->emu_time         = 0;
	this->emu_track_ended_ = true;
	this->track_ended      = true;
	this->fade_start       = (blargg_long)(LONG_MAX / 2 + 1);
	this->fade_step        = 1;
	this->silence_time     = 0;
	this->silence_count    = 0;
	this->buf_remain       = 0;
}

void Gbs_init( struct Gbs_Emu* this )
{	
	this->sample_rate_ = 0;
	this->mute_mask_   = 0;
	this->tempo_       = (int)(FP_ONE_TEMPO);
	
	// Unload
	this->header.timer_mode = 0;
	clear_track_vars( this );

	this->ignore_silence     = false;
	this->silence_lookahead = 6;
	this->max_initial_silence = 21;
	Sound_set_gain( this, 1.2 );
	
	Rom_init( &this->rom, 0x4000 );
	
	Apu_init( &this->apu );
	Cpu_init( &this->cpu );
	
	this->tempo = tempo_unit;
	this->sound_hardware = sound_gbs;
	
	// Reduce apu sound clicks?
	Apu_reduce_clicks( &this->apu, true );
}

static blargg_err_t check_gbs_header( void const* header )
{
	if ( memcmp( header, "GBS", 3 ) )
		return gme_wrong_file_type;
	return 0;
}

// Setup

blargg_err_t Gbs_load( struct Gbs_Emu* this, void* data, long size )
{
	// Unload
	this->header.timer_mode = 0;
	this->voice_count_ = 0;
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
	Apu_volume( &this->apu, this->gain_ );

	// Change clock rate & setup buffer
	this->clock_rate_ = 4194304;
	Buffer_clock_rate( &this->stereo_buf, 4194304 );
	this->buf_changed_count = Buffer_channels_changed_count( &this->stereo_buf );

	// Post load
	Sound_set_tempo( this, this->tempo_ );

	// Remute voices
	Sound_mute_voices( this, this->mute_mask_ );

	// Reset track count
	this->track_count = this->header.track_count;
	return 0;
}

// Emulation

// see gb_cpu_io.h for read/write functions

void Set_bank( struct Gbs_Emu* this, int n )
{
	addr_t addr = mask_addr( n * this->rom.bank_size, this->rom.mask );
	if ( addr == 0 && this->rom.size > this->rom.bank_size )
		addr = this->rom.bank_size; // MBC1&2 behavior, bank 0 acts like bank 1
	Cpu_map_code( &this->cpu, this->rom.bank_size, this->rom.bank_size, Rom_at_addr( &this->rom, addr ) );
}

void Update_timer( struct Gbs_Emu* this )
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
void Jsr_then_stop( struct Gbs_Emu* this, byte const addr [] )
{
	check( this->cpu.r.sp == get_le16( this->header.stack_ptr ) );
	this->cpu.r.pc = get_le16( addr );
	Write_mem( this, --this->cpu.r.sp, idle_addr >> 8 );
	Write_mem( this, --this->cpu.r.sp, idle_addr      );
}

blargg_err_t Run_until( struct Gbs_Emu* this, int end )
{
	this->end_time = end;
	Cpu_set_time( &this->cpu, Cpu_time( &this->cpu ) - end );
	while ( true )
	{
		Run_cpu( this );
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
			Jsr_then_stop( this, this->header.play_addr );
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

blargg_err_t End_frame( struct Gbs_Emu* this, int end )
{
	RETURN_ERR( Run_until( this, end ) );
	
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

blargg_err_t Run_clocks( struct Gbs_Emu* this, blip_time_t duration )
{
	return End_frame( this, duration );
}

blargg_err_t play_( struct Gbs_Emu* this, long count, sample_t* out )
{
	long remain = count;
	while ( remain )
	{
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
			blip_time_t clocks_emulated = (blargg_long) msec * this->clock_rate_ / 1000;
			RETURN_ERR( Run_clocks( this, clocks_emulated ) );
			assert( clocks_emulated );
			Buffer_end_frame( &this->stereo_buf, clocks_emulated );
		}
	}
	return 0;
}

blargg_err_t Gbs_set_sample_rate( struct Gbs_Emu* this, long rate )
{
	require( !this->sample_rate_ ); // sample rate can't be changed once set
	Buffer_init( &this->stereo_buf );
	RETURN_ERR( Buffer_set_sample_rate( &this->stereo_buf, rate, 1000 / 20 ) );
	
	// Set bass frequency
	Buffer_bass_freq( &this->stereo_buf, 300 );
	
	this->sample_rate_ = rate;
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
			struct channel_t ch = Buffer_channel( &this->stereo_buf );
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
	Update_timer( this );
}

void fill_buf( struct Gbs_Emu* this );
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
	static byte const sound_data [] ICONST_ATTR = {
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
	Set_bank( this, this->rom.size > this->rom.bank_size );
	
	Update_timer( this );
	this->next_play = this->play_period;
	this->cpu.r.rp.fa  = track;
	this->cpu.r.sp = get_le16( this->header.stack_ptr );
	this->cpu_time  = 0;
	Jsr_then_stop( this, this->header.init_addr );
	
	this->emu_track_ended_ = false;
	this->track_ended     = false;
	
	if ( !this->ignore_silence )
	{
		// play until non-silence or end of track
		long end;
		for ( end = this->max_initial_silence * stereo * this->sample_rate_; this->emu_time < end; )
		{
			fill_buf( this );
			if ( this->buf_remain | (int) this->emu_track_ended_ )
				break;
		}
		
		this->emu_time      = this->buf_remain;
		this->out_time      = 0;
		this->silence_time  = 0;
		this->silence_count = 0;
	}
	/* return track_ended() ? warning() : 0; */
	return 0;
}


// Track

blargg_long msec_to_samples( blargg_long msec, long sample_rate )
{
	blargg_long sec = msec / 1000;
	msec -= sec * 1000;
	return (sec * sample_rate + msec * sample_rate / 1000) * stereo;
}

long Track_tell( struct Gbs_Emu* this ) 
{
	blargg_long rate = this->sample_rate_ * stereo;
	blargg_long sec = this->out_time / rate;
	return sec * 1000 + (this->out_time - sec * rate) * 1000 / rate;
}

blargg_err_t Track_seek( struct Gbs_Emu* this, long msec )
{
	blargg_long time = msec_to_samples( msec, this->sample_rate_ );
	if ( time < this->out_time )
		RETURN_ERR( Gbs_start_track( this, this->current_track_ ) );
	return Track_skip( this, time - this->out_time );
}

blargg_err_t skip_( struct Gbs_Emu* this, long count )
{
	// for long skip, mute sound
	const long threshold = 30000;
	if ( count > threshold )
	{
		int saved_mute = this->mute_mask_;
		Sound_mute_voices( this, ~0 );
		
		while ( count > threshold / 2 && !this->emu_track_ended_ )
		{
			RETURN_ERR( play_( this, buf_size, this->buf ) );
			count -= buf_size;
		}
		
		Sound_mute_voices( this, saved_mute );
	}
	
	while ( count && !this->emu_track_ended_ )
	{
		long n = buf_size;
		if ( n > count )
			n = count;
		count -= n;
		RETURN_ERR( play_( this, n, this->buf ) );
	}
	return 0;
}

blargg_err_t Track_skip( struct Gbs_Emu* this, long count )
{
	require( this->current_track_ >= 0 ); // start_track() must have been called already
	this->out_time += count;
	
	// remove from silence and buf first
	{
		long n = min( count, this->silence_count );
		this->silence_count -= n;
		count -= n;
		
		n = min( count, this->buf_remain );
		this->buf_remain -= n;
		count -= n;
	}
		
	if ( count && !this->emu_track_ended_ )
	{
		this->emu_time += count;
		// End track if error
		if ( skip_( this, count ) )
			this->emu_track_ended_ = true;
	}
	
	if ( !(this->silence_count | this->buf_remain) ) // caught up to emulator, so update track ended
		this->track_ended |= this->emu_track_ended_;
	
	return 0;
}

// Fading

void Track_set_fade( struct Gbs_Emu* this, long start_msec, long length_msec )
{
	this->fade_step = this->sample_rate_ * length_msec / (fade_block_size * fade_shift * 1000 / stereo);
	this->fade_start = msec_to_samples( start_msec, this->sample_rate_ );
}

// unit / pow( 2.0, (double) x / step )
static int int_log( blargg_long x, int step, int unit )
{
	int shift = x / step;
	int fraction = (x - shift * step) * unit / step;
	return ((unit - fraction) + (fraction >> 1)) >> shift;
}

void handle_fade( struct Gbs_Emu* this, long out_count, sample_t* out )
{
	int i;
	for ( i = 0; i < out_count; i += fade_block_size )
	{
		int const shift = 14;
		int const unit = 1 << shift;
		int gain = int_log( (this->out_time + i - this->fade_start) / fade_block_size,
				this->fade_step, unit );
		if ( gain < (unit >> fade_shift) )
			this->track_ended = this->emu_track_ended_ = true;
		
		sample_t* io = &out [i];
		int count;
		for ( count = min( fade_block_size, out_count - i ); count; --count )
		{
			*io = (sample_t) ((*io * gain) >> shift);
			++io;
		}
	}
}

// Silence detection

void emu_play( struct Gbs_Emu* this, long count, sample_t* out )
{
	check( current_track_ >= 0 );
	this->emu_time += count;
	if ( this->current_track_ >= 0 && !this->emu_track_ended_ ) {
		// End track if error
		if ( play_( this, count, out ) ) this->emu_track_ended_ = true; 
	}
	else
		memset( out, 0, count * sizeof *out );
}

// number of consecutive silent samples at end
static long count_silence( sample_t* begin, long size )
{
	sample_t first = *begin;
	*begin = silence_threshold; // sentinel
	sample_t* p = begin + size;
	while ( (unsigned) (*--p + silence_threshold / 2) <= (unsigned) silence_threshold ) { }
	*begin = first;
	return size - (p - begin);
}

// fill internal buffer and check it for silence
void fill_buf( struct Gbs_Emu* this )
{
	assert( !this->buf_remain );
	if ( !this->emu_track_ended_ )
	{
		emu_play( this, buf_size, this->buf );
		long silence = count_silence( this->buf, buf_size );
		if ( silence < buf_size )
		{
			this->silence_time = this->emu_time - silence;
			this->buf_remain   = buf_size;
			return;
		}
	}
	this->silence_count += buf_size;
}

blargg_err_t Gbs_play( struct Gbs_Emu* this, long out_count, sample_t* out )
{
	if ( this->track_ended )
	{
		memset( out, 0, out_count * sizeof *out );
	}
	else
	{
		require( this->current_track_ >= 0 );
		require( out_count % stereo == 0 );
		
		assert( this->emu_time >= this->out_time );
		
		long pos = 0;
		if ( this->silence_count )
		{
			// during a run of silence, run emulator at >=2x speed so it gets ahead
			long ahead_time = this->silence_lookahead * (this->out_time + out_count - this->silence_time) + this->silence_time;
			while ( this->emu_time < ahead_time && !(this->buf_remain | this->emu_track_ended_) )
				fill_buf( this );
			
			// fill with silence
			pos = min( this->silence_count, out_count );
			memset( out, 0, pos * sizeof *out );
			this->silence_count -= pos;
			
			if ( this->emu_time - this->silence_time > silence_max * stereo * this->sample_rate_ )
			{
				this->track_ended  = this->emu_track_ended_ = true;
				this->silence_count = 0;
				this->buf_remain    = 0;
			}
		}
		
		if ( this->buf_remain )
		{
			// empty silence buf
			long n = min( this->buf_remain, out_count - pos );
			memcpy( &out [pos], this->buf + (buf_size - this->buf_remain), n * sizeof *out );
			this->buf_remain -= n;
			pos += n;
		}
		
		// generate remaining samples normally
		long remain = out_count - pos;
		if ( remain )
		{
			emu_play( this, remain, out + pos );
			this->track_ended |= this->emu_track_ended_;
			
			if ( !this->ignore_silence || this->out_time > this->fade_start )
			{
				// check end for a new run of silence
				long silence = count_silence( out + pos, remain );
				if ( silence < remain )
					this->silence_time = this->emu_time - silence;
				
				if ( this->emu_time - this->silence_time >= buf_size )
					fill_buf( this ); // cause silence detection on next play()
			}
		}
		
		if ( this->out_time > this->fade_start )
			handle_fade( this, out_count, out );
	}
	this->out_time += out_count;
	return 0;
}
