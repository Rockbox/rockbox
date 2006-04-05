/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Ulf Ralberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef THREAD_H
#define THREAD_H

#include <stdbool.h>

#if CONFIG_CODEC == SWCODEC
#define MAXTHREADS	16
#else
#define MAXTHREADS  11
#endif

#define DEFAULT_STACK_SIZE 0x400 /* Bytes */

int create_thread(void (*function)(void), void* stack, int stack_size,
                  const char *name);
void remove_thread(int threadnum);
void switch_thread(void);
void sleep_thread(void);
void wake_up_thread(void);
void init_threads(void);
int thread_stack_usage(int threadnum);
#ifdef RB_PROFILE
void profile_thread(void);
#endif

#endif
