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

extern const struct plugin_api* rb;
extern bool noprintf;
extern bool doomexit;

/* libc functions */
int printf(const char *fmt, ...);
int fileexists(const char * fname);
int my_open(const char *file, int flags);
int my_close(int id);
char *my_strtok( char * s, const char * delim );
#define alloca             __builtin_alloca
#define fdprintf(...)       rb->fdprintf(__VA_ARGS__)
#define vsnprintf(...)     rb->vsnprintf(__VA_ARGS__)
#define read_line(a,b,c)   rb->read_line((a),(b),(c))

#ifdef SIMULATOR
#undef open
#undef close
#undef lseek
#undef filesize
#undef read
#undef write
#define open(a,b)          rb->sim_open((a),(b))
#define close(a)           rb->sim_close((a))
#define lseek(a,b,c)       rb->sim_lseek((a),(b),(c))
#define filesize(a)        rb->sim_filesize((a))
#define read(a,b,c)        rb->sim_read((a),(b),(c))
#define write(a,b,c)       rb->sim_write((a),(b),(c))
#else /* !SIMULATOR */
#define open(a,b)          my_open((a),(b))
#define close(a)           my_close((a))
#define lseek(a,b,c)       rb->lseek((a),(b),(c))
#define filesize(a)        rb->filesize((a))
#define read(a,b,c)        rb->read((a),(b),(c))
#define write(a,b,c)       rb->write((a),(b),(c))
#endif /* !SIMULATOR */

#define strtok(a,b)        my_strtok((a),(b))
#define strcat(a,b)        rb->strcat((a),(b))
#define memset(a,b,c)      rb->memset((a),(b),(c))
#define memmove(a,b,c)     rb->memmove((a),(b),(c))
#define memcmp(a,b,c)      rb->memcmp((a),(b),(c))
#define memchr(a,b,c)      rb->memchr((a),(b),(c))
#define strcpy(a,b)        rb->strcpy((a),(b))
#define strncpy(a,b,c)     rb->strncpy((a),(b),(c))
#define strlen(a)          rb->strlen((a))
#define strcmp(a,b)        rb->strcmp((a),(b))
#define strncmp(a,b,c)     rb->strncmp((a),(b),(c))
#define strchr(a,b)        rb->strchr((a),(b))
#define strrchr(a,b)       rb->strrchr((a),(b))
#define strcasecmp(a,b)    rb->strcasecmp((a),(b))
#define strncasecmp(a,b,c) rb->strncasecmp((a),(b),(c))
#define srand(a)           rb->srand((a))
#define rand()             rb->rand()
#define atoi(a)            rb->atoi((a))
#define strcat(a,b)        rb->strcat((a),(b))
#define snprintf           rb->snprintf

/* Using #define isn't enough with GCC 4.0.1 */
inline void* memcpy(void* dst, const void* src, size_t size);

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
