// Game_Music_Emu 0.6-pre. http://www.slack.net/~ant/

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

const char gme_wrong_file_type [] = "Wrong file type for this emulator";

// number of frames until play interrupts init
int const initial_play_delay = 7; // KikiKaikai needed this to work
int const bank_size = 0x1000;
int const rom_addr  = 0x8000;

static void clear_track_vars( struct Nsf_Emu* this )
{
	this->current_track    = -1;
	track_stop( &this->track_filter );
}

static int pcm_read( void* emu, int addr )
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
	this->tfilter = *track_get_setup( &this->track_filter );
	this->tfilter.max_initial = 2;
	this->tfilter.lookahead   = 6;
	this->track_filter.silence_ignored_ = false;
	
	// Set sound gain
	Sound_set_gain( this, (int)(FP_ONE_GAIN*1.2) );
	
	// Init rom
	Rom_init( &this->rom, bank_size );
	
	// Init & clear nsfe info
	Info_init( &this->info );
	Info_unload( &this->info ); // TODO: extremely hacky!
	
	Cpu_init( &this->cpu );
	Apu_init( &this->apu );
	Apu_dmc_reader( &this->apu, pcm_read, this );
	
	// Unload
	this->voice_count = 0;
	memset( this->voice_types, 0, sizeof this->voice_types );
	clear_track_vars( this );
}

// Setup

static void append_voices( struct Nsf_Emu* this, int const types [], int count )
{
	assert( this->voice_count + count < max_voices );
	int i;
	for ( i = 0; i < count; i++ ) {
		this->voice_types [this->voice_count + i] = types [i];
	}
	this->voice_count += count;
}

static blargg_err_t init_sound( struct Nsf_Emu* this )
{
/* if ( header_.chip_flags & ~(fds_flag | namco_flag | vrc6_flag | fme7_flag) )
		warning( "Uses unsupported audio expansion hardware" ); **/
	
	{
		static int const types [apu_osc_count] = {
			wave_type+1, wave_type+2, mixed_type+1, noise_type+0, mixed_type+1
		};
		append_voices( this, types, apu_osc_count );
	}
	
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
			
			static int const types [vrc6_osc_count] = {
				wave_type+3, wave_type+4, wave_type+5,
			};
			append_voices( this, types, vrc6_osc_count );
		}
			
		if ( fme7_enabled( this ) )
		{
			Fme7_init( &this->fme7 );
			adjusted_gain = (adjusted_gain*3) / 4;
			
			static int const types [fme7_osc_count] = {
				wave_type+3, wave_type+4, wave_type+5,
			};
			append_voices( this, types, fme7_osc_count );
		}
		
		if ( mmc5_enabled( this ) )
		{
			Mmc5_init( &this->mmc5 );
			adjusted_gain = (adjusted_gain*3) / 4;
			
			
			static int const types [mmc5_osc_count] = {
				wave_type+3, wave_type+4, mixed_type+2
			};
			append_voices( this, types, mmc5_osc_count );
		}
		
		if ( fds_enabled( this ) )
		{
			Fds_init( &this->fds );
			adjusted_gain = (adjusted_gain*3) / 4;
			
			static int const types [fds_osc_count] = {
				wave_type+0
			};
			append_voices( this, types, fds_osc_count );
		}
		
		if ( namco_enabled( this ) )
		{
			Namco_init( &this->namco );
			adjusted_gain = (adjusted_gain*3) / 4;
			
			static int const types [namco_osc_count] = {
				wave_type+3, wave_type+4, wave_type+5, wave_type+ 6,
				wave_type+7, wave_type+8, wave_type+9, wave_type+10,
			};
			append_voices( this, types, namco_osc_count );
		}
		
			#ifndef NSF_EMU_NO_VRC7
			if ( vrc7_enabled( this ) )
			{
				
				Vrc7_init( &this->vrc7 );
				Vrc7_set_rate( &this->vrc7, this->sample_rate );

				adjusted_gain = (adjusted_gain*3) / 4;
				
				static int const types [vrc7_osc_count] = {
					wave_type+3, wave_type+4, wave_type+5, wave_type+6,
					wave_type+7, wave_type+8
				};
				append_voices( this, types, vrc7_osc_count );
			}
			
			if ( vrc7_enabled( this )  ) Vrc7_volume( &this->vrc7,    adjusted_gain );
		#endif
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
static bool valid_tag( struct header_t* this )
{
	return 0 == memcmp( this->tag, "NESM\x1A", 5 );
}

