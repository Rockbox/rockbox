// Game_Music_Emu 0.5.2. http://www.slack.net/~ant/

#include "hes_emu.h"

#include "blargg_endian.h"
#include "blargg_source.h"

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

int const timer_mask  = 0x04;
int const vdp_mask    = 0x02;
int const i_flag_mask = 0x04;
int const unmapped    = 0xFF;

int const period_60hz = 262 * 455; // scanlines * clocks per scanline

const char gme_wrong_file_type [] = "Wrong file type for this emulator";

static void clear_track_vars( struct Hes_Emu* this )
{
	this->current_track_    = -1;
	track_stop( &this->track_filter );
}

void Hes_init( struct Hes_Emu* this )
{
	this->sample_rate_ = 0;
	this->mute_mask_   = 0;
	this->tempo_       = (int)(FP_ONE_TEMPO);

	// defaults
	this->tfilter = *track_get_setup( &this->track_filter );
	this->tfilter.max_initial = 2;
	this->tfilter.lookahead   = 6;
	this->track_filter.silence_ignored_ = false;

	this->timer.raw_load = 0;
	Sound_set_gain( this, (int)(FP_ONE_GAIN*1.11) );

	Rom_init( &this->rom, 0x2000 );

	Apu_init( &this->apu );
	Adpcm_init( &this->adpcm );
	Cpu_init( &this->cpu );

	/* Set default track count */
	this->track_count = 255;
	
	// clears fields
	this->voice_count_ = 0;
	this->voice_types_ = 0;
	clear_track_vars( this );
}

static blargg_err_t check_hes_header( void const* header )
{
	if ( memcmp( header, "HESM", 4 ) )
		return gme_wrong_file_type;
	return 0;
}

// Setup

blargg_err_t Hes_load_mem( struct Hes_Emu* this, void* data, long size )
{
	// Unload
	this->voice_count_ = 0;
	this->track_count = 255;
	this->m3u.size = 0;
	clear_track_vars( this );
             
	assert( offsetof (struct header_t,unused [4]) == header_size );
	RETURN_ERR( Rom_load( &this->rom, data, size, header_size, &this->header, unmapped ) );
	
	RETURN_ERR( check_hes_header( this->header.tag ) );
	
	/* if ( header_.vers != 0 )
		warning( "Unknown file version" );
	
	if ( memcmp( header_.data_tag, "DATA", 4 ) )
		warning( "Data header missing" );
	
	if ( memcmp( header_.unused, "\0\0\0\0", 4 ) )
		warning( "Unknown header data" ); */
	
	// File spec supports multiple blocks, but I haven't found any, and
	// many files have bad sizes in the only block, so it's simpler to
	// just try to load the damn data as best as possible.
	
	int addr = get_le32( this->header.addr );
	/* int rom_size = get_le32( this->header.size ); */
	int const rom_max = 0x100000;
	if ( (unsigned) addr >= (unsigned) rom_max )
	{
		/* warning( "Invalid address" ); */
		addr &= rom_max - 1;
	}
	/* if ( (unsigned) (addr + size) > (unsigned) rom_max )
		warning( "Invalid size" );
	
	if ( rom_size != rom.file_size() )
	{
		if ( size <= rom.file_size() - 4 && !memcmp( rom.begin() + size, "DATA", 4 ) )
			warning( "Multiple DATA not supported" );
		else if ( size < rom.file_size() )
			warning( "Extra file data" );
		else
			warning( "Missing file data" );
	} */
	
	Rom_set_addr( &this->rom, addr );
	
	this->voice_count_ = osc_count + adpcm_osc_count;
	static int const types [osc_count + adpcm_osc_count] = {
		wave_type+0, wave_type+1, wave_type+2, wave_type+3, mixed_type+0, mixed_type+1, mixed_type+2
	};
	this->voice_types_ = types;
	
	Apu_volume( &this->apu, this->gain_ );
	Adpcm_volume( &this->adpcm, this->gain_ );

    // Setup buffer	
	this->clock_rate_ = 7159091;
	Buffer_clock_rate( &this->stereo_buf, 7159091 );
	RETURN_ERR( Buffer_set_channel_count( &this->stereo_buf, this->voice_count_, this->voice_types_ ) );
	this->buf_changed_count = Buffer_channels_changed_count( &this->stereo_buf );
	
	Sound_set_tempo( this, this->tempo_ );
	Sound_mute_voices( this, this->mute_mask_ );
	
	return 0;
}

// Emulation

static void recalc_timer_load( struct Hes_Emu* this )
{
	this->timer.load = this->timer.raw_load * this->timer_base + 1;
}

// Hardware

