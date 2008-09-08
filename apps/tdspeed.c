/*
 * Audio time domain speed scaling / time compression/stretch.
 *
 * Copyright (C) 2006 by Nicolas Pitre <nico@cam.org>
 *
 * Modified by Stéphane Doyon to adapt to Rockbox
 * Copyright (C) 2006-2007 by Stéphane Doyon <s.doyon@videotron.ca>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "system.h"
#include "tdspeed.h"

#define mylog(...)
#define assert(...)

#if 0
#define assert(cond) \
    if(!(cond)) { \
        badlog("Assertion failed in %s at line %d: %s\n",        \
                __FUNCTION__, __LINE__, #cond); \
    }
#endif

#define MIN_RATE 8000
#define MAX_RATE 48000 /* double buffer for double rate */
#define MIN_FACTOR 35
#define MAX_FACTOR 250

//#define FIXED_BUFSIZE 3072 /* 48KHz factor 3.0 */
#define FIXED_BUFSIZE 3524 /* worse seen, not sure how to calc */

struct tdspeed_state_s {
    bool stereo;
    int shift_max;        /* maximum displacement on a frame */
    int src_step;        /* source window pace */
    int dst_step;        /* destination window pace */
    int dst_order;        /* power of two for dst_step */
    int ovl_shift;        /* overlap buffer frame shift */
    int ovl_size;        /* overlap buffer used size */
    int ovl_space;        /* overlap buffer size */
    int32_t *ovl_buff[2];        /* overlap buffer */
};
static struct tdspeed_state_s tdspeed_state;

static int32_t overlap_buffer[2][FIXED_BUFSIZE];
static int32_t outbuf[2][TDSPEED_OUTBUFSIZE];

bool tdspeed_init(int samplerate, bool stereo, int factor)
{
    struct tdspeed_state_s *st = &tdspeed_state;
    int src_frame_sz;

    if(factor == 100)
        return false;
    if (samplerate < MIN_RATE || samplerate > MAX_RATE)
        return false;
    if(factor <MIN_FACTOR || factor > MAX_FACTOR)
        return false;

    st->stereo = !!stereo;

#define MINFREQ 100
    st->dst_step = samplerate / MINFREQ;
                    
    if (factor > 100)
        st->dst_step = st->dst_step *100 /factor;
    st->dst_order = 1;
                    
    while (st->dst_step >>= 1)
        st->dst_order++;
    st->dst_step = (1 << st->dst_order);
    st->src_step = st->dst_step * factor /100;
    st->shift_max = (st->dst_step > st->src_step) ? st->dst_step : st->src_step;

    src_frame_sz = st->shift_max + st->dst_step;
    if (st->dst_step > st->src_step)
        src_frame_sz += st->dst_step - st->src_step;
    st->ovl_space = ((src_frame_sz - 2)/st->src_step) * st->src_step 
        + src_frame_sz;
    if (st->src_step > st->dst_step)
        st->ovl_space += 2*st->src_step - st->dst_step;

    mylog(
            "shift_max %d, src_step %d, dst_step %d, dst_order %d, src_frame_sz %d, ovl_space %d\n",
            st->shift_max, st->src_step, st->dst_step, st->dst_order, src_frame_sz, st->ovl_space);
    
    assert(st->ovl_space <= FIXED_BUFSIZE);

    st->ovl_size = 0;
    st->ovl_shift = 0;

    st->ovl_buff[0] = overlap_buffer[0];
    if(stereo)
        st->ovl_buff[1] = overlap_buffer[1];
    else st->ovl_buff[1] = st->ovl_buff[0];

    return true;
}

int tdspeed_apply(int32_t *buf_out[2], int32_t *buf_in[2], int data_len,
                int last, int out_size)
