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
#include "config.h"
#include <stdbool.h>
#include "thread.h"
#include "panic.h"
#include "system.h"
#include "kernel.h"
#include "cpu.h"


#define DEADBEEF ((unsigned int)0xdeadbeef)
/* Cast to the the machine int type, whose size could be < 4. */

struct core_entry cores[NUM_CORES] IBSS_ATTR;

static const char main_thread_name[] = "main";

extern int stackbegin[];
extern int stackend[];

#ifdef CPU_PP
#ifndef BOOTLOADER
extern int cop_stackbegin[];
extern int cop_stackend[];
#else
/* The coprocessor stack is not set up in the bootloader code, but the
   threading is.  No threads are run on the coprocessor, so set up some dummy
   stack */
int *cop_stackbegin = stackbegin;
int *cop_stackend = stackend;
#endif
#endif

void switch_thread(void) ICODE_ATTR;
static inline void store_context(void* addr) __attribute__ ((always_inline));
static inline void load_context(const void* addr) __attribute__ ((always_inline));

#ifdef RB_PROFILE
#include <profile.h>
void profile_thread(void) {
    profstart(cores[CURRENT_CORE].current_thread);
}
#endif

#if defined(CPU_ARM)
/*---------------------------------------------------------------------------
 * Store non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void store_context(void* addr)
{
    asm volatile(
        "stmia  %0, { r4-r11, sp, lr }\n"
        : : "r" (addr)
    );
}

/*---------------------------------------------------------------------------
 * Load non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void load_context(const void* addr)
{
    asm volatile(
        "ldmia  %0, { r4-r11, sp, lr }\n" /* load regs r4 to r14 from context */
        "ldr    r0, [%0, #40]    \n" /* load start pointer */
        "mov    r1, #0           \n"
        "cmp    r0, r1           \n" /* check for NULL */
        "strne  r1, [%0, #40]    \n" /* if it's NULL, we're already running */
        "movne  pc, r0           \n" /* not already running, so jump to start */
        : : "r" (addr) : "r0", "r1"
    );
}

#elif defined(CPU_COLDFIRE)
/*---------------------------------------------------------------------------
 * Store non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void store_context(void* addr)
{
    asm volatile (
        "move.l  %%macsr,%%d0    \n"
        "movem.l %%d0/%%d2-%%d7/%%a2-%%a7,(%0)   \n"
        : : "a" (addr) : "d0" /* only! */
    );
}

/*---------------------------------------------------------------------------
 * Load non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void load_context(const void* addr)
{
    asm volatile (
        "movem.l (%0),%%d0/%%d2-%%d7/%%a2-%%a7   \n"  /* Load context */
        "move.l  %%d0,%%macsr    \n"
        "move.l  (52,%0),%%d0    \n"  /* Get start address */
        "beq.b   .running        \n"  /* NULL -> already running */
        "clr.l   (52,%0)         \n"  /* Clear start address.. */
        "move.l  %%d0,%0         \n"
        "jmp     (%0)            \n"  /* ..and start the thread */
    ".running:                   \n"
        : : "a" (addr) : "d0" /* only! */
    );
}

#elif CONFIG_CPU == SH7034
/*---------------------------------------------------------------------------
 * Store non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void store_context(void* addr)
{
    asm volatile (
        "add     #36,%0    \n"
        "sts.l   pr, @-%0  \n"
        "mov.l   r15,@-%0  \n"
        "mov.l   r14,@-%0  \n"
        "mov.l   r13,@-%0  \n"
        "mov.l   r12,@-%0  \n"
        "mov.l   r11,@-%0  \n"
        "mov.l   r10,@-%0  \n"
        "mov.l   r9, @-%0  \n"
        "mov.l   r8, @-%0  \n"
        : : "r" (addr)
    );
}

/*---------------------------------------------------------------------------
 * Load non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void load_context(const void* addr)
{
    asm volatile (
        "mov.l   @%0+,r8   \n"
        "mov.l   @%0+,r9   \n"
        "mov.l   @%0+,r10  \n"
        "mov.l   @%0+,r11  \n"
        "mov.l   @%0+,r12  \n"
        "mov.l   @%0+,r13  \n"
        "mov.l   @%0+,r14  \n"
        "mov.l   @%0+,r15  \n"
        "lds.l   @%0+,pr   \n"
        "mov.l   @%0,r0    \n"  /* Get start address */
        "tst     r0,r0     \n"
        "bt      .running  \n"  /* NULL -> already running */
        "lds     r0,pr     \n"
        "mov     #0,r0     \n"
        "rts               \n"  /* Start the thread */
        "mov.l   r0,@%0    \n"  /* Clear start address */
    ".running:             \n"
        : : "r" (addr) : "r0" /* only! */
    );
}

