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

long const period_60hz = 262 * 455L; // scanlines * clocks per scanline

int const stereo = 2; // number of channels for stereo
int const silence_max = 6; // seconds
int const silence_threshold = 0x10;
long const fade_block_size = 512;
int const fade_shift = 8; // fade ends with gain at 1.0 / (1 << fade_shift)

const char gme_wrong_file_type [] ICONST_ATTR = "Wrong file type for this emulator";

void clear_track_vars( struct Hes_Emu* this )
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

void Hes_init( struct Hes_Emu* this )
{
	this->sample_rate_ = 0;
	this->mute_mask_   = 0;
	this->tempo_       = (int)(FP_ONE_TEMPO);

	// defaults
	this->max_initial_silence = 2;
	this->ignore_silence     = false;

	// Unload
	this->voice_count_ = 0;
	clear_track_vars( this );

	this->timer.raw_load = 0;
	this->silence_lookahead = 6;
	Sound_set_gain( this, (int)(FP_ONE_GAIN*1.11) );

	Rom_init( &this->rom, 0x2000 );

	Apu_init( &this->apu );
	Adpcm_init( &this->adpcm );
	Cpu_init( &this->cpu );

	/* Set default track count */
	this->track_count = 255;
}

static blargg_err_t check_hes_header( void const* header )
{
	if ( memcmp( header, "HESM", 4 ) )
		return gme_wrong_file_type;
	return 0;
}

// Setup