/* data_len in samples */
{
    struct tdspeed_state_s *st = &tdspeed_state;
    int32_t *curr, *prev, *dest[2], *d;
    int i, j, next_frame, prev_frame, shift, src_frame_sz;
    bool stereo = buf_in[0] != buf_in[1];
    assert(stereo == st->stereo);

    src_frame_sz = st->shift_max + st->dst_step;
    if (st->dst_step > st->src_step)
        src_frame_sz += st->dst_step - st->src_step;

    /* deal with overlap data first, if any */
    if (st->ovl_size) {
        int have, copy, steps;
        have = st->ovl_size;
        if (st->ovl_shift > 0)
            have -= st->ovl_shift;
        /* append just enough data to have all of the overlap buffer consumed*/ 
        steps = (have - 1) / st->src_step;
        copy = steps * st->src_step + src_frame_sz - have;
        if (copy < src_frame_sz - st->dst_step)
            copy += st->src_step;  /* one more step to allow for pregap data */
        if (copy > data_len) copy = data_len;
        assert(st->ovl_size +copy <= FIXED_BUFSIZE);
        memcpy(st->ovl_buff[0] + st->ovl_size, buf_in[0],
               copy * sizeof(int32_t));
        if(stereo)
            memcpy(st->ovl_buff[1] + st->ovl_size, buf_in[1],
                   copy * sizeof(int32_t));
        if (!last && have + copy < src_frame_sz) {
            /* still not enough to process at least one frame */
            st->ovl_size += copy;
            return 0;
        }
        /* recursively call ourselves to process the overlap buffer */
        have = st->ovl_size;
        st->ovl_size = 0;
        if (copy == data_len) {
            assert( (have+copy) <= FIXED_BUFSIZE);
            return tdspeed_apply(buf_out, st->ovl_buff, have+copy, last, 
                               out_size);
        }
        assert( (have+copy) <= FIXED_BUFSIZE);
        i = tdspeed_apply(buf_out, st->ovl_buff, have+copy, -1, out_size);
        dest[0] = buf_out[0] + i;
        dest[1] = buf_out[1] + i;
        /* readjust pointers to account for data already consumed */
        next_frame = copy - src_frame_sz + st->src_step;
        prev_frame = next_frame - st->ovl_shift;
    } else {
        dest[0] = buf_out[0];
        dest[1] = buf_out[1];
        next_frame = prev_frame = 0;
        if (st->ovl_shift > 0)
            next_frame += st->ovl_shift;
        else
          prev_frame += -st->ovl_shift;
    }
    st->ovl_shift = 0;

    /* process all complete frames */
    while (data_len -next_frame >= src_frame_sz) {
        /* find frame overlap by autocorelation */
        long long min_delta = ~(1ll << 63);  /* most positive */
        shift = 0;
#define INC1 8
#define INC2 32
        /* Power of 2 of a 28bit number requires 56bits, can accumulate
           256times in a 64bit variable. */
        assert(st->dst_step /INC2 <= 256);
        assert(next_frame+st->shift_max-1 +st->dst_step-1 < data_len);
        assert(prev_frame +st->dst_step-1 < data_len);
        for (i = 0; i < st->shift_max; i += INC1) {
            long long delta = 0;
            curr = buf_in[0] +next_frame + i;
            prev = buf_in[0] +prev_frame;
            for (j = 0; j < st->dst_step;
                 j += INC2, curr += INC2, prev += INC2) {
                int32_t diff = *curr - *prev;
                delta += (long long)diff * diff;
                if (delta >= min_delta)
                    goto skip;
            }
            if(stereo) {
                curr = buf_in[1] +next_frame + i;
                prev = buf_in[1] +prev_frame;
                for (j = 0; j < st->dst_step;
                     j += INC2, curr += INC2, prev += INC2) {
                    int32_t diff = *curr - *prev;
                    delta += (long long)diff * diff;
                    if (delta >= min_delta)
                        goto skip;
                }
            }
            min_delta = delta;
            shift = i;
            skip:;
        }

        /* overlap fading-out previous frame with fading-in current frame */
        curr = buf_in[0] +next_frame + shift;
        prev = buf_in[0] +prev_frame;
        d = dest[0];
        assert(next_frame+shift +st->dst_step-1 < data_len);
        assert(prev_frame +st->dst_step-1 < data_len);
        assert(dest[0]-buf_out[0] +st->dst_step-1 < out_size);
        for (i = 0, j = st->dst_step; j; i++, j--) {
            *d++ = (*curr++ * (long long)i 
                    + *prev++ * (long long)j) >> st->dst_order;
        }
        dest[0] = d;
        if(stereo) {
            curr = buf_in[1] +next_frame + shift;
            prev = buf_in[1] +prev_frame;
            d = dest[1];
            for (i = 0, j = st->dst_step; j; i++, j--) {
                assert(d < buf_out[1] +out_size);
                *d++ = (*curr++ * (long long)i 
                        + *prev++ * (long long)j) >> st->dst_order;
            }
            dest[1] = d;
        }

        /* adjust pointers for next frame */
        prev_frame = next_frame + shift + st->dst_step;
        //assert(prev_frame == curr[0] -buf_in[0]);
        next_frame += st->src_step;
        /* here next_frame - prev_frame = src_step - dst_step - shift */
        assert(next_frame - prev_frame == st->src_step - st->dst_step - shift);
    }

    /* now deal with remaining partial frames */
    if (last == -1) {
        /* special overlap buffer processing: remember frame shift only */
        st->ovl_shift = next_frame - prev_frame;
    } else if (last != 0) {
        /* last call: purge all remaining data to output buffer */
        i = data_len -prev_frame;
        assert(dest[0] +i <= buf_out[0] +out_size);
        memcpy(dest[0], buf_in[0] +prev_frame, i * sizeof(int32_t));
        dest[0] += i;
        if(stereo) {
            assert(dest[1] +i <= buf_out[1] +out_size);
            memcpy(dest[1], buf_in[1] +prev_frame, i * sizeof(int32_t));
            dest[1] += i;
        }
    } else {
        /* preserve remaining data + needed overlap data for next call */
        st->ovl_shift = next_frame - prev_frame;
        i = (st->ovl_shift < 0) ? next_frame : prev_frame;
        st->ovl_size = data_len - i;
        assert(st->ovl_size <= FIXED_BUFSIZE);
        memcpy(st->ovl_buff[0], buf_in[0]+i, st->ovl_size * sizeof(int32_t));
        if(stereo)
            memcpy(st->ovl_buff[1], buf_in[1]+i, st->ovl_size * sizeof(int32_t));
    }

    return dest[0] - buf_out[0];
}

