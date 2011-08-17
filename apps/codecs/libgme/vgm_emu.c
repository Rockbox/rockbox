// Game_Music_Emu 0.5.5. http://www.slack.net/~ant/

#include "vgm_emu.h"

#include "blargg_endian.h"
#include <string.h>
#include <math.h>

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

const char* const gme_wrong_file_type = "Wrong file type for this emulator";

int const fm_gain = 3; // FM emulators are internally quieter to avoid 16-bit overflow

int const silence_max = 6; // seconds
int const silence_threshold = 0x10;
long const fade_block_size = 512;
int const fade_shift = 8; // fade ends with gain at 1.0 / (1 << fade_shift)

// VGM commands (Spec v1.50)
enum {
	cmd_gg_stereo       = 0x4F,
	cmd_psg             = 0x50,
	cmd_ym2413          = 0x51,
	cmd_ym2612_port0    = 0x52,
	cmd_ym2612_port1    = 0x53,
	cmd_ym2151          = 0x54,
	cmd_delay           = 0x61,
	cmd_delay_735       = 0x62,
	cmd_delay_882       = 0x63,
	cmd_byte_delay      = 0x64,
	cmd_end             = 0x66,
	cmd_data_block      = 0x67,
	cmd_short_delay     = 0x70,
	cmd_pcm_delay       = 0x80,
	cmd_pcm_seek        = 0xE0,
	
	pcm_block_type      = 0x00,
	ym2612_dac_port     = 0x2A,
	ym2612_dac_pan_port = 0xB6
};

static void clear_track_vars( struct Vgm_Emu* this )
{
	this->out_time         = 0;
	this->emu_time         = 0;
	this->emu_track_ended_ = true;
	this->track_ended     = true;
	this->fade_start       = INT_MAX / 2 + 1;
	this->fade_step        = 1;
	this->silence_time     = 0;
	this->silence_count    = 0;
	this->buf_remain       = 0;
}

int play_frame( struct Vgm_Emu* this, blip_time_t blip_time, int sample_count, sample_t* buf );
static int play_frame_( void* data, blip_time_t blip_time, int sample_count, short int* buf )
{
	return play_frame( (struct Vgm_Emu*) data, blip_time, sample_count, buf );
} 

void Vgm_init( struct Vgm_Emu* this )
{
	this->sample_rate = 0;
	this->mute_mask_ = 0;
	this->tempo = (int)(FP_ONE_TEMPO);
	
	// defaults
	this->max_initial_silence = 2;
	this->silence_lookahead = 1; // tracks should already be trimmed
	this->ignore_silence     = false;

	// Disable oversampling by default
	this->disable_oversampling = true;
	this->psg_rate   = 0;
	
	Sms_apu_init( &this->psg );
	Synth_init( &this->pcm );
	
	Buffer_init( &this->buf );
	Blip_init( &this->blip_buf );

	// Init fm chips	
	Ym2413_init( &this->ym2413 );
	Ym2612_init( &this->ym2612 );
	
	// Init resampler
	Resampler_init( &this->resampler );
	Resampler_set_callback( &this->resampler, play_frame_, this );

	// Set sound gain, a value too high
	// will cause saturation
	Sound_set_gain(this, (int)(FP_ONE_GAIN*0.7));
	
	// Unload
	this->voice_count = 0;
	clear_track_vars( this );	
}

// Track info

static byte const* skip_gd3_str( byte const* in, byte const* end )
{
	while ( end - in >= 2 )
	{
		in += 2;
		if ( !(in [-2] | in [-1]) )
			break;
	}
	return in;
}

static byte const* get_gd3_str( byte const* in, byte const* end, char* field )
{
	byte const* mid = skip_gd3_str( in, end );
	int i, len = (mid - in) / 2 - 1;
	if ( field && len > 0 )
	{
		len = min( len, (int) gme_max_field );
		field [len] = 0;
		for ( i = 0; i < len; i++ )
			field [i] = (in [i * 2 + 1] ? '?' : in [i * 2]); // TODO: convert to utf-8
	}
	return mid;
}

