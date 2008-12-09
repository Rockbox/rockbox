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
#include "wm8978.h"
#include "audio.h"

void audio_set_output_source(int source)
{
    (void)source; /* TODO */
}

void audio_input_mux(int source, unsigned int flags)
{
    (void)flags;
    switch (source)
    {
        case AUDIO_SRC_PLAYBACK:
            /* deselect bypass patths and set volume to -15dB */
            wmc_clear(WMC_LEFT_MIXER_CTRL, (WMC_BYPL2LMIX) | (7<<2));
            wmc_clear(WMC_RIGHT_MIXER_CTRL, (WMC_BYPR2RMIX) | (7<<2));
            /* disable L2/R2 inputs and boost stage */
            wmc_clear(WMC_POWER_MANAGEMENT2,
                      WMC_INPPGAENR | WMC_INPPGAENL | WMC_BOOSTENL | WMC_BOOSTENR);
            break;

        case AUDIO_SRC_FMRADIO:
            /* enable L2/R2 inputs and boost stage */
            wmc_set(WMC_POWER_MANAGEMENT2,
                    WMC_INPPGAENR | WMC_INPPGAENL | WMC_BOOSTENL | WMC_BOOSTENR);
            /* select bypass patths and set volume to 0dB */
            wmc_set(WMC_LEFT_MIXER_CTRL, (WMC_BYPL2LMIX) | (5<<2));
            wmc_set(WMC_RIGHT_MIXER_CTRL, (WMC_BYPR2RMIX) | (5<<2));
            break;

        default:
            source = AUDIO_SRC_PLAYBACK;
    }
}

