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

static bool surround_enabled = false;
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
/*only need to buffer right channel */
static int32_t *b0, *b2, *bb, *hh, *cl;
static int32_t temp_buffer[2];
static int32_t mid, side;

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

static void surround_buffer_alloc(void)
{
    if (handle > 0)
        return; /* already-allocated */

    unsigned int total_len = B0_DLY + B2_DLY + BB_DLY + HH_DLY + CL_DLY;
    handle = core_alloc("dsp_surround_buffer",sizeof(int32_t) * total_len);

    if (handle < 0)
    {
        surround_enabled = false;
        return;
    }
    memset(core_get_data(handle),0,sizeof(int32_t) * total_len);
}

static void surround_buffer_get_data(void)
{
    if (handle < 0)
        return;
    b0 = core_get_data(handle);
    b2 = b0 + B0_DLY;
    bb = b2 + B2_DLY;
    hh = bb + BB_DLY;
    cl = hh + HH_DLY;
}

static void dsp_surround_flush(void)
{
    if (!surround_enabled)
        return;

    unsigned int total_len = B0_DLY + B2_DLY + BB_DLY + HH_DLY + CL_DLY;
    if (handle > 0)
        memset(core_get_data(handle),0,sizeof(int32_t) * total_len);
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
    dsp_surround_flush();
    surround_side_only = var;
}

void dsp_surround_mix(int var)
{
    surround_mix = var;
}

void dsp_surround_set_cutoff(int frq_l, int frq_h)
{
    cutoff_l = frq_l;/*fx2*/
    cutoff_h = frq_h;/*fx1*/

    struct dsp_config *dsp = dsp_get_config(CODEC_IDX_AUDIO);
    unsigned int fout = dsp_get_output_frequency(dsp);
    surround_update_filter(fout);
}

static void surround_set_stepsize(int surround_strength)
{
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

    bool was_enabled = surround_strength > 0;
    surround_strength = var;
    surround_set_stepsize(surround_strength);

    bool now_enabled = var > 0;

    if (was_enabled == now_enabled && !now_enabled)
        return; /* No change in enabled status */

    if (now_enabled == false && handle > 0)
    {
        core_free(handle);
        handle = -1;
    }
    surround_enabled = now_enabled;

    struct dsp_config *dsp = dsp_get_config(CODEC_IDX_AUDIO);
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

    surround_buffer_get_data();

    for (i = 0; i < count; i++)
    {
        mid = buf->p32[0][i] /2 + buf->p32[1][i] /2;
        side = buf->p32[0][i] - buf->p32[1][i];

        if (!surround_side_only)
        {
            /*clone the left channal*/
            temp_buffer[0]= buf->p32[0][i];
            /*keep the middle band of right channel*/
            temp_buffer[1]= FRACMUL(buf->p32[1][i], tcoef1) -
                            FRACMUL(buf->p32[1][i], tcoef2);
        }
        else /* apply haas to side only*/
        {
            temp_buffer[0] = side / 2;
            temp_buffer[1] = FRACMUL(-side,tcoef1)/2 -
                             FRACMUL(-side, tcoef2)/2;
        }

        /* inverted crossfeed delay (left channel) to make sound wider*/
        x = temp_buffer[1]/100 * 35;
        temp_buffer[0] += dequeue(cl, &cl_r, dly);
        enqueue(-x, cl, &cl_w, dly);

        /* apply 1/8 delay to frequency below fx2 */
        x = buf->p32[1][i] - FRACMUL(buf->p32[1][i], tcoef1);
        temp_buffer[1] += dequeue(b0, &b0_r, dly_shift3);
        enqueue(x, b0, &b0_w, dly_shift3 );

        /* cut frequency below half fx2*/
        temp_buffer[1] = FRACMUL(temp_buffer[1], bcoef);

        /* apply 1/4 delay to frequency below half fx2 */
        /* use different delay to fake the sound direction*/
        x = buf->p32[1][i] - FRACMUL(buf->p32[1][i], bcoef);
        temp_buffer[1] += dequeue(bb, &bb_r, dly_shift2);
        enqueue(x, bb, &bb_w, dly_shift2 );

        /* apply full delay to higher band */
        x = FRACMUL(buf->p32[1][i], tcoef2);
        temp_buffer[1] += dequeue(b2, &b2_r, dly);
        enqueue(x, b2, &b2_w, dly );

        /* do the same direction trick again */
        temp_buffer[1] -= FRACMUL(temp_buffer[1], hcoef);

        x = FRACMUL(buf->p32[1][i], hcoef);
        temp_buffer[1] += dequeue(hh, &hh_r, dly_shift1);
        enqueue(x, hh, &hh_w, dly_shift1 );
        /*balance*/
        if (surround_balance > 0  && !surround_side_only)
        {
            temp_buffer[0] -= temp_buffer[0]/200  * surround_balance;
            temp_buffer[1] += temp_buffer[1]/200  * surround_balance;
        }
        else if (surround_balance > 0)
        {
            temp_buffer[0] += temp_buffer[0]/200  * surround_balance;
            temp_buffer[1] -= temp_buffer[1]/200  * surround_balance;
        }

        if  (surround_side_only)
        {
            temp_buffer[0] += mid;
            temp_buffer[1] += mid;
        }

        if (surround_mix == 100)
        {
            buf->p32[0][i] = temp_buffer[0];
            buf->p32[1][i] = temp_buffer[1];
        }
        else
        {
            /*dry wet mix*/
            buf->p32[0][i] = buf->p32[0][i]/100 * (100-surround_mix) +
                             temp_buffer[0]/100 * surround_mix;
            buf->p32[1][i] = buf->p32[1][i]/100 * (100-surround_mix) +
                             temp_buffer[1]/100 * surround_mix;
        }
    }
    (void)this;
}

/* DSP message hook */
static intptr_t surround_configure(struct dsp_proc_entry *this,
                                     struct dsp_config *dsp,
                                     unsigned int setting,
                                     intptr_t value)
{
    unsigned int fout = dsp_get_output_frequency(dsp);
    switch (setting)
    {
    case DSP_PROC_INIT:
        if (value == 0)
        {
            this->process = surround_process;
            surround_buffer_alloc();
            dsp_surround_flush();
            dsp_proc_activate(dsp, DSP_PROC_SURROUND, true);
        }
        else
            surround_update_filter(fout);
        break;
    case DSP_FLUSH:
        dsp_surround_flush();
        break;
   case DSP_SET_OUT_FREQUENCY:
        surround_update_filter(value);
        break;
   case DSP_PROC_CLOSE:
        break;
    }

    return 1;
    (void)dsp;
}

/* Database entry */
DSP_PROC_DB_ENTRY(
    SURROUND,
    surround_configure);

