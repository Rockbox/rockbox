/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Chiwen Chang
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
#include "config.h"
#include "fixedpoint.h"
#include "fracmul.h"
#include "settings.h"
#include "dsp_proc_entry.h"
#include "dsp_misc.h"
#include "dsp_filter.h"

/* Perceptual bass enhancement */

#define B3_DLY 72    /* ~800 uS */
#define B2_DLY 180   /* ~2000 uS*/
#define B0_DLY 276   /* ~3050 uS */

#define B3_SIZE (B3_DLY+1)
#define B2_SIZE (B2_DLY+1)
#define B0_SIZE (B0_DLY+1)

static int pbe_strength = 100;
static int pbe_precut = 0;
static int32_t tcoef1, tcoef2, tcoef3;
static int32_t b0[2][B0_SIZE], b2[2][B2_SIZE], b3[2][B3_SIZE];
static int b0_r[2],b2_r[2],b3_r[2],b0_w[2],b2_w[2],b3_w[2];
int32_t temp_buffer;
static struct dsp_filter pbe_filter IBSS_ATTR;

static void dsp_pbe_flush(void)
{
    memset(b0[0], 0, B0_DLY * sizeof(int32_t));
    memset(b0[1], 0, B0_DLY * sizeof(int32_t));
    memset(b2[0], 0, B2_DLY * sizeof(int32_t));
    memset(b2[1], 0, B2_DLY * sizeof(int32_t));
    memset(b3[0], 0, B3_DLY * sizeof(int32_t));
    memset(b3[1], 0, B3_DLY * sizeof(int32_t));
    b0_r[0] = 0; b0_w[0] = B0_DLY;
    b0_r[1] = 0; b0_w[1] = B0_DLY;
    b2_r[0] = 0; b2_w[0] = B2_DLY;
    b2_r[1] = 0; b2_w[1] = B2_DLY;
    b3_r[0] = 0; b3_w[0] = B3_DLY;
    b3_r[1] = 0; b3_w[1] = B3_DLY;

    filter_flush(&pbe_filter);
}

static void pbe_update_filter(unsigned int fout)
{
    tcoef1 = fp_div(160, fout, 31);
    tcoef2 = fp_div(500, fout, 31);
    tcoef3 = fp_div(1150, fout, 31);

    filter_bishelf_coefs(fp_div(200, fout, 32),
                         fp_div(1280, fout, 32),
                         50, 30, -5 + pbe_precut,
                         &pbe_filter);
}

void dsp_pbe_precut(int var)
{
    if (var == pbe_precut)
        return; /* No change */

    pbe_precut = var;

    if (pbe_strength == 0)
        return; /* Not currently enabled */

    /* Push more DSP_PROC_INIT messages to force filter updates
       (with value = 1) */
    struct dsp_config *dsp = dsp_get_config(CODEC_IDX_AUDIO);
    dsp_proc_enable(dsp, DSP_PROC_PBE, true);
}

void dsp_pbe_enable(int var)
{
    if (var == pbe_strength)
        return; /* No change */
    bool was_enabled = pbe_strength > 0;
    pbe_strength = var;

    bool now_enabled = var > 0;

    if (now_enabled == was_enabled)
        return; /* No change in enabled status */

    struct dsp_config *dsp = dsp_get_config(CODEC_IDX_AUDIO);
    dsp_proc_enable(dsp, DSP_PROC_PBE, now_enabled);
}

static void pbe_process(struct dsp_proc_entry *this,
                        struct dsp_buffer **buf_p)
{
    struct dsp_buffer *buf = *buf_p;
    int count = buf->remcount;
    int num_channels = buf->format.num_channels;
    int b2_level = (B2_DLY * pbe_strength) / 100;
    int b0_level = (B0_DLY * pbe_strength) / 100;
    int32_t x;

    for(int ch = 0; ch < num_channels; ch++)
    {
        for (int i = 0; i < count; i++)
        {
            /* 160hz - 500hz no delay */
            temp_buffer = FRACMUL(buf->p32[ch][i], tcoef1) -
                          FRACMUL(buf->p32[ch][i], tcoef2);

            /* delay below 160hz*/
            x = buf->p32[ch][i] -
                FRACMUL(buf->p32[ch][i], tcoef1);
            temp_buffer += dequeue(b0[ch], &b0_r[ch], b0_level);
            enqueue(x, b0[ch], &b0_w[ch], b0_level );

            /* delay 500-1150hz */
            x = FRACMUL(buf->p32[ch][i], tcoef2) -
                FRACMUL(buf->p32[ch][i], tcoef3);
            temp_buffer += dequeue(b2[ch], &b2_r[ch], b2_level);
            enqueue(x, b2[ch], &b2_w[ch], b2_level );

            /* delay anything beyond 1150hz */
            x = FRACMUL(buf->p32[ch][i], tcoef3);
            temp_buffer += dequeue(b3[ch], &b3_r[ch], B3_DLY);
            enqueue(x, b3[ch], &b3_w[ch], B3_DLY );

            buf->p32[ch][i] = temp_buffer;
        }
    }

    /* bishelf boost */
    filter_process(&pbe_filter, buf->p32, buf->remcount,
                   buf->format.num_channels);

    (void)this;
}

/* DSP message hook */
static intptr_t pbe_configure(struct dsp_proc_entry *this,
                                     struct dsp_config *dsp,
                                     unsigned int setting,
                                     intptr_t value)
{
    /* This only attaches to the audio (codec) DSP */

    switch (setting)
    {
    case DSP_PROC_INIT:
        if (value == 0)
        {
            /* Coming online; was disabled */
            this->process = pbe_process;
            dsp_pbe_flush();
            dsp_proc_activate(dsp, DSP_PROC_PBE, true);
        }
        /* else additional forced messages */

        pbe_update_filter(dsp_get_output_frequency(dsp));
        break;

    case DSP_FLUSH:
        /* Discontinuity; clear filters */
        dsp_pbe_flush();
        break;

    case DSP_SET_OUT_FREQUENCY:
        /* New output frequency */
        pbe_update_filter(value);
        break;
    }

    return 1;
}

/* Database entry */
DSP_PROC_DB_ENTRY(
    PBE,
    pbe_configure);
