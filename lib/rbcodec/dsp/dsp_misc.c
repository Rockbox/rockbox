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
 * Copyright (C) 2005 Magnus Holmgren
 * Copyright (C) 2007 Thom Johansen
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
#include "replaygain.h"
#include "dsp_proc_entry.h"
#include "dsp_misc.h"
#include "pga.h"
#include "channel_mode.h"
#ifdef HAVE_SW_TONE_CONTROLS
#include "tone_controls.h"
#endif
#include <string.h>

/** Replaygain settings **/
static struct replaygain_settings current_settings;
static struct dsp_replay_gains current_gains;

static void dsp_replaygain_update(
    const struct replaygain_settings *settings,
    const struct dsp_replay_gains *gains)
{
    if (settings != &current_settings)
        current_settings = *settings;   /* Stash settings */

    if (gains != &current_gains)
        current_gains = *gains;         /* Stash gains */

    int32_t gain = PGA_UNITY;

    if (settings->type != REPLAYGAIN_OFF || settings->noclip)
    {
        bool track_mode = settings->type == REPLAYGAIN_TRACK &&
                          gains->track_gain != 0;

        int32_t peak = (track_mode || gains->album_peak == 0) ?
                            gains->track_peak : gains->album_peak;

        if (settings->type != REPLAYGAIN_OFF)
        {
            gain = (track_mode || gains->album_gain == 0) ?
                gains->track_gain : gains->album_gain;

            if (settings->preamp != 0)
            {
                int32_t preamp = get_replaygain_int(settings->preamp * 10);
                gain = fp_mul(gain, preamp, 24);
            }
        }

        if (gain == 0)
        {
            /* So that noclip can work even with no gain information. */
            gain = PGA_UNITY;
        }

        if (settings->noclip && peak != 0 &&
            fp_mul(gain, peak, 24) >= PGA_UNITY)
        {
            gain = fp_div(PGA_UNITY, peak, 24);
        }
    }

    pga_set_gain(PGA_REPLAYGAIN, gain);
    pga_enable_gain(PGA_REPLAYGAIN, gain != PGA_UNITY);
}

static void dsp_replaygain_set_gains(const struct dsp_replay_gains *gains)
{
    if (gains == NULL)
    {
        /* Set defaults */
        gains = &current_gains;
        memset((void *)gains, 0, sizeof (*gains));
    }

    dsp_replaygain_update(&current_settings, gains);
}

void dsp_replaygain_set_settings(const struct replaygain_settings *settings)
{
    dsp_replaygain_update(settings, &current_gains);
}


/** Pitch Settings **/

#ifdef HAVE_PITCHCONTROL
static int32_t pitch_ratio = PITCH_SPEED_100;

static void dsp_pitch_update(struct dsp_config *dsp)
{
    dsp_configure(dsp, DSP_SET_PITCH,
                  fp_div(pitch_ratio, PITCH_SPEED_100, 16));
}

void dsp_set_pitch(int32_t percent)
{
    if (percent <= 0)
        percent = PITCH_SPEED_100;

    if (percent == pitch_ratio)
        return;

    pitch_ratio = percent;

    dsp_pitch_update(dsp_get_config(CODEC_IDX_AUDIO));
}

int32_t dsp_get_pitch(void)
{
    return pitch_ratio;
}
#endif /* HAVE_PITCHCONTROL */

/* Set output samplerate for all DSPs */
void dsp_set_all_output_frequency(unsigned int samplerate)
{

    struct dsp_config *dsp;
    for (int i = 0; (dsp = dsp_get_config(i)); i++)
        dsp_configure(dsp, DSP_SET_OUT_FREQUENCY, samplerate);
}

/* Return DSP's output samplerate */
unsigned int dsp_get_output_frequency(struct dsp_config *dsp)
{
    return dsp_configure(dsp, DSP_GET_OUT_FREQUENCY, 0);
}

static void INIT_ATTR misc_dsp_init(struct dsp_config *dsp,
                                    enum dsp_ids dsp_id)
{
    /* Enable us for the audio DSP at startup */
    if (dsp_id == CODEC_IDX_AUDIO)
        dsp_proc_enable(dsp, DSP_PROC_MISC_HANDLER, true);
}

/* This is a null-processing stage that monitors as an enabled stage but never
 * becomes active in processing samples. It only hooks messages. */

/* DSP message hook */
static intptr_t misc_handler_configure(struct dsp_proc_entry *this,
                                       struct dsp_config *dsp,
                                       unsigned setting,
                                       intptr_t value)
{
    switch (setting)
    {
    case DSP_INIT:
        misc_dsp_init(dsp, (enum dsp_ids)value);
        break;

    case DSP_PROC_CLOSE:
        /* This stage should be enabled at all times */
        DEBUGF("DSP_PROC_MISC_HANDLER - Error: Closing!\n");
        break;

    case DSP_RESET:
#ifdef HAVE_PITCHCONTROL
        dsp_pitch_update(dsp);
#endif
        value = (intptr_t)NULL; /* Default gains */
    case REPLAYGAIN_SET_GAINS:
        dsp_replaygain_set_gains((void *)value);
        break;

#ifdef HAVE_PITCHCONTROL
    case DSP_SET_FREQUENCY:
        dsp_pitch_update(dsp);
        break;
#endif
    }

    return 0;
    (void)this;
}

/* Database entry */
DSP_PROC_DB_ENTRY(
    MISC_HANDLER,
    misc_handler_configure);
