/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 Amaury Pouly
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
#include "nwzlinux_codec.h"

int audio_channels = 2;
int audio_output_source = AUDIO_SRC_PLAYBACK;

void audio_set_output_source(int source)
{
    if((unsigned)source >= AUDIO_NUM_SOURCES)
        source = AUDIO_SRC_PLAYBACK;

    audio_output_source = source;
}

void audio_input_mux(int source, unsigned flags)
{
    static int last_source = AUDIO_SRC_PLAYBACK;

    (void)flags;

    switch (source)
    {
        default: /* playback - no recording */
            source = AUDIO_SRC_PLAYBACK;
        case AUDIO_SRC_PLAYBACK:
            audio_channels = 2;
            if (source != last_source)
                audiohw_set_playback_src(NWZ_PLAYBACK);
            break;

        case AUDIO_SRC_FMRADIO: /* recording and playback */
            audio_channels = 2;
            if (source == last_source)
                break;

            audiohw_set_playback_src(NWZ_RADIO);
            break;
    } /* end switch */

    last_source = source;
} /* audio_input_mux */
