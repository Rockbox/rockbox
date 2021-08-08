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
#include "general.h"

int audio_channels = 2;

#if CONFIG_CPU == AS3525
int audio_output_source = AUDIO_SRC_PLAYBACK;
#endif

void audio_set_output_source(int source)
{
    bitset32(&CGU_PERI, CGU_I2SOUT_APB_CLOCK_ENABLE);

    if ((unsigned)source >= AUDIO_NUM_SOURCES)
        source = AUDIO_SRC_PLAYBACK;

    bool loopback = source != AUDIO_SRC_PLAYBACK;

#if CONFIG_CPU == AS3525
    loopback = loopback && audio_channels > 1;

    audio_output_source = source;
#endif

    if (loopback)
        I2SOUT_CONTROL |= (1<<5);  /* loopback from i2sin fifo */
    else
        I2SOUT_CONTROL &= ~(1<<5); /* normal i2sout */
}

void audio_input_mux(int source, unsigned flags)
{
    static int last_source = AUDIO_SRC_PLAYBACK;
#if defined(HAVE_RECORDING) && (INPUT_SRC_CAPS & SRC_CAP_FMRADIO)
    static bool last_recording = false;
    const bool recording = flags & SRCF_RECORDING;
#else
    (void) flags;
#endif

    switch (source)
    {
        default:                        /* playback - no recording */
            source = AUDIO_SRC_PLAYBACK;
        /*fallthrough*/
        case AUDIO_SRC_PLAYBACK:
            if (source != last_source)
            {
                audio_channels = 2;
#if defined(HAVE_RECORDING) || defined(HAVE_FMRADIO_IN)
                audiohw_set_monitor(false);
#endif
#ifdef HAVE_RECORDING
                audiohw_disable_recording();
#endif
            }
            break;

#if defined(HAVE_RECORDING) && (INPUT_SRC_CAPS & SRC_CAP_MIC)
        case AUDIO_SRC_MIC:             /* recording only */
            if (source != last_source)
            {
                audio_channels = 1;
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

            audio_channels = 2;
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

#if CONFIG_CPU == AS3525
    /* Sync on behalf of change in number of channels */
    audio_set_output_source(audio_output_source);
#endif
}

#ifdef CONFIG_SAMPR_TYPES
unsigned int pcm_sampr_to_hw_sampr(unsigned int samplerate,
                                   unsigned int type)
{
#ifdef HAVE_RECORDING
    if (samplerate != HW_SAMPR_RESET && type == SAMPR_TYPE_REC)
    {
        /* Check if the samplerate is in the list of recordable rates.
         * Fail to default if not */
        int index = round_value_to_list32(samplerate, rec_freq_sampr,
                                          REC_NUM_FREQ, false);
        if (samplerate != rec_freq_sampr[index])
            samplerate = REC_SAMPR_DEFAULT;

        samplerate *= 2; /* Recording rates are 1/2 the codec clock */
    }
#endif /* HAVE_RECORDING */

    return samplerate;
    (void)type;
}
#endif /* CONFIG_SAMPR_TYPES */

