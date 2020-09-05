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
#include "surround.h"
#include "config.h"
#include "fixedpoint.h"
#include "fracmul.h"
#include "settings.h"
#include "dsp_proc_entry.h"
#include "dsp_filter.h"
#include "core_alloc.h"

static int surround_balance = 0;
static bool surround_side_only = false;
static int surround_mix = 100;
static int surround_strength = 0;
/*1 sample ~ 11ns */
#define DLY_5MS  454
#define DLY_8MS  727
#define DLY_10MS 909
#define DLY_15MS 1363
#define DLY_30MS 2727
#define MAX_DLY DLY_30MS

#define B0_DLY  (MAX_DLY/8 + 1)
#define B2_DLY  (MAX_DLY   + 1)
#define BB_DLY  (MAX_DLY/4 + 1)
#define HH_DLY  (MAX_DLY/2 + 1)
#define CL_DLY  B2_DLY

/*voice from 300hz - 3400hz ?*/
static int32_t tcoef1,tcoef2,bcoef,hcoef;

static int dly_size = MAX_DLY;
static int cutoff_l = 320;
static int cutoff_h = 3400;

static int b0_r=0,b0_w=0,
           b2_r=0,b2_w=0,
           bb_r=0,bb_w=0,
           hh_r=0,hh_w=0,
           cl_r=0,cl_w=0;
static int handle = -1;

#define SURROUND_BUFSIZE ((B0_DLY + B2_DLY + BB_DLY + HH_DLY + CL_DLY)*sizeof (int32_t))

static int surround_buffer_alloc(void)
{
    handle = core_alloc("dsp_surround_buffer", SURROUND_BUFSIZE);
    return handle;
}

static void surround_buffer_free(void)
{
    if (handle < 0)
        return;

    core_free(handle);
    handle = -1;
}

static void dsp_surround_flush(void)
{
    if (handle >= 0)
        memset(core_get_data(handle), 0, SURROUND_BUFSIZE);
}

static void surround_update_filter(unsigned int fout)
{
    tcoef1 = fp_div(cutoff_l, fout, 31);
    tcoef2 = fp_div(cutoff_h, fout, 31);
    bcoef  = fp_div(cutoff_l / 2, fout, 31);
    hcoef  = fp_div(cutoff_h * 2, fout, 31);
}

void dsp_surround_set_balance(int var)
{
    surround_balance = var;
}

void dsp_surround_side_only(bool var)
{
    surround_side_only = var;
}

void dsp_surround_mix(int var)
{
    surround_mix = var;
}

void dsp_surround_set_cutoff(int frq_l, int frq_h)
{
    if (cutoff_l == frq_l && cutoff_h == frq_h)
        return; /* No settings change */

    cutoff_l = frq_l;/*fx2*/
    cutoff_h = frq_h;/*fx1*/

    struct dsp_config *dsp = dsp_get_config(CODEC_IDX_AUDIO);
    if (!dsp_proc_enabled(dsp, DSP_PROC_SURROUND))
        return;

    surround_update_filter(dsp_get_output_frequency(dsp));
}

static void surround_set_stepsize(int surround_strength)
{
    if (handle >= 0)
        dsp_surround_flush();

    switch(surround_strength)
    {
    case 1:
        dly_size =  DLY_5MS;
        break;
    case 2:
        dly_size =  DLY_8MS;
        break;
    case 3:
        dly_size =  DLY_10MS;
        break;
    case 4:
        dly_size =  DLY_15MS;
        break;
    case 5:
        dly_size =  DLY_30MS;
        break;
    }
}

void dsp_surround_enable(int var)
{
    if (var == surround_strength)
        return; /* No setting change */

    surround_strength = var;

    struct dsp_config *dsp = dsp_get_config(CODEC_IDX_AUDIO);
    bool was_enabled = dsp_proc_enabled(dsp, DSP_PROC_SURROUND);
    bool now_enabled = var > 0;

    if (was_enabled == now_enabled)
        return; /* No change in enabled status */

    if (now_enabled)
        surround_set_stepsize(var);

    /* If changing status, enable or disable it; if already enabled push
       additional DSP_PROC_INIT messages with value = 1 to force-update the
       filters */
    dsp_proc_enable(dsp, DSP_PROC_SURROUND, now_enabled);
}

static void surround_process(struct dsp_proc_entry *this,
                               struct dsp_buffer **buf_p)
{

    struct dsp_buffer *buf = *buf_p;
    int count = buf->remcount;

    int dly_shift3 = dly_size/8;
    int dly_shift2 = dly_size/4;
    int dly_shift1 = dly_size/2;
    int dly = dly_size;
    int i;
    int32_t x;

    /*only need to buffer right channel */
    static int32_t *b0, *b2, *bb, *hh, *cl;

    b0 = core_get_data(handle);
    b2 = b0 + B0_DLY;
    bb = b2 + B2_DLY;
    hh = bb + BB_DLY;
    cl = hh + HH_DLY;

