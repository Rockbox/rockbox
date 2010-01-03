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
 * Copyright (C) 2009 Jonathan Gordon
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

#ifndef _SKIN_BUFFER_H_
#define _SKIN_BUFFER_H_


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* int the global buffer */
void skin_buffer_init(void);

/* get the number of bytes currently being used */
size_t skin_buffer_usage(void);
size_t skin_buffer_freespace(void);

/* Allocate size bytes from the buffer */
void* skin_buffer_alloc(size_t size);


/* Get a pointer to the skin buffer and the count of how much is free
 * used to do your own buffer management. 
 * Any memory used will be overwritten next time wps_buffer_alloc()
 * is called unless skin_buffer_increment() is called first
 * 
 * This is from the start of the buffer, it is YOUR responsility to make
 * sure you dont ever use more then *freespace, and bear in mind this will only
 * be valid untill skin_buffer_alloc() is next called...
 * so call skin_buffer_increment() and skin_buffer_freespace() regularly
 */
void* skin_buffer_grab(size_t *freespace);

/* Use after skin_buffer_grab() to specify how much buffer was used.
 * align should always be true unless there is a possibility that you will need
 * more space *immediatly* after the previous allocation. (i.e in an array).
 * NEVER leave the buffer unaligned */
void skin_buffer_increment(size_t used, bool align);

/* free previously skin_buffer_increment()'ed space. This just moves the pointer
 * back 'used' bytes so make sure you actually want to do this */
void skin_buffer_free_from_front(size_t used);

#endif /* _SKIN_BUFFER_H_ */
