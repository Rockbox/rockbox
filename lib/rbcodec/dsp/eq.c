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
#include "fixedpoint.h"
#include "fracmul.h"
#include "dsp.h"
#include "replaygain.h"
#include <string.h>
#include <debug.h>

#if EQ_NUM_BANDS < 2
/* No good. Expect at least low/high shelving filters */
#error Band count must be greater than or equal to 2
#endif

/**
 * Current setup is one lowshelf filters three peaking filters and one
 *  highshelf filter. Varying the number of shelving filters make no sense,
 *  but adding peaking filters is possible. Check EQ_NUM_BANDS to have
 *  2 shelving filters and EQ_NUM_BANDS-2 peaking filters.
 */
static const struct eq_band_params
{
    void (*coef_gen)(unsigned long cutoff, unsigned long Q, long db,
                     int32_t *c);
    enum filter_shift shift;
} band_params[EQ_NUM_BANDS] =
{
    [0] =
        { .coef_gen = filter_ls_coefs, .shift = FILTER_SHELF_SHIFT },
#if EQ_NUM_BANDS > 2
    /* Yes, we haz peaking filters (but 0 ducks) */
    [1 ... EQ_NUM_BANDS-2] =
        { .coef_gen = filter_pk_coefs, .shift = FILTER_PEAK_SHIFT  }, 
#endif
    [EQ_NUM_BANDS-1] =
        { .coef_gen = filter_hs_coefs, .shift = FILTER_SHELF_SHIFT },
};

static struct eq_state
{
    uint32_t enabled;                        /* Mask of enabled bands */
    uint8_t bands[EQ_NUM_BANDS+1];           /* Indexes of enabled bands */
    struct dsp_filter filters[EQ_NUM_BANDS]; /* Data for each filter */
} eq_data IBSS_ATTR;

/* Clear histories of all enabled bands */
static void eq_flush(void)
{
    if (eq_data.enabled == 0)
        return; /* Not initialized yet/no bands on */

    for (uint8_t *b = eq_data.bands; *b < EQ_NUM_BANDS; b++)
        filter_flush(&eq_data.filters[*b]);
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

    /* NOTE: The coef functions assume the EMAC unit is in fractional mode,
       which it should be, since we're executed from the main thread. */

    uint32_t mask = eq_data.enabled;
    struct dsp_filter *filter = &eq_data.filters[band];

    /* Assume a band is disabled if the gain is zero */
    mask &= ~BIT_N(band);

    if (setting->gain != 0)
    {
        mask |= BIT_N(band);

        /* Convert user settings to format required by coef generator
           functions */
        const struct eq_band_params *parms = &band_params[band];
        filter->shift = parms->shift;
        parms->coef_gen(0xffffffff / NATIVE_FREQUENCY * setting->cutoff,
                        setting->q ?: 1, setting->gain, filter->coefs);
    }

    if (mask == eq_data.enabled)
        return; /* No change in band-enable state */

    if (mask & BIT_N(band))
        filter_flush(filter); /* Coming online */

    eq_data.enabled = mask;

    /* Only be active if there are bands to process - if EQ is off, then
       this call has no effect */
    struct dsp_config *dsp = dsp_get_config(CODEC_IDX_AUDIO);
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
    int channels = buf->format.num_channels;

    for (uint8_t *b = eq_data.bands; *b < EQ_NUM_BANDS; b++)
        filter_process(buf->p32, &eq_data.filters[*b], count, channels);

    (void)this;
}

/* DSP message hook */
static void eq_configure(struct dsp_proc_entry *this,
                         struct dsp_config *dsp,
                         enum dsp_settings setting,
                         intptr_t value)
{
    switch (setting)
    {
    case DSP_PROC_INIT:
        if (value != 0)
            break;
        this->process[0] = eq_process;
    case DSP_PROC_CLOSE:
        pga_enable_gain(PGA_EQ_PRECUT, setting == DSP_PROC_INIT);
        break;
        
    case DSP_FLUSH:
        eq_flush();
        break;

    default:
        break;
    }

    (void)dsp;
}

/* Database entry */
const struct dsp_proc_db_entry eq_proc_db_entry =
{
    .id = DSP_PROC_EQUALIZER,
    .configure = eq_configure
};
