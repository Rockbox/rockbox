/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Nicolas Pitre <nico@cam.org>
 * Copyright (C) 2006-2007 by Stéphane Doyon <s.doyon@videotron.ca>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "sound.h"
#include "core_alloc.h"
#include "system.h"
#include "tdspeed.h"
#include "settings.h"
#include "dsp-util.h"

#define assert(cond)

#define MIN_RATE 8000
#define MAX_RATE 48000 /* double buffer for double rate */
#define MINFREQ 100

#define FIXED_BUFSIZE 3072 /* 48KHz factor 3.0 */

static int32_t** dsp_src;
static int handles[4];
static int32_t *overlap_buffer[2] = { NULL, NULL };
static int32_t *outbuf[2] = { NULL, NULL };

static int move_callback(int handle, void* current, void* new)
{
    /* TODO */
    (void)handle;
    if (dsp_src)
    {
        int ch = (current == outbuf[0]) ? 0 : 1;
        dsp_src[ch] = outbuf[ch] = new;
    }
    return BUFLIB_CB_OK;
}

static struct buflib_callbacks ops = {
    .move_callback = move_callback,
    .shrink_callback = NULL,
};

static int ovl_move_callback(int handle, void* current, void* new)
{
    /* TODO */
    (void)handle;
    if (dsp_src)
    {
        int ch = (current == overlap_buffer[0]) ? 0 : 1;
        overlap_buffer[ch] = new;
    }
    return BUFLIB_CB_OK;
}

static struct buflib_callbacks ovl_ops = {
    .move_callback = ovl_move_callback,
    .shrink_callback = NULL,
};


static struct tdspeed_state_s
{
    bool stereo;
    int32_t shift_max;      /* maximum displacement on a frame */
    int32_t src_step;       /* source window pace */
    int32_t dst_step;       /* destination window pace */
    int32_t dst_order;      /* power of two for dst_step */
    int32_t ovl_shift;      /* overlap buffer frame shift */
    int32_t ovl_size;       /* overlap buffer used size */
    int32_t ovl_space;      /* overlap buffer size */
    int32_t *ovl_buff[2];   /* overlap buffer */
} tdspeed_state;

void tdspeed_init(void)
{
    if (!global_settings.timestretch_enabled)
        return;

    /* Allocate buffers */
    if (overlap_buffer[0] == NULL)
    {
        handles[0] = core_alloc_ex("tdspeed ovl left", FIXED_BUFSIZE * sizeof(int32_t), &ovl_ops);
        overlap_buffer[0] = core_get_data(handles[0]);
    }
    if (overlap_buffer[1] == NULL)
    {
        handles[1] = core_alloc_ex("tdspeed ovl right", FIXED_BUFSIZE * sizeof(int32_t), &ovl_ops);
        overlap_buffer[1] = core_get_data(handles[1]);
    }
    if (outbuf[0] == NULL)
    {
        handles[2] = core_alloc_ex("tdspeed left", TDSPEED_OUTBUFSIZE * sizeof(int32_t), &ops);
        outbuf[0] = core_get_data(handles[2]);
    }
    if (outbuf[1] == NULL)
    {
        handles[3] = core_alloc_ex("tdspeed right", TDSPEED_OUTBUFSIZE * sizeof(int32_t), &ops);
        outbuf[1] = core_get_data(handles[3]);
    }
}

void tdspeed_finish(void)
{
    for(unsigned i = 0; i < ARRAYLEN(handles); i++)
    {
        if (handles[i] > 0)
        {
            core_free(handles[i]);
            handles[i] = 0;
        }
    }
    overlap_buffer[0] = overlap_buffer[1] = NULL;
    outbuf[0]         = outbuf[1]         = NULL;
}

