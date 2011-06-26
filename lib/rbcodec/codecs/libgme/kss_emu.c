// Game_Music_Emu 0.6-pre. http://www.slack.net/~ant/

#include "kss_emu.h"

#include "blargg_endian.h"

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

#include "blargg_source.h"

int const clock_rate = 3579545;

const char gme_wrong_file_type [] = "Wrong file type for this emulator";

static void clear_track_vars( struct Kss_Emu* this )
{
	this->current_track    = -1;
	track_stop( &this->track_filter );
}

static blargg_err_t init_opl_apu( enum opl_type_t type, struct Opl_Apu* out )
{
	blip_time_t const period = 72;
	int const rate = clock_rate / period;
	return Opl_init( out, rate * period, rate, period, type );
}

void Kss_init( struct Kss_Emu* this )
{
	this->sample_rate   = 0;
	this->mute_mask_    = 0;
	this->tempo       = (int)(FP_ONE_TEMPO);
	this->gain        = (int)FP_ONE_GAIN;
	this->chip_flags = 0;
	
	// defaults
	this->tfilter = *track_get_setup( &this->track_filter );
	this->tfilter.max_initial = 2;
	this->tfilter.lookahead   = 6;
	this->track_filter.silence_ignored_ = false;
	
	memset( this->unmapped_read, 0xFF, sizeof this->unmapped_read );
	
	// Init all stuff
	Buffer_init( &this->stereo_buf );
	
	Z80_init( &this->cpu );
	Rom_init( &this->rom, page_size );
	
	// Initialize all apus just once (?)
	Sms_apu_init( &this->sms.psg);
	Ay_apu_init( &this->msx.psg );
	Scc_init( &this->msx.scc );
	
#ifndef KSS_EMU_NO_FMOPL
	init_opl_apu( type_smsfmunit, &this->sms.fm );
	init_opl_apu( type_msxmusic, &this->msx.music );
	init_opl_apu( type_msxaudio, &this->msx.audio );
#endif

	this->voice_count = 0;
	this->voice_types = 0;
	clear_track_vars( this );
}

// Track info

static blargg_err_t check_kss_header( void const* header )
{
	if ( memcmp( header, "KSCC", 4 ) && memcmp( header, "KSSX", 4 ) )
		return gme_wrong_file_type;
	return 0;
}

// Setup

static void update_gain_( struct Kss_Emu* this )
{
	int g = this->gain;
	if ( msx_music_enabled( this ) || msx_audio_enabled( this ) 
	  || sms_fm_enabled( this ) )
	{
		g = (g*3) / 4; //g *= 0.75;
	}
	else
	{
		if ( this->scc_accessed )
			g = (g*6) / 5; //g *= 1.2;
	}
	
	if ( sms_psg_enabled( this ) ) Sms_apu_volume( &this->sms.psg, g );
	if ( sms_fm_enabled( this )  ) Opl_volume( &this->sms.fm, g );
	if ( msx_psg_enabled( this ) ) Ay_apu_volume( &this->msx.psg, g );
	if ( msx_scc_enabled( this ) ) Scc_volume( &this->msx.scc, g );
	if ( msx_music_enabled( this ) ) Opl_volume( &this->msx.music, g );
	if ( msx_audio_enabled( this ) ) Opl_volume( &this->msx.audio, g );
}

static void update_gain( struct Kss_Emu* this )
{
	if ( this->scc_accessed )
	{
		/* dprintf( "SCC accessed\n" ); */
		update_gain_( this );
	}
}

