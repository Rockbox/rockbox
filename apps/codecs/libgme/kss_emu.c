// Game_Music_Emu 0.5.5. http://www.slack.net/~ant/

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

long const clock_rate = 3579545;

const char gme_wrong_file_type [] = "Wrong file type for this emulator";

int const stereo = 2; // number of channels for stereo
int const silence_max = 6; // seconds
int const silence_threshold = 0x10;
long const fade_block_size = 512;
int const fade_shift = 8; // fade ends with gain at 1.0 / (1 << fade_shift)

void clear_track_vars( struct Kss_Emu* this )
{
	this->current_track   = -1;
	this->out_time         = 0;
	this->emu_time         = 0;
	this->emu_track_ended_ = true;
	this->track_ended      = true;
	this->fade_start       = INT_MAX / 2 + 1;
	this->fade_step        = 1;
	this->silence_time     = 0;
	this->silence_count    = 0;
	this->buf_remain       = 0;
	// warning(); // clear warning
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
	this->gain        = 1.0;
	this->chip_flags = 0;
	
	// defaults
	this->max_initial_silence = 2;
	this->silence_lookahead   = 6;
	this->ignore_silence     = false;
	
	this->voice_count = 0;
	clear_track_vars( this );
	
	memset( this->unmapped_read, 0xFF, sizeof this->unmapped_read );
	
	// Init all stuff
	Buffer_init( &this->stereo_buffer );
	
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
}

// Track info

static blargg_err_t check_kss_header( void const* header )
{
	if ( memcmp( header, "KSCC", 4 ) && memcmp( header, "KSSX", 4 ) )
		return gme_wrong_file_type;
	return 0;
}

// Setup

void update_gain( struct Kss_Emu* this )
{
	double g = this->gain;
	if ( msx_music_enabled( this ) || msx_audio_enabled( this ) 
	  || sms_fm_enabled( this ) )
	{
		g *= 0.75;
	}
	else
	{
		if ( this->scc_accessed )
			g *= 1.2;
	}
	
	if ( sms_psg_enabled( this ) ) Sms_apu_volume( &this->sms.psg, g );
	if ( sms_fm_enabled( this )  ) Opl_volume( &this->sms.fm, g );
	if ( msx_psg_enabled( this ) ) Ay_apu_volume( &this->msx.psg, g );
	if ( msx_scc_enabled( this ) ) Scc_volume( &this->msx.scc, g );
	if ( msx_music_enabled( this ) ) Opl_volume( &this->msx.music, g );
	if ( msx_audio_enabled( this ) ) Opl_volume( &this->msx.audio, g );
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
		}
	}

	this->silence_lookahead = 6;
	if ( sms_fm_enabled( this ) || msx_music_enabled( this ) || msx_audio_enabled( this ) )
	{
		if ( !Opl_supported() )
			; /* warning( "FM sound not supported" ); */
		else
			this->silence_lookahead = 3; // Opl_Apu is really slow
	}

	this->clock_rate_ = clock_rate;
	Buffer_clock_rate( &this->stereo_buffer, clock_rate );
	this->buf_changed_count = Buffer_channels_changed_count( &this->stereo_buffer );
	
	Sound_set_tempo( this, this->tempo );
	Sound_mute_voices( this, this->mute_mask_ );
	return 0;
}

void set_voice( struct Kss_Emu* this, int i, struct Blip_Buffer* center, struct Blip_Buffer* left, struct Blip_Buffer* right )
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
		if ( msx_music_enabled( this ) && i < opl_osc_count ) Opl_set_output( &this->msx.music, center );
		if ( msx_audio_enabled( this ) && i < opl_osc_count ) Opl_set_output( &this->msx.audio, center );
	}
}

// Emulation

void jsr( struct Kss_Emu* this, byte const addr [] )
{
	this->ram [--this->cpu.r.sp] = idle_addr >> 8;
	this->ram [--this->cpu.r.sp] = idle_addr & 0xFF;
	this->cpu.r.pc = get_le16( addr );
}

void set_bank( struct Kss_Emu* this, int logical, int physical )
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

blargg_err_t run_clocks( struct Kss_Emu* this, blip_time_t* duration_ )
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


