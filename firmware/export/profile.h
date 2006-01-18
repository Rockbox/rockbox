/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Profiling routines counts ticks and calls to each profiled function.
 * 
 * Copyright (C) 2005 by Brandon Low
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 ****************************************************************************/

#ifndef _SYS_PROFILE_H
#define _SYS_PROFILE_H 1

#include <sys/types.h>

/* PFD is Profiled Function Data */

/* Indices are shorts which means that we use 4k of RAM */
#define INDEX_BITS 11    /* What is a reasonable size for this? */
#define INDEX_SIZE 2048  /* 2 ^ INDEX_BITS */
#define INDEX_MASK 0x7FF /* lower INDEX_BITS 1 */

/*
 * In the current setup (pfd has 4 longs and 2 shorts) this uses 20k of RAM 
 * for profiling, and allows for profiling sections of code with up-to 
 * 1024 function caller->callee pairs
 */
#define NUMPFDS 1024

struct pfd_struct {
    void *self_pc;
    unsigned long count;
    unsigned long time;
    unsigned short link;
    struct pfd_struct *caller;
};

/* Possible states of profiling */
#define PROF_ON 0x00
#define PROF_BUSY 0x01
#define PROF_ERROR 0x02
#define PROF_OFF 0x03
/* Masks for thread switches */
#define PROF_OFF_THREAD 0x10
#define PROF_ON_THREAD 0x0F

extern int current_thread;

/* Initialize and start profiling */
void profstart(int current_thread)
  NO_PROF_ATTR;

/* Clean up and write profile data */
void profstop (void)
  NO_PROF_ATTR;

/* Called every time a thread stops, we check if it's our thread and store
 * temporary timing data if it is */
void profile_thread_stopped(int current_thread)
  NO_PROF_ATTR;
/* Called when a thread starts, we check if it's our thread and resume timing */
void profile_thread_started(int current_thread)
  NO_PROF_ATTR;

void profile_func_exit(void *this_fn, void *call_site) 
  NO_PROF_ATTR ICODE_ATTR;
void profile_func_enter(void *this_fn, void *call_site)
  NO_PROF_ATTR ICODE_ATTR;

#endif /*_SYS_PROFILE_H*/
