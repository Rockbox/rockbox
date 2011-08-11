// Game_Music_Emu 0.5.5. http://www.slack.net/~ant/

#include "nsf_emu.h"
#include "multi_buffer.h"

#include "blargg_endian.h"

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

const char gme_wrong_file_type [] ICONST_ATTR = "Wrong file type for this emulator";
long const clock_divisor = 12;

int const stereo = 2; // number of channels for stereo
int const silence_max = 6; // seconds
int const silence_threshold = 0x10;
long const fade_block_size = 512;
int const fade_shift = 8; // fade ends with gain at 1.0 / (1 << fade_shift)

// number of frames until play interrupts init
int const initial_play_delay = 7; // KikiKaikai needed this to work
int const rom_addr  = 0x8000;

void clear_track_vars( struct Nsf_Emu* this )
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
}

static int pcm_read( void* emu, addr_t addr )
{
	return *Cpu_get_code( &((struct Nsf_Emu*) emu)->cpu, addr );
}

void Nsf_init( struct Nsf_Emu* this )
{
	this->sample_rate = 0;
	this->mute_mask_   = 0;
	this->tempo        = (int)(FP_ONE_TEMPO);
	this->gain         = (int)(FP_ONE_GAIN);
	
	// defaults
	this->max_initial_silence = 2;
	this->ignore_silence     = false;
	this->voice_count = 0;
	
	// Set sound gain
	Sound_set_gain( this, (int)(FP_ONE_GAIN*1.2) );
	
	// Unload
	clear_track_vars( this );
	
	// Init rom
	Rom_init( &this->rom, 0x1000 );
	
	// Init & clear nsfe info
	Info_init( &this->info );
	Info_unload( &this->info ); // TODO: extremely hacky!
	
	Cpu_init( &this->cpu );
	Apu_init( &this->apu );
	Apu_dmc_reader( &this->apu, pcm_read, this );
}

// Setup

blargg_err_t init_sound( struct Nsf_Emu* this )
{
	/* if ( header_.chip_flags & ~(fds_flag | namco_flag | vrc6_flag | fme7_flag) )
		warning( "Uses unsupported audio expansion hardware" ); **/
	
	this->voice_count = apu_osc_count;
	
	int adjusted_gain = (this->gain * 4) / 3;
	
	#ifdef NSF_EMU_APU_ONLY
	{
		if ( this->header_.chip_flags )
			set_warning( "Uses unsupported audio expansion hardware" );
	}
	#else
	{
		if ( vrc6_enabled( this ) )
		{
			Vrc6_init( &this->vrc6 );
			adjusted_gain = (adjusted_gain*3) / 4;
			
			this->voice_count += vrc6_osc_count;
		}
			
		if ( fme7_enabled( this ) )
		{
			Fme7_init( &this->fme7 );
			adjusted_gain = (adjusted_gain*3) / 4;
			
			this->voice_count += fme7_osc_count;
		}
		
		if ( mmc5_enabled( this ) )
		{
			Mmc5_init( &this->mmc5 );
			adjusted_gain = (adjusted_gain*3) / 4;
			
			this->voice_count += mmc5_osc_count;
		}
		
		if ( fds_enabled( this ) )
		{
			Fds_init( &this->fds );
			adjusted_gain = (adjusted_gain*3) / 4;
			
			this->voice_count += fds_osc_count ;
		}
		
		if ( namco_enabled( this ) )
		{
			Namco_init( &this->namco );
			adjusted_gain = (adjusted_gain*3) / 4;
			
			this->voice_count += namco_osc_count;
		}
		
		if ( vrc7_enabled( this ) )
		{
			#ifndef NSF_EMU_NO_VRC7
				Vrc7_init( &this->vrc7 );
				Vrc7_set_rate( &this->vrc7, this->sample_rate );
			#endif

			adjusted_gain = (adjusted_gain*3) / 4;
			
			this->voice_count += vrc7_osc_count;
		}
		
		if ( vrc7_enabled( this )  ) Vrc7_volume( &this->vrc7,    adjusted_gain );
		if ( namco_enabled( this ) ) Namco_volume( &this->namco,  adjusted_gain );
		if ( vrc6_enabled( this )  ) Vrc6_volume( &this->vrc6,    adjusted_gain );
		if ( fme7_enabled( this )  ) Fme7_volume( &this->fme7,    adjusted_gain );
		if ( mmc5_enabled( this )  ) Apu_volume( &this->mmc5.apu, adjusted_gain );
		if ( fds_enabled( this )   ) Fds_volume( &this->fds,      adjusted_gain );
	}
	#endif
	
	if ( adjusted_gain > this->gain )
		adjusted_gain = this->gain;
	
	Apu_volume( &this->apu, adjusted_gain );
	
	return 0;
}