#if 0
static int tdspeed_next_required_space(int data_len, int last)
{
    struct tdspeed_state_s *st = &tdspeed_state;
    int src_frame_sz, src_size, nb_frames, dst_space;

    src_frame_sz = st->shift_max + st->dst_step;
    if (st->dst_step > st->src_step)
        src_frame_sz += st->dst_step - st->src_step;
    src_size = data_len + st->ovl_size;
    if (st->ovl_shift > 0)
        src_size -= st->ovl_shift;
    if (src_size < src_frame_sz) {
        if (!last)
            return 0;
        dst_space = data_len + st->ovl_size;
        if (st->ovl_shift < 0)
            dst_space += st->ovl_shift;
    } else {
        nb_frames = (src_size - src_frame_sz) / st->src_step + 1;
        dst_space = nb_frames * st->dst_step;
        if (last) {
            dst_space += src_size - nb_frames * st->src_step;
            dst_space += st->src_step - st->dst_step;
        }
    }
    return dst_space;
}
#endif

long tdspeed_est_output_size(long size)
{
  //int _size = size;
#if 0
    size = tdspeed_next_required_space(size, false);
    if(size > TDSPEED_OUTBUFSIZE)
        size = TDSPEED_OUTBUFSIZE;
#else
    size = TDSPEED_OUTBUFSIZE;
#endif
    mylog("tdspeed_est_output_size of %d -> %d\n", _size, size);
    return size;
}
long tdspeed_est_input_size(long size)
{
    struct tdspeed_state_s *st = &tdspeed_state;
    //int _size = size;
    size = (size -st->ovl_size) *st->src_step /st->dst_step;
    if(size <0)
        size = 0;
    else size = size;
    mylog("tdspeed_est_input_size of %d -> %d\n", _size, size);
    return size;
}

int tdspeed_doit(int32_t *src[], int count)
{
    mylog("tdspeed_doit %d\n", count);
    count = tdspeed_apply( (int32_t *[2]) { outbuf[0], outbuf[1] },
                           src, count, 0, TDSPEED_OUTBUFSIZE);
    src[0] = outbuf[0];
    src[1] = outbuf[1];
    return count;
}
