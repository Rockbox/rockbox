/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* "helper functions" common to all codecs  */

#include "plugin.h"
#include "playback.h"
#include "codeclib.h"
#include "xxx2wav.h"
#include "id3.h"

struct codec_api *local_rb;

int codec_init(struct codec_api* rb)
{
    local_rb = rb;
    mem_ptr = 0;
    mallocbuf = (unsigned char *)rb->get_codec_memory((long *)&bufsize);
  
    return 0;
}

void codec_set_replaygain(struct mp3entry* id3)
{
    local_rb->configure(DSP_SET_TRACK_GAIN, (long *) id3->track_gain);
    local_rb->configure(DSP_SET_ALBUM_GAIN, (long *) id3->album_gain);
    local_rb->configure(DSP_SET_TRACK_PEAK, (long *) id3->track_peak);
    local_rb->configure(DSP_SET_ALBUM_PEAK, (long *) id3->album_peak);
}