// Header stuff
bool valid_tag( struct header_t* this )
{
	return 0 == memcmp( this->tag, "NESM\x1A", 5 );
}

// True if file supports only PAL speed
static bool pal_only( struct header_t* this )
{
	return (this->speed_flags & 3) == 1;
}
	
static double clock_rate( struct header_t* this )
{
	return pal_only( this ) ? 1662607.125 : 1789772.727272727;
}

int play_period( struct header_t* this )
{
	// NTSC
	int         clocks   = 29780;
	int         value    = 0x411A;
	byte const* rate_ptr = this->ntsc_speed;
	
	// PAL
	if ( pal_only( this ) )
	{
		clocks   = 33247;
		value    = 0x4E20;
		rate_ptr = this->pal_speed;
	}
	
	// Default rate
	int rate = get_le16( rate_ptr );
	if ( rate == 0 )
		rate = value;
	
	// Custom rate
	if ( rate != value )
		clocks = (int) (rate * clock_rate( this ) * (1.0/1000000.0));
	
	return clocks;
}

// Gets address, given pointer to it in file header. If zero, returns rom_addr.
addr_t get_addr( byte const in [] )
{
	addr_t addr = get_le16( in );
	if ( addr == 0 )
		addr = rom_addr;
	return addr;
}

static blargg_err_t check_nsf_header( struct header_t* h )
{
	if ( !valid_tag( h ) )
		return gme_wrong_file_type;
	return 0;
}

blargg_err_t Nsf_load( struct Nsf_Emu* this, void* data, long size )
{
	// Unload
	Info_unload( &this->info ); // TODO: extremely hacky!
	this->m3u.size = 0;

	this->voice_count = 0;
	clear_track_vars( this );
	
	assert( offsetof (struct header_t,unused [4]) == header_size );
	
	if ( !memcmp( data, "NESM\x1A", 5 ) ) {
		Nsf_disable_playlist( this, true );
		
		RETURN_ERR( Rom_load( &this->rom, data, size, header_size, &this->header, 0 ) );
		return Nsf_post_load( this );
	}
	
	blargg_err_t err = Info_load( &this->info, data, size, this );
	Nsf_disable_playlist( this, false );
	return err;
}

blargg_err_t Nsf_post_load( struct Nsf_Emu* this )
{
	RETURN_ERR( check_nsf_header( &this->header ) );
	
	/* if ( header_.vers != 1 )
		warning( "Unknown file version" ); */
	
	// set up data
	addr_t load_addr = get_le16( this->header.load_addr );
	
	/* if ( load_addr < (fds_enabled() ? sram_addr : rom_addr) )
		warning( "Load address is too low" ); */
		
	Rom_set_addr( &this->rom, load_addr % this->rom.bank_size );
	
	/* if ( header_.vers != 1 )
		warning( "Unknown file version" ); */
		
	set_play_period( this, play_period( &this->header ) );
		
	// sound and memory
	blargg_err_t err = init_sound( this );
	if ( err )
		return err;
	
	// Post load
	Sound_set_tempo( this, this->tempo );
	
	// Remute voices
	Sound_mute_voices( this, this->mute_mask_ );

	// Set track_count
	this->track_count = this->header.track_count;
	
	// Change clock rate & setup buffer
	this->clock_rate__ = (long) (clock_rate( &this->header ) + 0.5);
	Buffer_clock_rate( &this->stereo_buf, this->clock_rate__ );
	this->buf_changed_count = Buffer_channels_changed_count( &this->stereo_buf );
	return 0;
}