blargg_err_t Kss_load_mem( struct Kss_Emu* this, const void* data, long size )
{
	/* warning( core.warning() ); */
	memset( &this->header, 0, sizeof this->header );
	assert( offsetof (header_t,msx_audio_vol) == header_size - 1 );
	RETURN_ERR( Rom_load( &this->rom, data, size, header_base_size, &this->header, 0 ) );
	
	RETURN_ERR( check_kss_header( this->header.tag ) );
	
	this->chip_flags = 0;
	this->header.last_track [0] = 255;
	if ( this->header.tag [3] == 'C' )
	{
		if ( this->header.extra_header )
		{
			this->header.extra_header = 0;
			/* warning( "Unknown data in header" ); */
		}
		if ( this->header.device_flags & ~0x0F )
		{
			this->header.device_flags &= 0x0F;
			/* warning( "Unknown data in header" ); */
		}
	}
	else if ( this->header.extra_header )
	{
		if ( this->header.extra_header != header_ext_size )
		{
			this->header.extra_header = 0;
			/* warning( "Invalid extra_header_size" ); */
		}
		else
		{
			memcpy( this->header.data_size, this->rom.file_data, header_ext_size );
		}
	}
	
	#ifndef NDEBUG
	{
		int ram_mode = this->header.device_flags & 0x84; // MSX
		if ( this->header.device_flags & 0x02 ) // SMS
			ram_mode = (this->header.device_flags & 0x88);
		
		if ( ram_mode )
			blargg_dprintf_( "RAM not supported\n" ); // TODO: support
	}
	#endif

	this->track_count = get_le16( this->header.last_track ) + 1;
	this->m3u.size = 0;

	this->scc_enabled = false;
	if ( this->header.device_flags & 0x02 ) // Sega Master System
	{
		int const osc_count = sms_osc_count + opl_osc_count;

		// sms.psg
		this->voice_count = sms_osc_count;
		static int const types [sms_osc_count + opl_osc_count] = {
			wave_type+1, wave_type+3, wave_type+2, mixed_type+1, wave_type+0
		};
		this->voice_types = types;
		this->chip_flags |= sms_psg_flag;

		// sms.fm
		if ( this->header.device_flags & 0x01 )
		{
			this->voice_count = osc_count;
			this->chip_flags |= sms_fm_flag;
		}
	}
	else // MSX
	{
		int const osc_count = ay_osc_count + opl_osc_count;

		// msx.psg
		this->voice_count = ay_osc_count;
		static int const types [ay_osc_count + opl_osc_count] = {
			wave_type+1, wave_type+3, wave_type+2, wave_type+0
		};
		this->voice_types = types;
		this->chip_flags |= msx_psg_flag;

		/* if ( this->header.device_flags & 0x10 )
			warning( "MSX stereo not supported" ); */

		// msx.music
		if ( this->header.device_flags & 0x01 )
		{
			this->voice_count = osc_count;
			this->chip_flags |= msx_music_flag;
		}

	#ifndef KSS_EMU_NO_FMOPL
		// msx.audio
		if ( this->header.device_flags & 0x08 )
		{
			this->voice_count = osc_count;
			this->chip_flags |= msx_audio_flag;
		}
	#endif

		if ( !(this->header.device_flags & 0x80) )
		{
			if ( !(this->header.device_flags & 0x84) )
				this->scc_enabled = scc_enabled_true;

			// msx.scc
			this->chip_flags |= msx_scc_flag;
			this->voice_count = ay_osc_count + scc_osc_count;
			static int const types [ay_osc_count + scc_osc_count] = {
				wave_type+1, wave_type+3, wave_type+2,
				wave_type+0, wave_type+4, wave_type+5, wave_type+6, wave_type+7,
			};
			this->voice_types = types;
		}
	}

	this->tfilter.lookahead   = 6;
	if ( sms_fm_enabled( this ) || msx_music_enabled( this ) || msx_audio_enabled( this ) )
	{
		if ( !Opl_supported() )
			; /* warning( "FM sound not supported" ); */
		else
			this->tfilter.lookahead   = 3; // Opl_Apu is really slow
	}

	this->clock_rate_ = clock_rate;
	Buffer_clock_rate( &this->stereo_buf, clock_rate );
	RETURN_ERR( Buffer_set_channel_count( &this->stereo_buf, this->voice_count, this->voice_types ) );
	this->buf_changed_count = Buffer_channels_changed_count( &this->stereo_buf );
	
	Sound_set_tempo( this, this->tempo );
	Sound_mute_voices( this, this->mute_mask_ );
	return 0;
}

static void set_voice( struct Kss_Emu* this, int i, struct Blip_Buffer* center, struct Blip_Buffer* left, struct Blip_Buffer* right )
{
	if ( sms_psg_enabled( this ) ) // Sega Master System
	{
		i -= sms_osc_count;
		if ( i < 0 )
		{
			Sms_apu_set_output( &this->sms.psg, i + sms_osc_count, center, left, right );
			return;
		}

		if ( sms_fm_enabled( this ) && i < opl_osc_count )
			Opl_set_output( &this->sms.fm, center );
	}
	else if ( msx_psg_enabled( this ) ) // MSX
	{
		i -= ay_osc_count;
		if ( i < 0 )
		{
			Ay_apu_set_output( &this->msx.psg, i + ay_osc_count, center );
			return;
		}

		if ( msx_scc_enabled( this )   && i < scc_osc_count   ) Scc_set_output( &this->msx.scc, i, center );
		if ( msx_music_enabled( this ) && i < opl_osc_count )   Opl_set_output( &this->msx.music, center );
		if ( msx_audio_enabled( this ) && i < opl_osc_count )   Opl_set_output( &this->msx.audio, center );
	}
}

