/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Magnus Holmgren
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
#include "config.h"
#include "system.h"
#include "dsp-util.h"
#include "fixedpoint.h"
#include "fracmul.h"
#include "dsp_proc_entry.h"
#include "pga.h"

/* Implemented here or in target assembly code */
void pga_process(struct dsp_proc_entry *this, struct dsp_buffer **buf_p);

#define DEFAULT_PGA_GAIN (PGA_UNITY >> 1) /* s8.23 format */

static struct pga_data
{
    int32_t gain;                 /* 00h: Final gain in s8.23 format */
    uint32_t enabled;             /* Mask of enabled gains */
    int32_t gains[PGA_NUM_GAINS]; /* Individual gains in s7.24 format */
} pga_data =
{
    .gain = DEFAULT_PGA_GAIN,
    .enabled = 0,
    .gains[0 ... PGA_NUM_GAINS-1] = PGA_UNITY,
};

/* Combine all gains to a global gain and enable/disable the amplifier if
   the overall gain is not unity/unity */
static void pga_update(void)
{
    int32_t gain = PGA_UNITY;

    /* Multiply all gains with one another to get overall amp gain */
    for (int i = 0; i < PGA_NUM_GAINS; i++)
    {
        if (pga_data.enabled & BIT_N(i)) /* Only enabled gains factor in */
            gain = fp_mul(gain, pga_data.gains[i], 24);
    }

    gain >>= 1; /* s7.24 -> s8.23 format */

    if (gain == pga_data.gain)
        return;

    struct dsp_config *dsp = dsp_get_config(CODEC_IDX_AUDIO);
    pga_data.gain = gain;
    dsp_proc_enable(dsp, DSP_PROC_PGA, gain != DEFAULT_PGA_GAIN);
    dsp_proc_activate(dsp, DSP_PROC_PGA, true);
}


/** Amp controls **/

/* Set a particular gain value - doesn't have to be enabled */
void pga_set_gain(enum pga_gain_ids id, int32_t value)
{
    if (value == pga_data.gains[id])
        return;

    pga_data.gains[id] = value;

    if (BIT_N(id) & pga_data.enabled)
        pga_update();
}

/* Enable or disable the specified gain stage */
void pga_enable_gain(enum pga_gain_ids id, bool enable)
{
    uint32_t bit = BIT_N(id);

    if (enable != !(pga_data.enabled & bit))
        return;

    pga_data.enabled ^= bit;
    pga_update();
}


/** DSP interface **/

#if !defined(CPU_COLDFIRE) && !defined(CPU_ARM)
/* Apply a constant gain to the samples (e.g., for ReplayGain). */
void pga_process(struct dsp_proc_entry *this, struct dsp_buffer **buf_p)
{
    int32_t gain = ((struct pga_data *)this->data)->gain;
    struct dsp_buffer *buf = *buf_p;
    unsigned int channels = buf->format.num_channels;

    for (unsigned int ch = 0; ch < channels; ch++)
    {
        int32_t *d = buf->p32[ch];
        int count = buf->remcount;

        for (int i = 0; i < count; i++)
            d[i] = FRACMUL_SHL(d[i], gain, 8);
    }

    (void)this;
}
#endif /* CPU */

/* DSP message hook */
static intptr_t pga_configure(struct dsp_proc_entry *this,
                              struct dsp_config *dsp,
                              unsigned int setting,
                              intptr_t value)
{
    switch (setting)
    {
    case DSP_PROC_INIT:
        if (value != 0)
            break; /* Already initialized */
        this->data = (intptr_t)&pga_data;
        this->process[0] = pga_process;
        break;
    }

    return 1;
    (void)dsp;
}

/* Database entry */
DSP_PROC_DB_ENTRY(PGA,
                  pga_configure);