bool tdspeed_config(int samplerate, bool stereo, int32_t factor)
{
    struct tdspeed_state_s *st = &tdspeed_state;
    int src_frame_sz;

    /* Check buffers were allocated ok */
    if (overlap_buffer[0] == NULL || overlap_buffer[1] == NULL)
        return false;

    if (outbuf[0] == NULL || outbuf[1] == NULL)
        return false;

    /* Check parameters */
    if (factor == PITCH_SPEED_100)
        return false;

    if (samplerate < MIN_RATE || samplerate > MAX_RATE)
        return false;

    if (factor < STRETCH_MIN || factor > STRETCH_MAX)
        return false;

    st->stereo = stereo;
    st->dst_step = samplerate / MINFREQ;

    if (factor > PITCH_SPEED_100)
        st->dst_step = st->dst_step * PITCH_SPEED_100 / factor;

    st->dst_order = 1;

    while (st->dst_step >>= 1)
        st->dst_order++;

    st->dst_step = (1 << st->dst_order);
    st->src_step = st->dst_step * factor / PITCH_SPEED_100;
    st->shift_max = (st->dst_step > st->src_step) ? st->dst_step : st->src_step;

    src_frame_sz = st->shift_max + st->dst_step;

    if (st->dst_step > st->src_step)
        src_frame_sz += st->dst_step - st->src_step;

    st->ovl_space = ((src_frame_sz - 2) / st->src_step) * st->src_step
                        + src_frame_sz;

    if (st->src_step > st->dst_step)
        st->ovl_space += 2*st->src_step - st->dst_step;

    if (st->ovl_space > FIXED_BUFSIZE)
        st->ovl_space = FIXED_BUFSIZE;

    st->ovl_size = 0;
    st->ovl_shift = 0;

    st->ovl_buff[0] = overlap_buffer[0];

    if (stereo)
        st->ovl_buff[1] = overlap_buffer[1];
    else
        st->ovl_buff[1] = st->ovl_buff[0];

    return true;
}

static int tdspeed_apply(int32_t *buf_out[2], int32_t *buf_in[2],
                         int data_len, int last, int out_size)