void Nsf_disable_playlist( struct Nsf_Emu* this, bool b )
{
	Info_disable_playlist( &this->info, b );
	this->track_count = this->info.track_count;
}

void Nsf_clear_playlist( struct Nsf_Emu* this )
{
	Nsf_disable_playlist( this, true );
}

void write_bank( struct Nsf_Emu* this, int bank, int data )
{
	// Find bank in ROM
	int offset = mask_addr( data * this->rom.bank_size, this->rom.mask );
	/* if ( offset >= rom.size() )
		warning( "invalid bank" ); */
	void const* rom_data = Rom_at_addr( &this->rom, offset );
	
	#ifndef NSF_EMU_APU_ONLY
		if ( bank < bank_count - fds_banks && fds_enabled( this ) )
		{
			// TODO: FDS bank switching is kind of hacky, might need to
			// treat ROM as RAM so changes won't get lost when switching.
			byte* out = sram( this );
			if ( bank >= fds_banks )
			{
				out = fdsram( this );
				bank -= fds_banks;
			}
			memcpy( &out [bank * this->rom.bank_size], rom_data, this->rom.bank_size );
			return;
		}
	#endif
	
	if ( bank >= fds_banks )
		Cpu_map_code( &this->cpu, (bank + 6) * this->rom.bank_size, this->rom.bank_size, rom_data, false );
}

void map_memory( struct Nsf_Emu* this )
{
	// Map standard things
	Cpu_reset( &this->cpu, unmapped_code( this ) );
	Cpu_map_code( &this->cpu, 0, 0x2000, this->low_ram, low_ram_size ); // mirrored four times
	Cpu_map_code( &this->cpu, sram_addr, sram_size, sram( this ), 0 );
	
	// Determine initial banks
	byte banks [bank_count];
	static byte const zero_banks [sizeof this->header.banks] = { 0 };
	if ( memcmp( this->header.banks, zero_banks, sizeof zero_banks ) )
	{
		banks [0] = this->header.banks [6];
		banks [1] = this->header.banks [7];
		memcpy( banks + fds_banks, this->header.banks, sizeof this->header.banks );
	}
	else
	{
		// No initial banks, so assign them based on load_addr
		int i, first_bank = (get_addr( this->header.load_addr ) - sram_addr) / this->rom.bank_size;
		unsigned total_banks = this->rom.size / this->rom.bank_size;
		for ( i = bank_count; --i >= 0; )
		{
			int bank = i - first_bank;
			if ( (unsigned) bank >= total_banks )
				bank = 0;
			banks [i] = bank;
		}
	}
	
	// Map banks
	int i;
	for ( i = (fds_enabled( this ) ? 0 : fds_banks); i < bank_count; ++i )
		write_bank( this, i, banks [i] );
	
	// Map FDS RAM
	if ( fds_enabled( this ) )
		Cpu_map_code( &this->cpu, rom_addr, fdsram_size, fdsram( this ), 0 );
}

void set_voice( struct Nsf_Emu* this, int i, struct Blip_Buffer* buf, struct Blip_Buffer* left, struct Blip_Buffer* right)
{
#if defined(ROCKBOX)
	(void) left;
	(void) right;
#endif

	if ( i < apu_osc_count )
	{
		Apu_osc_output( &this->apu, i, buf );
		return;
	}
	i -= apu_osc_count;
	
	#ifndef NSF_EMU_APU_ONLY
	{	
		if ( vrc6_enabled( this ) && (i -= vrc6_osc_count) < 0 )
		{
			Vrc6_osc_output( &this->vrc6, i + vrc6_osc_count, buf );
			return;
		}
		
		if ( fme7_enabled( this ) && (i -= fme7_osc_count) < 0 )
		{
			Fme7_osc_output( &this->fme7, i + fme7_osc_count, buf );
			return;
		}
		
		if ( mmc5_enabled( this ) && (i -= mmc5_osc_count) < 0 )
		{
			Mmc5_set_output( &this->mmc5, i + mmc5_osc_count, buf );
			return;
		}
		
		if ( fds_enabled( this ) && (i -= fds_osc_count) < 0 )
		{
			Fds_set_output( &this->fds, i + fds_osc_count, buf );
			return;
		}
		
		if ( namco_enabled( this ) && (i -= namco_osc_count) < 0 )
		{
			Namco_osc_output( &this->namco, i + namco_osc_count, buf );
			return;
		}
		
		if ( vrc7_enabled( this ) && (i -= vrc7_osc_count) < 0 )
		{
			Vrc7_set_output( &this->vrc7, i + vrc7_osc_count, buf );
			return;
		}
	}
	#endif
}

