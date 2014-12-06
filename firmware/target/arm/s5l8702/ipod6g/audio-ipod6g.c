/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: audio-nano2g.c 23095 2009-10-11 09:17:12Z dave $
 *
 * Copyright (C) 2006 by Michael Sevakis
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
#include "pmu-target.h"

extern int rec_hw_ver;

#if INPUT_SRC_CAPS != 0
void audio_set_output_source(int source)
{
    if ((unsigned)source >= AUDIO_NUM_SOURCES)
        source = AUDIO_SRC_PLAYBACK;
} /* audio_set_output_source */

void audio_input_mux(int source, unsigned flags)
{
    (void)flags;
    /* Prevent pops from unneeded switching */
    static int last_source = AUDIO_SRC_PLAYBACK;

    switch (source)
    {
        default:                        /* playback - no recording */
            source = AUDIO_SRC_PLAYBACK;
        case AUDIO_SRC_PLAYBACK:
#ifdef HAVE_RECORDING
            if (source != last_source)
            {
                audiohw_set_monitor(false);
                audiohw_disable_recording();

                /* Vcodec = 1800mV (900mV + value*100mV) */
                pmu_ldo_set_voltage(3, 0x9);

                if (rec_hw_ver == 1)
                    GPIOCMD = 0xe070e;
            }
#endif
        break;

#ifdef HAVE_MIC_REC
        case AUDIO_SRC_MIC:             /* recording only */
            if (source != last_source)
            {
                if (rec_hw_ver == 1)
                    GPIOCMD = 0xe070f;

                /* Vcodec = 2400mV (900mV + value*100mV) */
                pmu_ldo_set_voltage(3, 0xf);

                audiohw_set_monitor(false);
                audiohw_enable_recording(true);  /* source mic */
            }
        break;
#endif

#ifdef HAVE_LINE_REC
        case AUDIO_SRC_LINEIN:          /* recording only */
            if (source != last_source)
            {
                if (rec_hw_ver == 1)
                    GPIOCMD = 0xe070e;

                /* Vcodec = 2400mV (900mV + value*100mV) */
                pmu_ldo_set_voltage(3, 0xf);

                audiohw_set_monitor(false);
                audiohw_enable_recording(false); /* source line */
            }
        break;
#endif
    } /* end switch */

    last_source = source;
} /* audio_input_mux */
#endif /* INPUT_SRC_CAPS != 0 */