#elif CONFIG_CPU == TCC730
/*---------------------------------------------------------------------------
 * Store non-volatile context.
 *---------------------------------------------------------------------------
 */
#define store_context(addr)				\
    __asm__ volatile (                                  \
        "push r0,r1\n\t"				\
        "push r2,r3\n\t"				\
        "push r4,r5\n\t"				\
        "push r6,r7\n\t"				\
        "push a8,a9\n\t"				\
        "push a10,a11\n\t"				\
        "push a12,a13\n\t"				\
        "push a14\n\t"                                  \
        "ldw @[%0+0], a15\n\t" : : "a" (addr) );

/*---------------------------------------------------------------------------
 * Load non-volatile context.
 *---------------------------------------------------------------------------
 */
#define load_context(addr)                      \
    {                                           \
        if (!(addr)->started) {                 \
            (addr)->started = 1;                \
            __asm__ volatile (                  \
                "ldw a15, @[%0+0]\n\t"          \
                "ldw a14, @[%0+4]\n\t"		\
                "jmp a14\n\t" : : "a" (addr)	\
                );				\
        } else                                  \
            __asm__ volatile (                  \
                "ldw a15, @[%0+0]\n\t"		\
                "pop a14\n\t"			\
                "pop a13,a12\n\t"               \
                "pop a11,a10\n\t"               \
                "pop a9,a8\n\t"			\
                "pop r7,r6\n\t"			\
                "pop r5,r4\n\t"			\
                "pop r3,r2\n\t"			\
                "pop r1,r0\n\t" : : "a" (addr)	\
                );                              \
                                                \
    }

#endif

/*---------------------------------------------------------------------------
 * Switch thread in round robin fashion.
 *---------------------------------------------------------------------------
 */
void switch_thread(void)
{
#ifdef RB_PROFILE
    profile_thread_stopped(cores[CURRENT_CORE].current_thread);
#endif
    int current;
    unsigned int *stackptr;

#ifdef SIMULATOR
    /* Do nothing */
#else
    while (cores[CURRENT_CORE].num_sleepers == cores[CURRENT_CORE].num_threads)
    {
        /* Enter sleep mode, woken up on interrupt */
#ifdef CPU_COLDFIRE
        asm volatile ("stop #0x2000");
#elif CONFIG_CPU == SH7034
        and_b(0x7F, &SBYCR);
        asm volatile ("sleep");
#elif CONFIG_CPU == PP5020
        /* This should sleep the CPU. It appears to wake by itself on
           interrupts */
        CPU_CTL = 0x80000000;
#elif CONFIG_CPU == TCC730
	    /* Sleep mode is triggered by the SYS instr on CalmRisc16.
         * Unfortunately, the manual doesn't specify which arg to use.
         __asm__ volatile ("sys #0x0f");
         0x1f seems to trigger a reset;
         0x0f is the only one other argument used by Archos.
         */
#elif CONFIG_CPU == S3C2440
        CLKCON |= 2;
#endif
    }
#endif
    current = cores[CURRENT_CORE].current_thread;
    store_context(&cores[CURRENT_CORE].threads[current].context);

#if CONFIG_CPU != TCC730
    /* Check if the current thread stack is overflown */
    stackptr = cores[CURRENT_CORE].threads[current].stack;
    if(stackptr[0] != DEADBEEF)
       panicf("Stkov %s", cores[CURRENT_CORE].threads[current].name);
#endif

    if (++current >= cores[CURRENT_CORE].num_threads)
        current = 0;

    cores[CURRENT_CORE].current_thread = current;
    load_context(&cores[CURRENT_CORE].threads[current].context);
#ifdef RB_PROFILE
    profile_thread_started(cores[CURRENT_CORE].current_thread);
#endif
}

void sleep_thread(void)
{
    ++cores[CURRENT_CORE].num_sleepers;
    switch_thread();
}

void wake_up_thread(void)
{
    cores[CURRENT_CORE].num_sleepers = 0;
}


/*---------------------------------------------------------------------------
 * Create thread on the current core.
 * Return ID if context area could be allocated, else -1.
 *---------------------------------------------------------------------------
 */
int create_thread(void (*function)(void), void* stack, int stack_size,
                  const char *name)
{
    return create_thread_on_core(CURRENT_CORE, function, stack, stack_size,
                  name);
}

