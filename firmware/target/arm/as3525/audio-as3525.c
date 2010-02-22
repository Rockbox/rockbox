/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Bertrik Sikken
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
#include "cpu.h"
#include "audio.h"
#include "audiohw.h"
#include "sound.h"

void audio_set_output_source(int source)
{
    (void)source;
}

void audio_input_mux(int source, unsigned flags)
{
    static int last_source = AUDIO_SRC_PLAYBACK;
#ifdef HAVE_RECORDING
    static bool last_recording = false;
    const bool recording = flags & SRCF_RECORDING;
#else
    (void) flags;
#endif

    switch (source)
    {
        default:                        /* playback - no recording */
            source = AUDIO_SRC_PLAYBACK;
        case AUDIO_SRC_PLAYBACK:
            if (source != last_source)
            {
#if defined(HAVE_RECORDING) || defined(HAVE_FMRADIO_IN)
                audiohw_set_monitor(false);
#endif
#ifdef HAVE_RECORDING
                audiohw_disable_recording();
#endif
            }
            break;

#ifdef HAVE_RECORDING
        case AUDIO_SRC_MIC:             /* recording only */
            if (source != last_source)
            {
                audiohw_set_monitor(false);
                audiohw_enable_recording(true);  /* source mic */
            }
            break;
#endif

#if (INPUT_SRC_CAPS & SRC_CAP_FMRADIO)

        case AUDIO_SRC_FMRADIO:         /* recording and playback */
            if (source == last_source
#ifdef HAVE_RECORDING
                    && recording == last_recording
#endif
                )
                break;

#ifdef HAVE_RECORDING
            last_recording = recording;

            if (recording)
            {
                audiohw_set_monitor(false);
                audiohw_enable_recording(false);
            }
            else
#endif
            {
#ifdef HAVE_RECORDING
                audiohw_disable_recording();
#endif
#if defined(HAVE_RECORDING) || defined(HAVE_FMRADIO_IN)
                audiohw_set_monitor(true); /* line 2 analog audio path */
#endif
            }
            break;
#endif /* (INPUT_SRC_CAPS & SRC_CAP_FMRADIO) */
    }

    last_source = source;
}
