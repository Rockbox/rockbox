/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include "buffer.h"

#ifdef SIMULATOR
unsigned char mp3buffer[(MEM*1024-256)*1024];
unsigned char *mp3end = mp3buffer + sizeof(mp3buffer);
#else
/* defined in linker script */
extern unsigned char mp3buffer[];
#endif

unsigned char *mp3buf;

void buffer_init(void)
{
    mp3buf = mp3buffer;
}

void *buffer_alloc(size_t size)
{
    void *retval = mp3buf;
    
    mp3buf += size;
    return retval;
}