/* data_len in samples */
{
    struct tdspeed_state_s *st = &tdspeed_state;
    int32_t *dest[2];
    int32_t next_frame, prev_frame, src_frame_sz;
    bool stereo = buf_in[0] != buf_in[1];

    assert(stereo == st->stereo);

    src_frame_sz = st->shift_max + st->dst_step;

    if (st->dst_step > st->src_step)
        src_frame_sz += st->dst_step - st->src_step;

    /* deal with overlap data first, if any */
    if (st->ovl_size)
    {
        int32_t have, copy, steps;
        have = st->ovl_size;

        if (st->ovl_shift > 0)
            have -= st->ovl_shift;

        /* append just enough data to have all of the overlap buffer consumed */
        steps = (have - 1) / st->src_step;
        copy = steps * st->src_step + src_frame_sz - have;

        if (copy < src_frame_sz - st->dst_step)
            copy += st->src_step;  /* one more step to allow for pregap data */

        if (copy > data_len)
            copy = data_len;

        assert(st->ovl_size + copy <= FIXED_BUFSIZE);
        memcpy(st->ovl_buff[0] + st->ovl_size, buf_in[0],
               copy * sizeof(int32_t));

        if (stereo)
            memcpy(st->ovl_buff[1] + st->ovl_size, buf_in[1],
                   copy * sizeof(int32_t));

        if (!last && have + copy < src_frame_sz)
        {
            /* still not enough to process at least one frame */
            st->ovl_size += copy;
            return 0;
        }

        /* recursively call ourselves to process the overlap buffer */
        have = st->ovl_size;
        st->ovl_size = 0;

        if (copy == data_len)
        {
            assert(have + copy <= FIXED_BUFSIZE);
            return tdspeed_apply(buf_out, st->ovl_buff, have+copy, last,
                               out_size);
        }

        assert(have + copy <= FIXED_BUFSIZE);
        int i = tdspeed_apply(buf_out, st->ovl_buff, have+copy, -1, out_size);

        dest[0] = buf_out[0] + i;
        dest[1] = buf_out[1] + i;

        /* readjust pointers to account for data already consumed */
        next_frame = copy - src_frame_sz + st->src_step;
        prev_frame = next_frame - st->ovl_shift;
    }
    else
    {
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
    while (data_len - next_frame >= src_frame_sz)
    {
        /* find frame overlap by autocorelation */
        int const INC1 = 8;
        int const INC2 = 32;

        int64_t min_delta = INT64_MAX;  /* most positive */
        int shift = 0;

        /* Power of 2 of a 28bit number requires 56bits, can accumulate
           256times in a 64bit variable. */
        assert(st->dst_step / INC2 <= 256);
        assert(next_frame + st->shift_max - 1 + st->dst_step - 1 < data_len);
        assert(prev_frame + st->dst_step - 1 < data_len);

        for (int i = 0; i < st->shift_max; i += INC1)
        {
            int64_t delta = 0;

            int32_t *curr = buf_in[0] + next_frame + i;
            int32_t *prev = buf_in[0] + prev_frame;

            for (int j = 0; j < st->dst_step; j += INC2, curr += INC2, prev += INC2)
            {
                delta += ad_s32(*curr, *prev);

                if (delta >= min_delta)
                    goto skip;
            }

            if (stereo)
            {
                curr = buf_in[1] + next_frame + i;
                prev = buf_in[1] + prev_frame;

                for (int j = 0; j < st->dst_step; j += INC2, curr += INC2, prev += INC2)
                {
                    delta += ad_s32(*curr, *prev);

                    if (delta >= min_delta)
                        goto skip;
                }
            }

            min_delta = delta;
            shift = i;
skip:;
        }

        /* overlap fading-out previous frame with fading-in current frame */
        int32_t *curr = buf_in[0] + next_frame + shift;
        int32_t *prev = buf_in[0] + prev_frame;

        int32_t *d = dest[0];

        assert(next_frame + shift + st->dst_step - 1 < data_len);
        assert(prev_frame + st->dst_step - 1 < data_len);
        assert(dest[0] - buf_out[0] + st->dst_step - 1 < out_size);

        for (int i = 0, j = st->dst_step; j; i++, j--)
        {
            *d++ = (*curr++ * (int64_t)i +
                    *prev++ * (int64_t)j) >> st->dst_order;
        }

        dest[0] = d;

        if (stereo)
        {
            curr = buf_in[1] + next_frame + shift;
            prev = buf_in[1] + prev_frame;

            d = dest[1];

            for (int i = 0, j = st->dst_step; j; i++, j--)
            {
                assert(d < buf_out[1] + out_size);

                *d++ = (*curr++ * (int64_t)i +
                        *prev++ * (int64_t)j) >> st->dst_order;
            }

            dest[1] = d;
        }

        /* adjust pointers for next frame */
        prev_frame = next_frame + shift + st->dst_step;
        next_frame += st->src_step;

        /* here next_frame - prev_frame = src_step - dst_step - shift */
        assert(next_frame - prev_frame == st->src_step - st->dst_step - shift);
    }

    /* now deal with remaining partial frames */
    if (last == -1)
    {
        /* special overlap buffer processing: remember frame shift only */
        st->ovl_shift = next_frame - prev_frame;
    }
    else if (last != 0)
    {
        /* last call: purge all remaining data to output buffer */
        int i = data_len - prev_frame;

        assert(dest[0] + i <= buf_out[0] + out_size);
        memcpy(dest[0], buf_in[0] + prev_frame, i * sizeof(int32_t));

        dest[0] += i;

        if (stereo)
        {
            assert(dest[1] + i <= buf_out[1] + out_size);
            memcpy(dest[1], buf_in[1] + prev_frame, i * sizeof(int32_t));
            dest[1] += i;
        }
    }
    else
    {
        /* preserve remaining data + needed overlap data for next call */
        st->ovl_shift = next_frame - prev_frame;
        int i = (st->ovl_shift < 0) ? next_frame : prev_frame;
        st->ovl_size = data_len - i;

        assert(st->ovl_size <= FIXED_BUFSIZE);
        memcpy(st->ovl_buff[0], buf_in[0] + i, st->ovl_size * sizeof(int32_t));

        if (stereo)
            memcpy(st->ovl_buff[1], buf_in[1] + i, st->ovl_size * sizeof(int32_t));
    }

    return dest[0] - buf_out[0];
}

long tdspeed_est_output_size()
{
    return TDSPEED_OUTBUFSIZE;
}

long tdspeed_est_input_size(long size)
{
    struct tdspeed_state_s *st = &tdspeed_state;

    size = (size - st->ovl_size) * st->src_step / st->dst_step;

    if (size < 0)
        size = 0;

    return size;
}

int tdspeed_doit(int32_t *src[], int count)
{
    dsp_src = src;
    count = tdspeed_apply( (int32_t *[2]) { outbuf[0], outbuf[1] },
                           src, count, 0, TDSPEED_OUTBUFSIZE);

    src[0] = outbuf[0];
    src[1] = outbuf[1];

    return count;
}