static byte const* get_gd3_pair( byte const* in, byte const* end, char* field )
{
	return skip_gd3_str( get_gd3_str( in, end, field ), end );
}

static void parse_gd3( byte const* in, byte const* end, struct track_info_t* out )
{
	in = get_gd3_pair( in, end, out->song );
	in = get_gd3_pair( in, end, out->game );
	in = get_gd3_pair( in, end, NULL ); // Skip system
	in = get_gd3_pair( in, end, out->author );
}

int const gd3_header_size = 12;

static long check_gd3_header( byte const* h, long remain )
{
	if ( remain < gd3_header_size ) return 0;
	if ( memcmp( h, "Gd3 ", 4 ) ) return 0;
	if ( get_le32( h + 4 ) >= 0x200 ) return 0;
	
	long gd3_size = get_le32( h + 8 );
	if ( gd3_size > remain - gd3_header_size )
		gd3_size = remain - gd3_header_size;
	return gd3_size;
}

static byte const* gd3_data( struct Vgm_Emu* this, int* size )
{
	if ( size )
		*size = 0;
	
	long gd3_offset = get_le32( header( this )->gd3_offset ) - 0x2C;
	if ( gd3_offset < 0 )
		return 0;
	
	byte const* gd3 = this->file_begin + header_size + gd3_offset;
	long gd3_size = check_gd3_header( gd3, this->file_end - gd3 );
	if ( !gd3_size )
		return 0;
	
	if ( size )
		*size = gd3_size + gd3_header_size;
	
	return gd3;
}

static void get_vgm_length( struct header_t const* h, struct track_info_t* out )
{
	long length = get_le32( h->track_duration ) * 10 / 441;
	if ( length > 0 )
	{
		long loop = get_le32( h->loop_duration );
		if ( loop > 0 && get_le32( h->loop_offset ) )
		{
			out->loop_length = loop * 10 / 441;
			out->intro_length = length - out->loop_length;
		}
		else
		{
			out->length = length; // 1000 / 44100 (VGM files used 44100 as timebase)
			out->intro_length = length; // make it clear that track is no longer than length
			out->loop_length = 0;
		}
	}
}

static blargg_err_t track_info( struct Vgm_Emu* this, struct track_info_t* out )
{
	memset(out, 0, sizeof out);
	get_vgm_length( header( this ), out );
	
	int size;
	byte const* gd3 = gd3_data( this, &size );
	if ( gd3 )
		parse_gd3( gd3 + gd3_header_size, gd3 + size, out );
	
	return 0;
}

static blargg_err_t check_vgm_header( struct header_t* h )
{
	if ( memcmp( h->tag, "Vgm ", 4 ) )
		return gme_wrong_file_type;
	return 0;
}

static void set_voice( struct Vgm_Emu* this, int i, struct Blip_Buffer* c, struct Blip_Buffer* l, struct Blip_Buffer* r )
{
	if ( i < sms_osc_count ) {
		int j;
		for ( j = sms_osc_count; --j >= 0; )
			Sms_apu_set_output( &this->psg, j, c, l, r );
	}
}