void run_until( struct Hes_Emu* this, hes_time_t present )
{
	while ( this->vdp.next_vbl < present )
		this->vdp.next_vbl += this->play_period;
	
	hes_time_t elapsed = present - this->timer.last_time;
	if ( elapsed > 0 )
	{
		if ( this->timer.enabled )
		{
			this->timer.count -= elapsed;
			if ( this->timer.count <= 0 )
				this->timer.count += this->timer.load;
		}
		this->timer.last_time = present;
	}
}

void write_vdp( struct Hes_Emu* this, int addr, int data )
{
	switch ( addr )
	{
	case 0:
		this->vdp.latch = data & 0x1F;
		break;
	
	case 2:
		if ( this->vdp.latch == 5 )
		{
			/* if ( data & 0x04 )
				warning( "Scanline interrupt unsupported" ); */
			run_until( this, Cpu_time( &this->cpu ) );
			this->vdp.control = data;
			irq_changed( this );
		}
		/* else
		{
			dprintf( "VDP not supported: $%02X <- $%02X\n", vdp.latch, data );
		} */
		break;
	
	case 3:
		/* dprintf( "VDP MSB not supported: $%02X <- $%02X\n", vdp.latch, data ); */
		break;
	}
}

void write_mem_( struct Hes_Emu* this, hes_addr_t addr, int data )
{
	hes_time_t time = Cpu_time( &this->cpu );
	if ( (unsigned) (addr - apu_io_addr) < apu_io_size )
	{
		// Avoid going way past end when a long block xfer is writing to I/O space.
		// Not a problem for other registers below because they don't write to
		// Blip_Buffer.
		hes_time_t t = min( time, Cpu_end_time( &this->cpu ) + 8 );
		Apu_write_data( &this->apu, t, addr, data );
		return;
	}
	if ( (unsigned) (addr - adpcm_io_addr) < adpcm_io_size )
	{
		hes_time_t t = min( time, Cpu_end_time( &this->cpu ) + 6 );
		Adpcm_write_data( &this->adpcm, t, addr, data );
		return;
	}
	
	switch ( addr )
	{
	case 0x0000:
	case 0x0002:
	case 0x0003:
		write_vdp( this, addr, data );
		return;
	
	case 0x0C00: {
		run_until( this, time );
		this->timer.raw_load = (data & 0x7F) + 1;
		recalc_timer_load( this );
		this->timer.count = this->timer.load;
		break;
	}
	
	case 0x0C01:
		data &= 1;
		if ( this->timer.enabled == data )
			return;
		run_until( this, time );
		this->timer.enabled = data;
		if ( data )
			this->timer.count = this->timer.load;
		break;
	
	case 0x1402:
		run_until( this, time );
		this->irq.disables = data;
		/* if ( (data & 0xF8) && (data & 0xF8) != 0xF8 ) // flag questionable values
			dprintf( "Int mask: $%02X\n", data ); */
		break;
	
	case 0x1403:
		run_until( this, time );
		if ( this->timer.enabled )
			this->timer.count = this->timer.load;
		this->timer.fired = false;
		break;
	
#ifndef NDEBUG
	case 0x1000: // I/O port
	case 0x0402: // palette
	case 0x0403:
	case 0x0404:
	case 0x0405:
		return;
		
	default:
		/* dprintf( "unmapped write $%04X <- $%02X\n", addr, data ); */
		return;
#endif
	}
	
	irq_changed( this );
}

int read_mem_( struct Hes_Emu* this, hes_addr_t addr )
{
	hes_time_t time = Cpu_time( &this->cpu );
	addr &= page_size - 1;
	switch ( addr )
	{
	case 0x0000:
		if ( this->irq.vdp > time )
			return 0;
		this->irq.vdp = future_time;
		run_until( this, time );
		irq_changed( this );
		return 0x20;
		
	/* case 0x0002:
	case 0x0003:
		dprintf( "VDP read not supported: %d\n", addr );
		return 0; */
	
	case 0x0C01:
		//return timer.enabled; // TODO: remove?
	case 0x0C00:
		run_until( this, time );
		/* dprintf( "Timer count read\n" ); */
		return (unsigned) (this->timer.count - 1) / this->timer_base;
	
	case 0x1402:
		return this->irq.disables;
	
	case 0x1403:
		{
			int status = 0;
			if ( this->irq.timer <= time ) status |= timer_mask;
			if ( this->irq.vdp   <= time ) status |= vdp_mask;
			return status;
		}

	case 0x180A:
	case 0x180B:
	case 0x180C:
	case 0x180D:
		return Adpcm_read_data( &this->adpcm, time, addr );
		
	#ifndef NDEBUG
		case 0x1000: // I/O port
		//case 0x180C: // CD-ROM
		//case 0x180D:
			break;
		
		/* default:
			dprintf( "unmapped read  $%04X\n", addr ); */
	#endif
	}
	
	return unmapped;
}

