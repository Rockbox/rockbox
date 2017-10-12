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
#include "afr.h"
#include "config.h"
#include "fixedpoint.h"
#include "fracmul.h"
#include "settings.h"
#include "dsp_proc_entry.h"
#include "dsp_filter.h"

/* Auditory fatigue reduction */

static int afr_strength = 0;
static struct dsp_filter afr_filters[4];

static void dsp_afr_flush(void)
{
    for (int i = 0 ;i < 4;i++)
        filter_flush(&afr_filters[i]);
}

static void strength_update(unsigned int fout)
{
    int hs=0, ls=0, drop3k=0;

    switch (afr_strength)
    {
    /* not called if afr_strength == 0 (disabled) */
    case 1:
        hs=-29; ls=0; drop3k=-13;
        break;
    case 2:
        hs=-58; ls=-5; drop3k=-29;
        break;
    case 3:
        hs=-72; ls=-5; drop3k=-48;
        break;
    }

    filter_bishelf_coefs(fp_div(8000, fout, 32),
                         fp_div(20000, fout, 32),
                         0, (hs/2), 0,
                         &afr_filters[0]);

    filter_pk_coefs(fp_div(8000, fout, 32), 28, hs,
                     &afr_filters[1]);

    filter_pk_coefs(fp_div(3150, fout, 32), 28, drop3k,
                     &afr_filters[2]);

    filter_bishelf_coefs(fp_div(200, fout, 32),
                         fp_div(1280, fout, 32),
                         ls, 0, 0,
                         &afr_filters[3]);
}

void dsp_afr_enable(int var)
{
    if (var == afr_strength)
        return; /* No setting change */

    afr_strength = var;

    struct dsp_config *dsp = dsp_get_config(CODEC_IDX_AUDIO);
    bool was_enabled = dsp_proc_enabled(dsp, DSP_PROC_AFR);
    bool now_enabled = var > 0;

    if (was_enabled == now_enabled && !now_enabled)
        return;

    /* If changing status, enable or disable it; if already enabled push
       additional DSP_PROC_INIT messages with value = 1 to force-update the
       filters */
    dsp_proc_enable(dsp, DSP_PROC_AFR, now_enabled);
}

static void afr_reduce_process(struct dsp_proc_entry *this,
                               struct dsp_buffer **buf_p)
{
    struct dsp_buffer *buf = *buf_p;

    for (int i = 0; i < 4; i++)
        filter_process(&afr_filters[i], buf->p32, buf->remcount,
                       buf->format.num_channels);

    (void)this;
}

/* DSP message hook */
static intptr_t afr_configure(struct dsp_proc_entry *this,
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
            this->process = afr_reduce_process;
            dsp_afr_flush();
            dsp_proc_activate(dsp, DSP_PROC_AFR, true);
        }
        /* else additional forced messages */

        strength_update(dsp_get_output_frequency(dsp));
        break;

    case DSP_FLUSH:
        /* Discontinuity; clear filters */
        dsp_afr_flush();
        break;

    case DSP_SET_OUT_FREQUENCY:
        /* New output frequency */
        strength_update(value);
        break;
    }

    return 1;
}

/* Database entry */
DSP_PROC_DB_ENTRY(
    AFR,
    afr_configure);