// True if file supports only PAL speed
static bool pal_only( struct header_t* this )
{
	return (this->speed_flags & 3) == 1;
}
	
static int clock_rate( struct header_t* this )
{
	return pal_only( this ) ? (int)1662607.125 : (int)1789772.727272727;
}

static int play_period( struct header_t* this )
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
		clocks = (int) ((1LL * rate * clock_rate( this )) / 1000000);
	
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

blargg_err_t Nsf_load_mem( struct Nsf_Emu* this, void* data, long size )
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
	addr_t load_addr = get_addr( this->header.load_addr );
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

	// Set track_count
	this->track_count = this->header.track_count;
	
	// Change clock rate & setup buffer
	this->clock_rate__ = clock_rate( &this->header );
	Buffer_clock_rate( &this->stereo_buf, this->clock_rate__ );
	RETURN_ERR( Buffer_set_channel_count( &this->stereo_buf, this->voice_count, this->voice_types ) );
    this->buf_changed_count = Buffer_channels_changed_count( &this->stereo_buf );
	
	// Post load
	Sound_set_tempo( this, this->tempo );
	Sound_mute_voices( this, this->mute_mask_ );
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

static void map_memory( struct Nsf_Emu* this )
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

static void set_voice( struct Nsf_Emu* this, int i, struct Blip_Buffer* buf, struct Blip_Buffer* left, struct Blip_Buffer* right)
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
		
		#ifndef NSF_EMU_NO_VRC7
			if ( vrc7_enabled( this ) && (i -= vrc7_osc_count) < 0 )
			{
				Vrc7_set_output( &this->vrc7, i + vrc7_osc_count, buf );
				return;
			}
		#endif
	}
	#endif
}

// Emulation

// Music Emu

