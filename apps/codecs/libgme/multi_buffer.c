// Multi_Buffer 0.4.1. http://www.slack.net/~ant/

#include "multi_buffer.h"

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

// Tracked_Blip_Buffer

int const blip_buffer_extra = 32; // TODO: explain why this value

void Tracked_init( struct Tracked_Blip_Buffer* this )
{
	Blip_init( &this->blip );
	this->last_non_silence = 0;
}

void Tracked_clear( struct Tracked_Blip_Buffer* this )
{
	this->last_non_silence = 0;
	Blip_clear( &this->blip );
}

void Tracked_end_frame( struct Tracked_Blip_Buffer* this, blip_time_t t )
{
	Blip_end_frame( &this->blip, t );
	if ( this->blip.modified )
	{
		this->blip.modified = false;
		this->last_non_silence = Blip_samples_avail( &this->blip ) + blip_buffer_extra;
	}
}

unsigned Tracked_non_silent( struct Tracked_Blip_Buffer* this )
{
	return this->last_non_silence | unsettled( &this->blip );
}

static inline void remove_( struct Tracked_Blip_Buffer* this, int n )
{
	if ( (this->last_non_silence -= n) < 0 )
		this->last_non_silence = 0;
}

void Tracked_remove_silence( struct Tracked_Blip_Buffer* this, int n )
{
	remove_( this, n );
	Blip_remove_silence( &this->blip, n );
}

void Tracked_remove_samples( struct Tracked_Blip_Buffer* this, int n )
{
	remove_( this, n );
	Blip_remove_samples( &this->blip, n );
}

void Tracked_remove_all_samples( struct Tracked_Blip_Buffer* this )
{
	int avail = Blip_samples_avail( &this->blip );
	if ( !Tracked_non_silent( this ) )
		Tracked_remove_silence( this, avail );
	else
		Tracked_remove_samples( this, avail );
}

int Tracked_read_samples( struct Tracked_Blip_Buffer* this, blip_sample_t out [], int count )
{
	count = Blip_read_samples( &this->blip, out, count, false );
	remove_( this, count );
	return count;
}

// Stereo_Mixer

// mixers use a single index value to improve performance on register-challenged processors
// offset goes from negative to zero

void Mixer_init( struct Stereo_Mixer* this )
{
	this->samples_read = 0;
}

static void mix_mono( struct Stereo_Mixer* this, blip_sample_t out_ [], int count )
{
	int const bass = this->bufs [2]->blip.bass_shift_;
	delta_t const* center = this->bufs [2]->blip.buffer_ + this->samples_read;
	int center_sum = this->bufs [2]->blip.reader_accum_;

	typedef blip_sample_t stereo_blip_sample_t [stereo];
	stereo_blip_sample_t* BLARGG_RESTRICT out = (stereo_blip_sample_t*) out_ + count;
	int offset = -count;
	do
	{
		int s = center_sum >> delta_bits;

		center_sum -= center_sum >> bass;
		center_sum += center [offset];

		BLIP_CLAMP( s, s );

		out [offset] [0] = (blip_sample_t) s;
		out [offset] [1] = (blip_sample_t) s;
	}
	while ( ++offset );

	this->bufs [2]->blip.reader_accum_ = center_sum;
}

static void mix_stereo( struct Stereo_Mixer* this, blip_sample_t out_ [], int count )
{
	blip_sample_t* BLARGG_RESTRICT out = out_ + count * stereo;
	// do left + center and right + center separately to reduce register load
	struct Tracked_Blip_Buffer* const* buf = &this->bufs [2];
	while ( true ) // loop runs twice
	{
		--buf;
		--out;

		int const bass = this->bufs [2]->blip.bass_shift_;
		delta_t const* side = (*buf)->blip.buffer_ + this->samples_read;
		delta_t const* center = this->bufs [2]->blip.buffer_ + this->samples_read;

		int side_sum = (*buf)->blip.reader_accum_;
		int center_sum = this->bufs [2]->blip.reader_accum_;

		int offset = -count;
		do
		{
			int s = (center_sum + side_sum) >> delta_bits;

			side_sum   -= side_sum   >> bass;
			center_sum -= center_sum >> bass;

			side_sum   += side   [offset];
			center_sum += center [offset];

			BLIP_CLAMP( s, s );

			++offset; // before write since out is decremented to slightly before end
			out [offset * stereo] = (blip_sample_t) s;
		}
		while ( offset );

		(*buf)->blip.reader_accum_ = side_sum;

		if ( buf != this->bufs )
			continue;

		// only end center once
		this->bufs [2]->blip.reader_accum_ = center_sum;
		break;
	}
}

