/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Miika Pekkarinen
 * Copyright (C) 2012 Michael Sevakis
 * Copyright (C) 2013 Michael Giacomelli
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
#include "rbcodecconfig.h"
#include "platform.h"
#include "fracmul.h"
#include "fixedpoint.h"
#include "dsp_proc_entry.h"
#include "dsp_misc.h"
#include <string.h>

/**
 * Linear interpolation resampling that introduces a one sample delay because
 * of our inability to look into the future at the end of a frame.
 */

#if 1 /* Set to '0' to enable debug messages */
#undef DEBUGF
#define DEBUGF(...)
#endif

#define RESAMPLE_BUF_COUNT 192 /* Per channel, per DSP */

/* CODEC_IDX_AUDIO = left and right, CODEC_IDX_VOICE = mono */
static int32_t resample_out_bufs[3][RESAMPLE_BUF_COUNT] IBSS_ATTR;

/* Data for each resampler on each DSP */
static struct resample_data
{
    uint32_t delta;         /* 00h: Phase delta for each step in s15.16*/
    uint32_t phase;         /* 04h: Current phase [pos16|frac16] */
    int32_t  history[2][3]; /* 08h: Last samples for interpolation (L+R)
                                    0 = oldest, 2 = newest */
                            /* 20h */
    unsigned int frequency;         /* Virtual input samplerate */
    unsigned int frequency_out;     /* Resampler output samplerate */
    struct dsp_buffer resample_buf; /* Buffer descriptor for resampled data */
    int32_t *resample_out_p[2];     /* Actual output buffer pointers */
} resample_data[DSP_COUNT] IBSS_ATTR;

/* Actual worker function. Implemented here or in target assembly code. */
int resample_hermite(struct resample_data *data, struct dsp_buffer *src,
                     struct dsp_buffer *dst);

static void resample_flush_data(struct resample_data *data)
{
    data->phase = 0;
    memset(&data->history, 0, sizeof (data->history));
}

static void resample_flush(struct dsp_proc_entry *this)
{
    struct resample_data *data = (void *)this->data;
    data->resample_buf.remcount = 0;
    resample_flush_data(data);
}

static bool resample_new_delta(struct resample_data *data,
                               struct sample_format *format,
                               unsigned int fout)
{
    unsigned int frequency = format->frequency; /* virtual samplerate */

    data->frequency = frequency;
    data->frequency_out = fout;
    data->delta = fp_div(frequency, fout, 16);

    if (frequency == data->frequency_out)
    {
        /* NOTE: If fully glitch-free transistions from no resampling to
           resampling are desired, history should be maintained even when
           not resampling. */
        resample_flush_data(data);
        return false;
    }

    return true;
}

#if !defined(CPU_COLDFIRE) && !defined(CPU_ARM)
int resample_hermite(struct resample_data *data, struct dsp_buffer *src,
                     struct dsp_buffer *dst)
{
    int ch = src->format.num_channels - 1;
    uint32_t count = MIN(src->remcount, 0x8000);
    uint32_t delta = data->delta;
    uint32_t phase, pos;
    int32_t *d;

    do
    {
        const int32_t *s = src->p32[ch];

        d = dst->p32[ch];
        int32_t *dmax = d + dst->bufcount;

        /* Restore state */
        phase = data->phase;
        pos = phase >> 16;
        pos = MIN(pos, count);

        while (pos < count && d < dmax)
        {
            int x0, x1, x2, x3;

            if (pos < 3)
            {
                x3 = data->history[ch][pos+0];
                x2 = pos < 2 ? data->history[ch][pos+1] : s[pos-2];
                x1 = pos < 1 ? data->history[ch][pos+2] : s[pos-1];
            }
            else
            {
                x3 = s[pos-3];
                x2 = s[pos-2];
                x1 = s[pos-1];
            }

            x0 = s[pos];

            int32_t frac = (phase & 0xffff) << 15;

            /* 4-point, 3rd-order Hermite/Catmull-Rom spline (x-form):
             * c1 = -0.5*x3 + 0.5*x1
             *    = 0.5*(x1 - x3)                <--
             *
             * v = x1 - x2, -v = x2 - x1
             * c2 = x3 - 2.5*x2 + 2*x1 - 0.5*x0
             *    = x3 + 2*(x1 - x2) - 0.5*(x0 + x2)
             *    = x3 + 2*v - 0.5*(x0 + x2)     <--
             *
             * c3 = -0.5*x3 + 1.5*x2 - 1.5*x1 + 0.5*x0
             *    = 0.5*x0 - 0.5*x3 + 0.5*(x2 - x1) + (x2 - x1)
             *    = 0.5*(x0 - x3 - v) - v        <--
             *
             * polynomial coefficients */
            int32_t c1 = (x1 - x3) >> 1;
            int32_t v = x1 - x2;
            int32_t c2 = x3 + 2*v - ((x0 + x2) >> 1);
            int32_t c3 = ((x0 - x3 - v) >> 1) - v;

            /* Evaluate polynomial at time 'frac'; Horner's rule. */
            int32_t acc;
            acc = FRACMUL(c3, frac) + c2;
            acc = FRACMUL(acc, frac) + c1;
            acc = FRACMUL(acc, frac) + x2;

            *d++ = acc;

            phase += delta;
            pos = phase >> 16;
        }

        pos = MIN(pos, count);

        /* Save delay samples for next time. Must do this even if pos was
         * clamped before loop in order to keep record up to date. */
        data->history[ch][0] = pos < 3 ? data->history[ch][pos+0] : s[pos-3];
        data->history[ch][1] = pos < 2 ? data->history[ch][pos+1] : s[pos-2];
        data->history[ch][2] = pos < 1 ? data->history[ch][pos+2] : s[pos-1];
    }
    while (--ch >= 0);

