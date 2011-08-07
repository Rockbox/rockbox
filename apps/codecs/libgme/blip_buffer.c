// Blip_Buffer 0.4.1. http://www.slack.net/~ant/

#include "blip_buffer.h"

#include <assert.h>
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

#ifdef BLARGG_ENABLE_OPTIMIZER
	#include BLARGG_ENABLE_OPTIMIZER
#endif

int const silent_buf_size = 1; // size used for Silent_Blip_Buffer

void Blip_init( struct Blip_Buffer* this )
{
	this->factor_       = (blip_ulong)LONG_MAX;
	this->offset_       = 0;
	this->buffer_size_  = 0;
	this->sample_rate_  = 0;
	this->reader_accum_ = 0;
	this->bass_shift_   = 0;
	this->clock_rate_   = 0;
	this->bass_freq_    = 16;
	this->length_       = 0;
	
	// assumptions code makes about implementation-defined features
	#ifndef NDEBUG
		// right shift of negative value preserves sign
		buf_t_ i = -0x7FFFFFFE;
		assert( (i >> 1) == -0x3FFFFFFF );
		
		// casting to short truncates to 16 bits and sign-extends
		i = 0x18000;
		assert( (short) i == -0x8000 );
	#endif
}

void Blip_stop( struct Blip_Buffer* this )
{
	if ( this->buffer_size_ != silent_buf_size )
		free( this->buffer_ );
}

void Blip_clear( struct Blip_Buffer* this, int entire_buffer )
{
	this->offset_      = 0;
	this->reader_accum_ = 0;
	this->modified_    = 0;
	if ( this->buffer_ )
	{
		long count = (entire_buffer ? this->buffer_size_ : Blip_samples_avail( this ));
		memset( this->buffer_, 0, (count + blip_buffer_extra_) * sizeof (buf_t_) );
	}
}

blargg_err_t Blip_set_sample_rate( struct Blip_Buffer* this, long new_rate, int msec )
{
	if ( this->buffer_size_ == silent_buf_size )
	{
		assert( 0 );
		return "Internal (tried to resize Silent_Blip_Buffer)";
	}
	
	// start with maximum length that resampled time can represent
	long new_size = (ULONG_MAX >> BLIP_BUFFER_ACCURACY) - blip_buffer_extra_ - 64;
	if ( msec != blip_max_length )
	{
		long s = (new_rate * (msec + 1) + 999) / 1000;
		if ( s < new_size )
			new_size = s;
		else
			assert( 0 ); // fails if requested buffer length exceeds limit
	}
	
	if ( new_size > blip_buffer_max )
		return "Out of memory";
	
	this->buffer_size_ = new_size;
	assert( this->buffer_size_ != silent_buf_size );
	
	// update things based on the sample rate
	this->sample_rate_ = new_rate;
	this->length_ = new_size * 1000 / new_rate - 1;
	if ( msec )
		assert( this->length_ == msec ); // ensure length is same as that passed in
	if ( this->clock_rate_ )
		Blip_set_clock_rate( this, this->clock_rate_ );
	Blip_bass_freq( this, this->bass_freq_ );
	
	Blip_clear( this, 1 );
	
	return 0; // success
}

/* Not sure if this affects sound quality */
#if defined(ROCKBOX)
double floor(double x) {
	if ( x > 0 ) return (int)x;
	return (int)(x-0.9999999999999999);
}
#endif

blip_resampled_time_t Blip_clock_rate_factor( struct Blip_Buffer* this, long rate )
{
	double ratio = (double) this->sample_rate_ / rate;
	blip_long factor = (blip_long) floor( ratio * (1L << BLIP_BUFFER_ACCURACY) + 0.5 );
	assert( factor > 0 || !this->sample_rate_ ); // fails if clock/output ratio is too large
	return (blip_resampled_time_t) factor;
}

void Blip_bass_freq( struct Blip_Buffer* this, int freq )
{
	this->bass_freq_ = freq;
	int shift = 31;
	if ( freq > 0 )
	{
		shift = 13;
		long f = (freq << 16) / this->sample_rate_;
		while ( (f >>= 1) && --shift ) { }
	}
	this->bass_shift_ = shift;
}

void Blip_end_frame( struct Blip_Buffer* this, blip_time_t t )
{
	this->offset_ += t * this->factor_;
	assert( Blip_samples_avail( this ) <= (long) this->buffer_size_ ); // time outside buffer length
}