blargg_err_t setup_fm( struct Vgm_Emu* this );
blargg_err_t Vgm_load_mem( struct Vgm_Emu* this, byte const* new_data, long new_size, bool parse_info )
{
	// Unload
	this->voice_count = 0;
	clear_track_vars( this );
	
	// Clear info
	memset( &this->info, 0, sizeof this->info );

	assert( offsetof (struct header_t,unused2 [8]) == header_size );
	
	if ( new_size <= header_size )
		return gme_wrong_file_type;
			
	// Reset data pointers
	this->file_begin = new_data;
	this->file_end = new_data + new_size;
	
	struct header_t* h = (struct header_t*) new_data;
	RETURN_ERR( check_vgm_header( h ) );
	check( get_le32( h.version ) <= 0x150 );
	
	// If this was VGZ file gd3 parse info
	if ( parse_info ) {
 		track_info( this, &this->info );
		
		// If file was trimmed add an
		// incomplete token to the game tag
		if ( get_le32( h->data_size ) > (unsigned) new_size ) {
			*((char *) this->file_end) = cmd_end;
			strcat(this->info.game, "(Trimmed VGZ file)" );
		}
	}
	
	// Get loop
	this->loop_begin = this->file_end;
	
	// If file was trimmed don't loop
	if ( get_le32( h->loop_offset ) && get_le32( h->data_size ) <= (unsigned) new_size ) 
		this->loop_begin = &new_data [get_le32( h->loop_offset ) + offsetof (struct header_t,loop_offset)];

	// PSG rate
	this->psg_rate = get_le32( h->psg_rate );
	if ( !this->psg_rate )
		this->psg_rate = 3579545;
	
	Blip_set_clock_rate( &this->blip_buf, this->psg_rate );
	
	// Disable FM
	this->fm_rate = 0;
	Ym2612_enable( &this->ym2612, false );
	Ym2413_enable( &this->ym2413, false );
	
	Sound_set_tempo( this, (int)(FP_ONE_TEMPO) );
	
	this->voice_count = sms_osc_count;
	
	RETURN_ERR( setup_fm( this ) );
	
	// do after FM in case output buffer is changed
	// setup buffer
	this->clock_rate_ = this->psg_rate;
	Buffer_clock_rate( &this->buf, this->psg_rate );
	
	// Setup bass
	this->buf_changed_count = Buffer_channels_changed_count( &this->buf );
	
	// Post load
	Sound_set_tempo( this, this->tempo );
	Sound_mute_voices( this, this->mute_mask_ );
	
	return 0;
}

void update_fm_rates( struct Vgm_Emu* this, int* ym2413_rate, int* ym2612_rate );
static blargg_err_t init_fm( struct Vgm_Emu* this, int* rate )
{
	int ym2612_rate = get_le32( header( this )->ym2612_rate );
	int ym2413_rate = get_le32( header( this )->ym2413_rate );
	if ( ym2413_rate && get_le32( header( this )->version ) < 0x110 )
		update_fm_rates( this, &ym2413_rate, &ym2612_rate );
	
	if ( ym2612_rate )
	{
		if ( !*rate )
			*rate = ym2612_rate / 144;
		RETURN_ERR( Ym2612_set_rate( &this->ym2612, *rate, ym2612_rate ) );
		Ym2612_enable( &this->ym2612, true );
	}
	else if ( ym2413_rate )
	{
		if ( !*rate )
			*rate = ym2413_rate / 72;
		int result = Ym2413_set_rate( &this->ym2413, *rate, ym2413_rate );
		if ( result == 2 )
			return "YM2413 FM sound not supported";
		CHECK_ALLOC( !result );
		Ym2413_enable( &this->ym2413, true );
	}
	
	this->fm_rate = *rate;
	
	return 0;
}

blargg_err_t setup_fm( struct Vgm_Emu* this )
{
	int fm_rate = 0;
	if ( !this->disable_oversampling )
		fm_rate = (this->sample_rate * 3) / 2; // oversample factor = 1.5
	RETURN_ERR( init_fm( this, &fm_rate ) );
	
	if ( uses_fm( this ) )
	{
		this->voice_count = 8;
		RETURN_ERR( Resampler_setup( &this->resampler, fm_rate, fm_gain, this->sample_rate, this->gain ) );
		RETURN_ERR( Resampler_reset( &this->resampler, Blip_length( &this->blip_buf ) * this->sample_rate / 1000 ) );
		Sms_apu_volume( &this->psg, ((this->gain/5)-(this->gain*5)/1000) * fm_gain );
	}
	else
	{
		Sms_apu_volume( &this->psg, this->gain );
	}
	
	return 0;
}

// Emulation

blip_time_t run( struct Vgm_Emu* this, vgm_time_t end_time );
static blargg_err_t run_clocks( struct Vgm_Emu* this, blip_time_t* time_io, int msec )
{
	*time_io = run( this, msec * this->vgm_rate / 1000 );
	Sms_apu_end_frame( &this->psg, *time_io );
	return 0;
}



