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
#include "debug.h"

#define assert(cond)

#define MIN_RATE 8000
#define MAX_RATE 48000 /* double buffer for double rate */
#define MINFREQ 100

#define MAX_INPUTCOUNT       512
#define FIXED_BUFCOUNT      3072 /* 48KHz factor 3.0 */
#define FIXED_OUTBUFCOUNT   4096

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

static int handles[4] = { 0, 0, 0, 0 };
static int32_t *buffers[4] = { NULL, NULL, NULL, NULL };

#define overlap_buffer  (&buffers[0])
#define outbuf          (&buffers[2])
#define out_size        FIXED_OUTBUFCOUNT

/* Processed buffer passed out to later stages */
static struct dsp_buffer dsp_outbuf;

static int move_callback(int handle, void *current, void *new)
{
    struct dsp_config *dsp = (void *)dsp_configure(NULL, DSP_MYDSP,
                                                   CODEC_IDX_AUDIO);

    if (dsp_configure(dsp, DSP_IS_BUSY, 0))
        return BUFLIB_CB_CANNOT_MOVE; /* DSP processing in progress */

    for (unsigned int i = 0; i < ARRAYLEN(handles); i++)
    {
        if (handle != handles[i])
            continue;

        if (i >= 2)
        {
            /* Moving outbuf[0/1] pointer - dsp buffer needs adjusting */
            /* Whether it's already initialized makes no difference */
            ptrdiff_t amt = (int32_t *)new - buffers[i];

            if (i == 2)
            {
                if (dsp_outbuf.p32[0] == dsp_outbuf.p32[1])
                    dsp_outbuf.p32[1] += amt; /* mono mode */

                dsp_outbuf.p32[0] += amt;
            }
            else
            {
                dsp_outbuf.p32[1] += amt;
            }
        }

        buffers[i] = new;
        break;
    }

    return BUFLIB_CB_OK;

    (void)current;
}

static struct buflib_callbacks ops =
{
    .move_callback = move_callback,
    .shrink_callback = NULL,
};

void tdspeed_init(void)
{
    static const struct
    {
        const char *name;
        size_t size;
    } bufdefs[4] =
    {
        { "tdspeed ovl L", FIXED_BUFCOUNT * sizeof(int32_t) },
        { "tdspeed ovl R", FIXED_BUFCOUNT * sizeof(int32_t) },
        { "tdspeed out L", FIXED_OUTBUFCOUNT * sizeof(int32_t) },
        { "tdspeed out R", FIXED_OUTBUFCOUNT * sizeof(int32_t) },
    };

    /* Allocate buffers */
    for (unsigned i = 0; i < ARRAYLEN(bufdefs); i++)
    {
        if (buffers[i] == NULL)
        {
            handles[i] = core_alloc_ex(bufdefs[i].name, bufdefs[i].size, &ops);
            buffers[i] = core_get_data(handles[i]);
        }
    }
}

void tdspeed_finish(void)
{
    for (unsigned i = 0; i < ARRAYLEN(handles); i++)
    {
        if (handles[i] > 0)
        {
            core_free(handles[i]);
            handles[i] = 0;
        }

        buffers[i] = NULL;
    }
}

bool tdspeed_config(int samplerate, bool stereo, int32_t factor)
{
    struct tdspeed_state_s *st = &tdspeed_state;

    /* Check buffers were allocated ok */
    for (unsigned i = 0; i < ARRAYLEN(buffers); i++)
    {
        if (handles[i] <= 0 || buffers[i] == NULL)
            return false;
    }

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

    int src_frame_sz = st->shift_max + st->dst_step;

    if (st->dst_step > st->src_step)
        src_frame_sz += st->dst_step - st->src_step;

    st->ovl_space = ((src_frame_sz - 2) / st->src_step) * st->src_step
                        + src_frame_sz;

    if (st->src_step > st->dst_step)
        st->ovl_space += 2*st->src_step - st->dst_step;

    if (st->ovl_space > FIXED_BUFCOUNT)
        st->ovl_space = FIXED_BUFCOUNT;

    st->ovl_size = 0;
    st->ovl_shift = 0;

    st->ovl_buff[0] = overlap_buffer[0];

    if (stereo)
        st->ovl_buff[1] = overlap_buffer[1];
    else
        st->ovl_buff[1] = st->ovl_buff[0];

    dsp_outbuf.remcount = 0;

    return true;
}

static int tdspeed_apply(int32_t *buf_out[2], int32_t *buf_in[2],
                         int data_len, int last, int *consumed)
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

        assert(st->ovl_size + copy <= FIXED_BUFCOUNT);
        memcpy(st->ovl_buff[0] + st->ovl_size, buf_in[0],
               copy * sizeof(int32_t));

        if (stereo)
            memcpy(st->ovl_buff[1] + st->ovl_size, buf_in[1],
                   copy * sizeof(int32_t));

        *consumed += copy;

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
            assert(have + copy <= FIXED_BUFCOUNT);
            return tdspeed_apply(buf_out, st->ovl_buff, have+copy, last,
                                 consumed);
        }

        assert(have + copy <= FIXED_BUFCOUNT);
        int i = tdspeed_apply(buf_out, st->ovl_buff, have+copy, -1,
                              consumed);

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

        *consumed += i;
    }
    else
    {
        /* preserve remaining data + needed overlap data for next call */
        st->ovl_shift = next_frame - prev_frame;
        int i = (st->ovl_shift < 0) ? next_frame : prev_frame;
        st->ovl_size = data_len - i;

        assert(st->ovl_size <= FIXED_BUFCOUNT);
        memcpy(st->ovl_buff[0], buf_in[0] + i, st->ovl_size * sizeof(int32_t));

        if (stereo)
            memcpy(st->ovl_buff[1], buf_in[1] + i, st->ovl_size * sizeof(int32_t));
    }

    return dest[0] - buf_out[0];
}

int tdspeed_est_output_count(void)
{
    /* Return worst-case */
    return FIXED_OUTBUFCOUNT;
}

void tdspeed_doit(struct dsp_data *data, struct dsp_buffer **buf_p)
{
    struct dsp_buffer *src = *buf_p;
    struct dsp_buffer *dst = &dsp_outbuf;

    *buf_p = dst; /* switch to our buffer */

    int count = dst->remcount;

    if (count > 0)
        return; /* output remains from earlier call */

    dst->p32[0] = outbuf[0];
    dst->p32[1] = outbuf[data->num_channels - 1];
    dst->bufcount = 0; /* use this to get consumed src */

    count = tdspeed_apply(dst->p32, src->p32,
                          MIN(src->remcount, MAX_INPUTCOUNT), 0,
                          &dst->bufcount);

    /* advance src by samples consumed */
    if (dst->bufcount > 0)
        dsp_advance_buffer32(src, dst->bufcount);

    dst->remcount = count;
    dst->proc_mask = 0;
}