// Emulation

void jsr( struct Kss_Emu* this, byte const addr [] )
{
	this->ram [--this->cpu.r.sp] = idle_addr >> 8;
	this->ram [--this->cpu.r.sp] = idle_addr & 0xFF;
	this->cpu.r.pc = get_le16( addr );
}

static void set_bank( struct Kss_Emu* this, int logical, int physical )
{
	int const bank_size = (16 * 1024L) >> (this->header.bank_mode >> 7 & 1);
	
	int addr = 0x8000;
	if ( logical && bank_size == 8 * 1024 )
		addr = 0xA000;
	
	physical -= this->header.first_bank;
	if ( (unsigned) physical >= (unsigned) this->bank_count )
	{
		byte* data = this->ram + addr;
		Z80_map_mem( &this->cpu, addr, bank_size, data, data );
	}
	else
	{
		int offset, phys = physical * bank_size;
		for ( offset = 0; offset < bank_size; offset += page_size )
			Z80_map_mem( &this->cpu, addr + offset, page_size,
					this->unmapped_write, Rom_at_addr( &this->rom, phys + offset ) );

	}
}

void cpu_write( struct Kss_Emu* this, addr_t addr, int data )
{
	*Z80_write( &this->cpu, addr ) = data;
	if ( (addr & this->scc_enabled) == 0x8000 ) {
		// TODO: SCC+ support

		data &= 0xFF;
		switch ( addr )
		{
		case 0x9000:
			set_bank( this, 0, data );
			return;

		case 0xB000:
			set_bank( this, 1, data );
			return;

		case 0xBFFE: // selects between mapping areas (we just always enable both)
			if ( data == 0 || data == 0x20 )
				return;
		}

		int scc_addr = (addr & 0xDFFF) - 0x9800;
		if ( msx_scc_enabled( this ) && (unsigned) scc_addr < 0xB0 )
		{
			this->scc_accessed = true;
			//if ( (unsigned) (scc_addr - 0x90) < 0x10 )
			//  scc_addr -= 0x10; // 0x90-0x9F mirrors to 0x80-0x8F
			if ( scc_addr < scc_reg_count )
				Scc_write( &this->msx.scc, Z80_time( &this->cpu ), addr, data );
			return;
		}
	}
}