/*---------------------------------------------------------------------------
 * Create thread on a specific core.
 * Return ID if context area could be allocated, else -1.
 *---------------------------------------------------------------------------
 */
int create_thread_on_core(unsigned int core, void (*function)(void), void* stack, int stack_size,
                  const char *name)
{
    unsigned int i;
    unsigned int stacklen;
    unsigned int *stackptr;
    struct regs *regs;
    struct thread_entry *thread;

    if (cores[core].num_threads >= MAXTHREADS)
        return -1;

    /* Munge the stack to make it easy to spot stack overflows */
    stacklen = stack_size / sizeof(int);
    stackptr = stack;
    for(i = 0;i < stacklen;i++)
    {
        stackptr[i] = DEADBEEF;
    }

    /* Store interesting information */
    thread = &cores[core].threads[cores[core].num_threads];
    thread->name = name;
    thread->stack = stack;
    thread->stack_size = stack_size;
    regs = &thread->context;
#if defined(CPU_COLDFIRE) || (CONFIG_CPU == SH7034) || defined(CPU_ARM)
    /* Align stack to an even 32 bit boundary */
    regs->sp = (void*)(((unsigned int)stack + stack_size) & ~3);
#elif CONFIG_CPU == TCC730
   /* Align stack on word boundary */
    regs->sp = (void*)(((unsigned long)stack + stack_size - 2) & ~1);
    regs->started = 0;
#endif
    regs->start = (void*)function;

    wake_up_thread();
    return cores[core].num_threads++; /* return the current ID, e.g for remove_thread() */
}

/*---------------------------------------------------------------------------
 * Remove a thread on the current core from the scheduler.
 * Parameter is the ID as returned from create_thread().
 *---------------------------------------------------------------------------
 */
void remove_thread(int threadnum)
{
    remove_thread_on_core(CURRENT_CORE, threadnum);
}

/*---------------------------------------------------------------------------
 * Remove a thread on the specified core from the scheduler.
 * Parameters are the core and the ID as returned from create_thread().
 *---------------------------------------------------------------------------
 */
void remove_thread_on_core(unsigned int core, int threadnum)
{
    int i;

    if (threadnum >= cores[core].num_threads)
       return;

    cores[core].num_threads--;
    for (i=threadnum; i<cores[core].num_threads-1; i++)
    {   /* move all entries which are behind */
        cores[core].threads[i] = cores[core].threads[i+1];
    }

    if (cores[core].current_thread == threadnum) /* deleting the current one? */
        cores[core].current_thread = cores[core].num_threads; /* set beyond last, avoid store harm */
    else if (cores[core].current_thread > threadnum) /* within the moved positions? */
        cores[core].current_thread--; /* adjust it, point to same context again */
}

void init_threads(void)
{
    unsigned int core = CURRENT_CORE;

    cores[core].num_threads = 1; /* We have 1 thread to begin with */
    cores[core].current_thread = 0; /* The current thread is number 0 */
    cores[core].threads[0].name = main_thread_name;
/* In multiple core setups, each core has a different stack.  There is probably
   a much better way to do this. */
    if (core == CPU)
    {
        cores[CPU].threads[0].stack = stackbegin;
        cores[CPU].threads[0].stack_size = (int)stackend - (int)stackbegin;
    } else {
#if NUM_CORES > 1  /* This code path will not be run on single core targets */
        cores[COP].threads[0].stack = cop_stackbegin;
        cores[COP].threads[0].stack_size = (int)cop_stackend - (int)cop_stackbegin;
#endif
    }
#if CONFIG_CPU == TCC730
    cores[core].threads[0].context.started = 1;
#else
    cores[core].threads[0].context.start = 0; /* thread 0 already running */
#endif
    cores[core].num_sleepers = 0;
}

int thread_stack_usage(int threadnum)
{
    return thread_stack_usage_on_core(CURRENT_CORE, threadnum);
}

int thread_stack_usage_on_core(unsigned int core, int threadnum)
{
    unsigned int i;
    unsigned int *stackptr = cores[core].threads[threadnum].stack;

    if (threadnum >= cores[core].num_threads)
        return -1;

    for (i = 0;i < cores[core].threads[threadnum].stack_size/sizeof(int);i++)
    {
        if (stackptr[i] != DEADBEEF)
            break;
    }

    return ((cores[core].threads[threadnum].stack_size - i * sizeof(int)) * 100) /
        cores[core].threads[threadnum].stack_size;
}
