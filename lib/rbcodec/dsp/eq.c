/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Thom Johansen
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
#include "rbcodecconfig.h"
#include "fixedpoint.h"
#include "fracmul.h"
#include "dsp_filter.h"
#include "dsp_proc_entry.h"
#include "dsp_core.h"
#include "dsp_misc.h"
#include "eq.h"
#include "pga.h"
#include "replaygain.h"
#include <string.h>

/**
 * Current setup is one lowshelf filters eight peaking filters and one
 *  highshelf filter. Varying the number of shelving filters make no sense,
 *  but adding peaking filters is possible. Check EQ_NUM_BANDS to have
 *  2 shelving filters and EQ_NUM_BANDS-2 peaking filters.
 */

#if EQ_NUM_BANDS < 3
/* No good. Expect at least 1 peaking and low/high shelving filters */
#error Band count must be greater than or equal to 3
#endif

/* Cached band settings */
static struct eq_band_setting settings[EQ_NUM_BANDS];

static struct eq_state
{
    uint32_t enabled;                        /* Mask of enabled bands */
    uint8_t bands[EQ_NUM_BANDS+1];           /* Indexes of enabled bands */
    struct dsp_filter filters[EQ_NUM_BANDS]; /* Data for each filter */
} eq_data IBSS_ATTR;

#define FOR_EACH_ENB_BAND(b) \
    for (uint8_t *b = eq_data.bands; *b < EQ_NUM_BANDS; b++)

/* Clear histories of all enabled bands */
static void eq_flush(void)
{
    if (eq_data.enabled == 0)
        return; /* Not initialized yet/no bands on */

    FOR_EACH_ENB_BAND(b)
        filter_flush(&eq_data.filters[*b]);
}

static void update_band_filter(int band, unsigned int fout)
{
    /* Convert user settings to format required by coef generator
       functions */
    typeof (filter_pk_coefs) *coef_gen = filter_pk_coefs;

    /* Only first and last bands are not peaking filters */
    if (band == 0)
        coef_gen = filter_ls_coefs;
    else if (band == EQ_NUM_BANDS-1)
        coef_gen = filter_hs_coefs;

    const struct eq_band_setting *setting = &settings[band];
    struct dsp_filter *filter = &eq_data.filters[band];

    coef_gen(fp_div(setting->cutoff, fout, 32), setting->q ?: 1,
             setting->gain, filter);
}

/* Resync all bands to a new DSP output frequency */
static void update_samplerate(unsigned int fout)
{
    if (eq_data.enabled == 0)
        return; /* Not initialized yet/no bands on */

    FOR_EACH_ENB_BAND(b)
        update_band_filter(*b, fout);
}

/** DSP interface **/

/* Set the precut gain value */
void dsp_set_eq_precut(int precut)
{
    pga_set_gain(PGA_EQ_PRECUT, get_replaygain_int(precut * -10));
}

/* Update the filter configuration for the band */
void dsp_set_eq_coefs(int band, const struct eq_band_setting *setting)
{
    if (band < 0 || band >= EQ_NUM_BANDS)
        return;

    settings[band] = *setting; /* cache setting */

    struct dsp_config *dsp = dsp_get_config(CODEC_IDX_AUDIO);

    /* NOTE: The coef functions assume the EMAC unit is in fractional mode,
       which it should be, since we're executed from the main thread. */

    uint32_t mask = eq_data.enabled;

    /* Assume a band is disabled if the gain is zero */
    mask &= ~BIT_N(band);

    if (setting->gain != 0)
    {
        mask |= BIT_N(band);
        update_band_filter(band, dsp_get_output_frequency(dsp));
    }

    if (mask == eq_data.enabled)
        return; /* No change in band-enable state */

    if (mask & BIT_N(band))
        filter_flush(&eq_data.filters[band]); /* Coming online */

    eq_data.enabled = mask;

    /* Only be active if there are bands to process - if EQ is off, then
       this call has no effect */
    dsp_proc_activate(dsp, DSP_PROC_EQUALIZER, mask != 0);
  
    /* Prepare list of enabled bands for efficient iteration */
    for (band = 0; mask != 0; mask &= mask - 1, band++)
        eq_data.bands[band] = (uint8_t)find_first_set_bit(mask);

    eq_data.bands[band] = EQ_NUM_BANDS;
}

/* Enable or disable the equalizer */
void dsp_eq_enable(bool enable)
{
    struct dsp_config *dsp = dsp_get_config(CODEC_IDX_AUDIO);
    bool enabled = dsp_proc_enabled(dsp, DSP_PROC_EQUALIZER);

    if (enable == enabled)
        return;

    dsp_proc_enable(dsp, DSP_PROC_EQUALIZER, enable);

    if (enable && eq_data.enabled != 0)
        dsp_proc_activate(dsp, DSP_PROC_EQUALIZER, true);
}

/* Apply EQ filters to those bands that have got it switched on. */
static void eq_process(struct dsp_proc_entry *this,
                       struct dsp_buffer **buf_p)
{
    struct dsp_buffer *buf = *buf_p;
    int count = buf->remcount;
    unsigned int channels = buf->format.num_channels;

    FOR_EACH_ENB_BAND(b)
        filter_process(&eq_data.filters[*b], buf->p32, count, channels);

    (void)this;
}

/* DSP message hook */
static intptr_t eq_configure(struct dsp_proc_entry *this,
                             struct dsp_config *dsp,
                             unsigned int setting,
                             intptr_t value)
{
    switch (setting)
    {
    case DSP_PROC_INIT:
        this->process = eq_process;
        /* Wouldn't have been getting frequency updates */
        update_samplerate(dsp_get_output_frequency(dsp));
        /* Fall-through */
    case DSP_PROC_CLOSE:
        pga_enable_gain(PGA_EQ_PRECUT, setting == DSP_PROC_INIT);
        break;
        
    case DSP_FLUSH:
        eq_flush();
        break;

    case DSP_SET_OUT_FREQUENCY:
        update_samplerate(value);
        break;
    }

    return 0;
    (void)dsp;
}

/* Database entry */
DSP_PROC_DB_ENTRY(EQUALIZER,
                  eq_configure);