    /* Wrap phase accumulator back to start of next frame. */
    data->phase = phase - (pos << 16);

    dst->remcount = d - dst->p32[0];
    return pos;
}
#endif /* CPU */

/* Resample count stereo samples or stop when the destination is full.
 * Updates the src buffer and changes to its own output buffer to refer to
 * the resampled data. */
static void resample_process(struct dsp_proc_entry *this,
                             struct dsp_buffer **buf_p)
{
    struct resample_data *data = (void *)this->data;
    struct dsp_buffer *src = *buf_p;
    struct dsp_buffer *dst = &data->resample_buf;

    *buf_p = dst;

    if (dst->remcount > 0)
        return; /* data still remains */

    dst->remcount = 0;
    dst->p32[0] = data->resample_out_p[0];
    dst->p32[1] = data->resample_out_p[1];

    if (src->remcount > 0)
    {
        dst->bufcount = RESAMPLE_BUF_COUNT;

        int consumed = resample_hermite(data, src, dst);

        /* Advance src by consumed amount */
        if (consumed > 0)
            dsp_advance_buffer32(src, consumed);
    }
    /* else purged resample_buf */

    /* Inherit in-place processed mask from source buffer */
    dst->proc_mask = src->proc_mask;
}

/* Finish draining old samples then switch format or shut off */
static intptr_t resample_new_format(struct dsp_proc_entry *this,
                                    struct dsp_config *dsp,
                                    struct sample_format *format)
{
    struct resample_data *data = (void *)this->data;
    struct dsp_buffer *dst = &data->resample_buf;

    if (dst->remcount > 0)
        return PROC_NEW_FORMAT_TRANSITION;

    DSP_PRINT_FORMAT(DSP_PROC_RESAMPLE, *format);

    unsigned int frequency = data->frequency;
    unsigned int fout = dsp_get_output_frequency(dsp);
    bool active = dsp_proc_active(dsp, DSP_PROC_RESAMPLE);

    if ((unsigned int)format->frequency != frequency ||
        data->frequency_out != fout)
    {
        DEBUGF("  DSP_PROC_RESAMPLE- new settings: %u %u\n",
               format->frequency, fout);
        active = resample_new_delta(data, format, fout);
        dsp_proc_activate(dsp, DSP_PROC_RESAMPLE, active);
    }

    /* Everything after us is fout */
    dst->format = *format;
    dst->format.frequency = fout;
    dst->format.codec_frequency = fout;

    if (active)
        return PROC_NEW_FORMAT_OK;

    /* No longer needed */
    DEBUGF("  DSP_PROC_RESAMPLE- deactivated\n");
    return PROC_NEW_FORMAT_DEACTIVATED;
}

static void INIT_ATTR resample_dsp_init(struct dsp_config *dsp,
                                        enum dsp_ids dsp_id)
{
    int32_t *lbuf, *rbuf;

    switch (dsp_id)
    {
    case CODEC_IDX_AUDIO:
        lbuf = resample_out_bufs[0];
        rbuf = resample_out_bufs[1];
        break;

    case CODEC_IDX_VOICE:
        lbuf = rbuf = resample_out_bufs[2]; /* Always mono */
        break;

    default:
        /* huh? */
        DEBUGF("DSP_PROC_RESAMPLE- unknown DSP %d\n", (int)dsp_id);
        return;
    }

    /* Always enable resampler so that format changes may be monitored and
     * it self-activated when required */
    dsp_proc_enable(dsp, DSP_PROC_RESAMPLE, true);
    resample_data[dsp_id].resample_out_p[0] = lbuf;
    resample_data[dsp_id].resample_out_p[1] = rbuf;
}

static void INIT_ATTR resample_proc_init(struct dsp_proc_entry *this,
                                         struct dsp_config *dsp)
{
    struct resample_data *data = &resample_data[dsp_get_id(dsp)];
    this->data = (intptr_t)data;
    dsp_proc_set_in_place(dsp, DSP_PROC_RESAMPLE, false);
    data->frequency_out = DSP_OUT_DEFAULT_HZ;
    this->process = resample_process;
}

/* DSP message hook */
static intptr_t resample_configure(struct dsp_proc_entry *this,
                                   struct dsp_config *dsp,
                                   unsigned int setting,
                                   intptr_t value)
{
    intptr_t retval = 0;

    switch (setting)
    {
    case DSP_INIT:
        resample_dsp_init(dsp, (enum dsp_ids)value);
        break;

    case DSP_FLUSH:
        resample_flush(this);
        break;

    case DSP_PROC_INIT:
        resample_proc_init(this, dsp);
        break;

    case DSP_PROC_CLOSE:
        /* This stage should be enabled at all times */
        DEBUGF("DSP_PROC_RESAMPLE- Error: Closing!\n");
        break;

    case DSP_PROC_NEW_FORMAT:
        retval = resample_new_format(this, dsp, (struct sample_format *)value);
        break;

    case DSP_SET_OUT_FREQUENCY:
        dsp_proc_want_format_update(dsp, DSP_PROC_RESAMPLE);
        break;
    }

    return retval;
}

/* Database entry */
DSP_PROC_DB_ENTRY(RESAMPLE,
                  resample_configure);
