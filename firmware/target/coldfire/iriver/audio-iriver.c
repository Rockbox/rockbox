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

void audio_set_output_source(int source)
{
    static const unsigned char txsrc_select[AUDIO_NUM_SOURCES+1] =
    {
        [AUDIO_SRC_PLAYBACK+1] = 3, /* PDOR3        */
        [AUDIO_SRC_MIC+1]      = 4, /* IIS1 RcvData */
        [AUDIO_SRC_LINEIN+1]   = 4, /* IIS1 RcvData */
        [AUDIO_SRC_FMRADIO+1]  = 4, /* IIS1 RcvData */
    #ifdef HAVE_SPDIF_IN
        [AUDIO_SRC_SPDIF+1]    = 7, /* EBU1 RcvData */
    #endif
    };

    int level = set_irq_level(DMA_IRQ_LEVEL);

    if ((unsigned)source >= AUDIO_NUM_SOURCES)
        source = AUDIO_SRC_PLAYBACK;

    IIS2CONFIG = (IIS2CONFIG & ~(7 << 8)) | (txsrc_select[source+1] << 8);

    restore_irq(level);
} /* audio_set_output_source */

void audio_input_mux(int source, unsigned flags)
{
    /* Prevent pops from unneeded switching */
    static int last_source = AUDIO_SRC_PLAYBACK;
    bool recording = flags & SRCF_RECORDING;
    static bool last_recording = false;

    switch (source)
    {
        default:                        /* playback - no recording */
            source = AUDIO_SRC_PLAYBACK;
        case AUDIO_SRC_PLAYBACK:
            if (source != last_source)
            {
                audiohw_disable_recording();
                audiohw_set_monitor(false);
                coldfire_set_dataincontrol(0);
            }
        break;

        case AUDIO_SRC_MIC:             /* recording only */
            if (source != last_source)
            {
                audiohw_enable_recording(true);  /* source mic */
                audiohw_set_monitor(false);
                /* Int. when 6 samples in FIFO, PDIR2 src = iis1RcvData */
                coldfire_set_dataincontrol((3 << 14) | (4 << 3));
            }
        break;

        case AUDIO_SRC_LINEIN:          /* recording only */
            if (source != last_source)
            {
                audiohw_enable_recording(false); /* source line */
                audiohw_set_monitor(false);
                /* Int. when 6 samples in FIFO, PDIR2 src = iis1RcvData */
                coldfire_set_dataincontrol((3 << 14) | (4 << 3));
            }
        break;

#ifdef HAVE_SPDIF_IN
        case AUDIO_SRC_SPDIF:           /* recording only */
            if (source != last_source)
            {
                audiohw_disable_recording();
                audiohw_set_monitor(false);
                /* Int. when 6 samples in FIFO, PDIR2 src = ebu1RcvData */
                coldfire_set_dataincontrol((3 << 14) | (7 << 3));
            }
        break;
#endif /* HAVE_SPDIF_IN */

        case AUDIO_SRC_FMRADIO:         /* recording and playback */
            if (!recording)
                audiohw_set_recvol(0, 0, AUDIO_GAIN_LINEIN);

            if (source == last_source && recording == last_recording)
                break;

            last_recording = recording;

            /* Int. when 6 samples in FIFO, PDIR2 src = iis1RcvData */
            coldfire_set_dataincontrol(recording ?
                ((3 << 14) | (4 << 3)) : 0);

            /* I2S recording and playback */
            audiohw_enable_recording(false);    /* source line */
            audiohw_set_monitor(!recording);
        break;
    } /* end switch */

    /* set line multiplexer */
#if defined(IRIVER_H100_SERIES)
    #define MUX_BIT         (1 << 23)
#elif defined(IRIVER_H300_SERIES)
    #define MUX_BIT         (1 << 30)
#endif

    if (source == AUDIO_SRC_FMRADIO)
        or_l(MUX_BIT, &GPIO_OUT);       /* FM radio */
    else
        and_l(~MUX_BIT, &GPIO_OUT);     /* Line In */

    or_l(MUX_BIT, &GPIO_ENABLE);
    or_l(MUX_BIT, &GPIO_FUNCTION);

    last_source = source;
} /* audio_input_mux */