void cpu_out( struct Kss_Emu* this, kss_time_t time, kss_addr_t addr, int data )
{
	data &= 0xFF;
	switch ( addr & 0xFF )
	{
	case 0xA0:
		if ( msx_psg_enabled( this ) )
			Ay_apu_write_addr( &this->msx.psg, data );
		return;

	case 0xA1:
		if ( msx_psg_enabled( this ) )
			Ay_apu_write_data( &this->msx.psg, time, data );
		return;

	case 0x06:
		if ( sms_psg_enabled( this ) && (this->header.device_flags & 0x04) )
		{
			Sms_apu_write_ggstereo( &this->sms.psg, time, data );
			return;
		}
		break;

	case 0x7E:
	case 0x7F:
		if ( sms_psg_enabled( this ) )
		{
			Sms_apu_write_data( &this->sms.psg, time, data );
			return;
		}
		break;

	#define OPL_WRITE_HANDLER( base, name, opl )\
		case base  : if ( name##_enabled( this ) ) { Opl_write_addr( opl,       data ); return; } break;\
		case base+1: if ( name##_enabled( this ) ) { Opl_write_data( opl, time, data ); return; } break;

	OPL_WRITE_HANDLER( 0x7C, msx_music, &this->msx.music )
	OPL_WRITE_HANDLER( 0xC0, msx_audio, &this->msx.audio )
	OPL_WRITE_HANDLER( 0xF0, sms_fm, &this->sms.fm    )

	case 0xFE:
		set_bank( this, 0, data );
		return;

	#ifndef NDEBUG
	case 0xA8: // PPI
		return;
	#endif
	}

	/* cpu_out( time, addr, data ); */
}

int cpu_in( struct Kss_Emu* this, kss_time_t time, kss_addr_t addr )
{
	switch ( addr & 0xFF )
	{
	case 0xC0:
	case 0xC1:
		if ( msx_audio_enabled( this ) )
			return Opl_read( &this->msx.audio, time, addr & 1 );
		break;

	case 0xA2:
		if ( msx_psg_enabled( this ) )
			return Ay_apu_read( &this->msx.psg );
		break;

	#ifndef NDEBUG
	case 0xA8: // PPI
		return 0;
	#endif
	}

	/* return cpu_in( time, addr ); */
	return 0xFF;
}

static blargg_err_t run_clocks( struct Kss_Emu* this, blip_time_t* duration_ )
{
	blip_time_t duration = *duration_;
	RETURN_ERR( end_frame( this, duration ) );

	if ( sms_psg_enabled( this ) ) Sms_apu_end_frame( &this->sms.psg, duration );
	if ( sms_fm_enabled( this )  ) Opl_end_frame( &this->sms.fm, duration );
	if ( msx_psg_enabled( this ) ) Ay_apu_end_frame( &this->msx.psg, duration );
	if ( msx_scc_enabled( this ) ) Scc_end_frame( &this->msx.scc, duration );
	if ( msx_music_enabled( this ) ) Opl_end_frame( &this->msx.music, duration );
	if ( msx_audio_enabled( this ) ) Opl_end_frame( &this->msx.audio, duration );

	return 0;
}

blargg_err_t end_frame( struct Kss_Emu* this, kss_time_t end )
{
	while ( Z80_time( &this->cpu ) < end )
	{
		kss_time_t next = min( end, this->next_play );
		run_cpu( this, next );
		if ( this->cpu.r.pc == idle_addr )
			Z80_set_time( &this->cpu, next );
		
		if ( Z80_time( &this->cpu ) >= this->next_play )
		{
			this->next_play += this->play_period;
			if ( this->cpu.r.pc == idle_addr )
			{
				if ( !this->gain_updated )
				{
					this->gain_updated = true;
					update_gain( this );
				}
				
				jsr( this, this->header.play_addr );
			}
		}
	}
	
	this->next_play -= end;
	check( this->next_play >= 0 );
	Z80_adjust_time( &this->cpu, -end );

	return 0;
}

// MUSIC

blargg_err_t Kss_set_sample_rate( struct Kss_Emu* this, int rate )
{
	require( !this->sample_rate ); // sample rate can't be changed once set
	RETURN_ERR( Buffer_set_sample_rate( &this->stereo_buf, rate, 1000 / 20 ) );
	
	// Set bass frequency
	Buffer_bass_freq( &this->stereo_buf, 180 );
	
	this->sample_rate = rate;
	RETURN_ERR( track_init( &this->track_filter, this ) );
	this->tfilter.max_silence = 6 * stereo * this->sample_rate;
	return 0;
}

void Sound_mute_voice( struct Kss_Emu* this, int index, bool mute )
{
	require( (unsigned) index < (unsigned) this->voice_count );
	int bit = 1 << index;
	int mask = this->mute_mask_ | bit;
	if ( !mute )
		mask ^= bit;
	Sound_mute_voices( this, mask );
}

void Sound_mute_voices( struct Kss_Emu* this, int mask )
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

void Sound_set_tempo( struct Kss_Emu* this, int t )
{
	require( this->sample_rate ); // sample rate must be set first
	int const min = (int)(FP_ONE_TEMPO*0.02);
	int const max = (int)(FP_ONE_TEMPO*4.00);
	if ( t < min ) t = min;
	if ( t > max ) t = max;
	this->tempo = t;
	
	blip_time_t period =
		(this->header.device_flags & 0x40 ? clock_rate / 50 : clock_rate / 60);
	this->play_period = (blip_time_t) ((period * FP_ONE_TEMPO) / t);
}

blargg_err_t Kss_start_track( struct Kss_Emu* this, int track )
{
	clear_track_vars( this );
	
		// Remap track if playlist available
	if ( this->m3u.size > 0 ) {
		struct entry_t* e = &this->m3u.entries[track];
		track = e->track;
	}

	this->current_track = track;
	
	Buffer_clear( &this->stereo_buf );
	
	if ( sms_psg_enabled( this ) ) Sms_apu_reset( &this->sms.psg, 0, 0 );
	if ( sms_fm_enabled( this )  ) Opl_reset( &this->sms.fm );
	if ( msx_psg_enabled( this ) ) Ay_apu_reset( &this->msx.psg );
	if ( msx_scc_enabled( this ) ) Scc_reset( &this->msx.scc );
	if ( msx_music_enabled( this ) ) Opl_reset( &this->msx.music );
	if ( msx_audio_enabled( this ) ) Opl_reset( &this->msx.audio );

	this->scc_accessed = false;
	update_gain_( this );

	memset( this->ram, 0xC9, 0x4000 );
	memset( this->ram + 0x4000, 0, sizeof this->ram - 0x4000 );
	
	// copy driver code to lo RAM
	static byte const bios [] = {
		0xD3, 0xA0, 0xF5, 0x7B, 0xD3, 0xA1, 0xF1, 0xC9, // $0001: WRTPSG
		0xD3, 0xA0, 0xDB, 0xA2, 0xC9                    // $0009: RDPSG
	};
	static byte const vectors [] = {
		0xC3, 0x01, 0x00,   // $0093: WRTPSG vector
		0xC3, 0x09, 0x00,   // $0096: RDPSG vector
	};
	memcpy( this->ram + 0x01, bios,    sizeof bios );
	memcpy( this->ram + 0x93, vectors, sizeof vectors );
	
	// copy non-banked data into RAM
	int load_addr = get_le16( this->header.load_addr );
	int orig_load_size = get_le16( this->header.load_size );
	int load_size = min( orig_load_size, (int) this->rom.file_size );
	load_size = min( load_size, (int) mem_size - load_addr );
	/* if ( load_size != orig_load_size )
		warning( "Excessive data size" ); */
	memcpy( this->ram + load_addr, this->rom.file_data + this->header.extra_header, load_size );
	
	Rom_set_addr( &this->rom, -load_size - this->header.extra_header );
	
	// check available bank data
	int const bank_size = (16 * 1024L) >> (this->header.bank_mode >> 7 & 1);
	int max_banks = (this->rom.file_size - load_size + bank_size - 1) / bank_size;
	this->bank_count = this->header.bank_mode & 0x7F;
	if ( this->bank_count > max_banks )
	{
		this->bank_count = max_banks;
		/* warning( "Bank data missing" ); */
	}
	//dprintf( "load_size : $%X\n", load_size );
	//dprintf( "bank_size : $%X\n", bank_size );
	//dprintf( "bank_count: %d (%d claimed)\n", bank_count, this->header.bank_mode & 0x7F );
	
	this->ram [idle_addr] = 0xFF;
	Z80_reset( &this->cpu, this->unmapped_write, this->unmapped_read );
	Z80_map_mem( &this->cpu, 0, mem_size, this->ram, this->ram );
	
	this->cpu.r.sp = 0xF380;
	this->cpu.r.b.a = track;
	this->cpu.r.b.h = 0;
	this->next_play = this->play_period;
	this->gain_updated = false;
	jsr( this, this->header.init_addr );
	
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

int Track_tell( struct Kss_Emu* this )
{
	int rate = this->sample_rate * stereo;
	int sec = track_sample_count( &this->track_filter ) / rate;
	return sec * 1000 + (track_sample_count( &this->track_filter ) - sec * rate) * 1000 / rate;
}

blargg_err_t Track_seek( struct Kss_Emu* this, int msec )
{
	int time = msec_to_samples( msec, this->sample_rate );
	if ( time < track_sample_count( &this->track_filter ) )
	RETURN_ERR( Kss_start_track( this, this->current_track ) );
	return Track_skip( this, time - track_sample_count( &this->track_filter ) );
}

blargg_err_t skip_( void *emu, int count )
{
	struct Kss_Emu* this = (struct Kss_Emu*) emu;
	
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

blargg_err_t Track_skip( struct Kss_Emu* this, int count )
{
	require( this->current_track >= 0 ); // start_track() must have been called already
	return track_skip( &this->track_filter, count );
}

void Track_set_fade( struct Kss_Emu* this, int start_msec, int length_msec )
{
	track_set_fade( &this->track_filter, msec_to_samples( start_msec, this->sample_rate ),
		length_msec * this->sample_rate / (1000 / stereo) );
}

blargg_err_t Kss_play( struct Kss_Emu* this, int out_count, sample_t* out )
{
	require( this->current_track >= 0 );
	require( out_count % stereo == 0 );
	return track_play( &this->track_filter, out_count, out );
}

blargg_err_t play_( void *emu, int count, sample_t* out )
{
	struct Kss_Emu* this = (struct Kss_Emu*) emu;
	
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