    for (i = 0; i < count; i++)
    {
        int32_t mid = buf->p32[0][i] / 2 + buf->p32[1][i] / 2;
        int32_t side = buf->p32[0][i] - buf->p32[1][i];
        int32_t temp0, temp1;

        if (!surround_side_only)
        {
            /*clone the left channal*/
            temp0 = buf->p32[0][i];
            /*keep the middle band of right channel*/
            temp1 = FRACMUL(buf->p32[1][i], tcoef1) -
                    FRACMUL(buf->p32[1][i], tcoef2);
        }
        else /* apply haas to side only*/
        {
            temp0 = side / 2;
            temp1 = FRACMUL(-side, tcoef1) / 2 -
                    FRACMUL(-side, tcoef2) / 2;
        }

        /* inverted crossfeed delay (left channel) to make sound wider*/
        x = temp1/100 * 35;
        temp0 += dequeue(cl, &cl_r, dly);
        enqueue(-x, cl, &cl_w, dly);

        /* apply 1/8 delay to frequency below fx2 */
        x = buf->p32[1][i] - FRACMUL(buf->p32[1][i], tcoef1);
        temp1 += dequeue(b0, &b0_r, dly_shift3);
        enqueue(x, b0, &b0_w, dly_shift3 );

        /* cut frequency below half fx2*/
        temp1 = FRACMUL(temp1, bcoef);

        /* apply 1/4 delay to frequency below half fx2 */
        /* use different delay to fake the sound direction*/
        x = buf->p32[1][i] - FRACMUL(buf->p32[1][i], bcoef);
        temp1 += dequeue(bb, &bb_r, dly_shift2);
        enqueue(x, bb, &bb_w, dly_shift2 );

        /* apply full delay to higher band */
        x = FRACMUL(buf->p32[1][i], tcoef2);
        temp1 += dequeue(b2, &b2_r, dly);
        enqueue(x, b2, &b2_w, dly );

        /* do the same direction trick again */
        temp1 -= FRACMUL(temp1, hcoef);

        x = FRACMUL(buf->p32[1][i], hcoef);
        temp1 += dequeue(hh, &hh_r, dly_shift1);
        enqueue(x, hh, &hh_w, dly_shift1 );
        /*balance*/
        if (surround_balance > 0  && !surround_side_only)
        {
            temp0 -= temp0/200  * surround_balance;
            temp1 += temp1/200  * surround_balance;
        }
        else if (surround_balance > 0)
        {
            temp0 += temp0/200  * surround_balance;
            temp1 -= temp1/200  * surround_balance;
        }

        if  (surround_side_only)
        {
            temp0 += mid;
            temp1 += mid;
        }

        if (surround_mix == 100)
        {
            buf->p32[0][i] = temp0;
            buf->p32[1][i] = temp1;
        }
        else
        {
            /*dry wet mix*/
            buf->p32[0][i] = buf->p32[0][i]/100 * (100-surround_mix) +
                             temp0/100 * surround_mix;
            buf->p32[1][i] = buf->p32[1][i]/100 * (100-surround_mix) +
                             temp1/100 * surround_mix;
        }
    }
    (void)this;
}

/* Handle format changes and verify the format compatibility */
static intptr_t surround_new_format(struct dsp_proc_entry *this,
                                    struct dsp_config *dsp,
                                    struct sample_format *format)
{
    DSP_PRINT_FORMAT(DSP_PROC_SURROUND, *format);

    /* Stereo mode only */
    bool was_active = dsp_proc_active(dsp, DSP_PROC_SURROUND);
    bool now_active = format->num_channels > 1;
    dsp_proc_activate(dsp, DSP_PROC_SURROUND, now_active);

    if (now_active)
    {
        if (!was_active)
            dsp_surround_flush(); /* Going online */

        return PROC_NEW_FORMAT_OK;
    }

    /* Can't do this. Sleep until next change. */
    DEBUGF("  DSP_PROC_SURROUND- deactivated\n");
    return PROC_NEW_FORMAT_DEACTIVATED;

    (void)this;
}

/* DSP message hook */
static intptr_t surround_configure(struct dsp_proc_entry *this,
                                     struct dsp_config *dsp,
                                     unsigned int setting,
                                     intptr_t value)
{
    intptr_t retval = 0;

    switch (setting)
    {
    case DSP_PROC_INIT:
        /* Coming online; was disabled */
        retval = surround_buffer_alloc();
        if (retval < 0)
            break;

        this->process = surround_process;

        dsp_surround_flush();

        /* Wouldn't have been getting frequency updates */
        surround_update_filter(dsp_get_output_frequency(dsp));
        break;

    case DSP_PROC_CLOSE:
        /* Being disabled (called also if init fails) */
        surround_buffer_free();
        break;

    case DSP_FLUSH:
        /* Discontinuity; clear filters */
        dsp_surround_flush();
        break;

    case DSP_SET_OUT_FREQUENCY:
        /* New output frequency */
        surround_update_filter(value);
        break;

    case DSP_PROC_NEW_FORMAT:
        /* Source buffer format is changing (also sent when first enabled) */
        retval = surround_new_format(this, dsp, (struct sample_format *)value);
        break;
    }

    return retval;
}

/* Database entry */
DSP_PROC_DB_ENTRY(
    SURROUND,
    surround_configure);