static blargg_err_t play_( struct Vgm_Emu* this, long count, sample_t* out )
{
	if ( !uses_fm( this ) ) {
		long remain = count;
		while ( remain )
		{
			remain -= Buffer_read_samples( &this->buf, &out [count - remain], remain );
			if ( remain )
			{	
				if ( this->buf_changed_count != Buffer_channels_changed_count( &this->buf ) )
				{
					this->buf_changed_count = Buffer_channels_changed_count( &this->buf );
					
					// Remute voices
					Sound_mute_voices( this, this->mute_mask_ );
				}
				int msec = Buffer_length( &this->buf );
				blip_time_t clocks_emulated = (blargg_long) msec * this->clock_rate_ / 1000 - 100;
				RETURN_ERR( run_clocks( this, &clocks_emulated, msec ) );
				assert( clocks_emulated );
				Buffer_end_frame( &this->buf, clocks_emulated );
			}
		}
		
		return 0;
	}
		
	Resampler_play( &this->resampler, count, out, &this->blip_buf );
	return 0;
}

// Vgm_Emu_impl

static inline int command_len( int command )
{
	static byte const lens [0x10] = {
	// 0 1 2 3 4 5 6 7 8 9 A B C D E F
	   1,1,1,2,2,3,1,1,1,1,3,3,4,4,5,5
	};
	int len = lens [command >> 4];
	check( len != 1 );
	return len;
}
	
static inline fm_time_t to_fm_time( struct Vgm_Emu* this, vgm_time_t t )
{
	return (t * this->fm_time_factor + this->fm_time_offset) >> fm_time_bits;
}

static inline blip_time_t to_psg_time( struct Vgm_Emu* this, vgm_time_t t )
{
	return (t * this->blip_time_factor) >> blip_time_bits;
}

static void write_pcm( struct Vgm_Emu* this, vgm_time_t vgm_time, int amp )
{
	check( amp >= 0 );
	blip_time_t blip_time = to_psg_time( this, vgm_time );
	int old = this->dac_amp;
	int delta = amp - old;
	this->dac_amp = amp;
	Blip_set_modified( &this->blip_buf );
	if ( old >= 0 ) // first write is ignored, to avoid click
		Synth_offset_inline( &this->pcm, blip_time, delta, &this->blip_buf );
	else
		this->dac_amp |= this->dac_disabled;
}

