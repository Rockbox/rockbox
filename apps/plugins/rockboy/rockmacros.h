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

#include "autoconf.h"

#define malloc(a) my_malloc(a)
void *my_malloc(size_t size);

extern const struct plugin_api* rb;
extern int shut,cleanshut;
void vid_init(void);
inline void vid_begin(void);
void die(char *message, ...);
void doevents(void) ICODE_ATTR;
void ev_poll(void);
int do_user_menu(void);
void setvidmode(void);
#if defined(HAVE_LCD_COLOR)
void set_pal(void);
#else
void vid_update(int scanline);
#endif
#ifdef DYNAREC
extern struct dynarec_block newblock;
void dynamic_recompile (struct dynarec_block *newblock);
#endif

#define USER_MENU_QUIT -2

/* Disable IBSS when using dynarec since it won't fit */
#ifdef DYNAREC
#undef IBSS_ATTR
#define IBSS_ATTR
#endif

/* libc functions */
#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isalpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && ((c) <= 'Z')))
#define isalnum(c) (isdigit(c) || (isalpha(c)))

#ifdef SIMULATOR
#undef open
#define open(a,b)       rb->sim_open((a),(b))
#undef lseek
#define lseek(a,b,c)    rb->sim_lseek((a),(b),(c))
#undef close
#define close(a)        rb->sim_close((a))
#undef read
#define read(a,b,c)     rb->sim_read((a),(b),(c))
#undef write
#define write(a,b,c)    rb->sim_write((a),(b),(c))
#else /* !SIMULATOR */
#define open(a,b)       rb->open((a),(b))
#define lseek(a,b,c)    rb->lseek((a),(b),(c))
#define close(a)        rb->close((a))
#define read(a,b,c)     rb->read((a),(b),(c))
#define write(a,b,c)    rb->write((a),(b),(c))
#endif /* !SIMULATOR */

#define strcat(a,b)     rb->strcat((a),(b))
#define memset(a,b,c)   rb->memset((a),(b),(c))
#define strcpy(a,b)     rb->strcpy((a),(b))
#define strncpy(a,b,c)  rb->strncpy((a),(b),(c))
#define strlen(a)       rb->strlen((a))
#define strcmp(a,b)     rb->strcmp((a),(b))
#define strchr(a,b)     rb->strchr((a),(b))
#define strrchr(a,b)    rb->strrchr((a),(b))
#define strcasecmp(a,b) rb->strcasecmp((a),(b))
#define srand(a)        rb->srand((a))
#define rand()          rb->rand()
#define atoi(a)         rb->atoi((a))
#define strcat(a,b)     rb->strcat((a),(b))
#define snprintf(...)   rb->snprintf(__VA_ARGS__)
#define fdprintf(...)    rb->fdprintf(__VA_ARGS__)
#define tolower(_A_)    (isupper(_A_) ? (_A_ - 'A' + 'a') : _A_)

/* Using #define isn't enough with GCC 4.0.1 */
void* memcpy(void* dst, const void* src, size_t size) ICODE_ATTR;

struct options {
   int A, B, START, SELECT, MENU;
   int UP, DOWN, LEFT, RIGHT;
   int frameskip, fps, maxskip;
   int sound, scaling, showstats;
   int rotate;
   int pal;
   int dirty;
};

bool plugbuf;

extern struct options options;
#define savedir ROCKBOX_DIR "/rockboy"

#endif
