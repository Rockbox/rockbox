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

#ifndef SIMULATOR
/* Need to keep structures inside the header file because debug_menu
 * needs them. */
#ifdef CPU_COLDFIRE
struct regs
{
    unsigned int macsr;  /* EMAC status register */
    unsigned int d[6];   /* d2-d7 */
    unsigned int a[5];   /* a2-a6 */
    void         *sp;    /* Stack pointer (a7) */
    void         *start; /* Thread start address, or NULL when started */
};
#elif CONFIG_CPU == SH7034
struct regs
{
    unsigned int r[7];   /* Registers r8 thru r14 */
    void         *sp;    /* Stack pointer (r15) */
    void         *pr;    /* Procedure register */
    void         *start; /* Thread start address, or NULL when started */
};
#elif defined(CPU_ARM)
struct regs
{
    unsigned int r[8];   /* Registers r4-r11 */
    void         *sp;    /* Stack pointer (r13) */
    unsigned int lr;     /* r14 (lr) */
    void         *start; /* Thread start address, or NULL when started */
};
#elif CONFIG_CPU == TCC730
struct regs
{
    void *sp;    /* Stack pointer (a15) */
    void *start; /* Thread start address */
    int started; /* 0 when not started */
};
#endif

struct thread_entry {
    struct regs context;
    const char *name;
    void *stack;
    int stack_size;
};

struct core_entry {
    int num_threads;
    volatile int num_sleepers;
    int current_thread;
    struct thread_entry threads[MAXTHREADS];
};
#endif

int create_thread(void (*function)(void), void* stack, int stack_size,
                  const char *name);
int create_thread_on_core(unsigned int core, void (*function)(void), void* stack, int stack_size,
                  const char *name);
void remove_thread(int threadnum);
void remove_thread_on_core(unsigned int core, int threadnum);
void switch_thread(void);
void sleep_thread(void);
void wake_up_thread(void);
void init_threads(void);
int thread_stack_usage(int threadnum);
int thread_stack_usage_on_core(unsigned int core, int threadnum);
#ifdef RB_PROFILE
void profile_thread(void);
#endif

#endif
