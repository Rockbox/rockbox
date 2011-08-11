// Game_Music_Emu 0.6-pre. http://www.slack.net/~ant/

#include "ay_emu.h"

#include "blargg_endian.h"

/* Copyright (C) 2006-2009 Shay Green. This module is free software; you
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

int const stereo = 2; // number of channels for stereo
int const silence_max = 6; // seconds
int const silence_threshold = 0x10;
long const fade_block_size = 512;
int const fade_shift = 8; // fade ends with gain at 1.0 / (1 << fade_shift)

const char* const gme_wrong_file_type = "Wrong file type for this emulator";

// TODO: probably don't need detailed errors as to why file is corrupt

int const spectrum_clock = 3546900; // 128K Spectrum
int const spectrum_period = 70908;

//int const spectrum_clock = 3500000; // 48K Spectrum
//int const spectrum_period = 69888;

int const cpc_clock = 2000000;

void clear_track_vars( struct Ay_Emu *this )
{
	this->current_track    = -1;
	this->out_time         = 0;
	this->emu_time         = 0;
	this->emu_track_ended_ = true;
	this->track_ended      = true;
	this->fade_start       = INT_MAX / 2 + 1;
	this->fade_step        = 1;
	this->silence_time     = 0;
	this->silence_count    = 0;
	this->buf_remain       = 0;
	/* warning(); // clear warning */
}

void Ay_init( struct Ay_Emu *this )
{
	this->sample_rate  = 0;
	this->mute_mask_   = 0;
	this->tempo        = (int)FP_ONE_TEMPO;
	this->gain         = (int)FP_ONE_GAIN;
	this->track_count  = 0;
	
	// defaults
	this->max_initial_silence = 2;
	this->ignore_silence      = false;
	
	this->voice_count = 0;
	clear_track_vars( this );
	this->beeper_output = NULL;
	disable_beeper( this );
	
	Ay_apu_init( &this->apu );
	Z80_init( &this->cpu );

	this->silence_lookahead = 6 ;
}

// Track info

// Given pointer to 2-byte offset of data, returns pointer to data, or NULL if
// offset is 0 or there is less than min_size bytes of data available.
static byte const* get_data( struct file_t const* file, byte const ptr [], int min_size )
{
	int offset = (int16_t) get_be16( ptr );
	int pos  = ptr - (byte const*) file->header;
	int size = file->end - (byte const*) file->header;
	assert( (unsigned) pos <= (unsigned) size - 2 );
	int limit = size - min_size;
	if ( limit < 0 || !offset || (unsigned) (pos + offset) > (unsigned) limit )
		return NULL;
	return ptr + offset;
}

static blargg_err_t parse_header( byte const in [], int size, struct file_t* out )
{
	if ( size < header_size )
		return gme_wrong_file_type;
	
	out->header = (struct header_t const*) in;
	out->end    = in + size;
	struct header_t const* h = (struct header_t const*) in;
	if ( memcmp( h->tag, "ZXAYEMUL", 8 ) )
		return gme_wrong_file_type;
	
	out->tracks = get_data( out, h->track_info, (h->max_track + 1) * 4 );
	if ( !out->tracks )
		return "missing track data";

	return 0;
}

long Track_get_length( struct Ay_Emu* this, int n )
{
	long length = 0;

	byte const* track_info = get_data( &this->file, this->file.tracks + n * 4 + 2, 6 );
	if ( track_info )
		length = get_be16( track_info + 4 ) * (1000 / 50); // frames to msec 

	if ( (this->m3u.size > 0) && (n < this->m3u.size) ) {
		struct entry_t* entry = &this->m3u.entries [n];
		length = entry->length;
	} 
	
	if ( length <= 0 )
		length = 120 * 1000;  /* 2 minutes */ 

	return length;
}

// Setup

void change_clock_rate( struct Ay_Emu *this, long rate )
{
	this->clock_rate_ = rate;
	Buffer_clock_rate( &this->stereo_buf, rate );
}

blargg_err_t Ay_load_mem( struct Ay_Emu *this, byte const in [], int size )
{
	assert( offsetof (struct header_t,track_info [2]) == header_size );
	
	RETURN_ERR( parse_header( in, size, &this->file ) );
	
	/* if ( file.header->vers > 2 )
		warning( "Unknown file version" ); */
	
	this->voice_count = ay_osc_count + 1; // +1 for beeper
	Ay_apu_volume( &this->apu, this->gain);
	
	// Setup buffer
	change_clock_rate( this, spectrum_clock );
	this->buf_changed_count = Buffer_channels_changed_count( &this->stereo_buf );
	
	Sound_set_tempo( this, this->tempo );
	
	// Remute voices
	Sound_mute_voices( this, this->mute_mask_ );
	
	this->track_count = this->file.header->max_track + 1;
	this->m3u.size = 0;
	return 0;
}

