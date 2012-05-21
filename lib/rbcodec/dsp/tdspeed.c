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
 * Copyright (C) 2012 Michael Sevakis
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
#include "platform.h"
#include "config.h"
#include "sound.h"
#include "core_alloc.h"
#include "dsp-util.h"
#include "dsp_proc_entry.h"
#include "tdspeed.h"

#ifndef assert
#define assert(cond)
#endif

#define TIMESTRETCH_SET_FACTOR (DSP_PROC_SETTING+DSP_PROC_TIMESTRETCH)

#define MIN_RATE 8000
#define MAX_RATE 48000 /* double buffer for double rate */
#define MINFREQ 100

#define MAX_INPUTCOUNT       512 /* Max input count so dst doesn't overflow */
#define FIXED_BUFCOUNT      3072 /* 48KHz factor 3.0 */
#define FIXED_OUTBUFCOUNT   4096
#define NBUFFERS 4

enum tdspeed_ops
{
    TDSOP_PROCESS,
    TDSOP_LAST,
    TDSOP_PURGE,
};

static struct tdspeed_state_s
{
    struct dsp_proc_entry *this; /* this stage */
    struct dsp_config *dsp; /* the DSP we use */
    int channels;           /* number of audio channels */
    int32_t samplerate;     /* current samplerate of input data */
    int32_t factor;         /* stretch factor (perdecimille) */
    int32_t shift_max;      /* maximum displacement on a frame */
    int32_t src_step;       /* source window pace */
    int32_t dst_step;       /* destination window pace */
    int32_t dst_order;      /* power of two for dst_step */
    int32_t ovl_shift;      /* overlap buffer frame shift */
    int32_t ovl_size;       /* overlap buffer used size */
    int32_t *ovl_buff[2];   /* overlap buffer (L+R) */
} tdspeed_state;

static int32_t *buffers[NBUFFERS] = { NULL, NULL, NULL, NULL };

static const int buffer_sizes[NBUFFERS] =
{
    FIXED_BUFCOUNT * sizeof(int32_t),
    FIXED_BUFCOUNT * sizeof(int32_t),
    FIXED_OUTBUFCOUNT * sizeof(int32_t),
    FIXED_OUTBUFCOUNT * sizeof(int32_t)
};

#define overlap_buffer  (&buffers[0])
#define outbuf          (&buffers[2])
#define out_size        FIXED_OUTBUFCOUNT

/* Processed buffer passed out to later stages */
static struct dsp_buffer dsp_outbuf;

/* Blend overlapping frame samples according to position */
#if defined(CPU_COLDFIRE)
static inline int32_t blend_frame_samples(int32_t curr, int32_t prev,
                                          int i, int j, int order)
{
    int32_t a0, a1;
    asm (
        "mac.l     %2, %3, %%acc0 \n" /* acc = curr*(i<<(30-order)) >> 23 */
        "mac.l     %4, %5, %%acc0 \n" /* acc += prev*(j<<(30-order)) >> 23 */
        "moveq.l   #1, %0         \n" /* Prepare mask */
        "move.l    %%accext01, %1 \n" /* Get extension bits */
        "lsr.l     #7, %1         \n" /* Get bit 7 of LSb extension ... */
        "and.l     %0, %1         \n" /* ... into bit 0 */
        "movclr.l  %%acc0, %0     \n" /* Get result >> 8 */
        "asl.l     #1, %0         \n" /* Everything x2 */
        "or.l      %1, %0         \n" /* Insert proper LSb from extension */
        : "=d"(a0), "=d"(a1)
        : "r"(curr), "r"(i << order),
          "r"(prev), "r"(j << order));

    return a0;
}
#else
/* Generic */
static inline int32_t blend_frame_samples(int32_t curr, int32_t prev,
                                          int i, int j, int order)
{
    return (curr * (int64_t)i + prev * (int64_t)j) >> order;
}
#endif /* CPU_* */

/* Discard all data */
static void tdspeed_flush(void)
{
    struct tdspeed_state_s *st = &tdspeed_state;
    st->ovl_size = 0;
    st->ovl_shift = 0;
    dsp_outbuf.remcount = 0; /* Dump remaining output */
}

