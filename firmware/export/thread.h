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

#include "config.h"
#include <inttypes.h>
#include <stdbool.h>

/* Priority scheduling (when enabled with HAVE_PRIORITY_SCHEDULING) works
 * by giving high priority threads more CPU time than less priority threads
 * when they need it.
 * 
 * If software playback codec pcm buffer is going down to critical, codec
 * can change it own priority to REALTIME to override user interface and
 * prevent playback skipping.
 */
#define PRIORITY_REALTIME        1
#define PRIORITY_USER_INTERFACE  4 /* The main thread */
#define PRIORITY_RECORDING       4 /* Recording thread */
#define PRIORITY_PLAYBACK        4 /* or REALTIME when needed */
#define PRIORITY_BUFFERING       4 /* Codec buffering thread */
#define PRIORITY_SYSTEM          6 /* All other firmware threads */
#define PRIORITY_BACKGROUND      8 /* Normal application threads */

#if CONFIG_CODEC == SWCODEC
#define MAXTHREADS	16
#else
#define MAXTHREADS  11
#endif

#define DEFAULT_STACK_SIZE 0x400 /* Bytes */

#ifndef SIMULATOR
/* Need to keep structures inside the header file because debug_menu
 * needs them. */
# ifdef CPU_COLDFIRE
struct regs
{
    unsigned int macsr;  /* EMAC status register */
    unsigned int d[6];   /* d2-d7 */
    unsigned int a[5];   /* a2-a6 */
    void         *sp;    /* Stack pointer (a7) */
    void         *start; /* Thread start address, or NULL when started */
};
# elif CONFIG_CPU == SH7034
struct regs
{
    unsigned int r[7];   /* Registers r8 thru r14 */
    void         *sp;    /* Stack pointer (r15) */
    void         *pr;    /* Procedure register */
    void         *start; /* Thread start address, or NULL when started */
};
# elif defined(CPU_ARM)
struct regs
{
    unsigned int r[8];   /* Registers r4-r11 */
    void         *sp;    /* Stack pointer (r13) */
    unsigned int lr;     /* r14 (lr) */
    void         *start; /* Thread start address, or NULL when started */
};
# endif

#endif /* !SIMULATOR */

#define STATE_RUNNING             0x00000000
#define STATE_BLOCKED             0x20000000
#define STATE_SLEEPING            0x40000000
#define STATE_BLOCKED_W_TMO       0x60000000

#define THREAD_STATE_MASK         0x60000000
#define STATE_ARG_MASK            0x1FFFFFFF

#define GET_STATE_ARG(state)      (state & STATE_ARG_MASK)
#define GET_STATE(state)          (state & THREAD_STATE_MASK)
#define SET_STATE(var,state,arg)  (var = (state | ((arg) & STATE_ARG_MASK)))
#define CLEAR_STATE_ARG(var)      (var &= ~STATE_ARG_MASK)

#define STATE_BOOSTED             0x80000000
#define STATE_IS_BOOSTED(var)     (var & STATE_BOOSTED)
#define SET_BOOST_STATE(var)      (var |= STATE_BOOSTED)

struct thread_entry {
#ifndef SIMULATOR
    struct regs context;
#endif
    const char *name;
    void *stack;
    unsigned long statearg;
    unsigned short stack_size;
# if NUM_CORES > 1
    unsigned char core; /* To which core threads belongs to. */
# endif
#ifdef HAVE_PRIORITY_SCHEDULING
    unsigned char priority;
    unsigned char priority_x;
    long last_run;
#endif
    struct thread_entry *next, *prev;
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    intptr_t retval;
#endif
};

struct core_entry {
    struct thread_entry *running;
    struct thread_entry *sleeping;
    struct thread_entry *waking;
    struct thread_entry **wakeup_list;
#ifdef HAVE_PRIORITY_SCHEDULING
    long highest_priority;
#endif
#if NUM_CORES > 1
    volatile bool lock_issued;
    volatile bool kernel_running;
#endif
    long last_tick;
    int switch_to_irq_level;
    #define STAY_IRQ_LEVEL -1
};

