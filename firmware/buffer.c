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
unsigned char audiobuffer[(MEM*1024-256)*1024];
unsigned char *audiobufend = audiobuffer + sizeof(audiobuffer);
#else
/* defined in linker script */
extern unsigned char audiobuffer[];
#endif

unsigned char *audiobuf;

void buffer_init(void)
{
    audiobuf = audiobuffer;
}

void *buffer_alloc(size_t size)
{
    void *retval = audiobuf;
    
    audiobuf += size;
    return retval;
}