void irq_changed( struct Hes_Emu* this )
{
	hes_time_t present = Cpu_time( &this->cpu );
	
	if ( this->irq.timer > present )
	{
		this->irq.timer = future_time;
		if ( this->timer.enabled && !this->timer.fired )
			this->irq.timer = present + this->timer.count;
	}
	
	if ( this->irq.vdp > present )
	{
		this->irq.vdp = future_time;
		if ( this->vdp.control & 0x08 )
			this->irq.vdp = this->vdp.next_vbl;
	}
	
	hes_time_t time = future_time;
	if ( !(this->irq.disables & timer_mask) ) time = this->irq.timer;
	if ( !(this->irq.disables &   vdp_mask) ) time = min( time, this->irq.vdp );
	
	Cpu_set_irq_time( &this->cpu, time );
}

int cpu_done( struct Hes_Emu* this )
{
	check( Cpu_time( &this->cpu ) >= Cpu_end_time( &this->cpu ) ||
			(!(this->cpu.r.flags & i_flag_mask) && Cpu_time( &this->cpu ) >= Cpu_irq_time( &this->cpu )) );
	
	if ( !(this->cpu.r.flags & i_flag_mask) )
	{
		hes_time_t present = Cpu_time( &this->cpu );
		
		if ( this->irq.timer <= present && !(this->irq.disables & timer_mask) )
		{
			this->timer.fired = true;
			this->irq.timer = future_time;
			irq_changed( this ); // overkill, but not worth writing custom code
			return 0x0A;
		}
		
		if ( this->irq.vdp <= present && !(this->irq.disables & vdp_mask) )
		{
			// work around for bugs with music not acknowledging VDP
			//run_until( present );
			//irq.vdp = cpu.future_time;
			//irq_changed();
			return 0x08;
		}
	}
	return -1;
}

static void adjust_time( hes_time_t* time, hes_time_t delta )
{
	if ( *time < future_time )
	{
		*time -= delta;
		if ( *time < 0 )
			*time = 0;
	}
}

static blargg_err_t end_frame( struct Hes_Emu* this, hes_time_t duration )
{
	/* if ( run_cpu( this, duration ) )
		warning( "Emulation error (illegal instruction)" ); */
	run_cpu( this, duration );
	
	check( Cpu_time( &this->cpu ) >= duration );
	//check( time() - duration < 20 ); // Txx instruction could cause going way over
	
	run_until( this, duration );
	
	// end time frame
	this->timer.last_time -= duration;
	this->vdp.next_vbl    -= duration;
	Cpu_end_frame( &this->cpu, duration );
	adjust_time( &this->irq.timer, duration );
	adjust_time( &this->irq.vdp,   duration );
	Apu_end_frame( &this->apu, duration );
	Adpcm_end_frame( &this->adpcm, duration );
	
	return 0;
}

static blargg_err_t run_clocks( struct Hes_Emu* this, blip_time_t* duration_ )
{
	return end_frame( this, *duration_ );
}

blargg_err_t play_( void *emu, int count, sample_t out [] )
{
	struct Hes_Emu* this = (struct Hes_Emu*) emu;
	
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
			RETURN_ERR( run_clocks( this, &clocks_emulated ) );
			assert( clocks_emulated );
			Buffer_end_frame( &this->stereo_buf, clocks_emulated );
		}
	}
	return 0;
}

// Music emu

blargg_err_t Hes_set_sample_rate( struct Hes_Emu* this, int rate )
{
	require( !this->sample_rate_ ); // sample rate can't be changed once set
	Buffer_init( &this->stereo_buf );
	RETURN_ERR( Buffer_set_sample_rate( &this->stereo_buf, rate, 1000 / 20 ) );
	
	// Set bass frequency
	Buffer_bass_freq( &this->stereo_buf, 60 );
	
	this->sample_rate_ = rate;
	RETURN_ERR( track_init( &this->track_filter, this ) );
	this->tfilter.max_silence = 6 * stereo * this->sample_rate_;
	return 0;
}

void Sound_mute_voice( struct Hes_Emu* this, int index, bool mute )
{
	require( (unsigned) index < (unsigned) this->voice_count_ );
	int bit = 1 << index;
	int mask = this->mute_mask_ | bit;
	if ( !mute )
		mask ^= bit;
	Sound_mute_voices( this, mask );
}