blargg_err_t Hes_load( struct Hes_Emu* this, void* data, long size )
{
	// Unload
	this->voice_count_ = 0;
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
	
	long addr = get_le32( this->header.addr );
	/* long rom_size = get_le32( this->header.size ); */
	long const rom_max = 0x100000;
	if ( addr & ~(rom_max - 1) )
	{
		/* warning( "Invalid address" ); */
		addr &= rom_max - 1;
	}
	/* if ( (unsigned long) (addr + size) > (unsigned long) rom_max )
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
	
	Apu_volume( &this->apu, this->gain_ );
	Adpcm_volume( &this->adpcm, this->gain_ );

    // Setup buffer	
	this->clock_rate_ = 7159091;
	Buffer_clock_rate( &this->stereo_buf, 7159091 );
	this->buf_changed_count = Buffer_channels_changed_count( &this->stereo_buf );
	
	Sound_set_tempo( this, this->tempo_ );
	Sound_mute_voices( this, this->mute_mask_ );
	
	// Reset track count
	this->track_count = 255;
	this->m3u.size = 0;
	return 0;
}


// Emulation

void recalc_timer_load( struct Hes_Emu* this ) ICODE_ATTR;
void recalc_timer_load( struct Hes_Emu* this )
{
	this->timer.load = this->timer.raw_load * this->timer_base + 1;
}

// Hardware

void irq_changed( struct Hes_Emu* this ) ICODE_ATTR;
void run_until( struct Hes_Emu* this, hes_time_t present ) ICODE_ATTR;
void Cpu_write_vdp( struct Hes_Emu* this, int addr, int data )
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
		else
		{
			dprintf( "VDP not supported: $%02X <- $%02X\n", this->vdp.latch, data );
		}
		break;
	
	case 3:
		dprintf( "VDP MSB not supported: $%02X <- $%02X\n", this->vdp.latch, data );
		break;
	}
}

int Cpu_done( struct Hes_Emu* this )
{
	check( time() >= end_time() ||
			(!(r.status & i_flag_mask) && time() >= irq_time()) );
	
	if ( !(this->cpu.r.status & i_flag_mask) )
	{
		hes_time_t present = Cpu_time( &this->cpu );
		
		if ( this->irq.timer <= present && !(this->irq.disables & timer_mask) )
		{
			this->timer.fired = true;
			this->irq.timer = (hes_time_t)future_hes_time;
			irq_changed( this ); // overkill, but not worth writing custom code
			#if defined (GME_FRAME_HOOK_DEFINED)
			{
				unsigned const threshold = period_60hz / 30;
				unsigned long elapsed = present - last_frame_hook;
				if ( elapsed - period_60hz + threshold / 2 < threshold )
				{
					last_frame_hook = present;
					GME_FRAME_HOOK( this );
				}
			}
			#endif
			return 0x0A;
		}
		
		if ( this->irq.vdp <= present && !(this->irq.disables & vdp_mask) )
		{
			// work around for bugs with music not acknowledging VDP
			//run_until( present );
			//irq.vdp = future_hes_time;
			//irq_changed();
			#if defined(GME_FRAME_HOOK_DEFINED)
				last_frame_hook = present;
				GME_FRAME_HOOK( this );
			#endif
			return 0x08;
		}
	}
	return 0;
}

void Emu_cpu_write( struct Hes_Emu* this, hes_addr_t addr, int data )
{
	hes_time_t time = Cpu_time( &this->cpu );
	if ( (unsigned) (addr - start_addr) <= end_addr - start_addr )
	{
		GME_APU_HOOK( this, addr - apu.start_addr, data );
		// avoid going way past end when a long block xfer is writing to I/O space
		hes_time_t t = min( time, this->cpu.end_time + 8 );
		Apu_write_data( &this->apu, t, addr, data );
		return;
	}
	
	if ( (unsigned) (addr - io_addr) < io_size )
	{
		hes_time_t t = min( time, this->cpu.end_time + 6 );
		Adpcm_write_data( &this->adpcm, t, addr, data );
		return;
	}
	
	switch ( addr )
	{
	case 0x0000:
	case 0x0002:
	case 0x0003:
		Cpu_write_vdp( this, addr, data );
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
		
		// flag questionable values
		if ( (data & 0xF8) && (data & 0xF8) != 0xF8 ) {
			dprintf( "Int mask: $%02X\n", data );
		}
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
		dprintf( "unmapped write $%04X <- $%02X\n", addr, data );
		return;
#endif
	}
	
	irq_changed( this );
}

int Emu_cpu_read( struct Hes_Emu* this, hes_addr_t addr )
{
	hes_time_t time = Cpu_time( &this->cpu );
	addr &= page_size - 1;
	switch ( addr )
	{
	case 0x0000:
		if ( this->irq.vdp > time )
			return 0;
		this->irq.vdp = (hes_time_t)future_hes_time;
		run_until( this, time );
		irq_changed( this );
		return 0x20;
		
	case 0x0002:
	case 0x0003:
		dprintf( "VDP read not supported: %d\n", addr );
		return 0;
	
	case 0x0C01:
		//return timer.enabled; // TODO: remove?
	case 0x0C00:
		run_until( this, time );
		dprintf( "Timer count read\n" );
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
		// case 0x180C: // CD-ROM
		// case 0x180D:
			break;
		
		default:
			dprintf( "unmapped read  $%04X\n", addr );
	#endif
	}
	
	return unmapped;
}

// see hes_cpu_io.h for core read/write functions

// Emulation

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

void irq_changed( struct Hes_Emu* this )
{
	hes_time_t present = Cpu_time( &this->cpu );
	
	if ( this->irq.timer > present )
	{
		this->irq.timer = (hes_time_t)future_hes_time;
		if ( this->timer.enabled && !this->timer.fired )
			this->irq.timer = present + this->timer.count;
	}
	
	if ( this->irq.vdp > present )
	{
		this->irq.vdp = (hes_time_t)future_hes_time;
		if ( this->vdp.control & 0x08 )
			this->irq.vdp = this->vdp.next_vbl;
	}
	
	hes_time_t time = (hes_time_t)future_hes_time;
	if ( !(this->irq.disables & timer_mask) ) time = this->irq.timer;
	if ( !(this->irq.disables &   vdp_mask) ) time = min( time, this->irq.vdp );
	
	// Set cpu irq time
	this->cpu.state->time += Cpu_update_end_time( &this->cpu, this->cpu.r.status, 
					this->cpu.end_time, (this->cpu.irq_time = time) );
}

static void adjust_time( blargg_long* time, hes_time_t delta ) ICODE_ATTR;
static void adjust_time( blargg_long* time, hes_time_t delta )
{
	if ( *time < (blargg_long)future_hes_time )
	{
		*time -= delta;
		if ( *time < 0 )
			*time = 0;
	}
}

blargg_err_t run_clocks( struct Hes_Emu* this, blip_time_t* duration_ ) ICODE_ATTR;
blargg_err_t run_clocks( struct Hes_Emu* this, blip_time_t* duration_ )
{
	blip_time_t duration = *duration_; // cache
	
	Cpu_run( this, duration );
	/* warning( "Emulation error (illegal instruction)" ); */
	
	check( time() >= duration );
	//check( time() - duration < 20 ); // Txx instruction could cause going way over
	
	run_until( this, duration );
	
	// end time frame
	this->timer.last_time -= duration;
	this->vdp.next_vbl    -= duration;
	#if defined (GME_FRAME_HOOK_DEFINED)
		last_frame_hook -= *duration;
	#endif

	// End cpu frame
	this->cpu.state_.base -= duration;
	if ( this->cpu.irq_time < (hes_time_t)future_hes_time ) this->cpu.irq_time -= duration;
	if ( this->cpu.end_time < (hes_time_t)future_hes_time ) this->cpu.end_time -= duration;
	
	adjust_time( &this->irq.timer, duration );
	adjust_time( &this->irq.vdp,   duration );
	Apu_end_frame( &this->apu, duration );
	Adpcm_end_frame( &this->adpcm, duration );
	
	return 0;
}

blargg_err_t play_( struct Hes_Emu* this, long count, sample_t* out ) ICODE_ATTR;
blargg_err_t play_( struct Hes_Emu* this, long count, sample_t* out )
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
			RETURN_ERR( run_clocks( this, &clocks_emulated ) );
			assert( clocks_emulated );
			Buffer_end_frame( &this->stereo_buf, clocks_emulated );
		}
	}
	return 0;
}


// Music emu

