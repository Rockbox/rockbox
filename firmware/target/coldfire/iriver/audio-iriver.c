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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "system.h"
#include "cpu.h"
#include "audio.h"
#include "uda1380.h"

/**
 * Note that microphone is mono, only left value is used 
 * See uda1380_set_recvol() for exact ranges.
 *
 * @param type   AUDIO_GAIN_MIC, AUDIO_GAIN_LINEIN
 * 
 */
void audio_set_recording_gain(int left, int right, int type)
{
    //logf("rcmrec: t=%d l=%d r=%d", type, left, right);
    uda1380_set_recvol(left, right, type);
} /* audio_set_recording_gain */

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

    if ((unsigned)source >= AUDIO_NUM_SOURCES)
        source = AUDIO_SRC_PLAYBACK;

    IIS2CONFIG = (IIS2CONFIG & ~(7 << 8)) | (txsrc_select[source+1] << 8);
} /* audio_set_output_source */

void audio_set_source(int source, unsigned flags)
{
    /* Prevent pops from unneeded switching */
    static int last_source = AUDIO_SRC_PLAYBACK;
    bool recording = flags & SRCF_RECORDING;

    switch (source)
    {
        default:                        /* playback - no recording */
            source = AUDIO_SRC_PLAYBACK;
        case AUDIO_SRC_PLAYBACK:
            if (source != last_source)
            {
                uda1380_disable_recording();
                uda1380_set_monitor(false);
                /* Reset PDIR2 data flow */
                DATAINCONTROL = (1 << 9);
            }
        break;

        case AUDIO_SRC_MIC:             /* recording only */
            if (source != last_source)
            {
                uda1380_enable_recording(true);  /* source mic */
                uda1380_set_monitor(true);
                /* Int. when 6 samples in FIFO, PDIR2 src = iis1RcvData */
                DATAINCONTROL = (3 << 14) | (4 << 3);
            }
        break;

        case AUDIO_SRC_LINEIN:          /* recording only */
            if (source != last_source)
            {
                uda1380_enable_recording(false); /* source line */
                uda1380_set_monitor(true);
                /* Int. when 6 samples in FIFO, PDIR2 src = iis1RcvData */
                DATAINCONTROL = (3 << 14) | (4 << 3);
            }
        break;

#ifdef HAVE_SPDIF_IN
        case AUDIO_SRC_SPDIF:           /* recording only */
            if (source != last_source)
            {
                uda1380_disable_recording();
                uda1380_set_monitor(false);
                /* Int. when 6 samples in FIFO, PDIR2 src = ebu1RcvData */
                DATAINCONTROL = (3 << 14) | (7 << 3);
            }
        break;
#endif /* HAVE_SPDIF_IN */

        case AUDIO_SRC_FMRADIO:         /* recording and playback */
            if (recording)
            {
                /* Int. when 6 samples in FIFO, PDIR2 src = iis1RcvData */
                DATAINCONTROL = (3 << 14) | (4 << 3);
            }
            else
            {
                uda1380_set_recvol(0, 0, AUDIO_GAIN_LINEIN);
                /* Reset PDIR2 data flow */
                DATAINCONTROL = (1 << 9);
            }

            if (source != last_source)
            {
                /* I2S recording and playback */
                uda1380_enable_recording(false);    /* source line */
                uda1380_set_monitor(true);
            }
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
} /* audio_set_source */


