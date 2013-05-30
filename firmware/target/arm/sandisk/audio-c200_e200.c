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
#include "general.h"

int audio_channels = 2;
int audio_output_source = AUDIO_SRC_PLAYBACK;

void audio_set_output_source(int source)
{
    int oldmode = set_fiq_status(FIQ_DISABLED);

    if ((unsigned)source >= AUDIO_NUM_SOURCES)
        source = AUDIO_SRC_PLAYBACK;

    audio_output_source = source;

    if (source != AUDIO_SRC_PLAYBACK)
        IISCONFIG |= (1 << 29);

    set_fiq_status(oldmode);
} /* audio_set_output_source */

void audio_input_mux(int source, unsigned flags)
{
    static int last_source = AUDIO_SRC_PLAYBACK;
#ifdef HAVE_RECORDING
    static bool last_recording = false;
    bool recording = flags & SRCF_RECORDING;
#else
    (void) flags;
#endif

    switch (source)
    {
        default:                        /* playback - no recording */
            source = AUDIO_SRC_PLAYBACK;
        case AUDIO_SRC_PLAYBACK:
            audio_channels = 2;
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

#if defined(HAVE_RECORDING) && (INPUT_SRC_CAPS & SRC_CAP_MIC)
        case AUDIO_SRC_MIC:             /* recording only */
            audio_channels = 1;
            if (source != last_source)
            {
                audiohw_set_monitor(false);
                audiohw_enable_recording(true);  /* source mic */
            }
            break;
#endif

#if (INPUT_SRC_CAPS & SRC_CAP_FMRADIO)
        case AUDIO_SRC_FMRADIO:         /* recording and playback */
            audio_channels = 2;

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
#endif
            else
            {
#ifdef HAVE_RECORDING
                audiohw_disable_recording();
#endif
#if defined(HAVE_RECORDING) || defined(HAVE_FMRADIO_IN)
                audiohw_set_monitor(true); /* line 1 analog audio path */
#endif

            }
            break;
#endif /* (INPUT_SRC_CAPS & SRC_CAP_FMRADIO) */
    } /* end switch */

    last_source = source;
} /* audio_input_mux */


void audiohw_set_sampr_dividers(int fsel)
{
    /* Seems to predivide 24MHz by 2 for a source clock of 12MHz. Maybe
     * there's a way to set that? */
    static const struct
    {
        unsigned char iisclk;
        unsigned char iisdiv;
    } regvals[HW_NUM_FREQ] =
    {
        /* 8kHz - 24kHz work well but there seems to be a minor crackling
         * issue for playback at times and all possibilities were checked
         * for the divisors without any positive change.
         * 32kHz - 48kHz seem fine all around. */
#if 0
        [HW_FREQ_8] =       /* CLK / 1500 (8000Hz) */
        {
            .iisclk = 24,
            .iisdiv = 59,
        },
        [HW_FREQ_11] =      /* CLK / 1088 (~11029.41Hz) */
        {
            .iisclk = 33,
            .iisdiv = 31,
        },
        [HW_FREQ_12] =      /* CLK / 1000 (120000Hz) */
        {
            .iisclk = 49,
            .iisdiv = 19,
        },
        [HW_FREQ_16] =      /* CLK / 750 (16000Hz) */
        {
            .iisclk = 24,
            .iisdiv = 29,
        },
        [HW_FREQ_22] =      /* CLK / 544 (~22058.82Hz)  */
        {
            .iisclk = 33,
            .iisdiv = 15,
        },
        [HW_FREQ_24] =      /* CLK / 500 (24000Hz) */
        {
            .iisclk = 49,
            .iisdiv = 9,
        },
#endif
        [HW_FREQ_32] =      /* CLK / 375 (32000Hz) */
        {
            .iisclk = 24,
            .iisdiv = 14,
        },
        [HW_FREQ_44] =      /* CLK / 272 (~44117.68Hz) */
        {
            .iisclk = 33,
            .iisdiv = 7,
        },
        [HW_FREQ_48] =      /* CLK / 250 (48000Hz) */
        {
            .iisclk = 24,
            .iisdiv = 9
        },
        /* going a bit higher would be nice to get 64kHz play, 32kHz rec, but a
         * close enough division isn't obtainable unless CLK can be changed */
    };

    IISCLK = (IISCLK & ~0x17ff) | regvals[fsel].iisclk;
    IISDIV = (IISDIV & ~0xc000003f) | regvals[fsel].iisdiv;
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