#ifdef HAVE_PRIORITY_SCHEDULING
#define IF_PRIO(empty, type)  , type
#else
#define IF_PRIO(empty, type)
#endif

/* PortalPlayer chips have 2 cores, therefore need atomic mutexes 
 * Just use it for ARM, Coldfire and whatever else well...why not?
 */

/* Macros generate better code than an inline function is this case */
#if defined (CPU_PP) || defined (CPU_ARM)
#define test_and_set(x_, v_) \
({ \
    uint32_t old; \
    asm volatile ( \
        "swpb %[old], %[v], [%[x]] \r\n" \
        : [old]"=r"(old) \
        : [v]"r"((uint32_t)v_), [x]"r"((uint32_t *)x_) \
    ); \
    old; \
    })
#elif defined (CPU_COLDFIRE)
#define test_and_set(x_, v_) \
({ \
    uint8_t old; \
    asm volatile ( \
        "bset.l %[v], (%[x]) \r\n" \
        "sne.b  %[old]       \r\n" \
        : [old]"=d,d"(old) \
        : [v]"i,d"((uint32_t)v_), [x]"a,a"((uint32_t *)x_) \
    ); \
    old; \
    })
#elif CONFIG_CPU == SH7034
#define test_and_set(x_, v_) \
({   \
    uint32_t old; \
    asm volatile ( \
        "tas.b  @%[x]          \r\n" \
        "mov    #-1, %[old]    \r\n" \
        "negc   %[old], %[old] \r\n" \
        : [old]"=r"(old) \
        : [v]"M"((uint32_t)v_), /* Value of v_ must be 1 */ \
          [x]"r"((uint8_t *)x_) \
    ); \
    old; \
    })
#else
/* default for no asm version */
#define test_and_set(x_, v_) \
({ \
    uint32_t old = *(uint32_t *)x_; \
    *(uint32_t *)x_ = v_; \
    old; \
    })
#endif

#if NUM_CORES > 1
inline void lock_cores(void);
inline void unlock_cores(void);
#else
#define lock_cores(...)
#define unlock_cores(...)
#endif

struct thread_entry*
    create_thread(void (*function)(void), void* stack, int stack_size,
                  const char *name IF_PRIO(, int priority)
		  IF_COP(, unsigned int core, bool fallback));

#ifdef HAVE_SCHEDULER_BOOSTCTRL
void trigger_cpu_boost(void);
#else
#define trigger_cpu_boost()
#endif

void remove_thread(struct thread_entry *thread);
void switch_thread(bool save_context, struct thread_entry **blocked_list);
void sleep_thread(int ticks);
void block_thread(struct thread_entry **thread);
void block_thread_w_tmo(struct thread_entry **thread, int timeout);
void set_irq_level_and_block_thread(struct thread_entry **thread, int level);
void set_irq_level_and_block_thread_w_tmo(struct thread_entry **list,
                                          int timeout, int level);
void wakeup_thread(struct thread_entry **thread);
void wakeup_thread_irq_safe(struct thread_entry **thread);
#ifdef HAVE_PRIORITY_SCHEDULING
int thread_set_priority(struct thread_entry *thread, int priority);
int  thread_get_priority(struct thread_entry *thread);
/* Yield that guarantees thread execution once per round regardless of
   thread's scheduler priority - basically a transient realtime boost
   without altering the scheduler's thread precedence. */
void priority_yield(void);
#else
#define priority_yield  yield
#endif /* HAVE_PRIORITY_SCHEDULING */
struct thread_entry * thread_get_current(void);
void init_threads(void);
int thread_stack_usage(const struct thread_entry *thread);
int thread_get_status(const struct thread_entry *thread);
#ifdef RB_PROFILE
void profile_thread(void);
#endif

#endif
