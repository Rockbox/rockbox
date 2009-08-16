/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: wps_parser.c 19880 2009-01-29 20:49:43Z mcuelenaere $
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

/* Allocate size bytes from the buffer */
void* skin_buffer_alloc(size_t size);


/* Get a pointer to the skin buffer and the count of how much is free
 * used to do your own buffer management. 
 * Any memory used will be overwritten next time wps_buffer_alloc()
 * is called unless skin_buffer_increment() is called first
 */
void* skin_buffer_grab(size_t *freespace);

/* Use after skin_buffer_grab() to specify how much buffer was used */
void skin_buffer_increment(size_t used);

#endif /* _SKIN_BUFFER_H_ */