void set_beeper_output( struct Ay_Emu *this, struct Blip_Buffer* b )
{
	this->beeper_output = b;
	if ( b && !this->cpc_mode )
		this->beeper_mask = 0x10;
	else
		disable_beeper( this );
}

void set_voice( struct Ay_Emu *this, int i, struct Blip_Buffer* center )
{
	if ( i >= ay_osc_count )
		set_beeper_output( this, center );
	else
		Ay_apu_set_output( &this->apu, i, center );
}

blargg_err_t run_clocks( struct Ay_Emu *this, blip_time_t* duration, int msec )
{
#if defined(ROCKBOX)
	(void) msec;
#endif

	cpu_time_t *end = duration;	
	struct Z80_Cpu* cpu = &this->cpu;
	Z80_set_time( cpu, 0 );
	
	// Since detection of CPC mode will halve clock rate during the frame
	// and thus generate up to twice as much sound, we must generate half
	// as much until mode is known.
	if ( !(this->spectrum_mode | this->cpc_mode) )
		*end /= 2;
	
	while ( Z80_time( cpu ) < *end )
	{
		run_cpu( this, min( *end, this->next_play ) );
		
		if ( Z80_time( cpu ) >= this->next_play )
		{
			// next frame
			this->next_play += this->play_period;
			
			if ( cpu->r.iff1 )
			{
				// interrupt enabled
				
				if ( this->mem.ram [cpu->r.pc] == 0x76 )
					cpu->r.pc++; // advance past HALT instruction
				
				cpu->r.iff1 = 0;
				cpu->r.iff2 = 0;
				
				this->mem.ram [--cpu->r.sp] = (byte) (cpu->r.pc >> 8);
				this->mem.ram [--cpu->r.sp] = (byte) (cpu->r.pc);
				
				// fixed interrupt
				cpu->r.pc = 0x38;
				Z80_adjust_time( cpu, 12 );
				
				if ( cpu->r.im == 2 )
				{
					// vectored interrupt
					addr_t addr = cpu->r.i * 0x100 + 0xFF;
					cpu->r.pc = this->mem.ram [(addr + 1) & 0xFFFF] * 0x100 + this->mem.ram [addr];
					Z80_adjust_time( cpu, 6 );
				}
			}
		}
	}
	
	// End time frame
	*end = Z80_time( cpu );
	this->next_play -= *end;
	check( this->next_play >= 0 );
	Z80_adjust_time( cpu, -*end );
	Ay_apu_end_frame( &this->apu, *end );
	return 0;
}

// Emulation

void cpu_out_( struct Ay_Emu *this, cpu_time_t time, addr_t addr, int data )
{
	// Spectrum
	if ( !this->cpc_mode )
	{
		switch ( addr & 0xFEFF )
		{
		case 0xFEFD:
			this->spectrum_mode = true;
			Ay_apu_write_addr( &this->apu, data );
			return;
		
		case 0xBEFD:
			this->spectrum_mode = true;
			Ay_apu_write_data( &this->apu, time, data );
			return;
		}
	}
	
	// CPC
	if ( !this->spectrum_mode )
	{
		switch ( addr >> 8 )
		{
		case 0xF6:
			switch ( data & 0xC0 )
			{
			case 0xC0:
				Ay_apu_write_addr( &this->apu, this->cpc_latch );
				goto enable_cpc;
			
			case 0x80:
				Ay_apu_write_data( &this->apu, time, this->cpc_latch );
				goto enable_cpc;
			}
			break;
		
		case 0xF4:
			this->cpc_latch = data;
			goto enable_cpc;
		}
	}
	
	/* dprintf( "Unmapped OUT: $%04X <- $%02X\n", addr, data ); */
	return;
	
enable_cpc:
	if ( !this->cpc_mode )
	{
		this->cpc_mode = true;
		disable_beeper( this );
	
		change_clock_rate( this, cpc_clock );
		Sound_set_tempo( this, this->tempo );
	}
}

blargg_err_t Ay_set_sample_rate( struct Ay_Emu *this, long rate )
{
	require( !this->sample_rate ); // sample rate can't be changed once set
	Buffer_init( &this->stereo_buf );
	RETURN_ERR( Buffer_set_sample_rate( &this->stereo_buf, rate, 1000 / 20 ) );
	
	// Set buffer bass
	Buffer_bass_freq( &this->stereo_buf, 160 );
	
	this->sample_rate = rate;
	return 0;
}

void Sound_mute_voice( struct Ay_Emu *this, int index, bool mute )
{
	require( (unsigned) index < (unsigned) this->voice_count );
	int bit = 1 << index;
	int mask = this->mute_mask_ | bit;
	if ( !mute )
		mask ^= bit;
	Sound_mute_voices( this, mask );
}

