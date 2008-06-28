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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
    /* 32-bit aligned */
    audiobuf = (void *)(((unsigned long)audiobuffer + 3) & ~3);
}

void *buffer_alloc(size_t size)
{
    void *retval = audiobuf;
    
    audiobuf += size;
    /* 32-bit aligned */
    audiobuf = (void *)(((unsigned long)audiobuf + 3) & ~3);
    
    return retval;
}

