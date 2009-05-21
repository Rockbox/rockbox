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

#ifndef _ROCKCONF_H_
#define _ROCKCONF_H_

#include "plugin.h"

#undef LUAI_THROW
#undef LUAI_TRY
#undef luai_jmpbuf

#ifndef SIMULATOR
#include "../../codecs/lib/setjmp.h"
#else
#include <setjmp.h>
#endif

#include "lib/pluginlib_exit.h"

#define LUAI_THROW(L,c) longjmp((c)->b, 1)
#define LUAI_TRY(L,c,a) if (setjmp((c)->b) == 0) { a }
#define luai_jmpbuf jmp_buf

extern char curpath[MAX_PATH];
void *dlrealloc(void *ptr, size_t size);
void dlfree(void *ptr);
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *str, char **endptr, int base);
long floor(long x);
long pow(long x, long y);

/* Simple substitutions */
#define realloc  dlrealloc
#define free     dlfree
#define memchr   rb->memchr
#define snprintf rb->snprintf
#define strcat   rb->strcat
#define strchr   rb->strchr
#define strcmp   rb->strcmp
#define strcpy   rb->strcpy
#define strncpy  rb->strncpy
#define strlen   rb->strlen

#endif /* _ROCKCONF_H_ */