blargg_err_t Kss_set_sample_rate( struct Kss_Emu* this, long rate )
{
	require( !this->sample_rate ); // sample rate can't be changed once set
	RETURN_ERR( Buffer_set_sample_rate( &this->stereo_buffer, rate, 1000 / 20 ) );
	
	// Set bass frequency
	Buffer_bass_freq( &this->stereo_buffer, 180 );
	this->sample_rate = rate;
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
			struct channel_t ch = Buffer_channel( &this->stereo_buffer );
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

void fill_buf( struct Kss_Emu* this );
blargg_err_t Kss_start_track( struct Kss_Emu* this, int track )
{
	clear_track_vars( this );
	
		// Remap track if playlist available
	if ( this->m3u.size > 0 ) {
		struct entry_t* e = &this->m3u.entries[track];
		track = e->track;
	}

	this->current_track = track;
	
	Buffer_clear( &this->stereo_buffer );
	
	if ( sms_psg_enabled( this ) ) Sms_apu_reset( &this->sms.psg, 0, 0 );
	if ( sms_fm_enabled( this )  ) Opl_reset( &this->sms.fm );
	if ( msx_psg_enabled( this ) ) Ay_apu_reset( &this->msx.psg );
	if ( msx_scc_enabled( this ) ) Scc_reset( &this->msx.scc );
	if ( msx_music_enabled( this ) ) Opl_reset( &this->msx.music );
	if ( msx_audio_enabled( this ) ) Opl_reset( &this->msx.audio );

	this->scc_accessed = false;
	update_gain( this );

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

// Tell/Seek

blargg_long msec_to_samples( blargg_long msec, long sample_rate )
{
	blargg_long sec = msec / 1000;
	msec -= sec * 1000;
	return (sec * sample_rate + msec * sample_rate / 1000) * stereo;
}

long Track_tell( struct Kss_Emu* this )
{
	blargg_long rate = this->sample_rate * stereo;
	blargg_long sec = this->out_time / rate;
	return sec * 1000 + (this->out_time - sec * rate) * 1000 / rate;
}

blargg_err_t Track_seek( struct Kss_Emu* this, long msec )
{
	blargg_long time = msec_to_samples( msec, this->sample_rate );
	if ( time < this->out_time )
		RETURN_ERR( Kss_start_track( this, this->current_track ) );
	return Track_skip( this, time - this->out_time );
}

blargg_err_t play_( struct Kss_Emu* this, long count, sample_t* out );
blargg_err_t skip_( struct Kss_Emu* this, long count )
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

blargg_err_t Track_skip( struct Kss_Emu* this, long count )
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
		if ( skip_( this, count ) )
			this->emu_track_ended_ = true;
	}
	
	if ( !(this->silence_count | this->buf_remain) ) // caught up to emulator, so update track ended
		this->track_ended |= this->emu_track_ended_;
	
	return 0;
}

// Fading

void Track_set_fade( struct Kss_Emu* this, long start_msec, long length_msec )
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

void handle_fade( struct Kss_Emu *this, long out_count, sample_t* out )
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

void emu_play( struct Kss_Emu* this, long count, sample_t* out )
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
void fill_buf( struct Kss_Emu* this )
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

blargg_err_t Kss_play( struct Kss_Emu* this, long out_count, sample_t* out )
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
			long ahead_time = this->silence_lookahead * (this->out_time + out_count -this->silence_time) + this->silence_time;
			while ( this->emu_time < ahead_time && !(this->buf_remain |this-> emu_track_ended_) )
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

blargg_err_t play_( struct Kss_Emu* this, long count, sample_t* out )
{
	long remain = count;
	while ( remain )
	{
		remain -= Buffer_read_samples( &this->stereo_buffer, &out [count - remain], remain );
		if ( remain )
		{
			if ( this->buf_changed_count != Buffer_channels_changed_count( &this->stereo_buffer ) )
			{
				this->buf_changed_count = Buffer_channels_changed_count( &this->stereo_buffer );
				Sound_mute_voices( this, this->mute_mask_ );
			}
			int msec = Buffer_length( &this->stereo_buffer );
			/* blip_time_t clocks_emulated = (blargg_long) msec * clock_rate_ / 1000; */
			blip_time_t clocks_emulated = msec * this->clock_rate_ / 1000 - 100;
			RETURN_ERR( run_clocks( this, &clocks_emulated ) );
			assert( clocks_emulated );
			Buffer_end_frame( &this->stereo_buffer, clocks_emulated );
		}
	}
	return 0;
}