// Emulation

// Music Emu

blargg_err_t Nsf_set_sample_rate( struct Nsf_Emu* this, long rate )
{
	require( !this->sample_rate ); // sample rate can't be changed once set
	Buffer_init( &this->stereo_buf );
	RETURN_ERR( Buffer_set_sample_rate( &this->stereo_buf, rate, 1000 / 20 ) );
	
	// Set bass frequency
	Buffer_bass_freq( &this->stereo_buf, 80 );
	
	this->sample_rate = rate;
	return 0;
}

void Sound_mute_voice( struct Nsf_Emu* this, int index, bool mute )
{
	require( (unsigned) index < (unsigned) this->voice_count );
	int bit = 1 << index;
	int mask = this->mute_mask_ | bit;
	if ( !mute )
		mask ^= bit;
	Sound_mute_voices( this, mask );
}

void Sound_mute_voices( struct Nsf_Emu* this, int mask )
{
	require( this->sample_rate ); // sample rate must be set first
	this->mute_mask_ = mask;
	
	int i;
	for ( i = this->voice_count; i--; )
	{
		if ( mask & (1 << i) )
		{
			set_voice( this, i, 0, 0, 0 );
		}
		else
		{
			struct channel_t ch = Buffer_channel( &this->stereo_buf );
			assert( (ch.center && ch.left && ch.right) ||
					(!ch.center && !ch.left && !ch.right) ); // all or nothing
			set_voice( this, i, ch.center, ch.left, ch.right );
		}
	}
}

void Sound_set_tempo( struct Nsf_Emu* this, int t )
{
	require( this->sample_rate ); // sample rate must be set first
	int const min = (int)(FP_ONE_TEMPO*0.02);
	int const max = (int)(FP_ONE_TEMPO*4.00);
	if ( t < min ) t = min;
	if ( t > max ) t = max;
	this->tempo = t;
	
	set_play_period( this, (int) ((play_period( &this->header ) * FP_ONE_TEMPO) / t) );

	Apu_set_tempo( &this->apu, t );
	
#ifndef NSF_EMU_APU_ONLY
	if ( fds_enabled( this ) )
		Fds_set_tempo( &this->fds, t );
#endif
}

inline void push_byte( struct Nsf_Emu* this, int b )
{
	this->low_ram [0x100 + this->cpu.r.sp--] = b;
}

// Jumps to routine, given pointer to address in file header. Pushes idle_addr
// as return address, NOT old PC.
void jsr_then_stop( struct Nsf_Emu* this, byte const addr [] )
{
	this->cpu.r.pc = get_addr( addr );
	push_byte( this, (idle_addr - 1) >> 8 );
	push_byte( this, (idle_addr - 1) );
}

int cpu_read( struct Nsf_Emu* this, addr_t addr )
{
	#ifndef NSF_EMU_APU_ONLY
	{
		if ( namco_enabled( this ) && addr == namco_data_reg_addr )
			return Namco_read_data( &this->namco );
		
		if ( fds_enabled( this ) && (unsigned) (addr - fds_io_addr) < fds_io_size )
			return Fds_read( &this->fds, Cpu_time( &this->cpu ), addr );
		
		if ( mmc5_enabled( this ) ) {
			int i = addr - 0x5C00;
			if ( (unsigned) i < mmc5_exram_size )
				return this->mmc5.exram [i];
		
			int m = addr - 0x5205;
			if ( (unsigned) m < 2 )
				return (this->mmc5_mul [0] * this->mmc5_mul [1]) >> (m * 8) & 0xFF;
		}	
	}
	#endif
	
	/* Unmapped read */
	return addr >> 8;
}

