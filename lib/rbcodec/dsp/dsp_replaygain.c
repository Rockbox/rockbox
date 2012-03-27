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
#include "settings.h"
#include "replaygain.h"
#include "dsp.h"
#include "fixedpoint.h"
#include <string.h>

/* This is a NULL processing stage that monitors as an enabled stage but never
 * becomes active in processing samples */
static struct dsp_replay_gains current_rpgains;

static void dsp_replaygain_update(const struct dsp_replay_gains *gains)
{
    if (gains == NULL)
    {
        /* Use defaults */
        memset(&current_rpgains, 0, sizeof (current_rpgains));
        gains = &current_rpgains;
    }
    else
    {
        current_rpgains = *gains; /* Stash settings */
    }

    int32_t gain = PGA_UNITY;

    if (global_settings.replaygain_type != REPLAYGAIN_OFF ||
        global_settings.replaygain_noclip)
    {
        bool track_mode =
            get_replaygain_mode(gains->track_gain != 0,
                                gains->album_gain != 0) == REPLAYGAIN_TRACK;

        int32_t peak = (track_mode || gains->album_peak == 0) ?
            gains->track_peak : gains->album_peak;

        if (global_settings.replaygain_type != REPLAYGAIN_OFF)
        {
            gain = (track_mode || gains->album_gain == 0) ?
                gains->track_gain : gains->album_gain;

            if (global_settings.replaygain_preamp)
            {
                int32_t preamp = get_replaygain_int(
                    global_settings.replaygain_preamp * 10);

                gain = fp_mul(gain, preamp, 24);
            }
        }

        if (gain == 0)
        {
            /* So that noclip can work even with no gain information. */
            gain = PGA_UNITY;
        }

        if (global_settings.replaygain_noclip && peak != 0 &&
            fp_mul(gain, peak, 24) >= PGA_UNITY)
        {
            gain = fp_div(PGA_UNITY, peak, 24);
        }
    }

    pga_set_gain(PGA_REPLAYGAIN, gain);
    pga_enable_gain(PGA_REPLAYGAIN, gain != PGA_UNITY);
}

int get_replaygain_mode(bool have_track_gain, bool have_album_gain)
{
    bool track = false;

    switch (global_settings.replaygain_type)
    {
    case REPLAYGAIN_TRACK:
        track = true;
        break;

    case REPLAYGAIN_SHUFFLE:
        track = global_settings.playlist_shuffle;
        break;
    }

    return (!track && have_album_gain) ?
        REPLAYGAIN_ALBUM : (have_track_gain ? REPLAYGAIN_TRACK : -1);
}

void dsp_set_replaygain(void)
{
    dsp_replaygain_update(&current_rpgains);
}

/* DSP message hook */
static void dsp_replaygain_configure(struct dsp_proc_entry *this,
                                     struct dsp_config *dsp,
                                     enum dsp_settings setting,
                                     intptr_t value)
{
    switch (setting)
    {
    case DSP_INIT:
        /* Enable us for the audio DSP at startup */
        if (value == CODEC_IDX_AUDIO)
            dsp_proc_enable(dsp, DSP_PROC_REPLAYGAIN_HANDLER, true);
        break;

    case DSP_RESET:
        value = (intptr_t)NULL;
    case DSP_SET_REPLAY_GAIN:
        dsp_replaygain_update((struct dsp_replay_gains *)value);
        break;

    default:
        break;
    }

    (void)this;
}

/* Database entry */
const struct dsp_proc_db_entry dsp_replaygain_proc_db_entry =
{
    .id = DSP_PROC_REPLAYGAIN_HANDLER,
    .configure = dsp_replaygain_configure
};