blip_time_t run( struct Vgm_Emu* this, vgm_time_t end_time )
{
	vgm_time_t vgm_time = this->vgm_time; 
	byte const* pos = this->pos;
	if ( pos >= this->file_end )
	{
		this->emu_track_ended_ = true;
		/* if ( pos > data_end )
			warning( "Stream lacked end event" ); */
	}
	
	while ( vgm_time < end_time && pos < this->file_end )
	{
		// TODO: be sure there are enough bytes left in stream for particular command
		// so we don't read past end
		switch ( *pos++ )
		{
		case cmd_end:
			pos = this->loop_begin; // if not looped, loop_begin == data_end
			break;
		
		case cmd_delay_735:
			vgm_time += 735;
			break;
		
		case cmd_delay_882:
			vgm_time += 882;
			break;
		
		case cmd_gg_stereo:
			Sms_apu_write_ggstereo( &this->psg, to_psg_time( this, vgm_time ), *pos++ );
			break;
		
		case cmd_psg:
			Sms_apu_write_data( &this->psg, to_psg_time( this, vgm_time ), *pos++ );
			break;
		
		case cmd_delay:
			vgm_time += pos [1] * 0x100 + pos [0];
			pos += 2;
			break;
		
		case cmd_byte_delay:
			vgm_time += *pos++;
			break;
		
		case cmd_ym2413:
			if ( Ym2413_run_until( &this->ym2413, to_fm_time( this, vgm_time ) ) )
				Ym2413_write( &this->ym2413, pos [0], pos [1] );
			pos += 2;
			break;
		
		case cmd_ym2612_port0:
			if ( pos [0] == ym2612_dac_port )
			{
				write_pcm( this, vgm_time, pos [1] );
			}
			else if ( Ym2612_run_until( &this->ym2612, to_fm_time( this, vgm_time ) ) )
			{
				if ( pos [0] == 0x2B )
				{
					this->dac_disabled = (pos [1] >> 7 & 1) - 1;
					this->dac_amp |= this->dac_disabled;
				}
				Ym2612_write0( &this->ym2612, pos [0], pos [1] );
			}
			pos += 2;
			break;
		
		case cmd_ym2612_port1:
			if ( Ym2612_run_until( &this->ym2612, to_fm_time( this, vgm_time ) ) )
				Ym2612_write1( &this->ym2612, pos [0], pos [1] );
			pos += 2;
			break;
			
		case cmd_data_block: {
			check( *pos == cmd_end );
			int type = pos [1];
			long size = get_le32( pos + 2 );
			pos += 6;
			if ( type == pcm_block_type )
				this->pcm_data = pos;
			pos += size;
			break;
		}
		
		case cmd_pcm_seek:
			this->pcm_pos = this->pcm_data + pos [3] * 0x1000000 + pos [2] * 0x10000 +
					pos [1] * 0x100 + pos [0];
			pos += 4;
			break;
		
		default: {
				int cmd = pos [-1];
				switch ( cmd & 0xF0 )
				{
					case cmd_pcm_delay:
						write_pcm( this, vgm_time, *this->pcm_pos++ );
						vgm_time += cmd & 0x0F;
						break;
					
					case cmd_short_delay:
						vgm_time += (cmd & 0x0F) + 1;
						break;
					
					case 0x50:
						pos += 2;
						break;
					
					default:
						pos += command_len( cmd ) - 1;
						/* warning( "Unknown stream event" ); */
				}
			}
		}
	}
	vgm_time -= end_time;
	this->pos = pos;
	this->vgm_time = vgm_time;
	
	return to_psg_time( this, end_time );
}

int play_frame( struct Vgm_Emu* this, blip_time_t blip_time, int sample_count, blip_sample_t out [] )
{
	// to do: timing is working mostly by luck
	int min_pairs = (unsigned) sample_count / 2;
	int vgm_time = (min_pairs << fm_time_bits) / this->fm_time_factor - 1;
	assert( to_fm_time( this, vgm_time ) <= min_pairs );
	int pairs;
	while ( (pairs = to_fm_time( this, vgm_time )) < min_pairs )
		vgm_time++;
	//debug_printf( "pairs: %d, min_pairs: %d\n", pairs, min_pairs );
	
	if ( Ym2612_enabled( &this->ym2612 ) )
	{
		Ym2612_begin_frame( &this->ym2612, out );
		memset( out, 0, pairs * stereo * sizeof *out );
	}
	else if ( Ym2413_enabled( &this->ym2413 ) )
	{
		Ym2413_begin_frame( &this->ym2413, out );
	}
	
	run( this, vgm_time );
	Ym2612_run_until( &this->ym2612, pairs );
	Ym2413_run_until( &this->ym2413, pairs );
	
	this->fm_time_offset = (vgm_time * this->fm_time_factor + this->fm_time_offset) - (pairs << fm_time_bits);
	
	Sms_apu_end_frame( &this->psg, blip_time );
	
	return pairs * stereo;
}

// Update pre-1.10 header FM rates by scanning commands
void update_fm_rates( struct Vgm_Emu* this, int* ym2413_rate, int* ym2612_rate )
{
	byte const* p = this->file_begin + 0x40;
	while ( p < this->file_end )
	{
		switch ( *p )
		{
		case cmd_end:
			return;
		
		case cmd_psg:
		case cmd_byte_delay:
			p += 2;
			break;
		
		case cmd_delay:
			p += 3;
			break;
		
		case cmd_data_block:
			p += 7 + get_le32( p + 3 );
			break;
		
		case cmd_ym2413:
			*ym2612_rate = 0;
			return;
		
		case cmd_ym2612_port0:
		case cmd_ym2612_port1:
			*ym2612_rate = *ym2413_rate;
			*ym2413_rate = 0;
			return;
		
		case cmd_ym2151:
			*ym2413_rate = 0;
			*ym2612_rate = 0;
			return;
		
		default:
			p += command_len( *p );
		}
	}
}


