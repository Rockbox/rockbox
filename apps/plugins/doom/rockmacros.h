/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Michiel van der Kolk, Jens Arnold
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
#ifndef __ROCKMACROS_H__
#define __ROCKMACROS_H__

#include "plugin.h"
#include "ctype.h"
#include "autoconf.h"
#include "z_zone.h"

extern bool noprintf;
extern bool doomexit;

/* libc functions */
int printf(const char *fmt, ...);
int fileexists(const char * fname);
char *my_strtok( char * s, const char * delim );
#undef  alloca
#define alloca             __builtin_alloca

#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
#define open(a, ...)       rb->open((a), __VA_ARGS__)
#define close(a)           rb->close((a))
#else
//int my_open(const char *file, int flags, ...);
//int my_close(int id);
//#define open(a, ...)       my_open((a), __VA_ARGS__)
//#define close(a)           my_close((a))
#endif
#undef  strtok
#define strtok(a,b)        my_strtok((a),(b))

#define PACKEDATTR __attribute__((packed)) // Needed for a few things
#define GAMEBASE ROCKBOX_DIR "/doom/"
//#define SIMPLECHECKS
#define NO_PREDEFINED_LUMPS
#define TABLES_AS_LUMPS // This frees up alot of space in the plugin buffer

#define MAKE_FOURCC(a,b,c,d) (uint32_t)((((a)<<24)|((b)<<16)|((c)<<8)|(d)))

/* Config file magic - increment the version number whenever the settings
   structure changes.
 */
#define DOOM_CONFIG_MAGIC     MAKE_FOURCC('D','O','O','M')
#define DOOM_CONFIG_VERSION   3

#endif
