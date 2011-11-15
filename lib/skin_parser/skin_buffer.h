/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: skin_buffer.c 25962 2010-05-12 09:31:40Z jdgordon $
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef _SKIN_BUFFFER_H_
#define _SKIN_BUFFFER_H_
void skin_buffer_init(char* buffer, size_t size);
/* Allocate size bytes from the buffer */

#ifndef __PCTOOL__
#define INVALID_OFFSET (-1)
#define IS_VALID_OFFSET(o) ((o) >= 0)
long skin_buffer_to_offset(void *pointer);
void* skin_buffer_from_offset(long offset);
#else
#define INVALID_OFFSET (NULL)
#define IS_VALID_OFFSET(o) ((o) != NULL)
#define skin_buffer_to_offset(p) p
#define skin_buffer_from_offset(o) o
#endif

/* #define DEBUG_SKIN_ALLOCATIONS */

#ifdef DEBUG_SKIN_ALLOCATIONS 
#define FOO(X) #X
#define STRNG(X) FOO(X)
#define skin_buffer_alloc(s) skin_buffer_alloc_ex(s, __FILE__ ":" STRNG(__LINE__))
void* skin_buffer_alloc_ex(size_t size, char* str);
#else
void* skin_buffer_alloc(size_t size);
#endif


/* get the number of bytes currently being used */
size_t skin_buffer_usage(void);
size_t skin_buffer_freespace(void);

#endif
