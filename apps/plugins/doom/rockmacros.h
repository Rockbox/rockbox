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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

extern struct plugin_api* rb;
extern bool noprintf;

/* libc functions */
int printf(const char *fmt, ...);
int fileexists(const char * fname);
int my_open(const char *file, int flags);
int my_close(int id);
char *my_strtok( char * s, const char * delim );
#define alloca             __builtin_alloca
#define fprintf(...)       rb->fdprintf(__VA_ARGS__)
#define vsnprintf(...)     rb->vsnprintf(__VA_ARGS__)

#ifdef SIMULATOR
#undef opendir
#undef closedir
#undef mkdir
#undef open
#undef lseek
#undef filesize
#define opendir(a)         rb->sim_opendir((a))
#define closedir(a)        rb->sim_closedir((a))
#define mkdir(a,b)         rb->sim_mkdir((a),(b))
#define open(a,b)          rb->sim_open((a),(b))
#define lseek(a,b,c)       rb->sim_lseek((a),(b),(c))
#define filesize(a)        rb->sim_filesize((a))
#else /* !SIMULATOR */
#define opendir(a)         rb->opendir((a))
#define closedir(a)        rb->closedir((a))
#define filesize(a)        rb->filesize((a))
#define mkdir(a)           rb->mkdir((a),0777)
#define open(a,b)          my_open((a),(b))
#define close(a)           my_close((a))
#define lseek(a,b,c)       rb->lseek((a),(b),(c))
#endif /* !SIMULATOR */

#define strcat(a,b)        rb->strcat((a),(b))
#define read(a,b,c)        rb->read((a),(b),(c))
#define write(a,b,c)       rb->write((a),(b),(c))
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
#define GAMEBASE "/games/doom/"
//#define SIMPLECHECKS
#define NO_PREDEFINED_LUMPS
#define TABLES_AS_LUMPS // This frees up alot of space in the plugin buffer
//#define FANCY_MENU  // This is a call to allow load_main_backdrop to run in doom
#endif
