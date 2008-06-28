/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Michael Sevakis
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
#include "system.h"
#include "cpu.h"
#include "audio.h"
#include "sound.h"

int audio_channels = 2;
int audio_output_source = AUDIO_SRC_PLAYBACK;

void audio_set_output_source(int source)
{
    int oldmode = set_fiq_status(FIQ_DISABLED);

    if ((unsigned)source >= AUDIO_NUM_SOURCES)
        source = AUDIO_SRC_PLAYBACK;

    audio_output_source = source;

    /*if (source != AUDIO_SRC_PLAYBACK)
        IISCONFIG |= (1 << 29);*/

    set_fiq_status(oldmode);
}

void audio_input_mux(int source, unsigned flags)
{
    static int last_source = AUDIO_SRC_PLAYBACK;
    static bool last_recording = false;
    bool recording = flags & SRCF_RECORDING;

    switch (source)
    {
        default:                        /* playback - no recording */
            source = AUDIO_SRC_PLAYBACK;
        case AUDIO_SRC_PLAYBACK:
            audio_channels = 2;
            if (source != last_source)
            {
                /*audiohw_set_monitor(false);
                audiohw_disable_recording();*/
            }
            break;

        case AUDIO_SRC_MIC:             /* recording only */
            audio_channels = 1;
            if (source != last_source)
            {
                /*audiohw_set_monitor(false);
                audiohw_enable_recording(true);  /. source mic */
            }
            break;

        case AUDIO_SRC_FMRADIO:         /* recording and playback */
            audio_channels = 2;

            if (source == last_source && recording == last_recording)
                break;

            last_recording = recording;

            if (recording)
            {
                /*audiohw_set_monitor(false);
                audiohw_enable_recording(false);*/
            }
            else
            {
                /*audiohw_disable_recording();
                audiohw_set_monitor(true); /. line 1 analog audio path */
            }
            break;
    } /* end switch */

    last_source = source;
} /* audio_input_mux */
