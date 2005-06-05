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

/* Various "helper functions" common to all the xxx2wav decoder plugins  */

#include "plugin.h"
#include "playback.h"
#include "codeclib.h"

struct plugin_api* local_rb;

int codec_init(struct plugin_api* rb, struct codec_api* ci) {
    local_rb = rb;
    
    xxx2wav_set_api(rb);
    mem_ptr = 0;
    mallocbuf = (unsigned char *)ci->get_codec_memory((size_t *)&bufsize);
  
    return 0;
}
