// Game_Music_Emu 0.6-pre. http://www.slack.net/~ant/

#include "track_filter.h"

/* Copyright (C) 2003-2008 Shay Green. This module is free software; you
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

int const fade_block_size = 512;
int const fade_shift = 8; // fade ends with gain at 1.0 / (1 << fade_shift)
int const silence_threshold = 8;

void track_create( struct Track_Filter* this )
{
	this->emu_               = NULL;
	this->setup_.max_initial = 0;
	this->setup_.lookahead   = 0;
	this->setup_.max_silence = indefinite_count;
	this->silence_ignored_   = false;
	track_stop( this );
}

blargg_err_t track_init( struct Track_Filter* this, void* emu )
{
	this->emu_ = emu;
	return 0;
}

static void clear_time_vars( struct Track_Filter* this )
{
	this->emu_time      = this->buf_remain;
	this->out_time      = 0;
	this->silence_time  = 0;
	this->silence_count = 0;
}

void track_stop( struct Track_Filter* this )
{
	this->emu_track_ended_ = true;
	this->track_ended_     = true;
	this->fade_start       = indefinite_count;
	this->fade_step        = 1;
	this->buf_remain       = 0;
	this->emu_error        = NULL;
	clear_time_vars( this );
}

blargg_err_t track_start( struct Track_Filter* this )
{
	this->emu_error = NULL;
	track_stop( this );
	
	this->emu_track_ended_ = false;
	this->track_ended_     = false;
	
	if ( !this->silence_ignored_ )
	{
		// play until non-silence or end of track
		while ( this->emu_time < this->setup_.max_initial )
		{
			fill_buf( this );
			if ( this->buf_remain | this->emu_track_ended_ )
				break;
		}
	}
	
	clear_time_vars( this );
	return this->emu_error;
}

static void end_track_if_error( struct Track_Filter* this, blargg_err_t err )
{
	if ( err )
	{
		this->emu_error        = err;
		this->emu_track_ended_ = true;
	}
}

blargg_err_t track_skip( struct Track_Filter* this, int count )
{
	this->emu_error = NULL;
	this->out_time += count;
	
	// remove from silence and buf first
	{
		int n = min( count, this->silence_count );
		this->silence_count -= n;
		count         -= n;
		
		n = min( count, this->buf_remain );
		this->buf_remain -= n;
		count      -= n;
	}
	
	if ( count && !this->emu_track_ended_ )
	{
		this->emu_time += count;
		this->silence_time = this->emu_time; // would otherwise be invalid
		end_track_if_error( this, skip_( this->emu_, count ) );
	}
	
	if ( !(this->silence_count | this->buf_remain) ) // caught up to emulator, so update track ended
		this->track_ended_ |= this->emu_track_ended_;
	
	return this->emu_error;
}

blargg_err_t skippy_( struct Track_Filter* this, int count )
{
	while ( count && !this->emu_track_ended_ )
	{
		int n = buf_size;
		if ( n > count )
			n = count;
		count -= n;
		RETURN_ERR( play_( this->emu_, n, this->buf ) );
	}
	return 0;
}

// Fading

void track_set_fade( struct Track_Filter* this, int start, int length )
{
	this->fade_start = start;
	this->fade_step  = length / (fade_block_size * fade_shift);
	if ( this->fade_step < 1 )
		this->fade_step = 1;
}

static bool is_fading( struct Track_Filter* this )
{
	return this->out_time >= this->fade_start && this->fade_start != indefinite_count;
}

// unit / pow( 2.0, (double) x / step )
static int int_log( int x, int step, int unit )
{
	int shift = x / step;
	int fraction = (x - shift * step) * unit / step;
	return ((unit - fraction) + (fraction >> 1)) >> shift;
}

static void handle_fade( struct Track_Filter* this, sample_t out [], int out_count )
{
	int i;
	for ( i = 0; i < out_count; i += fade_block_size )
	{
		int const shift = 14;
		int const unit = 1 << shift;
		int gain = int_log( (this->out_time + i - this->fade_start) / fade_block_size,
				this->fade_step, unit );
		if ( gain < (unit >> fade_shift) )
			this->track_ended_ = this->emu_track_ended_ = true;
		
		sample_t* io = &out [i];
		for ( int count = min( fade_block_size, out_count - i ); count; --count )
		{
			*io = (sample_t) ((*io * gain) >> shift);
			++io;
		}
	}
}

// Silence detection

static void emu_play( struct Track_Filter* this, sample_t out [], int count )
{
	this->emu_time += count;
	if ( !this->emu_track_ended_ )
		end_track_if_error( this, play_( this->emu_, count, out ) );
	else
		memset( out, 0, count * sizeof *out );
}

// number of consecutive silent samples at end
static int count_silence( sample_t begin [], int size )
{
	sample_t first = *begin;
	*begin = silence_threshold * 2; // sentinel
	sample_t* p = begin + size;
	while ( (unsigned) (*--p + silence_threshold) <= (unsigned) silence_threshold * 2 ) { }
	*begin = first;
	return size - (p - begin);
}

// fill internal buffer and check it for silence
void fill_buf( struct Track_Filter* this )
{
	assert( !this->buf_remain );
	if ( !this->emu_track_ended_ )
	{
		emu_play( this, this->buf, buf_size );
		int silence = count_silence( this->buf, buf_size );
		if ( silence < buf_size )
		{
			this->silence_time = this->emu_time - silence;
			this->buf_remain   = buf_size;
			return;
		}
	}
	this->silence_count += buf_size;
}

blargg_err_t track_play( struct Track_Filter* this, int out_count, sample_t out [] )
{
	this->emu_error = NULL;
	if ( this->track_ended_ )
	{
		memset( out, 0, out_count * sizeof *out );
	}
	else
	{
		assert( this->emu_time >= this->out_time );
		
		// prints nifty graph of how far ahead we are when searching for silence
		//dprintf( "%*s \n", int ((emu_time - out_time) * 7 / 44100), "*" );
		
		// use any remaining silence samples
		int pos = 0;
		if ( this->silence_count )
		{
			if ( !this->silence_ignored_ )
			{
				// during a run of silence, run emulator at >=2x speed so it gets ahead
				int ahead_time = this->setup_.lookahead * (this->out_time + out_count - this->silence_time) +
				this->silence_time;
				while ( this->emu_time < ahead_time && !(this->buf_remain | this->emu_track_ended_) )
					fill_buf( this );
				
				// end track if sufficient silence has been found
				if ( this->emu_time - this->silence_time > this->setup_.max_silence )
				{
					this->track_ended_  = this->emu_track_ended_ = true;
					this->silence_count = out_count;
					this->buf_remain    = 0;
				}
			}
			
			// fill from remaining silence
			pos = min( this->silence_count, out_count );
			memset( out, 0, pos * sizeof *out );
			this->silence_count -= pos;
		}
		
		// use any remaining samples from buffer
		if ( this->buf_remain )
		{
			int n = min( this->buf_remain, (int) (out_count - pos) );
			memcpy( out + pos, this->buf + (buf_size - this->buf_remain), n * sizeof *out );
			this->buf_remain -= n;
			pos += n;
		}
		
		// generate remaining samples normally
		int remain = out_count - pos;
		if ( remain )
		{
			emu_play( this, out + pos, remain );
			this->track_ended_ |= this->emu_track_ended_;
			
			if ( this->silence_ignored_ && !is_fading( this ) )
			{
				// if left unupdated, ahead_time could become too large
				this->silence_time = this->emu_time;
			}
			else
			{
				// check end for a new run of silence
				int silence = count_silence( out + pos, remain );
				if ( silence < remain )
					this->silence_time = this->emu_time - silence;
				
				if ( this->emu_time - this->silence_time >= buf_size )
					fill_buf( this ); // cause silence detection on next play()
			}
		}
		
		if ( is_fading( this ) )
			handle_fade( this, out, out_count );
	}
	this->out_time += out_count;
	return this->emu_error;
}