void Mixer_read_pairs( struct Stereo_Mixer* this, blip_sample_t out [], int count )
{
	// TODO: if caller never marks buffers as modified, uses mono
	// except that buffer isn't cleared, so caller can encounter
	// subtle problems and not realize the cause.
	this->samples_read += count;
	if ( Tracked_non_silent( this->bufs [0] ) | Tracked_non_silent( this->bufs [1] ) )
	mix_stereo( this, out, count );
	else
	mix_mono( this, out, count );
}

// Multi_Buffer

void Buffer_init( struct Multi_Buffer* this )
{
	int const spf = 2;

	Tracked_init( &this->bufs [0] );
	Tracked_init( &this->bufs [1] );
	Tracked_init( &this->bufs [2] );

	Mixer_init( &this->mixer );

	this->length_                 = 0;
	this->sample_rate_            = 0;
	this->channels_changed_count_ = 1;
	this->channel_types_          = NULL;
	this->channel_count_          = 0;
	this->samples_per_frame_      = spf;
	this->immediate_removal_      = true;

	this->mixer.bufs [2] = &this->bufs [2];
	this->mixer.bufs [0] = &this->bufs [0];
	this->mixer.bufs [1] = &this->bufs [1];

	this->chan.center = &this->bufs [2].blip;
	this->chan.left   = &this->bufs [0].blip;
	this->chan.right  = &this->bufs [1].blip;
}

blargg_err_t Buffer_set_sample_rate( struct Multi_Buffer* this, int rate, int msec )
{
	int i;
	for ( i = bufs_size; --i >= 0; )
		RETURN_ERR( Blip_set_sample_rate( &this->bufs [i].blip, rate, msec ) );

	this->sample_rate_ = Blip_sample_rate( &this->bufs [0].blip );
	this->length_      = Blip_length( &this->bufs [0].blip );
	return 0;
}

void Buffer_clock_rate( struct Multi_Buffer* this, int rate )
{
	int i;
	for ( i = bufs_size; --i >= 0; )
		Blip_set_clock_rate( &this->bufs [i].blip, rate );
}

void Buffer_bass_freq( struct Multi_Buffer* this, int bass )
{
	int i;
	for ( i = bufs_size; --i >= 0; )
		Blip_bass_freq( &this->bufs [i].blip, bass );
}

blargg_err_t Buffer_set_channel_count( struct Multi_Buffer* this, int n, int const* types )
{
	this->channel_count_ = n;
	this->channel_types_ = types;
	return 0;
}

struct channel_t Buffer_channel( struct Multi_Buffer* this, int i )
{
	(void) i;
	return this->chan;
}

void Buffer_clear( struct Multi_Buffer* this )
{
	int i;
	this->mixer.samples_read = 0;
	for ( i = bufs_size; --i >= 0; )
		Tracked_clear( &this->bufs [i] );
}

void Buffer_end_frame( struct Multi_Buffer* this, blip_time_t clock_count )
{
    int i;
    for ( i = bufs_size; --i >= 0; )
		Tracked_end_frame( &this->bufs [i], clock_count );
}

int Buffer_read_samples( struct Multi_Buffer* this, blip_sample_t out [], int out_size )
{
	require( (out_size & 1) == 0 ); // must read an even number of samples
	out_size = min( out_size, Buffer_samples_avail( this ) );

	int pair_count = (int) (out_size >> 1);
	if ( pair_count )
	{
		Mixer_read_pairs( &this->mixer, out, pair_count );

		if ( Buffer_samples_avail( this ) <= 0 || this->immediate_removal_ )
		{
			int i;
			for ( i = bufs_size; --i >= 0; )
			{
				buf_t* b = &this->bufs [i];
				// TODO: might miss non-silence settling since it checks END of last read
				if ( !Tracked_non_silent( b ) )
					Tracked_remove_silence( b, this->mixer.samples_read );
				else
					Tracked_remove_samples( b, this->mixer.samples_read );
			}
			this->mixer.samples_read = 0;
		}
	}

	return out_size;
}
