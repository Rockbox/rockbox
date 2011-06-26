// Game_Music_Emu 0.5.5. http://www.slack.net/~ant/

#include "resampler.h"

#include <stdlib.h>
#include <string.h>

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

// TODO: fix this. hack since resampler holds back some output.
unsigned const resampler_extra = 34;

enum { shift = 14 };
int const unit = 1 << shift;

blargg_err_t Resampler_setup( struct Resampler* this, int fm_rate, int fm_gain, int rate, int gain )
{
    this->gain_ = (int)( ((1LL << gain_bits) * fm_gain * gain) / FP_ONE_GAIN );
    this->step  = (int)( ((1LL << shift) * fm_rate) / rate + 1);
    this->rate_ = this->step;
    return 0;
}

blargg_err_t Resampler_reset( struct Resampler* this, int pairs )
{
    // expand allocations a bit
    this->sample_buffer_size = (pairs + (pairs >> 2)) * 2;
    Resampler_resize( this, pairs );
    this->resampler_size = this->oversamples_per_frame + (this->oversamples_per_frame >> 2);

    this->buffer_size = this->resampler_size;
    this->pos = 0;
    this->write_pos = 0;

    Resampler_clear( this );
    return 0;
}

void Resampler_resize( struct Resampler* this, int pairs )
{
    int new_sample_buf_size = pairs * 2;
    if ( this->sample_buf_size != new_sample_buf_size )
    {
        if ( new_sample_buf_size > this->sample_buffer_size )
        {
            check(false);
            return;
        }

        this->sample_buf_size = new_sample_buf_size;
        this->oversamples_per_frame = (int) ((pairs * this->rate_ * 2LL) / unit) + 2;
        Resampler_clear( this );
    }
}

static void mix_samples( struct Resampler* this, struct Blip_Buffer* blip_buf, dsample_t out_ [] )
{
    int const bass = BLIP_READER_BASS( *blip_buf );
    BLIP_READER_BEGIN( sn, *blip_buf );

    int count = this->sample_buf_size >> 1;
    BLIP_READER_ADJ_( sn, count );

    typedef dsample_t stereo_dsample_t [2];
    stereo_dsample_t* BLARGG_RESTRICT out = (stereo_dsample_t*) out_ + count;
    stereo_dsample_t const* BLARGG_RESTRICT in =
        (stereo_dsample_t const*) this->sample_buf + count;
    int offset = -count;
    int const gain = this->gain_;
    do
    {
        int s = BLIP_READER_READ_RAW( sn ) >> (blip_sample_bits - 16);
        BLIP_READER_NEXT_IDX_( sn, bass, offset );

        int l = (in [offset] [0] * gain >> gain_bits) + s;
        int r = (in [offset] [1] * gain >> gain_bits) + s;

        BLIP_CLAMP( l, l );
        out [offset] [0] = (blip_sample_t) l;

        BLIP_CLAMP( r, r );
        out [offset] [1] = (blip_sample_t) r;
    }
    while ( ++offset );

    BLIP_READER_END( sn, *blip_buf );
}

static dsample_t const* resample_( struct Resampler* this, dsample_t** out_,
        dsample_t const* out_end, dsample_t const in [], int in_size )
{
    in_size -= write_offset;
    if ( in_size > 0 )
    {
        dsample_t* BLARGG_RESTRICT out = *out_;
        dsample_t const* const in_end = in + in_size;

        int const step = this->step;
        int       pos  = this->pos;

        // TODO: IIR filter, then linear resample
        // TODO: detect skipped sample, allowing merging of IIR and resample?

        do
        {
            #define INTERP( i, out )\
                out = (in [0 + i] * (unit - pos) + ((in [2 + i] + in [4 + i] + in [6 + i]) << shift) +\
                in [8 + i] * pos) >> (shift + 2);

            int out_0;
            INTERP( 0,                  out_0 )
            INTERP( 1, out [0] = out_0; out [1] )
            out += stereo;

            pos += step;
            in += ((unsigned) pos >> shift) * stereo;
            pos &= unit - 1;
        }
        while ( in < in_end && out < out_end );

        this->pos = pos;
        *out_ = out;
    }
    return in;
}

static inline int resample_wrapper( struct Resampler* this, dsample_t out [], int* out_size,
        dsample_t const in [], int in_size )
{
    assert( Resampler_rate( this ) );

    dsample_t* out_ = out;
    int result = resample_( this, &out_, out + *out_size, in, in_size ) - in;
    assert( out_ <= out + *out_size );
    assert( result <= in_size );

    *out_size = out_ - out;
    return result;
}

static int skip_input( struct Resampler* this, int count )
{
    this->write_pos -= count;
    if ( this->write_pos < 0 ) // occurs when downsampling
    {
        count += this->write_pos;
        this->write_pos = 0;
    }
    memmove( this->buf, &this->buf [count], this->write_pos * sizeof this->buf [0] );
    return count;
}

static void play_frame_( struct Resampler* this, struct Blip_Buffer* blip_buf, dsample_t* out )
{
    int pair_count = this->sample_buf_size >> 1;
    blip_time_t blip_time = Blip_count_clocks( blip_buf, pair_count );
    int sample_count = this->oversamples_per_frame - this->write_pos + resampler_extra;

    int new_count = this->callback( this->callback_data, blip_time, sample_count, &this->buf [this->write_pos] );
    assert( new_count < this->resampler_size );

    Blip_end_frame( blip_buf, blip_time );
    assert( Blip_samples_avail( blip_buf ) == pair_count );

    this->write_pos += new_count;
    assert( (unsigned) this->write_pos <= this->buffer_size );

    int count = this->sample_buf_size;
    if ( count )
        skip_input( this, resample_wrapper( this, this->sample_buf, &count, this->buf, this->write_pos ) );
    assert( count == this->sample_buf_size );

    mix_samples( this, blip_buf, out );
    Blip_remove_samples( blip_buf, pair_count );
}

void Resampler_play( struct Resampler* this, int count, dsample_t* out, struct Blip_Buffer* blip_buf )
{
    // empty extra buffer
    int remain = this->sample_buf_size - this->buf_pos;
    if ( remain )
    {
        if ( remain > count )
            remain = count;
        count -= remain;
        memcpy( out, &this->sample_buf [this->buf_pos], remain * sizeof *out );
        out += remain;
        this->buf_pos += remain;
    }

    // entire frames
    while ( count >= this->sample_buf_size )
    {
        play_frame_( this, blip_buf, out );
        out += this->sample_buf_size;
        count -= this->sample_buf_size;
    }

    // extra
    if ( count )
    {
        play_frame_( this, blip_buf, this->sample_buf );
        this->buf_pos = count;
        memcpy( out, this->sample_buf, count * sizeof *out );
        out += count;
    }
}
