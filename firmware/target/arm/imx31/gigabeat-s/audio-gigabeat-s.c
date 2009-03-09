/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Nils Wallménius
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
#include "audiohw.h"
#include "audio.h"

/* Set the audio source for IIS TX */
void audio_set_output_source(int source)
{
    switch (source)
    {
    default:
    case AUDIO_SRC_PLAYBACK:
        /* Receive data from PORT2 (SSI2) */
        AUDMUX_PDCR4 = AUDMUX_PDCR_RXDSEL_PORT2;
        /* wmc_clear(WMC_COMPANDING_CTRL, WMC_LOOPBACK); */
        break;

    case AUDIO_SRC_FMRADIO:
        /* External source - receive data from self (loopback to TX) */
        AUDMUX_PDCR4 = AUDMUX_PDCR_RXDSEL_PORT4;
        /* wmc_set(WMC_COMPANDING_CTRL, WMC_LOOPBACK); */
        break;
    }
}

void audio_input_mux(int source, unsigned int flags)
{
    /* Prevent pops from unneeded switching */
    static int last_source = AUDIO_SRC_PLAYBACK;
    bool recording = flags & SRCF_RECORDING;
    static bool last_recording = false;

    switch (source)
    {
        default:
            source = AUDIO_SRC_PLAYBACK;
            /* Fallthrough */
        case AUDIO_SRC_PLAYBACK: /* playback - no recording */
            if (source != last_source)
                audiohw_set_recsrc(AUDIO_SRC_PLAYBACK, false);
            break;

        case AUDIO_SRC_FMRADIO: /* recording and playback */
            if (source != last_source || recording != last_recording)
                audiohw_set_recsrc(AUDIO_SRC_FMRADIO, recording);
            break;
    }

    last_source = source;
    last_recording = recording;
}