int unmapped_read( struct Nsf_Emu* this, addr_t addr )
{
	(void) this;
	
	switch ( addr )
	{
	case 0x2002:
	case 0x4016:
	case 0x4017:
		return addr >> 8;
	}

	// Unmapped read
	return addr >> 8;
}

void cpu_write( struct Nsf_Emu* this, addr_t addr, int data )
{
	#ifndef NSF_EMU_APU_ONLY
	{
		nes_time_t time = Cpu_time( &this->cpu );
		if ( fds_enabled( this) && (unsigned) (addr - fds_io_addr) < fds_io_size )
		{
			Fds_write( &this->fds, time, addr, data );
			return;
		}
		
		if ( namco_enabled( this) )
		{
			if ( addr == namco_addr_reg_addr )
			{
				Namco_write_addr( &this->namco, data );
				return;
			}
			
			if ( addr == namco_data_reg_addr )
			{
				Namco_write_data( &this->namco, time, data );
				return;
			}
		}
		
		if ( vrc6_enabled( this) )
		{
			int reg = addr & (vrc6_addr_step - 1);
			int osc = (unsigned) (addr - vrc6_base_addr) / vrc6_addr_step;
			if ( (unsigned) osc < vrc6_osc_count && (unsigned) reg < vrc6_reg_count )
			{
				Vrc6_write_osc( &this->vrc6, time, osc, reg, data );
				return;
			}
		}
		
		if ( fme7_enabled( this) && addr >= fme7_latch_addr )
		{
			switch ( addr & fme7_addr_mask )
			{
			case fme7_latch_addr:
				Fme7_write_latch( &this->fme7, data );
				return;
			
			case fme7_data_addr:
				Fme7_write_data( &this->fme7, time, data );
				return;
			}
		}
		
		if ( mmc5_enabled( this) )
		{
			if ( (unsigned) (addr - mmc5_regs_addr) < mmc5_regs_size )
			{
				Mmc5_write_register( &this->mmc5, time, addr, data );
				return;
			}
			
			int m = addr - 0x5205;
			if ( (unsigned) m < 2 )
			{
				this->mmc5_mul [m] = data;
				return;
			}
			
			int i = addr - 0x5C00;
			if ( (unsigned) i < mmc5_exram_size )
			{
				this->mmc5.exram [i] = data;
				return;
			}
		}
		
		if ( vrc7_enabled( this) )
		{
			if ( addr == 0x9010 )
			{
				Vrc7_write_reg( &this->vrc7, data );
				return;
			}
			
			if ( (unsigned) (addr - 0x9028) <= 0x08 )
			{
				Vrc7_write_data( &this->vrc7, time, data );
				return;
			}
		}
	}
	#endif
	
	// Unmapped_write
}

void unmapped_write( struct Nsf_Emu* this, addr_t addr, int data )
{
	(void) data;
	
	switch ( addr )
	{
	case 0x8000: // some write to $8000 and $8001 repeatedly
	case 0x8001:
	case 0x4800: // probably namco sound mistakenly turned on in MCK
	case 0xF800:
	case 0xFFF8: // memory mapper?
		return;
	}
	
	if ( mmc5_enabled( this ) && addr == 0x5115 ) return;
	
	// FDS memory
	if ( fds_enabled( this ) && (unsigned) (addr - 0x8000) < 0x6000 ) return;
}

