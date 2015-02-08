/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 Chiwen Chang
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
#include "sound.h"
#include "fixedpoint.h"
#include "fracmul.h"
#include "dsp_proc_entry.h"
#include "sample_delay.h"
#include "dsp_filter.h"
#include <string.h>


#define DLY_2MS 182
#define DEFAULT_DLY 128 /* ~1.4ms */
static int32_t ch_buf[DLY_2MS];
static int r = 0, w = 0,ch_dly=0;

static void dsp_sample_delay_flush(void)
{
    memset(ch_buf,0,sizeof(int32_t) * DLY_2MS);
}

void sample_delay_process(struct dsp_proc_entry *this,
                                 struct dsp_buffer **buf_p)
{
    struct dsp_buffer *buf = *buf_p;
    int count = buf->remcount;

    int32_t x;
    int ch  = (ch_dly > 0)?1:0; 
    int dly = (ch_dly > 0)?ch_dly:-ch_dly;

    for (int i = 0; i < count; i++)
    {
        x =  buf->p32[ch][i];
        buf->p32[ch][i] = dequeue(ch_buf, &r, dly);
        enqueue(x, ch_buf, &w, dly);
    }
    (void)this;
}

void dsp_sample_delay_config(int var)
{
    if (var == ch_dly)
        return; /* No setting change */

    bool was_enabled = (ch_dly != 0);
    ch_dly = var;

    bool now_enabled = (var != 0);

    if (was_enabled == now_enabled && !now_enabled)
        return;

    /* If changing status, enable or disable it; if already enabled push
       additional DSP_PROC_INIT messages with value = 1 to force-update the
       filters */
    struct dsp_config *dsp = dsp_get_config(CODEC_IDX_AUDIO);
    dsp_proc_enable(dsp, DSP_PROC_SAMPLE_DELAY, now_enabled);
}

/* DSP message hook */
static intptr_t sample_delay_configure(struct dsp_proc_entry *this,
                                       struct dsp_config *dsp,
                                       unsigned int setting,
                                       intptr_t value)
{
   switch (setting)
    {
    case DSP_PROC_INIT:
        if (value == 0)
        {
            /* Coming online; was disabled */
            this->process = sample_delay_process;
            dsp_sample_delay_flush();
            dsp_proc_activate(dsp, DSP_PROC_SAMPLE_DELAY, true);
        }
        /* else additional forced messages */

        break;

    case DSP_FLUSH:
        /* Discontinuity; clear filters */
        dsp_sample_delay_flush();
        break;

    case DSP_SET_OUT_FREQUENCY:
        /* New output frequency */

        break;
    }

    return 1;
}

/* Database entry */
DSP_PROC_DB_ENTRY(
    SAMPLE_DELAY,
    sample_delay_configure);