blargg_err_t Nsf_set_sample_rate( struct Nsf_Emu* this, int rate )
{
	require( !this->sample_rate ); // sample rate can't be changed once set
	Buffer_init( &this->stereo_buf );
	RETURN_ERR( Buffer_set_sample_rate( &this->stereo_buf, rate, 1000 / 20 ) );
	
	// Set bass frequency
	Buffer_bass_freq( &this->stereo_buf, 80 );
	
	this->sample_rate = rate;
	RETURN_ERR( track_init( &this->track_filter, this ) );
	this->tfilter.max_silence = 6 * stereo * this->sample_rate;
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
			struct channel_t ch = Buffer_channel( &this->stereo_buf, i );
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

static inline void push_byte( struct Nsf_Emu* this, int b )
{
	this->low_ram [0x100 + this->cpu.r.sp--] = b;
}

// Jumps to routine, given pointer to address in file header. Pushes idle_addr
// as return address, NOT old PC.
static void jsr_then_stop( struct Nsf_Emu* this, byte const addr [] )
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

void cpu_write( struct Nsf_Emu* this, addr_t addr, int data )
{
	#ifndef NSF_EMU_APU_ONLY
	{
		if ( fds_enabled( this) && (unsigned) (addr - fds_io_addr) < fds_io_size )
		{
			Fds_write( &this->fds, Cpu_time( &this->cpu ), addr, data );
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
				Namco_write_data( &this->namco, Cpu_time( &this->cpu ), data );
				return;
			}
		}
		
		if ( vrc6_enabled( this) )
		{
			int reg = addr & (vrc6_addr_step - 1);
			int osc = (unsigned) (addr - vrc6_base_addr) / vrc6_addr_step;
			if ( (unsigned) osc < vrc6_osc_count && (unsigned) reg < vrc6_reg_count )
			{
				Vrc6_write_osc( &this->vrc6, Cpu_time( &this->cpu ), osc, reg, data );
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
				Fme7_write_data( &this->fme7, Cpu_time( &this->cpu ), data );
				return;
			}
		}
		
		if ( mmc5_enabled( this) )
		{
			if ( (unsigned) (addr - mmc5_regs_addr) < mmc5_regs_size )
			{
				Mmc5_write_register( &this->mmc5, Cpu_time( &this->cpu ), addr, data );
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
		
		#ifndef NSF_EMU_NO_VRC7
			if ( vrc7_enabled( this) )
			{
				if ( addr == 0x9010 )
				{
					Vrc7_write_reg( &this->vrc7, data );
					return;
				}
			
				if ( (unsigned) (addr - 0x9028) <= 0x08 )
				{
					Vrc7_write_data( &this->vrc7, Cpu_time( &this->cpu ), data );
					return;
				}
			}
		#endif
	}
	#endif
	
	// Unmapped_write
}

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
		#ifndef NSF_EMU_NO_VRC7
			if ( vrc7_enabled( this )  ) Vrc7_reset( &this->vrc7 );
		#endif
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
	
	// convert filter times to samples
	struct setup_t s = this->tfilter;
	s.max_initial *= this->sample_rate * stereo;
	#ifdef GME_DISABLE_SILENCE_LOOKAHEAD
		s.lookahead = 1;
	#endif
	track_setup( &this->track_filter, &s );
	
	return track_start( &this->track_filter );
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

static void end_frame( struct Nsf_Emu* this, nes_time_t end )
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
		#ifndef NSF_EMU_NO_VRC7
		if ( vrc7_enabled( this )  ) Vrc7_end_frame( &this->vrc7, end );
		#endif
	#endif
}

// Tell/Seek

static int msec_to_samples( int msec, int sample_rate )
{
	int sec = msec / 1000;
	msec -= sec * 1000;
	return (sec * sample_rate + msec * sample_rate / 1000) * stereo;
}

int Track_tell( struct Nsf_Emu* this )
{
	int rate = this->sample_rate * stereo;
	int sec = track_sample_count( &this->track_filter ) / rate;
	return sec * 1000 + (track_sample_count( &this->track_filter ) - sec * rate) * 1000 / rate;
}

blargg_err_t Track_seek( struct Nsf_Emu* this, int msec )
{
	int time = msec_to_samples( msec, this->sample_rate );
	if ( time < track_sample_count( &this->track_filter ) )
	RETURN_ERR( Nsf_start_track( this, this->current_track ) );
	return Track_skip( this, time - track_sample_count( &this->track_filter ) );
}

blargg_err_t Track_skip( struct Nsf_Emu* this, int count )
{
	require( this->current_track >= 0 ); // start_track() must have been called already
	return track_skip( &this->track_filter, count );
}

blargg_err_t skip_( void *emu, int count )
{
	struct Nsf_Emu* this = (struct Nsf_Emu*) emu;
	
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

// Fading

void Track_set_fade( struct Nsf_Emu* this, int start_msec, int length_msec )
{
	track_set_fade( &this->track_filter, msec_to_samples( start_msec, this->sample_rate ),
		length_msec * this->sample_rate / (1000 / stereo) );
}

blargg_err_t Nsf_play( struct Nsf_Emu* this, int out_count, sample_t* out )
{
	require( this->current_track >= 0 );
	require( out_count % stereo == 0 );
	return track_play( &this->track_filter, out_count, out );
}

blargg_err_t play_( void* emu, int count, sample_t* out )
{
	struct Nsf_Emu* this = (struct Nsf_Emu*) emu;
	
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
			blip_time_t clocks_emulated = msec * this->clock_rate__ / 1000 - 100;
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
