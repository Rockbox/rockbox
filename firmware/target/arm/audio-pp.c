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
#include "sound.h"

void audio_set_output_source(int source)
{
    if ((unsigned)source >= AUDIO_NUM_SOURCES)
        source = AUDIO_SRC_PLAYBACK;
} /* audio_set_output_source */

void audio_set_source(int source, unsigned flags)
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
            }
        break;

        case AUDIO_SRC_MIC:             /* recording only */
            if (source != last_source)
            {
                audiohw_enable_recording(true);  /* source mic */
                audiohw_set_monitor(false);
            }
        break;

        case AUDIO_SRC_LINEIN:          /* recording only */
            if (source != last_source)
            {
                audiohw_enable_recording(false); /* source line */
                audiohw_set_monitor(false);
            }
        break;
#ifdef CONFIG_TUNER
        case AUDIO_SRC_FMRADIO:         /* recording and playback */
            if (!recording)
                audiohw_set_recvol(0, 0, AUDIO_GAIN_LINEIN);

            if (source == last_source && recording == last_recording)
                break;

            last_recording = recording;

            /* I2S recording and playback */
            audiohw_enable_recording(false);    /* source line */
            audiohw_set_monitor(!recording);
        break;
#endif
    } /* end switch */

    last_source = source;
} /* audio_set_source */


