/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Dan Everton (safetydan)
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

#include "plugin.h"
#include "rockmalloc.h"
#include "malloc.c"

void *rocklua_morecore(int size)
{
    static char *sbrk_top = NULL;
    static ssize_t total_size = 0;
    void *ptr = 0;

    if (size > 0) {
        size = size + 4 - (size % 4);
        
        if (sbrk_top == NULL) {
            sbrk_top = rb->plugin_get_buffer((size_t *) &total_size);
        }
        
        if (sbrk_top == NULL || size + 4 > abs(total_size)) {
            /* Try the audio buffer */
            sbrk_top = rb->plugin_get_audio_buffer((size_t *) &total_size);

            if(sbrk_top == NULL || size + 4 > abs(total_size)) {
                printf("malloc failed for %d!\n", size);
                return (void *) MFAIL;
            }
        }
        
        ptr = sbrk_top + 4;
        *((unsigned int *) sbrk_top) = size;

        sbrk_top += size + 4;
        total_size -= size + 4;
        
        return ptr;
    } else if (size < 0) {
      /* we don't currently support shrink behavior */
      return (void *) MFAIL;
    } else {
      return sbrk_top;
    }
}

