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

#include <plugin.h>

/* workaround for cygwin not defining endian macros */
#if !defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN) && defined(_X86_)
#define LITTLE_ENDIAN
#endif

#define malloc(a) my_malloc(a)
void *my_malloc(size_t size);

extern struct plugin_api* rb;
extern int shut,cleanshut;
void vid_update(int scanline);
void vid_init(void);
void vid_begin(void);
void vid_end(void);
void die(char *message, ...);
void setmallocpos(void *pointer);
void vid_settitle(char *title);
void *sys_timer(void);
int  sys_elapsed(long *oldtick);
void sys_sleep(int us);
int  pcm_submit(void);
void pcm_init(void);
void doevents(void);
int  isupper(int c);
int  isdigit(int c);
void ev_poll(void);

#ifdef SIMULATOR
#undef opendir
#define opendir(a)      rb->sim_opendir((a))
#undef closedir
#define closedir(a)     rb->sim_closedir((a))
#undef mkdir
#define mkdir(a,b)      rb->sim_mkdir((a),(b))
#undef open
#define open(a,b)       rb->sim_open((a),(b))
#undef lseek
#define lseek(a,b,c)    rb->sim_lseek((a),(b),(c))
#define ICODE_ATTR	 
#define IDATA_ATTR	 
#else /* !SIMULATOR */
#define opendir(a)      rb->opendir((a))
#define closedir(a)     rb->closedir((a))
#define mkdir(a,b)      rb->mkdir((a),(b))
#define open(a,b)       rb->open((a),(b))
#define lseek(a,b,c)    rb->lseek((a),(b),(c))
#if CONFIG_KEYPAD == IRIVER_H100_PAD
#define ICODE_ATTR	__attribute__ ((section(".icode")))
#define IDATA_ATTR	__attribute__ ((section(".idata")))
#define USE_IRAM	1
#else
#define ICODE_ATTR	
#define IDATA_ATTR	 
#endif
#endif /* !SIMULATOR */

#define strcat(a,b)     rb->strcat((a),(b))
#define close(a)        rb->close((a))
#define read(a,b,c)     rb->read((a),(b),(c))
#define write(a,b,c)    rb->write((a),(b),(c))
#define memset(a,b,c)   rb->memset((a),(b),(c))
#define memcpy(a,b,c)   rb->memcpy((a),(b),(c))
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

