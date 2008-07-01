/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Michael Sevakis
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
#ifndef MPEG_ALLOC_H
#define MPEG_ALLOC_H

/* returns the remaining mpeg2 buffer and it's size */
void * mpeg2_get_buf(size_t *size);
void *mpeg_malloc(size_t size, mpeg2_alloc_t reason);
/* Grabs all the buffer available sans margin */
void *mpeg_malloc_all(size_t *size_out, mpeg2_alloc_t reason);
/* Initializes the malloc buffer with the given base buffer */
bool mpeg_alloc_init(unsigned char *buf, size_t mallocsize);

#endif /* MPEG_ALLOC_H */
