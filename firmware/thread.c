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
#include "thread.h"

struct regs
{
    unsigned int  r[7]; /* Registers r8 thru r14 */
    void          *sp;  /* Stack pointer (r15) */
    unsigned int  mach;
    unsigned int  macl;
    unsigned int  sr;   /* Status register */
    void*         pr;   /* Procedure register */
};

static int num_threads;
static int current_thread;
static struct regs thread_contexts[MAXTHREADS];

/*--------------------------------------------------------------------------- 
 * Store non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void store_context(void* addr)
{
    asm volatile ("add #48, %0\n\t"
                  "sts.l pr,  @-%0\n\t"
                  "stc.l sr,  @-%0\n\t"
                  "sts.l macl,@-%0\n\t"
                  "sts.l mach,@-%0\n\t"
                  "mov.l r15, @-%0\n\t"
                  "mov.l r14, @-%0\n\t"
                  "mov.l r13, @-%0\n\t"
                  "mov.l r12, @-%0\n\t"
                  "mov.l r11, @-%0\n\t"
                  "mov.l r10, @-%0\n\t"
                  "mov.l r9,  @-%0\n\t"
                  "mov.l r8,  @-%0" : : "r" (addr));
}

/*--------------------------------------------------------------------------- 
 * Load non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void load_context(void* addr)
{
    asm volatile ("mov.l @%0+,r8\n\t"
                  "mov.l @%0+,r9\n\t"
                  "mov.l @%0+,r10\n\t"
                  "mov.l @%0+,r11\n\t"
                  "mov.l @%0+,r12\n\t"
                  "mov.l @%0+,r13\n\t"
                  "mov.l @%0+,r14\n\t"
                  "mov.l @%0+,r15\n\t"
                  "lds.l @%0+,mach\n\t"
                  "lds.l @%0+,macl\n\t"
                  "ldc.l @%0+,sr\n\t"
                  "mov.l @%0,%0\n\t"
                  "lds %0,pr\n\t"
                  "mov.l %0, @(0, r15)" : "+r" (addr));
}

/*--------------------------------------------------------------------------- 
 * Switch thread in round robin fashion.
 *---------------------------------------------------------------------------
 */
void switch_thread(void)
{
    int current;
    int next;

    next = current = current_thread;
    if (++next >= num_threads)
        next = 0;
    current_thread = next;
    store_context(&thread_contexts[current]);
    load_context(&thread_contexts[next]);
}

/*--------------------------------------------------------------------------- 
 * Create thread.
 * Return 0 if context area could be allocated, else -1.
 *---------------------------------------------------------------------------
 */
int create_thread(void* function, void* stack, int stack_size)
{
    if (num_threads >= MAXTHREADS)
        return -1;
    else
    {
        struct regs* regs = &thread_contexts[num_threads++];
        store_context(regs);
        /* Subtract 4 to leave room for the PR push in ldctx()
           Align it on an even 32 bit boundary */
        regs->sp = (void*)(((unsigned int)stack + stack_size - 4) & ~3);
        regs->sr = 0;
        regs->pr = function;
    }
    return 0;
}

void init_threads(void)
{
    num_threads = 1; /* We have 1 thread to begin with */
    current_thread = 0; /* The current thread is number 0 */
}
