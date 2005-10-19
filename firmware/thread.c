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
#elif CONFIG_CPU == TCC730
struct regs
{
    void *sp;    /* Stack pointer (a15) */
    void *start; /* Thread start address */
    int started; /* 0 when not started */
};
#endif

#define DEADBEEF ((unsigned int)0xdeadbeef)
/* Cast to the the machine int type, whose size could be < 4. */


int num_threads;
static volatile int num_sleepers;
static int current_thread;
static struct regs thread_contexts[MAXTHREADS] IBSS_ATTR;
const char *thread_name[MAXTHREADS];
void *thread_stack[MAXTHREADS];
int thread_stack_size[MAXTHREADS];
static const char main_thread_name[] = "main";

extern int stackbegin[];
extern int stackend[];

void switch_thread(void) ICODE_ATTR;
static inline void store_context(void* addr) __attribute__ ((always_inline));
static inline void load_context(const void* addr) __attribute__ ((always_inline));

#ifdef CPU_COLDFIRE
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
    int current;
    unsigned int *stackptr;

#ifdef SIMULATOR
    /* Do nothing */
#else

    while (num_sleepers == num_threads)
    {
        /* Enter sleep mode, woken up on interrupt */
#ifdef CPU_COLDFIRE
        asm volatile ("stop #0x2000");
#elif CONFIG_CPU == SH7034
        and_b(0x7F, &SBYCR);
        asm volatile ("sleep");
#elif CONFIG_CPU == TCC730
	    /* Sleep mode is triggered by the SYS instr on CalmRisc16.
         * Unfortunately, the manual doesn't specify which arg to use.
         __asm__ volatile ("sys #0x0f");
         0x1f seems to trigger a reset;
         0x0f is the only one other argument used by Archos.
         */
#endif
    }
    
#endif
    current = current_thread;
    store_context(&thread_contexts[current]);
    
#if CONFIG_CPU != TCC730
    /* Check if the current thread stack is overflown */
    stackptr = thread_stack[current];
    if(stackptr[0] != DEADBEEF)
       panicf("Stkov %s", thread_name[current]);
#endif

    if (++current >= num_threads)
        current = 0;

    current_thread = current;
    load_context(&thread_contexts[current]);
}

void sleep_thread(void)
{
    ++num_sleepers;
    switch_thread();
}

void wake_up_thread(void)
{
    num_sleepers = 0;
}

/*--------------------------------------------------------------------------- 
 * Create thread.
 * Return ID if context area could be allocated, else -1.
 *---------------------------------------------------------------------------
 */
int create_thread(void (*function)(void), void* stack, int stack_size,
                  const char *name)
{
    unsigned int i;
    unsigned int stacklen;
    unsigned int *stackptr;
    struct regs *regs;

    if (num_threads >= MAXTHREADS)
        return -1;

    /* Munge the stack to make it easy to spot stack overflows */
    stacklen = stack_size / sizeof(int);
    stackptr = stack;
    for(i = 0;i < stacklen;i++)
    {
        stackptr[i] = DEADBEEF;
    }

    /* Store interesting information */
    thread_name[num_threads] = name;
    thread_stack[num_threads] = stack;
    thread_stack_size[num_threads] = stack_size;
    regs = &thread_contexts[num_threads];
#if defined(CPU_COLDFIRE) || (CONFIG_CPU == SH7034)
    /* Align stack to an even 32 bit boundary */
    regs->sp = (void*)(((unsigned int)stack + stack_size) & ~3);
#elif CONFIG_CPU == TCC730
   /* Align stack on word boundary */
    regs->sp = (void*)(((unsigned long)stack + stack_size - 2) & ~1);
    regs->started = 0;
#endif
    regs->start = (void*)function;

    wake_up_thread();
    return num_threads++; /* return the current ID, e.g for remove_thread() */
}

/*--------------------------------------------------------------------------- 
 * Remove a thread from the scheduler.
 * Parameter is the ID as returned from create_thread().
 *---------------------------------------------------------------------------
 */
void remove_thread(int threadnum)
{
    int i;

    if(threadnum >= num_threads)
       return;

    num_threads--;
    for (i=threadnum; i<num_threads-1; i++)
    {   /* move all entries which are behind */
        thread_name[i]       = thread_name[i+1];
        thread_stack[i]      = thread_stack[i+1];
        thread_stack_size[i] = thread_stack_size[i+1];
        thread_contexts[i]   = thread_contexts[i+1];
    }

    if (current_thread == threadnum) /* deleting the current one? */
        current_thread = num_threads; /* set beyond last, avoid store harm */
    else if (current_thread > threadnum) /* within the moved positions? */
        current_thread--; /* adjust it, point to same context again */
}

void init_threads(void)
{
    num_threads = 1; /* We have 1 thread to begin with */
    current_thread = 0; /* The current thread is number 0 */
    thread_name[0] = main_thread_name;
    thread_stack[0] = stackbegin;
    thread_stack_size[0] = (int)stackend - (int)stackbegin;
#if defined(CPU_COLDFIRE) || (CONFIG_CPU == SH7034)
    thread_contexts[0].start = 0; /* thread 0 already running */
#elif CONFIG_CPU == TCC730
    thread_contexts[0].started = 1;
#endif
    num_sleepers = 0;
}

int thread_stack_usage(int threadnum)
{
    unsigned int i;
    unsigned int *stackptr = thread_stack[threadnum];

    if(threadnum >= num_threads)
        return -1;

    for(i = 0;i < thread_stack_size[threadnum]/sizeof(int);i++)
    {
        if(stackptr[i] != DEADBEEF)
            break;
    }

    return ((thread_stack_size[threadnum] - i * sizeof(int)) * 100) /
        thread_stack_size[threadnum];
}       
