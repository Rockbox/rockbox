// Blip_Buffer 0.4.1. http://www.slack.net/~ant/

#include "blip_buffer.h"

#include <limits.h>
#include <string.h>
#include <stdlib.h>
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

void Blip_init( struct Blip_Buffer* this )
{
	this->factor_        = UINT_MAX/2 + 1;;
	this->buffer_center_ = NULL;
	this->buffer_size_   = 0;
	this->sample_rate_   = 0;
	this->bass_shift_    = 0;
	this->clock_rate_    = 0;
	this->bass_freq_     = 16;
	this->length_        = 0;

	// assumptions code makes about implementation-defined features
	#ifndef NDEBUG
		// right shift of negative value preserves sign
		buf_t_ i = -0x7FFFFFFE;
		assert( (i >> 1) == -0x3FFFFFFF );

		// casting to short truncates to 16 bits and sign-extends
		i = 0x18000;
		assert( (short) i == -0x8000 );
	#endif

	Blip_clear( this );
}

void Blip_clear( struct Blip_Buffer* this )
{
	bool const entire_buffer = true;

	this->offset_       = 0;
	this->reader_accum_ = 0;
	this->modified      = false;

	if ( this->buffer_ )
	{
		int count = (entire_buffer ? this->buffer_size_ : Blip_samples_avail( this ));
		memset( this->buffer_, 0, (count + blip_buffer_extra_) * sizeof (delta_t) );
	}
}

blargg_err_t Blip_set_sample_rate( struct Blip_Buffer* this, int new_rate, int msec )
{
	// Limit to maximum size that resampled time can represent
	int max_size = (((blip_resampled_time_t) -1) >> BLIP_BUFFER_ACCURACY) -
		blip_buffer_extra_ - 64; // TODO: -64 isn't needed
	int new_size = (new_rate * (msec + 1) + 999) / 1000;
	if ( new_size > max_size )
		new_size = max_size;

	// Resize buffer
	if ( this->buffer_size_ != new_size ) {
		this->buffer_center_ = this->buffer_ + BLIP_MAX_QUALITY/2;
		this->buffer_size_ = new_size;
    }

	// update things based on the sample rate
	this->sample_rate_ = new_rate;
	this->length_      = new_size * 1000 / new_rate - 1;
	if ( this->clock_rate_ )
		Blip_set_clock_rate( this, this->clock_rate_ );
	Blip_bass_freq( this, this->bass_freq_ );

	Blip_clear( this );

	return 0; // success
}

blip_resampled_time_t Blip_clock_rate_factor( struct Blip_Buffer* this, int rate )
{
	int factor = (int) ( this->sample_rate_ * (1LL << BLIP_BUFFER_ACCURACY) / rate);
	assert( factor > 0 || !this->sample_rate_ ); // fails if clock/output ratio is too large
	return (blip_resampled_time_t) factor;
}

void Blip_bass_freq( struct Blip_Buffer* this, int freq )
{
	this->bass_freq_ = freq;
	int shift = 31;
	if ( freq > 0 && this->sample_rate_ )
	{
		shift = 13;
		int f = (freq << 16) / this->sample_rate_;
		while ( (f >>= 1) && --shift ) { }
	}
    this->bass_shift_ = shift;
}

void Blip_end_frame( struct Blip_Buffer* this, blip_time_t t )
{
	this->offset_ += t * this->factor_;
	assert( Blip_samples_avail( this ) <= (int) this->buffer_size_ ); // time outside buffer length
}

int Blip_count_samples( struct Blip_Buffer* this, blip_time_t t )
{
	blip_resampled_time_t last_sample  = Blip_resampled_time( this, t ) >> BLIP_BUFFER_ACCURACY;
	blip_resampled_time_t first_sample = this->offset_                  >> BLIP_BUFFER_ACCURACY;
	return (int) (last_sample - first_sample);
}

blip_time_t Blip_count_clocks( struct Blip_Buffer* this, int count )
{
	if ( count > this->buffer_size_ )
		count = this->buffer_size_;
	blip_resampled_time_t time = (blip_resampled_time_t) count << BLIP_BUFFER_ACCURACY;
	return (blip_time_t) ((time - this->offset_ + this->factor_ - 1) / this->factor_);
}

void Blip_remove_samples( struct Blip_Buffer* this, int count )
{
	if ( count )
	{
		Blip_remove_silence( this, count );

		// copy remaining samples to beginning and clear old samples
		int remain = Blip_samples_avail( this ) + blip_buffer_extra_;
		memmove( this->buffer_, this->buffer_ + count, remain * sizeof *this->buffer_ );
		memset( this->buffer_ + remain, 0, count * sizeof *this->buffer_ );
	}
}

int Blip_read_samples( struct Blip_Buffer* this, blip_sample_t out_ [], int max_samples, bool stereo )
{
	int count = Blip_samples_avail( this );
	if ( count > max_samples )
		count = max_samples;

	if ( count )
	{
		int const bass = this->bass_shift_;
		delta_t const* reader = this->buffer_ + count;
		int reader_sum = this->reader_accum_;

		blip_sample_t* BLARGG_RESTRICT out = out_ + count;
		if ( stereo )
			out += count;
		int offset = -count;

		if ( !stereo )
		{
			do
			{
				int s = reader_sum >> delta_bits;

				reader_sum -= reader_sum >> bass;
				reader_sum += reader [offset];

				BLIP_CLAMP( s, s );
				out [offset] = (blip_sample_t) s;
			}
			while ( ++offset );
		}
		else
		{
			do
			{
				int s = reader_sum >> delta_bits;

				reader_sum -= reader_sum >> bass;
				reader_sum += reader [offset];

				BLIP_CLAMP( s, s );
				out [offset * 2] = (blip_sample_t) s;
			}
			while ( ++offset );
		}

		this->reader_accum_ = reader_sum;

		Blip_remove_samples( this, count );
	}
	return count;
}

void Blip_mix_samples( struct Blip_Buffer* this,  blip_sample_t const in [], int count )
{
	delta_t* out = this->buffer_center_ + (this->offset_ >> BLIP_BUFFER_ACCURACY);

	int const sample_shift = blip_sample_bits - 16;
	int prev = 0;
	while ( --count >= 0 )
	{
		int s = *in++ << sample_shift;
		*out += s - prev;
		prev = s;
		++out;
	}
	*out -= prev;
}

// Blip_Synth

void volume_unit( struct Blip_Synth* this, int new_unit )
{
	this->delta_factor = (int) (new_unit * (1LL << blip_sample_bits) / FP_ONE_VOLUME);
}

void Synth_init( struct Blip_Synth* this )
{
	this->buf = 0;
	this->last_amp = 0;
	this->delta_factor = 0;
}
