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

const char* const gme_wrong_file_type = "Wrong file type for this emulator";

// TODO: probably don't need detailed errors as to why file is corrupt

int const spectrum_clock = 3546900; // 128K Spectrum
int const spectrum_period = 70908;

//int const spectrum_clock = 3500000; // 48K Spectrum
//int const spectrum_period = 69888;

int const cpc_clock = 2000000;

static void clear_track_vars( struct Ay_Emu *this )
{
	this->current_track    = -1;
	track_stop( &this->track_filter );
}

void Ay_init( struct Ay_Emu *this )
{
	this->sample_rate  = 0;
	this->mute_mask_   = 0;
	this->tempo        = (int)FP_ONE_TEMPO;
	this->gain         = (int)FP_ONE_GAIN;
	this->track_count  = 0;
	
	// defaults
	this->tfilter = *track_get_setup( &this->track_filter );
	this->tfilter.max_initial = 2;
	this->tfilter.lookahead   = 6;
	this->track_filter.silence_ignored_ = false;
	
	this->beeper_output = NULL;
	disable_beeper( this );
	
	Ay_apu_init( &this->apu );
	Z80_init( &this->cpu );
	
	// clears fields
	this->voice_count = 0;
	this->voice_types = 0;
	clear_track_vars( this );
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

// Setup

static void change_clock_rate( struct Ay_Emu *this, int rate )
{
	this->clock_rate_ = rate;
	Buffer_clock_rate( &this->stereo_buf, rate );
}

blargg_err_t Ay_load_mem( struct Ay_Emu *this, byte const in [], long size )
{
	// Unload
	this->voice_count = 0;
	this->track_count = 0;
	this->m3u.size = 0;
	clear_track_vars( this );

	assert( offsetof (struct header_t,track_info [2]) == header_size );
	
	RETURN_ERR( parse_header( in, size, &this->file ) );
	
	/* if ( file.header->vers > 2 )
		warning( "Unknown file version" ); */
	
	this->voice_count = ay_osc_count + 1; // +1 for beeper
	static int const types [ay_osc_count + 1] = {
		wave_type+0, wave_type+1, wave_type+2, mixed_type+1
	};
	this->voice_types = types;
	
	Ay_apu_volume( &this->apu, this->gain);
	
	// Setup buffer
	change_clock_rate( this, spectrum_clock );
	RETURN_ERR( Buffer_set_channel_count( &this->stereo_buf, this->voice_count, this->voice_types ) );
	this->buf_changed_count = Buffer_channels_changed_count( &this->stereo_buf );
	
	Sound_set_tempo( this, this->tempo );
	Sound_mute_voices( this, this->mute_mask_ );
	
	this->track_count = this->file.header->max_track + 1;
	return 0;
}

static void set_beeper_output( struct Ay_Emu *this, struct Blip_Buffer* b )
{
	this->beeper_output = b;
	if ( b && !this->cpc_mode )
		this->beeper_mask = 0x10;
	else
		disable_beeper( this );
}

static void set_voice( struct Ay_Emu *this, int i, struct Blip_Buffer* center )
{
	if ( i >= ay_osc_count )
		set_beeper_output( this, center );
	else
		Ay_apu_set_output( &this->apu, i, center );
}

static blargg_err_t run_clocks( struct Ay_Emu *this, blip_time_t* duration, int msec )
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

blargg_err_t Ay_set_sample_rate( struct Ay_Emu *this, int rate )
{
	require( !this->sample_rate ); // sample rate can't be changed once set
	Buffer_init( &this->stereo_buf );
	RETURN_ERR( Buffer_set_sample_rate( &this->stereo_buf, rate, 1000 / 20 ) );
	
	// Set buffer bass
	Buffer_bass_freq( &this->stereo_buf, 160 );
	
	this->sample_rate = rate;
	RETURN_ERR( track_init( &this->track_filter, this ) );
	this->tfilter.max_silence = 6 * stereo * this->sample_rate;
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
			struct channel_t ch = Buffer_channel( &this->stereo_buf, i );
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

int Track_tell( struct Ay_Emu *this )
{
	int rate = this->sample_rate * stereo;
	int sec = track_sample_count( &this->track_filter ) / rate;
	return sec * 1000 + (track_sample_count( &this->track_filter ) - sec * rate) * 1000 / rate;
}

blargg_err_t Track_seek( struct Ay_Emu *this, int msec )
{
	int time = msec_to_samples( msec, this->sample_rate );
	if ( time < track_sample_count( &this->track_filter ) )
	RETURN_ERR( Ay_start_track( this, this->current_track ) );
	return Track_skip( this, time - track_sample_count( &this->track_filter ) );
}

blargg_err_t skip_( void *emu, int count )
{
	struct Ay_Emu* this = (struct Ay_Emu*) emu;
	
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

blargg_err_t Track_skip( struct Ay_Emu *this, int count )
{
	require( this->current_track >= 0 ); // start_track() must have been called already
	return track_skip( &this->track_filter, count );
}

int Track_get_length( struct Ay_Emu* this, int n )
{
	int length = 0;

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

void Track_set_fade( struct Ay_Emu *this, int start_msec, int length_msec )
{
	track_set_fade( &this->track_filter, msec_to_samples( start_msec, this->sample_rate ),
		length_msec * this->sample_rate / (1000 / stereo) );
}

blargg_err_t Ay_play( struct Ay_Emu *this, int out_count, sample_t* out )
{
	require( this->current_track >= 0 );
	require( out_count % stereo == 0 );
	return track_play( &this->track_filter, out_count, out );
}

blargg_err_t play_( void *emu, int count, sample_t* out )
{
	struct Ay_Emu* this = (struct Ay_Emu*) emu;
	
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