void fill_buf( struct Nsf_Emu* this );
blargg_err_t Nsf_start_track( struct Nsf_Emu* this, int track )
{
	clear_track_vars( this );
	
	// Remap track if playlist available
	if ( this->m3u.size > 0 ) {
		struct entry_t* e = &this->m3u.entries[track];
		track = e->track;
	}
	else track = Info_remap_track( &this->info, track );
	
	this->current_track = track;
	Buffer_clear( &this->stereo_buf );
	
	#ifndef NSF_EMU_APU_ONLY
		if ( mmc5_enabled( this ) )
		{
			this->mmc5_mul [0] = 0;
			this->mmc5_mul [1] = 0;
			memset( this->mmc5.exram, 0, mmc5_exram_size );
		}

		if ( fds_enabled( this )   ) Fds_reset( &this->fds );
		if ( namco_enabled( this ) ) Namco_reset( &this->namco );
		if ( vrc6_enabled( this )  ) Vrc6_reset( &this->vrc6 );
		if ( fme7_enabled( this )  ) Fme7_reset( &this->fme7 );
		if ( mmc5_enabled( this )  ) Apu_reset( &this->mmc5.apu, false, 0 );
		if ( vrc7_enabled( this )  ) Vrc7_reset( &this->vrc7 );
	#endif
	
	int speed_flags = 0;
	#ifdef NSF_EMU_EXTRA_FLAGS
		speed_flags = this->header.speed_flags;
	#endif
	
	Apu_reset( &this->apu, pal_only( &this->header ), (speed_flags & 0x20) ? 0x3F : 0 );
	Apu_write_register( &this->apu, 0, 0x4015, 0x0F );
	Apu_write_register( &this->apu, 0, 0x4017, (speed_flags & 0x10) ? 0x80 : 0 );
	
	memset( unmapped_code( this ), halt_opcode, unmapped_size );
	memset( this->low_ram, 0, low_ram_size );
	memset( sram( this ), 0, sram_size );
	
	map_memory( this );
	
	// Arrange time of first call to play routine
	this->play_extra = 0;
	this->next_play  = this->play_period;
	
	this->play_delay = initial_play_delay;
	this->saved_state.pc = idle_addr;
	
	// Setup for call to init routine
	this->cpu.r.a  = track;
	this->cpu.r.x  = pal_only( &this->header );
	this->cpu.r.sp = 0xFF;
	jsr_then_stop( this, this->header.init_addr );
	/* if ( this->cpu.r.pc < get_addr( header.load_addr ) )
		warning( "Init address < load address" ); */
	
	this->emu_track_ended_ = false;
	this->track_ended     = false;
	
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

void run_once( struct Nsf_Emu* this, nes_time_t end )
{
	// Emulate until next play call if possible
	if ( run_cpu_until( this, min( this->next_play, end ) ) )
	{
		// Halt instruction encountered
		
		if ( this->cpu.r.pc != idle_addr )
		{
			// special_event( "illegal instruction" );
			Cpu_set_time( &this->cpu, this->cpu.end_time );
			return;
		}

		// Init/play routine returned
		this->play_delay = 1; // play can now be called regularly
		
		if ( this->saved_state.pc == idle_addr )
		{
			// nothing to run
			nes_time_t t = this->cpu.end_time;
			if ( Cpu_time( &this->cpu ) < t )
				Cpu_set_time( &this->cpu, t );
		}
		else
		{
			// continue init routine that was interrupted by play routine
			this->cpu.r = this->saved_state;
			this->saved_state.pc = idle_addr;
		}
	}
	
	if ( Cpu_time( &this->cpu ) >= this->next_play )
	{
		// Calculate time of next call to play routine
		this->play_extra ^= 1; // extra clock every other call
		this->next_play += this->play_period + this->play_extra;
		
		// Call routine if ready
		if ( this->play_delay && !--this->play_delay )
		{
			// Save state if init routine is still running
			if ( this->cpu.r.pc != idle_addr )
			{
				check( this->saved_state.pc == idle_addr );
				this->saved_state = this->cpu.r;
				// special_event( "play called during init" );
			}
			
			jsr_then_stop( this, this->header.play_addr );
		}
	}
}

void run_until( struct Nsf_Emu* this, nes_time_t end )
{
	while ( Cpu_time( &this->cpu ) < end )
		run_once( this, end );
}

void end_frame( struct Nsf_Emu* this, nes_time_t end )
{
	if ( Cpu_time( &this->cpu ) < end )
		run_until( this, end );
	Cpu_adjust_time( &this->cpu, -end );
	
	// Localize to new time frame
	this->next_play -= end;
	check( this->next_play >= 0 );
	if ( this->next_play < 0 )
		this->next_play = 0;
	
	Apu_end_frame( &this->apu, end );
	
	#ifndef NSF_EMU_APU_ONLY
		if ( fds_enabled( this )   ) Fds_end_frame( &this->fds, end );
		if ( fme7_enabled( this )  ) Fme7_end_frame( &this->fme7, end );
		if ( mmc5_enabled( this )  ) Apu_end_frame( &this->mmc5.apu, end );
		if ( namco_enabled( this ) ) Namco_end_frame( &this->namco, end );
		if ( vrc6_enabled( this )  ) Vrc6_end_frame( &this->vrc6, end );
		if ( vrc7_enabled( this )  ) Vrc7_end_frame( &this->vrc7, end );
	#endif
}

// Tell/Seek

blargg_long msec_to_samples( long sample_rate, blargg_long msec )
{
	blargg_long sec = msec / 1000;
	msec -= sec * 1000;
	return (sec * sample_rate + msec * sample_rate / 1000) * stereo;
}

long Track_tell( struct Nsf_Emu* this )
{
	blargg_long rate = this->sample_rate * stereo;
	blargg_long sec = this->out_time / rate;
	return sec * 1000 + (this->out_time - sec * rate) * 1000 / rate;
}

blargg_err_t Track_seek( struct Nsf_Emu* this, long msec )
{
	blargg_long time = msec_to_samples( this->sample_rate, msec );
	if ( time < this->out_time )
		RETURN_ERR( Nsf_start_track( this, this->current_track ) );
	return Track_skip( this, time - this->out_time );
}

blargg_err_t skip_( struct Nsf_Emu* this, long count ) ICODE_ATTR;
blargg_err_t Track_skip( struct Nsf_Emu* this, long count )
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

blargg_err_t play_( struct Nsf_Emu* this, long count, sample_t* out ) ICODE_ATTR;
blargg_err_t skip_( struct Nsf_Emu* this, long count )
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

// Fading

void Track_set_fade( struct Nsf_Emu* this, long start_msec, long length_msec )
{
	this->fade_step = this->sample_rate * length_msec / (fade_block_size * fade_shift * 1000 / stereo);
	this->fade_start = msec_to_samples( this->sample_rate, start_msec );
}

// unit / pow( 2.0, (double) x / step )
static int int_log( blargg_long x, int step, int unit )
{
	int shift = x / step;
	int fraction = (x - shift * step) * unit / step;
	return ((unit - fraction) + (fraction >> 1)) >> shift;
}

void handle_fade( struct Nsf_Emu* this, long out_count, sample_t* out )
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

void emu_play( struct Nsf_Emu* this, long count, sample_t* out ) ICODE_ATTR;
void emu_play( struct Nsf_Emu* this, long count, sample_t* out )
{
	check( current_track_ >= 0 );
	this->emu_time += count;
	if ( this->current_track >= 0 && !this->emu_track_ended_ ) {
		
		// End track if error
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
void fill_buf( struct Nsf_Emu* this )
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

blargg_err_t Nsf_play( struct Nsf_Emu* this, long out_count, sample_t* out )
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

blargg_err_t play_( struct Nsf_Emu* this, long count, sample_t* out )
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
			blip_time_t clocks_emulated = (blargg_long) msec * this->clock_rate__ / 1000 - 100;
			RETURN_ERR( run_clocks( this, &clocks_emulated, msec ) );
			assert( clocks_emulated );
			Buffer_end_frame( &this->stereo_buf, clocks_emulated );
		}
	}
	return 0;
}

blargg_err_t run_clocks( struct Nsf_Emu* this, blip_time_t* duration, int msec )
{
#if defined(ROCKBOX)
	(void) msec;
#endif

	end_frame( this, *duration );
	return 0;
}