// Music Emu

blargg_err_t Vgm_set_sample_rate( struct Vgm_Emu* this, long rate )
{
	require( !this->sample_rate ); // sample rate can't be changed once set
	RETURN_ERR( Blip_set_sample_rate( &this->blip_buf, rate, 1000 / 30 ) );
	RETURN_ERR( Buffer_set_sample_rate( &this->buf, rate, 1000 / 20 ) );
	
	// Set bass frequency
	Buffer_bass_freq( &this->buf, 80 );
	
	this->sample_rate = rate;
	return 0;
}

void Sound_mute_voice( struct Vgm_Emu* this, int index, bool mute )
{
	require( (unsigned) index < (unsigned) this->voice_count );
	int bit = 1 << index;
	int mask = this->mute_mask_ | bit;
	if ( !mute )
		mask ^= bit;
	Sound_mute_voices( this, mask );
}

void Sound_mute_voices( struct Vgm_Emu* this, int mask )
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
			struct channel_t ch = Buffer_channel( &this->buf );
			assert( (ch.center && ch.left && ch.right) ||
					(!ch.center && !ch.left && !ch.right) ); // all or nothing
			set_voice( this, i, ch.center, ch.left, ch.right );
		}
	}
	
	// TODO: what was this for?
	//core.pcm.output( &core.blip_buf );
	
	// TODO: silence PCM if FM isn't used?
	if ( uses_fm( this ) )
	{
		for ( i = sms_osc_count; --i >= 0; )
			Sms_apu_set_output( &this->psg, i, ( mask & 0x80 ) ? 0 : &this->blip_buf, NULL, NULL );
		if ( Ym2612_enabled( &this->ym2612 ) )
		{
			Synth_volume( &this->pcm, (mask & 0x40) ? 0 : (int)((long long)(0.1115*FP_ONE_VOLUME) / 256 * fm_gain * this->gain / FP_ONE_VOLUME) );
			Ym2612_mute_voices( &this->ym2612, mask );
		}
		
		if ( Ym2413_enabled( &this->ym2413 ) )
		{
			int m = mask & 0x3F;
			if ( mask & 0x20 )
				m |= 0x01E0; // channels 5-8
			if ( mask & 0x40 )
				m |= 0x3E00;
			Ym2413_mute_voices( &this->ym2413, m );
		}
	}
}

void Sound_set_tempo( struct Vgm_Emu* this, int t )
{
	require( this->sample_rate ); // sample rate must be set first
	int const min = (int)(FP_ONE_TEMPO*0.02);
	int const max = (int)(FP_ONE_TEMPO*4.00);
	if ( t < min ) t = min;
	if ( t > max ) t = max;
	this->tempo = t;
	
	if ( this->file_begin )
	{
		this->vgm_rate = (long) ((44100LL * t) / FP_ONE_TEMPO);
		this->blip_time_factor = (int) (((1LL << blip_time_bits) * Blip_clock_rate( &this->blip_buf )) / this->vgm_rate);
		//debug_printf( "blip_time_factor: %ld\n", blip_time_factor );
		//debug_printf( "vgm_rate: %ld\n", vgm_rate );
		// TODO: remove? calculates vgm_rate more accurately (above differs at most by one Hz only)
		//blip_time_factor = (long) floor( double (1L << blip_time_bits) * psg_rate / 44100 / t + 0.5 );
		//vgm_rate = (long) floor( double (1L << blip_time_bits) * psg_rate / blip_time_factor + 0.5 );
		
		this->fm_time_factor = 2 + (int) ((this->fm_rate * (1LL << fm_time_bits)) / this->vgm_rate);
	}
}