void Sound_mute_voices( struct Hes_Emu* this, int mask )
{
	require( this->sample_rate_ ); // sample rate must be set first
	this->mute_mask_ = mask;

	// Set adpcm voice
	struct channel_t ch = Buffer_channel( &this->stereo_buf, this->voice_count_ );
	if ( mask & (1 << this->voice_count_ ) )
		Adpcm_set_output( &this->adpcm, 0, 0, 0, 0 );
	else
		Adpcm_set_output( &this->adpcm, 0, ch.center, ch.left, ch.right );
	
	// Set apu voices
	int i = this->voice_count_ - 1;
	for ( ; i--; )
	{
		if ( mask & (1 << i) )
		{
			Apu_osc_output( &this->apu, i, 0, 0, 0 );
		}
		else
		{
			struct channel_t ch = Buffer_channel( &this->stereo_buf, i );
			assert( (ch.center && ch.left && ch.right) ||
					(!ch.center && !ch.left && !ch.right) ); // all or nothing
			Apu_osc_output( &this->apu, i, ch.center, ch.left, ch.right );
		}
	}
}

void Sound_set_tempo( struct Hes_Emu* this, int t )
{
	require( this->sample_rate_ ); // sample rate must be set first
	int const min = (int)(FP_ONE_TEMPO*0.02);
	int const max = (int)(FP_ONE_TEMPO*4.00);
	if ( t < min ) t = min;
	if ( t > max ) t = max;
	this->play_period = (hes_time_t) ((period_60hz*FP_ONE_TEMPO) / t);
	this->timer_base = (int) ((1024*FP_ONE_TEMPO) / t);
	recalc_timer_load( this );
	this->tempo_ = t;
}

blargg_err_t Hes_start_track( struct Hes_Emu* this, int track )
{
	clear_track_vars( this );
	
	// Remap track if playlist available
	if ( this->m3u.size > 0 ) {
		struct entry_t* e = &this->m3u.entries[track];
		track = e->track;
	}
	
	this->current_track_ = track;
	
	Buffer_clear( &this->stereo_buf );
	
	memset( this->ram, 0, sizeof this->ram ); // some HES music relies on zero fill
	memset( this->sgx, 0, sizeof this->sgx );
	
	Apu_reset( &this->apu );
	Adpcm_reset( &this->adpcm );
	Cpu_reset( &this->cpu );
	
	unsigned i;
	for ( i = 0; i < sizeof this->header.banks; i++ )
		set_mmr( this, i, this->header.banks [i] );
	set_mmr( this, page_count, 0xFF ); // unmapped beyond end of address space
	
	this->irq.disables  = timer_mask | vdp_mask;
	this->irq.timer     = future_time;
	this->irq.vdp       = future_time;
	
	this->timer.enabled = false;
	this->timer.raw_load= 0x80;
	this->timer.count   = this->timer.load;
	this->timer.fired   = false;
	this->timer.last_time = 0;
	
	this->vdp.latch     = 0;
	this->vdp.control   = 0;
	this->vdp.next_vbl  = 0;
	
	this->ram [0x1FF] = (idle_addr - 1) >> 8;
	this->ram [0x1FE] = (idle_addr - 1) & 0xFF;
	this->cpu.r.sp = 0xFD;
	this->cpu.r.pc = get_le16( this->header.init_addr );
	this->cpu.r.a  = track;
	
	recalc_timer_load( this );
	
	// convert filter times to samples
	struct setup_t s = this->tfilter;
	s.max_initial *= this->sample_rate_ * stereo;
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

int Track_tell( struct Hes_Emu* this )
{
	int rate = this->sample_rate_ * stereo;
	int sec = track_sample_count( &this->track_filter ) / rate;
	return sec * 1000 + (track_sample_count( &this->track_filter ) - sec * rate) * 1000 / rate;
}

blargg_err_t Track_seek( struct Hes_Emu* this, int msec )
{
	int time = msec_to_samples( msec, this->sample_rate_ );
	if ( time < track_sample_count( &this->track_filter ) )
	RETURN_ERR( Hes_start_track( this, this->current_track_ ) );
	return Track_skip( this, time - track_sample_count( &this->track_filter ) );
}

blargg_err_t skip_( void* emu, int count )
{
	struct Hes_Emu* this = (struct Hes_Emu*) emu;
	
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

blargg_err_t Track_skip( struct Hes_Emu* this, int count )
{
	require( this->current_track_ >= 0 ); // start_track() must have been called already
	return track_skip( &this->track_filter, count );
}

void Track_set_fade( struct Hes_Emu* this, int start_msec, int length_msec )
{
	track_set_fade( &this->track_filter, msec_to_samples( start_msec, this->sample_rate_ ),
		length_msec * this->sample_rate_ / (1000 / stereo) );
}

blargg_err_t Hes_play( struct Hes_Emu* this, int out_count, sample_t* out )
{
	require( this->current_track_ >= 0 );
	require( out_count % stereo == 0 );
	return track_play( &this->track_filter, out_count, out );
}