blargg_err_t Hes_set_sample_rate( struct Hes_Emu* this, long rate )
{
	require( !this->sample_rate_ ); // sample rate can't be changed once set
	Buffer_init( &this->stereo_buf );
	RETURN_ERR( Buffer_set_sample_rate( &this->stereo_buf, rate, 1000 / 20 ) );
	
	// Set bass frequency
	Buffer_bass_freq( &this->stereo_buf, 60 );
	
	this->sample_rate_ = rate;
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
	struct channel_t ch = Buffer_channel( &this->stereo_buf );
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

void fill_buf( struct Hes_Emu* this ) ICODE_ATTR;
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
	
	memset( this->cpu.ram, 0, sizeof this->cpu.ram ); // some HES music relies on zero fill
	memset( this->sgx, 0, sizeof this->sgx );
	
	Apu_reset( &this->apu );
	Adpcm_reset( &this->adpcm );
	Cpu_reset( &this->cpu );
	
	unsigned i;
	for ( i = 0; i < sizeof this->header.banks; i++ )
		Cpu_set_mmr( this, i, this->header.banks [i] );
	Cpu_set_mmr( this, page_count, 0xFF ); // unmapped beyond end of address space
	
	this->irq.disables  = timer_mask | vdp_mask;
	this->irq.timer     = (hes_time_t)future_hes_time;
	this->irq.vdp       = (hes_time_t)future_hes_time;
	
	this->timer.enabled = false;
	this->timer.raw_load= 0x80;
	this->timer.count   = this->timer.load;
	this->timer.fired   = false;
	this->timer.last_time = 0;
	
	this->vdp.latch     = 0;
	this->vdp.control   = 0;
	this->vdp.next_vbl  = 0;
	
	this->cpu.ram [0x1FF] = (idle_addr - 1) >> 8;
	this->cpu.ram [0x1FE] = (idle_addr - 1) & 0xFF;
	this->cpu.r.sp = 0xFD;
	this->cpu.r.pc = get_le16( this->header.init_addr );
	this->cpu.r.a  = track;
	
	recalc_timer_load( this );
	this->last_frame_hook = 0;
	
	this->emu_track_ended_ = false;
	this->track_ended     = false;
	
	if ( !this->ignore_silence )
	{
		// play until non-silence or end of track
		long end;
		for ( end =this-> max_initial_silence * stereo * this->sample_rate_; this->emu_time < end; )
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

long Track_tell( struct Hes_Emu* this )
{
	blargg_long rate = this->sample_rate_ * stereo;
	blargg_long sec = this->out_time / rate;
	return sec * 1000 + (this->out_time - sec * rate) * 1000 / rate;
}

blargg_err_t Track_seek( struct Hes_Emu* this, long msec )
{
	blargg_long time = msec_to_samples( msec, this->sample_rate_ );
	if ( time < this->out_time )
		RETURN_ERR( Hes_start_track( this, this->current_track_ ) );
	return Track_skip( this, time - this->out_time );
}

blargg_err_t skip_( struct Hes_Emu* this, long count ) ICODE_ATTR;
blargg_err_t skip_( struct Hes_Emu* this, long count )
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

blargg_err_t Track_skip( struct Hes_Emu* this, long count )
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

void Track_set_fade( struct Hes_Emu* this, long start_msec, long length_msec )
{
	this->fade_step = this->sample_rate_ * length_msec / (fade_block_size * fade_shift * 1000 / stereo);
	this->fade_start = msec_to_samples( start_msec, this->sample_rate_ );
}

// unit / pow( 2.0, (double) x / step )
static int int_log( blargg_long x, int step, int unit ) ICODE_ATTR;
static int int_log( blargg_long x, int step, int unit )
{
	int shift = x / step;
	int fraction = (x - shift * step) * unit / step;
	return ((unit - fraction) + (fraction >> 1)) >> shift;
}

void handle_fade( struct Hes_Emu* this, long out_count, sample_t* out ) ICODE_ATTR;
void handle_fade( struct Hes_Emu* this, long out_count, sample_t* out )
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

void emu_play( struct Hes_Emu* this, long count, sample_t* out ) ICODE_ATTR;
void emu_play( struct Hes_Emu* this, long count, sample_t* out )
{
	check( current_track_ >= 0 );
	this->emu_time += count;
	if ( this->current_track_ >= 0 && !this->emu_track_ended_ ) {
		
		// End track if error
		if ( play_( this, count, out ) )
			this->emu_track_ended_ = true;
	}
	else
		memset( out, 0, count * sizeof *out );
}

// number of consecutive silent samples at end
static long count_silence( sample_t* begin, long size ) ICODE_ATTR;
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
void fill_buf( struct Hes_Emu* this )
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

blargg_err_t Hes_play( struct Hes_Emu* this, long out_count, sample_t* out )
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
		
		// prints nifty graph of how far ahead we are when searching for silence
		//dprintf( "%*s \n", int ((emu_time - out_time) * 7 / sample_rate()), "*" );
		
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
