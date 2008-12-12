/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#ifdef HAVE_FMRADIO_REC 
    bool recording = flags & SRCF_RECORDING;
    static bool last_recording = false;
#endif

#if defined(IPOD_COLOR) || defined (IPOD_4G)
    /* The usual magic from IPL - I'm guessing this configures the headphone
       socket to be input or output. */
    if ((flags & SRCF_RECORDING) && source != AUDIO_SRC_PLAYBACK)
    {
        /* input */
        GPIO_CLEAR_BITWISE(GPIOI_OUTPUT_VAL, 0x40);
        GPIO_CLEAR_BITWISE(GPIOA_OUTPUT_VAL, 0x04);
    }
    else
    {
        /* output */
        GPIO_SET_BITWISE(GPIOI_OUTPUT_VAL, 0x40);
        GPIO_SET_BITWISE(GPIOA_OUTPUT_VAL, 0x04);
    }
#endif /* IPOD_COLOR || IPOD_4G */

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
            }
#endif
        break;
#ifdef HAVE_MIC_REC
        case AUDIO_SRC_MIC:             /* recording only */
            if (source != last_source)
            {
                audiohw_set_monitor(false);
                audiohw_enable_recording(true);  /* source mic */
            }
        break;
#endif
#ifdef HAVE_LINE_REC
        case AUDIO_SRC_LINEIN:          /* recording only */
#if defined(IRIVER_H10) || defined(IRIVER_H10_5GB)
            /* Switch line in source to line-in */
            GPIO_SET_BITWISE(GPIOB_OUTPUT_VAL, 0x04);
#endif
            if (source != last_source)
            {
                audiohw_set_monitor(false);
                audiohw_enable_recording(false); /* source line */
            }
        break;
#endif
#ifdef HAVE_FMRADIO_REC
        case AUDIO_SRC_FMRADIO:         /* recording and playback */
#if defined(IRIVER_H10) || defined(IRIVER_H10_5GB)
            /* Switch line in source to tuner */
            GPIO_CLEAR_BITWISE(GPIOB_OUTPUT_VAL, 0x04);
            /* Set line-in vol to +12dB, which is proper for H10's */
            if (!recording)
                audiohw_set_recvol(0x1f, 0x1f, AUDIO_GAIN_LINEIN);
#else            /* Set line-in vol to 0dB*/
            if (!recording)
                audiohw_set_recvol(0x17, 0x17, AUDIO_GAIN_LINEIN);
#endif
            if (source == last_source && recording == last_recording)
                break;

            last_recording = recording;

            if (recording)
            {
                audiohw_set_monitor(false);  /* disable bypass mode */
                audiohw_enable_recording(false); /* select line-in source */
            }
            else
            {
                audiohw_disable_recording();
                audiohw_set_monitor(true);  /* enable bypass mode */
            }
        break;
#endif
    } /* end switch */

    last_source = source;
} /* audio_input_mux */
#endif /* INPUT_SRC_CAPS != 0 */