void Blip_remove_silence( struct Blip_Buffer* this, long count )
{
	assert( count <= Blip_samples_avail( this ) ); // tried to remove more samples than available
	this->offset_ -= (blip_resampled_time_t) count << BLIP_BUFFER_ACCURACY;
}

long Blip_count_samples( struct Blip_Buffer* this, blip_time_t t )
{
	unsigned long last_sample  = Blip_resampled_time( this, t ) >> BLIP_BUFFER_ACCURACY;
	unsigned long first_sample = this->offset_ >> BLIP_BUFFER_ACCURACY;
	return (long) (last_sample - first_sample);
}

blip_time_t Blip_count_clocks( struct Blip_Buffer* this, long count )
{
	if ( !this->factor_ )
	{
		assert( 0 ); // sample rate and clock rates must be set first
		return 0;
	}
	
	if ( count > this->buffer_size_ )
		count = this->buffer_size_;
	blip_resampled_time_t time = (blip_resampled_time_t) count << BLIP_BUFFER_ACCURACY;
	return (blip_time_t) ((time - this->offset_ + this->factor_ - 1) / this->factor_);
}

void Blip_remove_samples( struct Blip_Buffer* this, long count )
{
	if ( count )
	{
		Blip_remove_silence( this, count );
		
		// copy remaining samples to beginning and clear old samples
		long remain = Blip_samples_avail( this ) + blip_buffer_extra_;
		memmove( this->buffer_, this->buffer_ + count, remain * sizeof *this->buffer_ );
		memset( this->buffer_ + remain, 0, count * sizeof *this->buffer_ );
	}
}

long Blip_read_samples( struct Blip_Buffer* this, blip_sample_t* BLIP_RESTRICT out, long max_samples, int stereo )
{
	long count = Blip_samples_avail( this );
	if ( count > max_samples )
		count = max_samples;
	
	if ( count )
	{
		int const bass = BLIP_READER_BASS( *this );
		BLIP_READER_BEGIN( reader, *this );
		
		if ( !stereo )
		{
			blip_long n;
			for ( n = count; n; --n )
			{
				blip_long s = BLIP_READER_READ( reader );
				if ( (blip_sample_t) s != s )
					s = 0x7FFF - (s >> 24);
				*out++ = (blip_sample_t) s;
				BLIP_READER_NEXT( reader, bass );
			}
		}
		else
		{
			blip_long n;
			for ( n = count; n; --n )
			{
				blip_long s = BLIP_READER_READ( reader );
				if ( (blip_sample_t) s != s )
					s = 0x7FFF - (s >> 24);
				*out = (blip_sample_t) s;
				out += 2;
				BLIP_READER_NEXT( reader, bass );
			}
		}
		BLIP_READER_END( reader, *this );
		
		Blip_remove_samples( this, count );
	}
	return count;
}

void Blip_mix_samples( struct Blip_Buffer* this,  blip_sample_t const* in, long count )
{
	if ( this->buffer_size_ == silent_buf_size )
	{
		assert( 0 );
		return;
	}
	
	buf_t_* out = this->buffer_ + (this->offset_ >> BLIP_BUFFER_ACCURACY) + blip_widest_impulse_ / 2;
	
	int const sample_shift = blip_sample_bits - 16;
	int prev = 0;
	while ( count-- )
	{
		blip_long s = (blip_long) *in++ << sample_shift;
		*out += s - prev;
		prev = s;
		++out;
	}
	*out -= prev;
}

void Blip_set_modified( struct Blip_Buffer* this ) 
{ 
	this->modified_ = 1; 
}

int Blip_clear_modified( struct Blip_Buffer* this )
{ 
	int b = this->modified_;
	this->modified_ = 0;
	return b; 
}

blip_resampled_time_t Blip_resampled_duration( struct Blip_Buffer* this, int t )
{
	return t * this->factor_;
}

blip_resampled_time_t Blip_resampled_time( struct Blip_Buffer* this, blip_time_t t )
{
	return t * this->factor_ + this->offset_;
}


// Blip_Synth

void Synth_init( struct Blip_Synth* this )
{
	this->buf = 0;
	this->last_amp = 0;
	this->delta_factor = 0;
}

// Set overall volume of waveform
void Synth_volume( struct Blip_Synth* this, double v )
{
	this->delta_factor = (int) (v * (1L << blip_sample_bits) + 0.5);
}