static bool tdspeed_update(int32_t samplerate, int32_t factor)
{
    struct tdspeed_state_s *st = &tdspeed_state;

    /* Save parameters we'll need later if format changes */
    st->samplerate = samplerate;
    st->factor     = factor;

    /* Check parameters */
    if (factor == PITCH_SPEED_100)
        return false;

    if (samplerate < MIN_RATE || samplerate > MAX_RATE)
        return false;

    if (factor < STRETCH_MIN || factor > STRETCH_MAX)
        return false;

    st->dst_step = samplerate / MINFREQ;

    if (factor > PITCH_SPEED_100)
        st->dst_step = st->dst_step * PITCH_SPEED_100 / factor;

    st->dst_order = 1;

    while (st->dst_step >>= 1)
        st->dst_order++;

    st->dst_step = (1 << st->dst_order);
#ifdef CPU_COLDFIRE
    /* blend_frame_samples works in s0.31 mode. Also must shift by
       one less bit before mac in order not to overflow. */
    st->dst_order = 30 - st->dst_order;
#endif
    st->src_step = st->dst_step * factor / PITCH_SPEED_100;
    st->shift_max = (st->dst_step > st->src_step) ?
                        st->dst_step : st->src_step;

    /* just discard remaining input data */
    st->ovl_size = 0;
    st->ovl_shift = 0;

    st->ovl_buff[0] = overlap_buffer[0];
    st->ovl_buff[1] = overlap_buffer[1]; /* ignored if mono */

    return true;
}

static int tdspeed_apply(int32_t *buf_out[2], int32_t *buf_in[2],
                         int data_len, enum tdspeed_ops op, int *consumed)