void fill_buf( struct Vgm_Emu *this );
blargg_err_t Vgm_start_track( struct Vgm_Emu* this )
{
	clear_track_vars( this );
	
	Sms_apu_reset( &this->psg, get_le16( header( this )->noise_feedback ), header( this )->noise_width );
	
	this->dac_disabled = -1;
	this->pos          = this->file_begin + header_size;
	this->pcm_data     = this->pos;
	this->pcm_pos      = this->pos;
	this->dac_amp      = -1;
	this->vgm_time     = 0;
	if ( get_le32( header( this )->version ) >= 0x150 )
	{
		long data_offset = get_le32( header( this )->data_offset );
		check( data_offset );
		if ( data_offset )
			this->pos += data_offset + offsetof (struct header_t,data_offset) - 0x40;
	}
	
	if ( uses_fm( this ) )
	{
		if ( Ym2413_enabled( &this->ym2413 ) )
			Ym2413_reset( &this->ym2413 );
		
		if ( Ym2612_enabled( &this->ym2612 ) )
			Ym2612_reset( &this->ym2612 );
		
		Blip_clear( &this->blip_buf, 1 );
		Resampler_clear( &this->resampler );
	}
	
	this->fm_time_offset = 0;
	
	Buffer_clear( &this->buf );
	
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

static blargg_long msec_to_samples( blargg_long msec, long sample_rate )
{
	blargg_long sec = msec / 1000;
	msec -= sec * 1000;
	return (sec * sample_rate + msec * sample_rate / 1000) * stereo;
}

long Track_tell( struct Vgm_Emu* this )
{
	blargg_long rate = this->sample_rate * stereo;
	blargg_long sec = this->out_time / rate;
	return sec * 1000 + (this->out_time - sec * rate) * 1000 / rate;
}

blargg_err_t Track_seek( struct Vgm_Emu* this, long msec )
{
	blargg_long time = msec_to_samples( msec, this->sample_rate );
	if ( time < this->out_time )
		RETURN_ERR( Vgm_start_track( this ) );
	return Track_skip( this, time - this->out_time );
}

blargg_err_t skip_( struct Vgm_Emu* this, long count );
blargg_err_t Track_skip( struct Vgm_Emu* this, long count )
{
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

blargg_err_t skip_( struct Vgm_Emu* this, long count )
{
	// for long skip, mute sound
	const long threshold = 30000;
	if ( count > threshold )
	{
		int saved_mute = this->mute_mask_;
		Sound_mute_voices( this, ~0 );
		
		while ( count > threshold / 2 && !this->emu_track_ended_ )
		{
			RETURN_ERR( play_( this, buf_size, this->buf_ ) );
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
		RETURN_ERR( play_( this, n, this->buf_ ) );
	}
	return 0;
}

// Fading

void Track_set_fade( struct Vgm_Emu* this, long start_msec, long length_msec )
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

static void handle_fade( struct Vgm_Emu* this, long out_count, sample_t* out )
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

static void emu_play( struct Vgm_Emu* this, long count, sample_t* out )
{
	this->emu_time += count;
	if ( !this->emu_track_ended_ ) {
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
void fill_buf( struct Vgm_Emu* this )
{
	assert( !this->buf_remain );
	if ( !this->emu_track_ended_ )
	{
		emu_play( this, buf_size, this->buf_ );
		long silence = count_silence( this->buf_, buf_size );
		if ( silence < buf_size )
		{
			this->silence_time = this->emu_time - silence;
			this->buf_remain   = buf_size;
			return;
		}
	}
	this->silence_count += buf_size;
}

blargg_err_t Vgm_play( struct Vgm_Emu* this, long out_count, sample_t* out )
{
	if ( this->track_ended )
	{
		memset( out, 0, out_count * sizeof *out );
	}
	else
	{
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
				this->track_ended  = this->emu_track_ended_ = true;
				this->silence_count = 0;
				this->buf_remain    = 0;
			}
		}
		
		if ( this->buf_remain )
		{
			// empty silence buf
			long n = min( this->buf_remain, out_count - pos );
			memcpy( &out [pos], this->buf_ + (buf_size - this->buf_remain), n * sizeof *out );
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