void Sound_mute_voices( struct Ay_Emu *this, int mask )
{
	require( this->sample_rate ); // sample rate must be set first
	this->mute_mask_ = mask;
	
	int i;
	for ( i = this->voice_count; i--; )
	{
		if ( mask & (1 << i) )
		{
			set_voice( this, i, 0 );
		}
		else
		{
			struct channel_t ch = Buffer_channel( &this->stereo_buf );
			assert( (ch.center && ch.left && ch.right) ||
					(!ch.center && !ch.left && !ch.right) ); // all or nothing
			set_voice( this, i, ch.center );
		}
	}
}

void Sound_set_tempo( struct Ay_Emu *this, int t )
{
	require( this->sample_rate ); // sample rate must be set first
	int const min = (int)(FP_ONE_TEMPO*0.02);
	int const max = (int)(FP_ONE_TEMPO*4.00);
	if ( t < min ) t = min;
	if ( t > max ) t = max;
	this->tempo = t;
	
	int p = spectrum_period;
	if ( this->clock_rate_ != spectrum_clock )
		p = this->clock_rate_ / 50;
	
	this->play_period = (blip_time_t) ((p * FP_ONE_TEMPO) / t);
}

void fill_buf( struct Ay_Emu *this ) ICODE_ATTR;;
blargg_err_t Ay_start_track( struct Ay_Emu *this, int track )
{
	clear_track_vars( this );
	
	// Remap track if playlist available
	if ( this->m3u.size > 0 ) {
		struct entry_t* e = &this->m3u.entries[track];
		track = e->track;
	}
	
	this->current_track = track;
	Buffer_clear( &this->stereo_buf );
	
	byte* const mem = this->mem.ram;
	
	memset( mem + 0x0000, 0xC9, 0x100 ); // fill RST vectors with RET
	memset( mem + 0x0100, 0xFF, 0x4000 - 0x100 );
	memset( mem + ram_addr, 0x00, mem_size - ram_addr );

	// locate data blocks
	byte const* const data = get_data( &this->file, this->file.tracks + track * 4 + 2, 14 );
	if ( !data )
		return "file data missing";
	
	byte const* const more_data = get_data( &this->file, data + 10, 6 );
	if ( !more_data )
		return "file data missing";
	
	byte const* blocks = get_data( &this->file, data + 12, 8 );
	if ( !blocks )
		return "file data missing";
	
	// initial addresses
	unsigned addr = get_be16( blocks );
	if ( !addr )
		return "file data missing";
	
	unsigned init = get_be16( more_data + 2 );
	if ( !init )
		init = addr;
	
	// copy blocks into memory
	do
	{
		blocks += 2;
		unsigned len = get_be16( blocks ); blocks += 2;
		if ( addr + len > mem_size )
		{
			/* warning( "Bad data block size" ); */
			len = mem_size - addr;
		}
		check( len );
		byte const* in = get_data( &this->file, blocks, 0 ); blocks += 2;
		if ( len > (unsigned) (this->file.end - in) )
		{
			/* warning( "File data missing" ); */
			len = this->file.end - in;
		}

		memcpy( mem + addr, in, len );
		
		if ( this->file.end - blocks < 8 )
		{
			/* warning( "File data missing" ); */
			break;
		}
	}
	while ( (addr = get_be16( blocks )) != 0 );
	
	// copy and configure driver
	static byte const passive [] = {
		0xF3,       // DI
		0xCD, 0, 0, // CALL init
		0xED, 0x5E, // LOOP: IM 2
		0xFB,       // EI
		0x76,       // HALT
		0x18, 0xFA  // JR LOOP
	};
	static byte const active [] = {
		0xF3,       // DI
		0xCD, 0, 0, // CALL init
		0xED, 0x56, // LOOP: IM 1
		0xFB,       // EI
		0x76,       // HALT
		0xCD, 0, 0, // CALL play
		0x18, 0xF7  // JR LOOP
	};
	memcpy( mem, passive, sizeof passive );
	int const play_addr = get_be16( more_data + 4 );
	if ( play_addr )
	{
		memcpy( mem, active, sizeof active );
		mem [ 9] = play_addr;
		mem [10] = play_addr >> 8;
	}
	mem [2] = init;
	mem [3] = init >> 8;
	
	mem [0x38] = 0xFB; // Put EI at interrupt vector (followed by RET)
	
	// start at spectrum speed
	change_clock_rate( this, spectrum_clock );
	Sound_set_tempo( this, this->tempo );
	
	struct registers_t r;
	memset( &r, 0, sizeof(struct registers_t) );
	
	r.sp = get_be16( more_data );
	r.b.a     = r.b.b = r.b.d = r.b.h = data [8];
	r.b.flags = r.b.c = r.b.e = r.b.l = data [9];
	r.alt.w = r.w;
	r.ix = r.iy = r.w.hl;
	
	memset( this->mem.padding1, 0xFF, sizeof this->mem.padding1 );
	
	int const mirrored = 0x80; // this much is mirrored after end of memory
	memset( this->mem.ram + mem_size + mirrored, 0xFF, sizeof this->mem.ram - mem_size - mirrored );
	memcpy( this->mem.ram + mem_size, this->mem.ram, mirrored ); // some code wraps around (ugh)
	
	Z80_reset( &this->cpu, this->mem.padding1, this->mem.padding1 );
	Z80_map_mem( &this->cpu, 0, mem_size, this->mem.ram, this->mem.ram );
	this->cpu.r = r;
	
	this->beeper_delta   = (int) ((ay_amp_range*4)/5);
	this->last_beeper    = 0;
	this->next_play      = this->play_period;
	this->spectrum_mode  = false;
	this->cpc_mode       = false;
	this->cpc_latch      = 0;
	set_beeper_output( this, this->beeper_output );
	Ay_apu_reset( &this->apu );
	
	// a few tunes rely on channels having tone enabled at the beginning
	Ay_apu_write_addr( &this->apu, 7 );
	Ay_apu_write_data( &this->apu, 0, 0x38 );
	
	this->emu_track_ended_ = false;
	this->track_ended      = false;
	
	if ( !this->ignore_silence )
	{
		// play until non-silence or end of track
		long end;
		for ( end = this->max_initial_silence * stereo * this->sample_rate; this->emu_time < end; )
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

// Tell/Seek

blargg_long msec_to_samples( blargg_long msec, long sample_rate )
{
	blargg_long sec = msec / 1000;
	msec -= sec * 1000;
	return (sec * sample_rate + msec * sample_rate / 1000) * stereo;
}

long Track_tell( struct Ay_Emu *this )
{
	blargg_long rate = this->sample_rate * stereo;
	blargg_long sec = this->out_time / rate;
	return sec * 1000 + (this->out_time - sec * rate) * 1000 / rate;
}

blargg_err_t Track_seek( struct Ay_Emu *this, long msec )
{
	blargg_long time = msec_to_samples( msec, this->sample_rate );
	if ( time < this->out_time )
		RETURN_ERR( Ay_start_track( this, this->current_track ) );
	return Track_skip( this, time - this->out_time );
}

blargg_err_t play_( struct Ay_Emu *this, long count, sample_t* out ) ICODE_ATTR;
blargg_err_t skip_( struct Ay_Emu *this, long count )
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

blargg_err_t Track_skip( struct Ay_Emu *this, long count )
{
	require( this->current_track >= 0 ); // start_track() must have been called already
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

void Track_set_fade( struct Ay_Emu *this, long start_msec, long length_msec )
{
	this->fade_step = this->sample_rate * length_msec / (fade_block_size * fade_shift * 1000 / stereo);
	this->fade_start = msec_to_samples( start_msec, this->sample_rate );
}

// unit / pow( 2.0, (double) x / step )
static int int_log( blargg_long x, int step, int unit )
{
	int shift = x / step;
	int fraction = (x - shift * step) * unit / step;
	return ((unit - fraction) + (fraction >> 1)) >> shift;
}

void handle_fade( struct Ay_Emu *this, long out_count, sample_t* out )
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

void emu_play( struct Ay_Emu *this, long count, sample_t* out )
{
	check( current_track_ >= 0 );
	this->emu_time += count;
	if ( this->current_track >= 0 && !this->emu_track_ended_ ) {
		if ( play_( this, count, out ) )
			this->emu_track_ended_ = true;
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
void fill_buf( struct Ay_Emu *this )
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

blargg_err_t Ay_play( struct Ay_Emu *this, long out_count, sample_t* out )
{
	if ( this->track_ended )
	{
		memset( out, 0, out_count * sizeof *out );
	}
	else
	{
		require( this->current_track >= 0 );
		require( out_count % stereo == 0 );
		
		assert( this->emu_time >= this->out_time );
		
		// prints nifty graph of how far ahead we are when searching for silence
		//debug_printf( "%*s \n", int ((emu_time - out_time) * 7 / sample_rate()), "*" );
		
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
			
			if ( this->emu_time - this->silence_time > silence_max * stereo * this->sample_rate )
			{
				this->track_ended   = this->emu_track_ended_ = true;
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

blargg_err_t play_( struct Ay_Emu *this, long count, sample_t* out )
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
			blip_time_t clocks_emulated = (blargg_long) msec * this->clock_rate_ / 1000 - 100;
			RETURN_ERR( run_clocks( this, &clocks_emulated, msec ) );
			assert( clocks_emulated );
			Buffer_end_frame( &this->stereo_buf, clocks_emulated );
		}
	}
	return 0;
}
