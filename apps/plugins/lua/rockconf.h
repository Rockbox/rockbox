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
#include <tlsf.h>

#undef LUAI_THROW
#undef LUAI_TRY
#undef luai_jmpbuf

#undef LUA_PATH_DEFAULT
#define LUA_PATH_DEFAULT  "$/?.lua;" "$/?/init.lua;" VIEWERS_DIR"/lua/?.lua;" VIEWERS_DIR"/lua/?/init.lua;"
#define INBINARYSTRINGS /* Static strings stored as pointer rather than copied into lua state */

#include <setjmp.h>

#include "lib/pluginlib_exit.h"

#define LUAI_THROW(L,c) longjmp((c)->b, 1)
#define LUAI_TRY(L,c,a) if (setjmp((c)->b) == 0) { a }
#define luai_jmpbuf jmp_buf

extern char curpath[MAX_PATH];
struct tm *gmtime(const time_t *timep);
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *str, char **endptr, int base);
size_t strftime(char* dst, size_t max, const char* format, const struct tm* tm);
long lfloor(long x);
long lpow(long x, long y);

#define floor   lfloor
#define pow     lpow

/* Simple substitutions */
#define malloc   tlsf_malloc
#define realloc  tlsf_realloc
#define free     tlsf_free
#define memchr   rb()->memchr
#define snprintf rb()->snprintf
#define strcat   rb()->strcat
#define strchr   rb()->strchr
#define strcmp   rb()->strcmp
#define strcpy   rb()->strcpy
#define strlen   rb()->strlen
#define yield()  rb()->yield()

#endif /* _ROCKCONF_H_ */

/* Convoluted way to eliminate extra register load per rb-> plugin call
 * ARM assembly requires a extra register load of the rb-> struct for every
 * function call instead we memcpy the rb struct to rbp and use that instead
 * Caveat: rbp must be initialized before use or Bad Things WILL happen
 */
#if defined(__arm__) && !defined(rb)
 extern struct plugin_api rbp;
 #define rb() (&rbp)
#elif !defined(rb)
  #define rb() rb
  static inline void initialize_rb_plugin(void) {}
#endif