/* data_len in samples */
{
    struct tdspeed_state_s *const st = &tdspeed_state;
    int32_t src_frame_sz = st->shift_max + st->dst_step;

    if (st->dst_step > st->src_step)
        src_frame_sz += st->dst_step - st->src_step;

    int32_t *dest[2];
    int32_t next_frame, prev_frame;

    /* deal with overlap data first, if any */
    if (st->ovl_size)
    {
        int32_t have = st->ovl_size;

        if (st->ovl_shift > 0)
            have -= st->ovl_shift;

        /* append just enough data to have all of the overlap buffer consumed */
        int32_t steps = (have - 1) / st->src_step;
        int32_t copy = steps * st->src_step + src_frame_sz - have;

        if (copy < src_frame_sz - st->dst_step)
            copy += st->src_step;  /* one more step to allow for pregap data */

        if (copy > data_len)
            copy = data_len;

        assert(st->ovl_size + copy <= FIXED_BUFCOUNT);

        for (int ch = 0; ch < st->channels; ch++)
        {
            memcpy(st->ovl_buff[ch] + st->ovl_size, buf_in[ch],
                   copy * sizeof(int32_t));
        }

        if (consumed)
            *consumed = copy;

        if (op == TDSOP_PROCESS && have + copy < src_frame_sz)
        {
            /* still not enough to process at least one frame */
            st->ovl_size += copy;
            return 0;
        }

        /* recursively call ourselves to process the overlap buffer */
        have = st->ovl_size;
        st->ovl_size = 0;

        assert(have + copy <= FIXED_BUFCOUNT);

        if (copy == data_len)
        {
            return tdspeed_apply(buf_out, st->ovl_buff, have + copy,
                                 op, NULL);
        }

        int i = tdspeed_apply(buf_out, st->ovl_buff, have + copy,
                              TDSOP_LAST, NULL);

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
            next_frame = st->ovl_shift;
        else
            prev_frame = -st->ovl_shift;
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

        assert(next_frame + st->shift_max - 1 + st->dst_step <= data_len);
        assert(prev_frame + st->dst_step <= data_len);

        for (int i = 0; i < st->shift_max; i += INC1)
        {
            int64_t delta = 0;

            for (int ch = 0; ch < st->channels; ch++)
            {
                int32_t *curr = buf_in[ch] + next_frame + i;
                int32_t *prev = buf_in[ch] + prev_frame;

                for (int j = 0; j < st->dst_step;
                     j += INC2, curr += INC2, prev += INC2)
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
        for (int ch = 0; ch < st->channels; ch++)
        {
            int32_t *curr = buf_in[ch] + next_frame + shift;
            int32_t *prev = buf_in[ch] + prev_frame;
            int32_t *d = dest[ch];

            assert(next_frame + shift + st->dst_step <= data_len);
            assert(prev_frame + st->dst_step <= data_len);
            assert(dest[ch] - buf_out[ch] + st->dst_step <= out_size);

            for (int i = 0, j = st->dst_step; j; i++, j--)
            {
                assert(d < buf_out[ch] + out_size);
                *d++ = blend_frame_samples(*curr++, *prev++, i, j,
                                           st->dst_order);
            }

            dest[ch] = d;
        }

        /* adjust pointers for next frame */
        prev_frame = next_frame + shift + st->dst_step;
        next_frame += st->src_step;

        /* here next_frame - prev_frame = src_step - dst_step - shift */
        assert(next_frame - prev_frame == st->src_step - st->dst_step - shift);
    } /* while */

    /* now deal with remaining partial frames */
    switch (op)
    {
    case TDSOP_PROCESS:
    {
        /* preserve remaining data + needed overlap data for next call */
        st->ovl_shift = next_frame - prev_frame;
        int i = (st->ovl_shift < 0) ? next_frame : prev_frame;
        st->ovl_size = data_len - i;
        assert(st->ovl_size <= FIXED_BUFCOUNT);

        for (int ch = 0; ch < st->channels; ch++)
        {
            memmove(st->ovl_buff[ch], buf_in[ch] + i,
                    st->ovl_size * sizeof(int32_t));
        }

        if (consumed)
            *consumed = data_len;

        break;
        } /* TDSOP_PROCESS: */

    case TDSOP_LAST:
    {
        /* special overlap buffer processing: remember frame shift only */
        st->ovl_shift = next_frame - prev_frame;
        break;
        } /* TDSOP_LAST: */

    case TDSOP_PURGE:
    {
        /* last call: purge all remaining data to output buffer */
        int i = data_len - prev_frame;

        for (int ch = 0; ch < st->channels; ch++)
        {
            assert(dest[ch] + i <= buf_out[ch] + out_size);
            memcpy(dest[ch], buf_in[ch] + prev_frame, i * sizeof(int32_t));
            dest[ch] += i;
        }

        if (consumed)
            *consumed += i;

        break;
        } /* TDSOP_PURGE: */
    } /* switch */

    return dest[0] - buf_out[0];
}


/** DSP interface **/

static void tdspeed_process_new_format(struct dsp_proc_entry *this,
                                       struct dsp_buffer **buf_p);

/* Enable or disable the availability of timestretch */
void dsp_timestretch_enable(bool enabled)
{
    if (enabled != !tdspeed_state.this)
        return; /* No change */

    dsp_proc_enable(dsp_get_config(CODEC_IDX_AUDIO), DSP_PROC_TIMESTRETCH,
                    enabled);
}

/* Set the timestretch ratio */
void dsp_set_timestretch(int32_t percent)
{
    struct tdspeed_state_s *st = &tdspeed_state;

    if (!st->this)
        return; /* not enabled */

    if (percent <= 0)
        percent = PITCH_SPEED_100;

    if (percent == st->factor)
        return; /* no change */

    dsp_configure(st->dsp, TIMESTRETCH_SET_FACTOR, percent);
}

/* Return the timestretch ratio */
int32_t dsp_get_timestretch(void)
{
    return tdspeed_state.factor;
}

/* Return whether or not timestretch is enabled and initialized */
bool dsp_timestretch_available(void)
{
    return !!tdspeed_state.this;
}

/* Apply timestretch to the input buffer and switch to our output buffer */
static void tdspeed_process(struct dsp_proc_entry *this,
                            struct dsp_buffer **buf_p)
{
    struct dsp_buffer *src = *buf_p;
    struct dsp_buffer *dst = &dsp_outbuf;

    *buf_p = dst; /* switch to our buffer */

    int count = dst->remcount;

    if (count > 0)
        return; /* output remains from an earlier call */

    dst->p32[0] = outbuf[0];
    dst->p32[1] = outbuf[src->format.num_channels - 1];

    if (src->remcount > 0)
    {
        dst->bufcount = 0; /* use this to get consumed src */
        count = tdspeed_apply(dst->p32, src->p32,
                              MIN(src->remcount, MAX_INPUTCOUNT),
                              TDSOP_PROCESS, &dst->bufcount);

        /* advance src by samples consumed */
        if (dst->bufcount > 0)
            dsp_advance_buffer32(src, dst->bufcount);
    }
    /* else purged dsp_outbuf */

    dst->remcount = count;

    /* inherit in-place processed mask from source buffer */
    dst->proc_mask = src->proc_mask;

    (void)this;
}

/* Process format changes and settings changes */
static void tdspeed_process_new_format(struct dsp_proc_entry *this,
                                       struct dsp_buffer **buf_p)
{
    struct dsp_buffer *src = *buf_p;
    struct dsp_buffer *dst = &dsp_outbuf;

    if (dst->remcount > 0)
    {
        *buf_p = dst;
        return; /* output remains from an earlier call */
    }

    DSP_PRINT_FORMAT(DSP_PROC_TIMESTRETCH, DSP_PROC_TIMESTRETCH, src->format);

    struct tdspeed_state_s *st = &tdspeed_state;
    struct dsp_config *dsp = st->dsp;
    struct sample_format *format = &src->format;
    int channels = format->num_channels;

    if (format->codec_frequency != st->samplerate ||
        !dsp_proc_active(dsp, DSP_PROC_TIMESTRETCH))
    {
        /* relevent parameters are changing - all overlap will be discarded */
        st->channels = channels;

        DEBUGF("  DSP_PROC_TIMESTRETCH- new settings: "
               "ch:%u chz: %u, %d.%02d%%\n",
               channels,
               format->codec_frequency,
               st->factor / 100, st->factor % 100);
        bool active = tdspeed_update(format->codec_frequency, st->factor);
        dsp_proc_activate(dsp, DSP_PROC_TIMESTRETCH, active);

        if (!active)
        {
            DEBUGF("  DSP_PROC_RESAMPLE- not active\n");
            dst->format = src->format; /* Keep track */
            return; /* no more for now */
        }
    }
    else if (channels != st->channels)
    {
        /* channel count transistion - have to make old data in overlap
           buffer compatible with new format */
        DEBUGF("  DSP_PROC_TIMESTRETCH- new ch count: %u=>%u\n",
               st->channels, channels);

        st->channels = channels;

        if (channels > 1)
        {
            /* mono->stereo: Process the old mono as stereo now */
            memcpy(st->ovl_buff[1], st->ovl_buff[0],
                   st->ovl_size * sizeof (int32_t));
        }
        else
        {
            /* stereo->mono: Process the old stereo as mono now */
            for (int i = 0; i < st->ovl_size; i++)
            {
                st->ovl_buff[0][i] = st->ovl_buff[0][i] / 2 +
                                     st->ovl_buff[1][i] / 2;
            }
        }
    }

    struct sample_format f = *format;
    format_change_ack(format);

    if (EQU_SAMPLE_FORMAT(f, dst->format))
    {
        DEBUGF("  DSP_PROC_TIMESTRETCH- same dst format\n");
        format_change_ack(&f); /* nothing changed that matters downstream */
    }

    dst->format = f;

    /* return to normal processing */
    this->process[0] = tdspeed_process;
    dsp_proc_call(this, buf_p, 0);
}

/* DSP message hook */
static intptr_t tdspeed_configure(struct dsp_proc_entry *this,
                                  struct dsp_config *dsp,
                                  unsigned int setting,
                                  intptr_t value)
{
    struct tdspeed_state_s *st = &tdspeed_state;

    switch (setting)
    {
    case DSP_INIT:
        /* everything is at 100% until dsp_set_timestretch is called with
           some other value and timestretch is enabled at the time */
        if (value == CODEC_IDX_AUDIO)
            st->factor = PITCH_SPEED_100;
        break;

    case DSP_FLUSH:
        tdspeed_flush();
        break;

    case DSP_PROC_INIT:
        if (!tdspeed_alloc_buffers(buffers, buffer_sizes, NBUFFERS))
            return -1; /* fail the init */

        st->this = this;
        st->dsp = dsp;
        this->ip_mask = 0; /* not in-place */
        this->process[0] = tdspeed_process;
        this->process[1] = tdspeed_process_new_format;
        break;

    case DSP_PROC_CLOSE:
        st->this = NULL;
        st->factor = PITCH_SPEED_100;
        dsp_outbuf.remcount = 0;
        tdspeed_free_buffers(buffers, NBUFFERS);
        break;

    case TIMESTRETCH_SET_FACTOR:
        /* force update as a format change */
        st->samplerate = 0;
        st->factor = (int32_t)value;
        st->this->process[0] = tdspeed_process_new_format;
        dsp_proc_activate(st->dsp, DSP_PROC_TIMESTRETCH, true);
        break;
    }

    return 1;
    (void)value;
}

void tdspeed_move(int i, void* current, void* new)
{
    ptrdiff_t shift = (int32_t *)new - (int32_t *)current;
    int32_t **p32 = dsp_outbuf.p32;

    switch (i)
    {
    case 0: case 1:
        /* moving overlap (input) buffers */
        tdspeed_state.ovl_buff[i] = new;
        break;

    case 2:
        /* moving outbuf left channel and dsp_outbuf.p32[0] */
        if (p32[0] == p32[1])
            p32[1] += shift; /* mono mode */

        p32[0] += shift;
        break;

    case 3:
        /* moving outbuf right channel and dsp_outbuf.p32[1] */
        p32[1] += shift;
        break;
    }

    buffers[i] = new;
}

/* Database entry */
DSP_PROC_DB_ENTRY(TIMESTRETCH,
                  tdspeed_configure);
