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

static int pbe_strength = 100;
static int pbe_precut = 0;
static int32_t tcoef1, tcoef2, tcoef3;
static int32_t b0[2][B0_DLY], b2[2][B2_DLY], b3[2][B3_DLY];
static int32_t temp_buffer[2][3072 * 2];
static struct dsp_filter pbe_filter IBSS_ATTR;

static void dsp_pbe_flush(void)
{
    memset(b0[0], 0, B0_DLY * sizeof(int32_t));
    memset(b0[1], 0, B0_DLY * sizeof(int32_t));
    memset(b2[0], 0, B2_DLY * sizeof(int32_t));
    memset(b2[1], 0, B2_DLY * sizeof(int32_t));
    memset(b3[0], 0, B3_DLY * sizeof(int32_t));
    memset(b3[1], 0, B3_DLY * sizeof(int32_t));

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
    bool was_enabled = pbe_strength > 0;
    bool now_enabled = var > 0;

    pbe_strength = var;

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
    int b2_level = (B2_DLY * pbe_strength) / 100;
    int b0_level = (B0_DLY * pbe_strength) / 100;

    if (count < B0_DLY)
    {
        dsp_pbe_flush();
        return;
    }

    /* add 160hz - 500hz no delay */
    for (int i = 0; i < count; i++)
    {
        temp_buffer[0][i] = FRACMUL(buf->p32[0][i], tcoef1) -
                            FRACMUL(buf->p32[0][i], tcoef2);
        temp_buffer[1][i] = FRACMUL(buf->p32[1][i], tcoef1) -
                            FRACMUL(buf->p32[1][i], tcoef2);
    }

    /* filter below 160 and with delays */
    for (int i = 0; i < b0_level; i++)
    {
         temp_buffer[0][i] += b0[0][i];
         temp_buffer[1][i] += b0[1][i];
    }

    for (int i = 0; i < count-b0_level; i++)
    {
         temp_buffer[0][i+b0_level] += buf->p32[0][i] -
                                       FRACMUL(buf->p32[0][i], tcoef1);
         temp_buffer[1][i+b0_level] += buf->p32[1][i] -
                                       FRACMUL(buf->p32[1][i], tcoef1);
    }

    for (int i = 0; i < b0_level; i++)
    {
         b0[0][i] = buf->p32[0][count-b0_level+i] -
                    FRACMUL(buf->p32[0][count-b0_level+i], tcoef1);
         b0[1][i] = buf->p32[1][count-b0_level+i] -
                    FRACMUL(buf->p32[1][count-b0_level+i], tcoef1);
    }

    /* add 500-1150hz with delays */
    for (int i = 0; i < b2_level; i++)
    {
        temp_buffer[0][i] += b2[0][i];
        temp_buffer[1][i] += b2[1][i];
    }

    for (int i = 0; i < count-b2_level; i++)
    {
        temp_buffer[0][i+b2_level] += FRACMUL(buf->p32[0][i], tcoef2) -
                                      FRACMUL(buf->p32[0][i], tcoef3);
        temp_buffer[1][i+b2_level] += FRACMUL(buf->p32[1][i], tcoef2) -
                                      FRACMUL(buf->p32[1][i], tcoef3);
    }

    for (int i = 0; i < b2_level; i++)
    {
         b2[0][i] = FRACMUL(buf->p32[0][count-b2_level+i], tcoef2) -
                    FRACMUL(buf->p32[0][count-b2_level+i], tcoef3);
         b2[1][i] = FRACMUL(buf->p32[1][count-b2_level+i], tcoef2) -
                    FRACMUL(buf->p32[1][count-b2_level+i], tcoef3);
    }

    /* add above 1150 with delays */
    for (int i = 0; i < B3_DLY; i++)
    {
        temp_buffer[0][i] += b3[0][i];
        temp_buffer[1][i] += b3[1][i];
    }

    for (int i = 0; i < count-B3_DLY; i++)
    {
        temp_buffer[0][i+B3_DLY] += FRACMUL(buf->p32[0][i], tcoef3);
        temp_buffer[1][i+B3_DLY] += FRACMUL(buf->p32[1][i], tcoef3);
    }

    for (int i = 0; i < B3_DLY; i++)
    {
        b3[0][i] = FRACMUL(buf->p32[0][count-B3_DLY+i], tcoef3);
        b3[1][i] = FRACMUL(buf->p32[1][count-B3_DLY+i], tcoef3);
    }

    /* end of repair */
    memcpy(buf->p32[0], temp_buffer[0],count * sizeof(int32_t));
    memcpy(buf->p32[1], temp_buffer[1],count * sizeof(int32_t));

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
