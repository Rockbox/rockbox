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
#include "sprintf.h"
#include "system.h"
#include "kernel.h"
#include "cpu.h"
#include "string.h"
#ifdef RB_PROFILE
#include <profile.h>
#endif

/* Define THREAD_EXTRA_CHECKS as 1 to enable additional state checks */
#ifdef DEBUG
#define THREAD_EXTRA_CHECKS 1 /* Always 1 for DEBUG */
#else
#define THREAD_EXTRA_CHECKS 0
#endif

/**
 * General locking order to guarantee progress. Order must be observed but
 * all stages are not nescessarily obligatory. Going from 1) to 3) is
 * perfectly legal.
 *
 * 1) IRQ
 * This is first because of the likelyhood of having an interrupt occur that
 * also accesses one of the objects farther down the list. Any non-blocking
 * synchronization done may already have a lock on something during normal
 * execution and if an interrupt handler running on the same processor as
 * the one that has the resource locked were to attempt to access the
 * resource, the interrupt handler would wait forever waiting for an unlock
 * that will never happen. There is no danger if the interrupt occurs on
 * a different processor because the one that has the lock will eventually
 * unlock and the other processor's handler may proceed at that time. Not
 * nescessary when the resource in question is definitely not available to
 * interrupt handlers.
 *  
 * 2) Kernel Object
 * 1) May be needed beforehand if the kernel object allows dual-use such as
 * event queues. The kernel object must have a scheme to protect itself from
 * access by another processor and is responsible for serializing the calls
 * to block_thread(_w_tmo) and wakeup_thread both to themselves and to each
 * other. If a thread blocks on an object it must fill-in the blk_ops members
 * for its core to unlock _after_ the thread's context has been saved and the
 * unlocking will be done in reverse from this heirarchy.
 * 
 * 3) Thread Slot
 * This locks access to the thread's slot such that its state cannot be
 * altered by another processor when a state change is in progress such as
 * when it is in the process of going on a blocked list. An attempt to wake
 * a thread while it is still blocking will likely desync its state with
 * the other resources used for that state.
 *
 * 4) Lists
 * Usually referring to a list (aka. queue) that a thread will be blocking
 * on that belongs to some object and is shareable amongst multiple
 * processors. Parts of the scheduler may have access to them without actually
 * locking the kernel object such as when a thread is blocked with a timeout
 * (such as calling queue_wait_w_tmo). Of course the kernel object also gets
 * it lists locked when the thread blocks so that all object list access is
 * synchronized. Failure to do so would corrupt the list links.
 *
 * 5) Core Lists
 * These lists are specific to a particular processor core and are accessible
 * by all processor cores and interrupt handlers. They are used when an
 * operation may only be performed by the thread's own core in a normal
 * execution context. The wakeup list is the prime example where a thread
 * may be added by any means and the thread's own core will remove it from
 * the wakeup list and put it on the running list (which is only ever
 * accessible by its own processor).
 */
#define DEADBEEF ((unsigned int)0xdeadbeef)
/* Cast to the the machine int type, whose size could be < 4. */
struct core_entry cores[NUM_CORES] IBSS_ATTR;
struct thread_entry threads[MAXTHREADS] IBSS_ATTR;

static const char main_thread_name[] = "main";
extern int stackbegin[];
extern int stackend[];

/* core_sleep procedure to implement for any CPU to ensure an asychronous wakup
 * never results in requiring a wait until the next tick (up to 10000uS!). May
 * require assembly and careful instruction ordering.
 *
 * 1) On multicore, stay awake if directed to do so by another. If so, goto step 4.
 * 2) If processor requires, atomically reenable interrupts and perform step 3.
 * 3) Sleep the CPU core. If wakeup itself enables interrupts (stop #0x2000 on Coldfire)
 *    goto step 5.
 * 4) Enable interrupts.
 * 5) Exit procedure.
 */
static inline void core_sleep(IF_COP_VOID(unsigned int core))
        __attribute__((always_inline));

static void check_tmo_threads(void)
        __attribute__((noinline));

static inline void block_thread_on_l(
    struct thread_queue *list, struct thread_entry *thread, unsigned state)
        __attribute__((always_inline));

static inline void block_thread_on_l_no_listlock(
    struct thread_entry **list, struct thread_entry *thread, unsigned state)
        __attribute__((always_inline));

static inline void _block_thread_on_l(
    struct thread_queue *list, struct thread_entry *thread,
    unsigned state IF_SWCL(, const bool single))
        __attribute__((always_inline));

IF_SWCL(static inline) struct thread_entry * _wakeup_thread(
    struct thread_queue *list IF_SWCL(, const bool nolock))
        __attribute__((IFN_SWCL(noinline) IF_SWCL(always_inline)));

IF_SWCL(static inline) void _block_thread(
    struct thread_queue *list IF_SWCL(, const bool nolock))
        __attribute__((IFN_SWCL(noinline) IF_SWCL(always_inline)));

static void add_to_list_tmo(struct thread_entry *thread)
        __attribute__((noinline));

static void core_schedule_wakeup(struct thread_entry *thread)
        __attribute__((noinline));

static inline void core_perform_wakeup(IF_COP_VOID(unsigned int core))
        __attribute__((always_inline));

#if NUM_CORES > 1
static inline void run_blocking_ops(
    unsigned int core, struct thread_entry *thread)
        __attribute__((always_inline));
#endif

static void thread_stkov(struct thread_entry *thread)
        __attribute__((noinline));

static inline void store_context(void* addr)
        __attribute__((always_inline));

static inline void load_context(const void* addr)
        __attribute__((always_inline));

void switch_thread(struct thread_entry *old)
        __attribute__((noinline));


/****************************************************************************
 * Processor-specific section
 */

#if defined(CPU_ARM)
/*---------------------------------------------------------------------------
 * Start the thread running and terminate it if it returns
 *---------------------------------------------------------------------------
 */
static void start_thread(void) __attribute__((naked,used));
static void start_thread(void)
{
    /* r0 = context */
    asm volatile (
        "ldr    sp, [r0, #32]          \n" /* Load initial sp */
        "ldr    r4, [r0, #40]          \n" /* start in r4 since it's non-volatile */
        "mov    r1, #0                 \n" /* Mark thread as running */
        "str    r1, [r0, #40]          \n"
#if NUM_CORES > 1
        "ldr    r0, =invalidate_icache \n" /* Invalidate this core's cache. */
        "mov    lr, pc                 \n" /* This could be the first entry into */
        "bx     r0                     \n" /* plugin or codec code for this core. */
#endif
        "mov    lr, pc                 \n" /* Call thread function */
        "bx     r4                     \n"
        "mov    r0, #0                 \n" /* remove_thread(NULL) */
        "ldr    pc, =remove_thread     \n"
        ".ltorg                        \n" /* Dump constant pool */
    ); /* No clobber list - new thread doesn't care */
}

/* For startup, place context pointer in r4 slot, start_thread pointer in r5
 * slot, and thread function pointer in context.start. See load_context for
 * what happens when thread is initially going to run. */
#define THREAD_STARTUP_INIT(core, thread, function) \
    ({ (thread)->context.r[0] = (unsigned int)&(thread)->context,  \
       (thread)->context.r[1] = (unsigned int)start_thread, \
       (thread)->context.start = (void *)function; })

/*---------------------------------------------------------------------------
 * Store non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void store_context(void* addr)
{
    asm volatile(
        "stmia  %0, { r4-r11, sp, lr } \n"
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
        "ldr     r0, [%0, #40]          \n" /* Load start pointer */
        "cmp     r0, #0                 \n" /* Check for NULL */
        "ldmneia %0, { r0, pc }         \n" /* If not already running, jump to start */ 
        "ldmia   %0, { r4-r11, sp, lr } \n" /* Load regs r4 to r14 from context */
        : : "r" (addr) : "r0" /* only! */
    );
}

#if defined (CPU_PP)

#if NUM_CORES > 1
extern int cpu_idlestackbegin[];
extern int cpu_idlestackend[];
extern int cop_idlestackbegin[];
extern int cop_idlestackend[];
static int * const idle_stacks[NUM_CORES] NOCACHEDATA_ATTR =
{
    [CPU] = cpu_idlestackbegin,
    [COP] = cop_idlestackbegin
};

#if CONFIG_CPU == PP5002
/* Bytes to emulate the PP502x mailbox bits */
struct core_semaphores
{
    volatile uint8_t intend_wake;  /* 00h */
    volatile uint8_t stay_awake;   /* 01h */
    volatile uint8_t intend_sleep; /* 02h */
    volatile uint8_t unused;       /* 03h */
};

static struct core_semaphores core_semaphores[NUM_CORES] NOCACHEBSS_ATTR;
#endif

#endif /* NUM_CORES */

#if CONFIG_CORELOCK == SW_CORELOCK
/* Software core locks using Peterson's mutual exclusion algorithm */

/*---------------------------------------------------------------------------
 * Initialize the corelock structure.
 *---------------------------------------------------------------------------
 */
void corelock_init(struct corelock *cl)
{
    memset(cl, 0, sizeof (*cl));
}

#if 1 /* Assembly locks to minimize overhead */
/*---------------------------------------------------------------------------
 * Wait for the corelock to become free and acquire it when it does.
 *---------------------------------------------------------------------------
 */
void corelock_lock(struct corelock *cl) __attribute__((naked));
void corelock_lock(struct corelock *cl)
{
    asm volatile (
        "mov    r1, %0               \n" /* r1 = PROCESSOR_ID */
        "ldrb   r1, [r1]             \n"
        "strb   r1, [r0, r1, lsr #7] \n" /* cl->myl[core] = core */
        "and    r2, r1, #1           \n" /* r2 = othercore */
        "strb   r2, [r0, #2]         \n" /* cl->turn = othercore */
    "1:                              \n"
        "ldrb   r3, [r0, r2]         \n" /* cl->myl[othercore] == 0 ? */
        "cmp    r3, #0               \n"
        "ldrneb r3, [r0, #2]         \n" /* || cl->turn == core ? */
        "cmpne  r3, r1, lsr #7       \n"
        "bxeq   lr                   \n" /* yes? lock acquired */
        "b      1b                   \n" /* keep trying */
        : : "i"(&PROCESSOR_ID)
    );
    (void)cl;
}

/*---------------------------------------------------------------------------
 * Try to aquire the corelock. If free, caller gets it, otherwise return 0.
 *---------------------------------------------------------------------------
 */
int corelock_try_lock(struct corelock *cl) __attribute__((naked));
int corelock_try_lock(struct corelock *cl)
{
    asm volatile (
        "mov    r1, %0               \n" /* r1 = PROCESSOR_ID */
        "ldrb   r1, [r1]             \n"
        "strb   r1, [r0, r1, lsr #7] \n" /* cl->myl[core] = core */
        "and    r2, r1, #1           \n" /* r2 = othercore */
        "strb   r2, [r0, #2]         \n" /* cl->turn = othercore */
    "1:                              \n"
        "ldrb   r3, [r0, r2]         \n" /* cl->myl[othercore] == 0 ? */
        "cmp    r3, #0               \n"
        "ldrneb r3, [r0, #2]         \n" /* || cl->turn == core? */
        "cmpne  r3, r1, lsr #7       \n"
        "moveq  r0, #1               \n" /* yes? lock acquired */
        "bxeq   lr                   \n"
        "mov    r2, #0               \n" /* cl->myl[core] = 0 */
        "strb   r2, [r0, r1, lsr #7] \n"
        "mov    r0, r2               \n"
        "bx     lr                   \n" /* acquisition failed */
        : : "i"(&PROCESSOR_ID)
    );

    return 0;
    (void)cl;
}

/*---------------------------------------------------------------------------
 * Release ownership of the corelock
 *---------------------------------------------------------------------------
 */
void corelock_unlock(struct corelock *cl) __attribute__((naked));
void corelock_unlock(struct corelock *cl)
{
    asm volatile (
        "mov    r1, %0               \n" /* r1 = PROCESSOR_ID */
        "ldrb   r1, [r1]             \n"
        "mov    r2, #0               \n" /* cl->myl[core] = 0 */
        "strb   r2, [r0, r1, lsr #7] \n"
        "bx     lr                   \n"
        : : "i"(&PROCESSOR_ID)
    );
    (void)cl;
}
#else /* C versions for reference */
/*---------------------------------------------------------------------------
 * Wait for the corelock to become free and aquire it when it does.
 *---------------------------------------------------------------------------
 */
void corelock_lock(struct corelock *cl)
{
    const unsigned int core = CURRENT_CORE;
    const unsigned int othercore = 1 - core;

    cl->myl[core] = core;
    cl->turn = othercore;

    for (;;)
    {
        if (cl->myl[othercore] == 0 || cl->turn == core)
            break;
    }
}

/*---------------------------------------------------------------------------
 * Try to aquire the corelock. If free, caller gets it, otherwise return 0.
 *---------------------------------------------------------------------------
 */
int corelock_try_lock(struct corelock *cl)
{
    const unsigned int core = CURRENT_CORE;
    const unsigned int othercore = 1 - core;

    cl->myl[core] = core;
    cl->turn = othercore;

    if (cl->myl[othercore] == 0 || cl->turn == core)
    {
        return 1;
    }

    cl->myl[core] = 0;
    return 0;
}

/*---------------------------------------------------------------------------
 * Release ownership of the corelock
 *---------------------------------------------------------------------------
 */
void corelock_unlock(struct corelock *cl)
{
    cl->myl[CURRENT_CORE] = 0;
}
#endif /* ASM / C selection */

#endif /* CONFIG_CORELOCK == SW_CORELOCK */

/*---------------------------------------------------------------------------
 * Put core in a power-saving state if waking list wasn't repopulated and if
 * no other core requested a wakeup for it to perform a task.
 *---------------------------------------------------------------------------
 */
#if NUM_CORES == 1
/* Shared single-core build debugging version */
static inline void core_sleep(void)
{
    PROC_CTL(CURRENT_CORE) = PROC_SLEEP;
    nop; nop; nop;
    set_interrupt_status(IRQ_FIQ_ENABLED, IRQ_FIQ_STATUS);
}
#elif defined (CPU_PP502x)
static inline void core_sleep(unsigned int core)
{
#if 1
    asm volatile (
        "mov    r0, #4                     \n" /* r0 = 0x4 << core */
        "mov    r0, r0, lsl %[c]           \n"
        "str    r0, [%[mbx], #4]           \n" /* signal intent to sleep */
        "ldr    r1, [%[mbx], #0]           \n" /* && !(MBX_MSG_STAT & (0x10<<core)) ? */
        "tst    r1, r0, lsl #2             \n"  
        "moveq  r1, #0x80000000            \n" /* Then sleep */
        "streq  r1, [%[ctl], %[c], lsl #2] \n"
        "moveq  r1, #0                     \n" /* Clear control reg */
        "streq  r1, [%[ctl], %[c], lsl #2] \n"
        "orr    r1, r0, r0, lsl #2         \n" /* Signal intent to wake - clear wake flag */
        "str    r1, [%[mbx], #8]           \n"
    "1:                                    \n" /* Wait for wake procedure to finish */
        "ldr    r1, [%[mbx], #0]           \n"
        "tst    r1, r0, lsr #2             \n"
        "bne    1b                         \n"
        "mrs    r1, cpsr                   \n" /* Enable interrupts */
        "bic    r1, r1, #0xc0              \n"
        "msr    cpsr_c, r1                 \n"
        :
        :  [ctl]"r"(&PROC_CTL(CPU)), [mbx]"r"(MBX_BASE), [c]"r"(core)
        : "r0", "r1");
#else /* C version for reference */
    /* Signal intent to sleep */
    MBX_MSG_SET = 0x4 << core;

    /* Something waking or other processor intends to wake us? */
    if ((MBX_MSG_STAT & (0x10 << core)) == 0)
    {
        PROC_CTL(core) = PROC_SLEEP; nop; /* Snooze */
        PROC_CTL(core) = 0;               /* Clear control reg */
    }

    /* Signal wake - clear wake flag */
    MBX_MSG_CLR = 0x14 << core;

    /* Wait for other processor to finish wake procedure */
    while (MBX_MSG_STAT & (0x1 << core));

    /* Enable IRQ, FIQ */
    set_interrupt_status(IRQ_FIQ_ENABLED, IRQ_FIQ_STATUS);
#endif /* ASM/C selection */
}
#elif CONFIG_CPU == PP5002
/* PP5002 has no mailboxes - emulate using bytes */
static inline void core_sleep(unsigned int core)
{
#if 1
    asm volatile (
        "mov    r0, #1                     \n" /* Signal intent to sleep */
        "strb   r0, [%[sem], #2]           \n"
        "ldrb   r0, [%[sem], #1]           \n" /* && stay_awake == 0? */
        "cmp    r0, #0                     \n"
        "moveq  r0, #0xca                  \n" /* Then sleep */
        "streqb r0, [%[ctl], %[c], lsl #2] \n"
        "nop                               \n" /* nop's needed because of pipeline */
        "nop                               \n"
        "nop                               \n"
        "mov    r0, #0                     \n" /* Clear stay_awake and sleep intent */
        "strb   r0, [%[sem], #1]           \n"
        "strb   r0, [%[sem], #2]           \n"
    "1:                                    \n" /* Wait for wake procedure to finish */
        "ldrb   r0, [%[sem], #0]           \n"
        "cmp    r0, #0                     \n"
        "bne    1b                         \n"
        "mrs    r0, cpsr                   \n" /* Enable interrupts */
        "bic    r0, r0, #0xc0              \n"
        "msr    cpsr_c, r0                 \n"
        :
        : [sem]"r"(&core_semaphores[core]), [c]"r"(core),
          [ctl]"r"(&PROC_CTL(CPU))
        : "r0"
        );
#else /* C version for reference */
    /* Signal intent to sleep */
    core_semaphores[core].intend_sleep = 1;

    /* Something waking or other processor intends to wake us? */
    if (core_semaphores[core].stay_awake == 0)
    {
        PROC_CTL(core) = PROC_SLEEP; /* Snooze */
        nop; nop; nop;
    }

    /* Signal wake - clear wake flag */
    core_semaphores[core].stay_awake = 0;
    core_semaphores[core].intend_sleep = 0;

    /* Wait for other processor to finish wake procedure */
    while (core_semaphores[core].intend_wake != 0);

    /* Enable IRQ, FIQ */
    set_interrupt_status(IRQ_FIQ_ENABLED, IRQ_FIQ_STATUS);
#endif /* ASM/C selection */
}
#endif /* CPU type */

/*---------------------------------------------------------------------------
 * Wake another processor core that is sleeping or prevent it from doing so
 * if it was already destined. FIQ, IRQ should be disabled before calling.
 *---------------------------------------------------------------------------
 */
#if NUM_CORES == 1
/* Shared single-core build debugging version */
void core_wake(void)
{
    /* No wakey - core already wakey */
}
#elif defined (CPU_PP502x)
void core_wake(unsigned int othercore)
{
#if 1
    /* avoid r0 since that contains othercore */
    asm volatile (
        "mrs    r3, cpsr                    \n" /* Disable IRQ */
        "orr    r1, r3, #0x80               \n"
        "msr    cpsr_c, r1                  \n"
        "mov    r2, #0x11                   \n" /* r2 = (0x11 << othercore) */
        "mov    r2, r2, lsl %[oc]           \n" /* Signal intent to wake othercore */
        "str    r2, [%[mbx], #4]            \n"
    "1:                                     \n" /* If it intends to sleep, let it first */
        "ldr    r1, [%[mbx], #0]            \n" /* (MSG_MSG_STAT & (0x4 << othercore)) != 0 ? */
        "eor    r1, r1, #0xc                \n"
        "tst    r1, r2, lsr #2              \n"
        "ldr    r1, [%[ctl], %[oc], lsl #2] \n" /* && (PROC_CTL(othercore) & PROC_SLEEP) == 0 ? */
        "tsteq  r1, #0x80000000             \n"
        "beq    1b                          \n" /* Wait for sleep or wake */
        "tst    r1, #0x80000000             \n" /* If sleeping, wake it */
        "movne  r1, #0x0                    \n"
        "strne  r1, [%[ctl], %[oc], lsl #2] \n"
        "mov    r1, r2, lsr #4              \n"
        "str    r1, [%[mbx], #8]            \n" /* Done with wake procedure */
        "msr    cpsr_c, r3                  \n" /* Restore int status */
        :
        : [ctl]"r"(&PROC_CTL(CPU)), [mbx]"r"(MBX_BASE),
          [oc]"r"(othercore)
        : "r1", "r2", "r3");
#else /* C version for reference */
    /* Disable interrupts - avoid reentrancy from the tick */
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);

    /* Signal intent to wake other processor - set stay awake */
    MBX_MSG_SET = 0x11 << othercore;

    /* If it intends to sleep, wait until it does or aborts */
    while ((MBX_MSG_STAT & (0x4 << othercore)) != 0 &&
           (PROC_CTL(othercore) & PROC_SLEEP) == 0);

    /* If sleeping, wake it up */
    if (PROC_CTL(othercore) & PROC_SLEEP)
        PROC_CTL(othercore) = 0;

    /* Done with wake procedure */
    MBX_MSG_CLR = 0x1 << othercore;
    set_irq_level(oldlevel);
#endif /* ASM/C selection */
}
#elif CONFIG_CPU == PP5002
/* PP5002 has no mailboxes - emulate using bytes */
void core_wake(unsigned int othercore)
{
#if 1
    /* avoid r0 since that contains othercore */
    asm volatile (
        "mrs    r3, cpsr                \n" /* Disable IRQ */
        "orr    r1, r3, #0x80           \n"
        "msr    cpsr_c, r1              \n"
        "mov    r1, #1                  \n" /* Signal intent to wake other core */
        "orr    r1, r1, r1, lsl #8      \n" /* and set stay_awake */
        "strh   r1, [%[sem], #0]        \n"
        "mov    r2, #0x8000             \n"
    "1:                                 \n" /* If it intends to sleep, let it first */
        "ldrb   r1, [%[sem], #2]        \n" /* intend_sleep != 0 ? */
        "cmp    r1, #1                  \n"
        "ldr    r1, [%[st]]             \n" /* && not sleeping ? */
        "tsteq  r1, r2, lsr %[oc]       \n"
        "beq    1b                      \n" /* Wait for sleep or wake */
        "tst    r1, r2, lsr %[oc]       \n"
        "ldrne  r2, =0xcf004054         \n" /* If sleeping, wake it */
        "movne  r1, #0xce               \n"
        "strneb r1, [r2, %[oc], lsl #2] \n"
        "mov    r1, #0                  \n" /* Done with wake procedure */
        "strb   r1, [%[sem], #0]        \n"
        "msr    cpsr_c, r3              \n" /* Restore int status */
        :
        : [sem]"r"(&core_semaphores[othercore]),
          [st]"r"(&PROC_STAT),
          [oc]"r"(othercore)
        : "r1", "r2", "r3"
    );
#else /* C version for reference */
    /* Disable interrupts - avoid reentrancy from the tick */
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);

    /* Signal intent to wake other processor - set stay awake */
    core_semaphores[othercore].intend_wake = 1;
    core_semaphores[othercore].stay_awake = 1;

    /* If it intends to sleep, wait until it does or aborts */
    while (core_semaphores[othercore].intend_sleep != 0 &&
           (PROC_STAT & PROC_SLEEPING(othercore)) == 0);

    /* If sleeping, wake it up */
    if (PROC_STAT & PROC_SLEEPING(othercore))
        PROC_CTL(othercore) = PROC_WAKE;

    /* Done with wake procedure */
    core_semaphores[othercore].intend_wake = 0;
    set_irq_level(oldlevel);
#endif  /* ASM/C selection */
}
#endif /* CPU type */

#if NUM_CORES > 1
/*---------------------------------------------------------------------------
 * Switches to a stack that always resides in the Rockbox core.
 *
 * Needed when a thread suicides on a core other than the main CPU since the
 * stack used when idling is the stack of the last thread to run. This stack
 * may not reside in the core in which case the core will continue to use a
 * stack from an unloaded module until another thread runs on it.
 *---------------------------------------------------------------------------
 */
static inline void switch_to_idle_stack(const unsigned int core)
{
    asm volatile (
        "str  sp, [%0] \n" /* save original stack pointer on idle stack */
        "mov  sp, %0   \n" /* switch stacks */
        : : "r"(&idle_stacks[core][IDLE_STACK_WORDS-1]));
    (void)core;
}

/*---------------------------------------------------------------------------
 * Perform core switch steps that need to take place inside switch_thread.
 *
 * These steps must take place while before changing the processor and after
 * having entered switch_thread since switch_thread may not do a normal return
 * because the stack being used for anything the compiler saved will not belong
 * to the thread's destination core and it may have been recycled for other
 * purposes by the time a normal context load has taken place. switch_thread
 * will also clobber anything stashed in the thread's context or stored in the
 * nonvolatile registers if it is saved there before the call since the
 * compiler's order of operations cannot be known for certain.
 */
static void core_switch_blk_op(unsigned int core, struct thread_entry *thread)
{
    /* Flush our data to ram */
    flush_icache();
    /* Stash thread in r4 slot */
    thread->context.r[0] = (unsigned int)thread;
    /* Stash restart address in r5 slot */
    thread->context.r[1] = (unsigned int)thread->context.start;
    /* Save sp in context.sp while still running on old core */
    thread->context.sp = (void*)idle_stacks[core][IDLE_STACK_WORDS-1];
}

/*---------------------------------------------------------------------------
 * Machine-specific helper function for switching the processor a thread is
 * running on. Basically, the thread suicides on the departing core and is
 * reborn on the destination. Were it not for gcc's ill-behavior regarding
 * naked functions written in C where it actually clobbers non-volatile
 * registers before the intended prologue code, this would all be much
 * simpler.  Generic setup is done in switch_core itself.
 */

/*---------------------------------------------------------------------------
 * This actually performs the core switch.
 */
static void switch_thread_core(unsigned int core, struct thread_entry *thread)
        __attribute__((naked));
static void switch_thread_core(unsigned int core, struct thread_entry *thread)
{
    /* Pure asm for this because compiler behavior isn't sufficiently predictable.
     * Stack access also isn't permitted until restoring the original stack and
     * context. */
    asm volatile (
        "stmfd  sp!, { r4-r12, lr }    \n" /* Stack all non-volatile context on current core */
        "ldr    r2, =idle_stacks       \n" /* r2 = &idle_stacks[core][IDLE_STACK_WORDS] */
        "ldr    r2, [r2, r0, lsl #2]   \n"
        "add    r2, r2, %0*4           \n"
        "stmfd  r2!, { sp }            \n" /* save original stack pointer on idle stack */
        "mov    sp, r2                 \n" /* switch stacks */
        "adr    r2, 1f                 \n" /* r2 = new core restart address */
        "str    r2, [r1, #40]          \n" /* thread->context.start = r2 */
        "mov    r0, r1                 \n" /* switch_thread(thread) */
        "ldr    pc, =switch_thread     \n" /* r0 = thread after call - see load_context */
    "1:                                \n"
        "ldr    sp, [r0, #32]          \n" /* Reload original sp from context structure */
        "mov    r1, #0                 \n" /* Clear start address */
        "str    r1, [r0, #40]          \n"
        "ldr    r0, =invalidate_icache \n" /* Invalidate new core's cache */
        "mov    lr, pc                 \n"
        "bx     r0                     \n"
        "ldmfd  sp!, { r4-r12, pc }    \n" /* Restore non-volatile context to new core and return */
        ".ltorg                        \n" /* Dump constant pool */
        : : "i"(IDLE_STACK_WORDS)
    );
    (void)core; (void)thread;
}
#endif /* NUM_CORES */

#elif CONFIG_CPU == S3C2440

/*---------------------------------------------------------------------------
 * Put core in a power-saving state if waking list wasn't repopulated.
 *---------------------------------------------------------------------------
 */
static inline void core_sleep(void)
{
    /* FIQ also changes the CLKCON register so FIQ must be disabled
       when changing it here */
    asm volatile (
        "mrs    r0, cpsr        \n" /* Prepare IRQ, FIQ enable */
        "bic    r0, r0, #0xc0   \n"
        "mov    r1, #0x4c000000 \n" /* CLKCON = 0x4c00000c */
        "ldr    r2, [r1, #0xc]  \n" /* Set IDLE bit */
        "orr    r2, r2, #4      \n"
        "str    r2, [r1, #0xc]  \n"
        "msr    cpsr_c, r0      \n" /* Enable IRQ, FIQ */
        "mov    r2, #0          \n" /* wait for IDLE */
    "1:                         \n"
        "add    r2, r2, #1      \n"
        "cmp    r2, #10         \n"
        "bne    1b              \n"
        "orr    r2, r0, #0xc0   \n" /* Disable IRQ, FIQ */
        "msr    cpsr_c, r2      \n"
        "ldr    r2, [r1, #0xc]  \n" /* Reset IDLE bit */
        "bic    r2, r2, #4      \n"
        "str    r2, [r1, #0xc]  \n"
        "msr    cpsr_c, r0      \n" /* Enable IRQ, FIQ */
        :  :  : "r0", "r1", "r2");
}
#elif defined(CPU_TCC77X)
static inline void core_sleep(void)
{
    #warning TODO: Implement core_sleep
}
#elif CONFIG_CPU == IMX31L
static inline void core_sleep(void)
{
    asm volatile (
        "mov r0, #0                \n"
        "mcr p15, 0, r0, c7, c0, 4 \n" /* Wait for interrupt */
        "mrs r0, cpsr              \n" /* Unmask IRQ/FIQ at core level */
        "bic r0, r0, #0xc0         \n"
        "msr cpsr_c, r0            \n"
        : : : "r0"
    );
}
#else
static inline void core_sleep(void)
{
    #warning core_sleep not implemented, battery life will be decreased
}
#endif /* CONFIG_CPU == */

#elif defined(CPU_COLDFIRE)
/*---------------------------------------------------------------------------
 * Start the thread running and terminate it if it returns
 *---------------------------------------------------------------------------
 */
void start_thread(void); /* Provide C access to ASM label */
static void __start_thread(void) __attribute__((used));
static void __start_thread(void)
{
    /* a0=macsr, a1=context */
    asm volatile (
    "start_thread:             \n" /* Start here - no naked attribute */
        "move.l  %a0, %macsr   \n" /* Set initial mac status reg */
        "lea.l   48(%a1), %a1  \n"
        "move.l  (%a1)+, %sp   \n" /* Set initial stack */
        "move.l  (%a1), %a2    \n" /* Fetch thread function pointer */
        "clr.l   (%a1)         \n" /* Mark thread running */
        "jsr     (%a2)         \n" /* Call thread function */
        "clr.l   -(%sp)        \n" /* remove_thread(NULL) */
        "jsr     remove_thread \n"
    );
}

/* Set EMAC unit to fractional mode with saturation for each new thread,
 * since that's what'll be the most useful for most things which the dsp
 * will do. Codecs should still initialize their preferred modes
 * explicitly. Context pointer is placed in d2 slot and start_thread
 * pointer in d3 slot. thread function pointer is placed in context.start.
 * See load_context for what happens when thread is initially going to
 * run.
 */
#define THREAD_STARTUP_INIT(core, thread, function) \
    ({ (thread)->context.macsr = EMAC_FRACTIONAL | EMAC_SATURATE, \
       (thread)->context.d[0] = (unsigned int)&(thread)->context, \
       (thread)->context.d[1] = (unsigned int)start_thread,       \
       (thread)->context.start = (void *)(function); })

/*---------------------------------------------------------------------------
 * Store non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void store_context(void* addr)
{
    asm volatile (
        "move.l  %%macsr,%%d0                  \n"
        "movem.l %%d0/%%d2-%%d7/%%a2-%%a7,(%0) \n"
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
        "move.l  52(%0), %%d0                   \n"  /* Get start address */
        "beq.b   1f                             \n"  /* NULL -> already running */
        "movem.l (%0), %%a0-%%a2                \n"  /* a0=macsr, a1=context, a2=start_thread */
        "jmp     (%%a2)                         \n"  /* Start the thread */
    "1:                                         \n"
        "movem.l (%0), %%d0/%%d2-%%d7/%%a2-%%a7 \n"  /* Load context */
        "move.l  %%d0, %%macsr                  \n"
        : : "a" (addr) : "d0" /* only! */
    );
}

/*---------------------------------------------------------------------------
 * Put core in a power-saving state if waking list wasn't repopulated.
 *---------------------------------------------------------------------------
 */
static inline void core_sleep(void)
{
    /* Supervisor mode, interrupts enabled upon wakeup */
    asm volatile ("stop #0x2000");
};

#elif CONFIG_CPU == SH7034
/*---------------------------------------------------------------------------
 * Start the thread running and terminate it if it returns
 *---------------------------------------------------------------------------
 */
void start_thread(void); /* Provide C access to ASM label */
static void __start_thread(void) __attribute__((used));
static void __start_thread(void)
{
    /* r8 = context */
    asm volatile (
    "_start_thread:            \n" /* Start here - no naked attribute */
        "mov.l  @(4, r8), r0   \n" /* Fetch thread function pointer */
        "mov.l  @(28, r8), r15 \n" /* Set initial sp */
        "mov    #0, r1         \n" /* Start the thread */
        "jsr    @r0            \n"
        "mov.l  r1, @(36, r8)  \n" /* Clear start address */
        "mov.l  1f, r0         \n" /* remove_thread(NULL) */
        "jmp    @r0            \n"
        "mov    #0, r4         \n"
    "1:                        \n"
        ".long  _remove_thread \n"
    );
}

/* Place context pointer in r8 slot, function pointer in r9 slot, and
 * start_thread pointer in context_start */
#define THREAD_STARTUP_INIT(core, thread, function) \
    ({ (thread)->context.r[0] = (unsigned int)&(thread)->context, \
       (thread)->context.r[1] = (unsigned int)(function),         \
       (thread)->context.start = (void*)start_thread; })

/*---------------------------------------------------------------------------
 * Store non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void store_context(void* addr)
{
    asm volatile (
        "add     #36, %0   \n" /* Start at last reg. By the time routine */
        "sts.l   pr, @-%0  \n" /* is done, %0 will have the original value */
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
        "mov.l  @(36, %0), r0 \n" /* Get start address */
        "tst    r0, r0        \n"
        "bt     .running      \n" /* NULL -> already running */
        "jmp    @r0           \n" /* r8 = context */
    ".running:                \n"
        "mov.l  @%0+, r8      \n" /* Executes in delay slot and outside it */
        "mov.l  @%0+, r9      \n"
        "mov.l  @%0+, r10     \n"
        "mov.l  @%0+, r11     \n"
        "mov.l  @%0+, r12     \n"
        "mov.l  @%0+, r13     \n"
        "mov.l  @%0+, r14     \n"
        "mov.l  @%0+, r15     \n"
        "lds.l  @%0+, pr      \n"
        : : "r" (addr) : "r0" /* only! */
    );
}

/*---------------------------------------------------------------------------
 * Put core in a power-saving state if waking list wasn't repopulated.
 *---------------------------------------------------------------------------
 */
static inline void core_sleep(void)
{
    asm volatile (
        "and.b  #0x7f, @(r0, gbr) \n" /* Clear SBY (bit 7) in SBYCR */
        "mov    #0, r1            \n" /* Enable interrupts */
        "ldc    r1, sr            \n" /* Following instruction cannot be interrupted */
        "sleep                    \n" /* Execute standby */
        : : "z"(&SBYCR-GBR) : "r1");
}

#endif /* CONFIG_CPU == */

/*
 * End Processor-specific section
 ***************************************************************************/

#if THREAD_EXTRA_CHECKS
static void thread_panicf(const char *msg, struct thread_entry *thread)
{
#if NUM_CORES > 1
    const unsigned int core = thread->core;
#endif
    static char name[32];
    thread_get_name(name, 32, thread);
    panicf ("%s %s" IF_COP(" (%d)"), msg, name IF_COP(, core));
}
static void thread_stkov(struct thread_entry *thread)
{
    thread_panicf("Stkov", thread);
}
#define THREAD_PANICF(msg, thread) \
    thread_panicf(msg, thread)
#define THREAD_ASSERT(exp, msg, thread) \
    ({ if (!({ exp; })) thread_panicf((msg), (thread)); })
#else
static void thread_stkov(struct thread_entry *thread)
{
#if NUM_CORES > 1
    const unsigned int core = thread->core;
#endif
    static char name[32];
    thread_get_name(name, 32, thread);
    panicf("Stkov %s" IF_COP(" (%d)"), name IF_COP(, core));
}
#define THREAD_PANICF(msg, thread)
#define THREAD_ASSERT(exp, msg, thread)
#endif /* THREAD_EXTRA_CHECKS */

/*---------------------------------------------------------------------------
 * Lock a list pointer and returns its value
 *---------------------------------------------------------------------------
 */
#if CONFIG_CORELOCK == SW_CORELOCK
/* Separate locking function versions */

/* Thread locking */
#define GET_THREAD_STATE(thread) \
    ({ corelock_lock(&(thread)->cl); (thread)->state; })
#define TRY_GET_THREAD_STATE(thread) \
    ({ corelock_try_lock(&thread->cl) ? thread->state : STATE_BUSY; })
#define UNLOCK_THREAD(thread, state) \
    ({ corelock_unlock(&(thread)->cl); })
#define UNLOCK_THREAD_SET_STATE(thread, _state) \
    ({ (thread)->state = (_state); corelock_unlock(&(thread)->cl); })

/* List locking */
#define LOCK_LIST(tqp) \
    ({ corelock_lock(&(tqp)->cl); (tqp)->queue; })
#define UNLOCK_LIST(tqp, mod) \
    ({ corelock_unlock(&(tqp)->cl); })
#define UNLOCK_LIST_SET_PTR(tqp, mod) \
    ({ (tqp)->queue = (mod); corelock_unlock(&(tqp)->cl); })

/* Select the queue pointer directly */
#define ADD_TO_LIST_L_SELECT(tc, tqp, thread) \
    ({ add_to_list_l(&(tqp)->queue, (thread)); })
#define REMOVE_FROM_LIST_L_SELECT(tc, tqp, thread) \
    ({ remove_from_list_l(&(tqp)->queue, (thread)); })

#elif CONFIG_CORELOCK == CORELOCK_SWAP
/* Native swap/exchange versions */

/* Thread locking */
#define GET_THREAD_STATE(thread) \
    ({ unsigned _s; \
       while ((_s = xchg8(&(thread)->state, STATE_BUSY)) == STATE_BUSY); \
       _s; })
#define TRY_GET_THREAD_STATE(thread) \
    ({ xchg8(&(thread)->state, STATE_BUSY); })
#define UNLOCK_THREAD(thread, _state) \
    ({ (thread)->state = (_state); })
#define UNLOCK_THREAD_SET_STATE(thread, _state) \
    ({ (thread)->state = (_state); })

/* List locking */
#define LOCK_LIST(tqp) \
    ({ struct thread_entry *_l; \
       while((_l = xchgptr(&(tqp)->queue, STATE_BUSYuptr)) == STATE_BUSYuptr); \
       _l; })
#define UNLOCK_LIST(tqp, mod) \
    ({ (tqp)->queue = (mod); })
#define UNLOCK_LIST_SET_PTR(tqp, mod) \
    ({ (tqp)->queue = (mod); })

/* Select the local queue pointer copy returned from LOCK_LIST */
#define ADD_TO_LIST_L_SELECT(tc, tqp, thread) \
    ({ add_to_list_l(&(tc), (thread)); })
#define REMOVE_FROM_LIST_L_SELECT(tc, tqp, thread) \
    ({ remove_from_list_l(&(tc), (thread)); })

#else
/* Single-core/non-locked versions */

/* Threads */
#define GET_THREAD_STATE(thread) \
    ({ (thread)->state; })
#define UNLOCK_THREAD(thread, _state)
#define UNLOCK_THREAD_SET_STATE(thread, _state) \
    ({ (thread)->state = (_state); })

/* Lists */
#define LOCK_LIST(tqp) \
    ({ (tqp)->queue; })
#define UNLOCK_LIST(tqp, mod)
#define UNLOCK_LIST_SET_PTR(tqp, mod) \
    ({ (tqp)->queue = (mod); })

/* Select the queue pointer directly */
#define ADD_TO_LIST_L_SELECT(tc, tqp, thread) \
    ({ add_to_list_l(&(tqp)->queue, (thread)); })
#define REMOVE_FROM_LIST_L_SELECT(tc, tqp, thread) \
    ({ remove_from_list_l(&(tqp)->queue, (thread)); })

#endif /* locking selection */

#if THREAD_EXTRA_CHECKS
/*---------------------------------------------------------------------------
 * Lock the thread slot to obtain the state and then unlock it. Waits for
 * it not to be busy. Used for debugging.
 *---------------------------------------------------------------------------
 */
static unsigned peek_thread_state(struct thread_entry *thread)
{
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    unsigned state = GET_THREAD_STATE(thread);
    UNLOCK_THREAD(thread, state);
    set_irq_level(oldlevel);
    return state;
}
#endif /* THREAD_EXTRA_CHECKS */

/*---------------------------------------------------------------------------
 * Adds a thread to a list of threads using "intert last". Uses the "l"
 * links.
 *---------------------------------------------------------------------------
 */
static void add_to_list_l(struct thread_entry **list,
                          struct thread_entry *thread)
{
    struct thread_entry *l = *list;

    if (l == NULL)
    {
        /* Insert into unoccupied list */
        thread->l.next = thread;
        thread->l.prev = thread;
        *list = thread;
        return;
    }

    /* Insert last */
    thread->l.next = l;
    thread->l.prev = l->l.prev;
    thread->l.prev->l.next = thread;
    l->l.prev = thread;

    /* Insert next
     thread->l.next = l->l.next;
     thread->l.prev = l;
     thread->l.next->l.prev = thread;
     l->l.next = thread;
     */
}

/*---------------------------------------------------------------------------
 * Locks a list, adds the thread entry and unlocks the list on multicore.
 * Defined as add_to_list_l on single-core.
 *---------------------------------------------------------------------------
 */
#if NUM_CORES > 1
static void add_to_list_l_locked(struct thread_queue *tq,
                                 struct thread_entry *thread)
{
    struct thread_entry *t = LOCK_LIST(tq);
    ADD_TO_LIST_L_SELECT(t, tq, thread);
    UNLOCK_LIST(tq, t);
    (void)t;
}
#else
#define add_to_list_l_locked(tq, thread) \
    add_to_list_l(&(tq)->queue, (thread))
#endif

/*---------------------------------------------------------------------------
 * Removes a thread from a list of threads. Uses the "l" links.
 *---------------------------------------------------------------------------
 */
static void remove_from_list_l(struct thread_entry **list,
                               struct thread_entry *thread)
{
    struct thread_entry *prev, *next;

    next = thread->l.next;

    if (thread == next)
    {
        /* The only item */
        *list = NULL;
        return;
    }

    if (thread == *list)
    {
        /* List becomes next item */
        *list = next;
    }

    prev = thread->l.prev;
    
    /* Fix links to jump over the removed entry. */
    prev->l.next = next;
    next->l.prev = prev;
}

/*---------------------------------------------------------------------------
 * Locks a list, removes the thread entry and unlocks the list on multicore.
 * Defined as remove_from_list_l on single-core.
 *---------------------------------------------------------------------------
 */
#if NUM_CORES > 1
static void remove_from_list_l_locked(struct thread_queue *tq,
                                      struct thread_entry *thread)
{
    struct thread_entry *t = LOCK_LIST(tq);
    REMOVE_FROM_LIST_L_SELECT(t, tq, thread);
    UNLOCK_LIST(tq, t);
    (void)t;
}
#else
#define remove_from_list_l_locked(tq, thread) \
    remove_from_list_l(&(tq)->queue, (thread))
#endif

/*---------------------------------------------------------------------------
 * Add a thread from the core's timout list by linking the pointers in its
 * tmo structure.
 *---------------------------------------------------------------------------
 */
static void add_to_list_tmo(struct thread_entry *thread)
{
    /* Insert first */
    struct thread_entry *t = cores[IF_COP_CORE(thread->core)].timeout;

    thread->tmo.prev = thread;
    thread->tmo.next = t;

    if (t != NULL)
    {
        /* Fix second item's prev pointer to point to this thread */
        t->tmo.prev = thread;
    }

    cores[IF_COP_CORE(thread->core)].timeout = thread;
}

/*---------------------------------------------------------------------------
 * Remove a thread from the core's timout list by unlinking the pointers in
 * its tmo structure. Sets thread->tmo.prev to NULL to indicate the timeout
 * is cancelled.
 *---------------------------------------------------------------------------
 */
static void remove_from_list_tmo(struct thread_entry *thread)
{
    struct thread_entry *next = thread->tmo.next;
    struct thread_entry *prev;

    if (thread == cores[IF_COP_CORE(thread->core)].timeout)
    {
        /* Next item becomes list head */
        cores[IF_COP_CORE(thread->core)].timeout = next;

        if (next != NULL)
        {
            /* Fix new list head's prev to point to itself. */
            next->tmo.prev = next;
        }

        thread->tmo.prev = NULL;
        return;
    }

    prev = thread->tmo.prev;

    if (next != NULL)
    {
        next->tmo.prev = prev;
    }

    prev->tmo.next = next;
    thread->tmo.prev = NULL;
}

/*---------------------------------------------------------------------------
 * Schedules a thread wakeup on the specified core. Threads will be made
 * ready to run when the next task switch occurs. Note that this does not
 * introduce an on-core delay since the soonest the next thread may run is
 * no sooner than that. Other cores and on-core interrupts may only ever
 * add to the list.
 *---------------------------------------------------------------------------
 */
static void core_schedule_wakeup(struct thread_entry *thread)
{
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    const unsigned int core = IF_COP_CORE(thread->core);
    add_to_list_l_locked(&cores[core].waking, thread);
#if NUM_CORES > 1
    if (core != CURRENT_CORE)
    {
        core_wake(core);
    }
#endif
    set_irq_level(oldlevel);
}

/*---------------------------------------------------------------------------
 * If the waking list was populated, move all threads on it onto the running
 * list so they may be run ASAP.
 *---------------------------------------------------------------------------
 */
static inline void core_perform_wakeup(IF_COP_VOID(unsigned int core))
{
    struct thread_entry *w = LOCK_LIST(&cores[IF_COP_CORE(core)].waking);
    struct thread_entry *r = cores[IF_COP_CORE(core)].running;

    /* Tranfer all threads on waking list to running list in one
       swoop */
    if (r != NULL)
    {
        /* Place waking threads at the end of the running list. */
        struct thread_entry *tmp;
        w->l.prev->l.next = r;
        r->l.prev->l.next = w;
        tmp = r->l.prev;
        r->l.prev = w->l.prev;
        w->l.prev = tmp;
    }
    else
    {
        /* Just transfer the list as-is */
        cores[IF_COP_CORE(core)].running = w;
    }
    /* Just leave any timeout threads on the timeout list. If a timeout check
     * is due, they will be removed there. If they do a timeout again before
     * being removed, they will just stay on the list with a new expiration
     * tick. */

    /* Waking list is clear - NULL and unlock it */
    UNLOCK_LIST_SET_PTR(&cores[IF_COP_CORE(core)].waking, NULL);
}

/*---------------------------------------------------------------------------
 * Check the core's timeout list when at least one thread is due to wake.
 * Filtering for the condition is done before making the call. Resets the
 * tick when the next check will occur.
 *---------------------------------------------------------------------------
 */
static void check_tmo_threads(void)
{
    const unsigned int core = CURRENT_CORE;
    const long tick = current_tick; /* snapshot the current tick */
    long next_tmo_check = tick + 60*HZ; /* minimum duration: once/minute */
    struct thread_entry *next = cores[core].timeout;

    /* If there are no processes waiting for a timeout, just keep the check
       tick from falling into the past. */
    if (next != NULL)
    {
        /* Check sleeping threads. */
        do
        {
            /* Must make sure noone else is examining the state, wait until
               slot is no longer busy */
            struct thread_entry *curr = next;
            next = curr->tmo.next;

            unsigned state = GET_THREAD_STATE(curr);

            if (state < TIMEOUT_STATE_FIRST)
            {
                /* Cleanup threads no longer on a timeout but still on the
                 * list. */
                remove_from_list_tmo(curr);
                UNLOCK_THREAD(curr, state);  /* Unlock thread slot */
            }
            else if (TIME_BEFORE(tick, curr->tmo_tick))
            {
                /* Timeout still pending - this will be the usual case */
                if (TIME_BEFORE(curr->tmo_tick, next_tmo_check))
                {
                    /* Earliest timeout found so far - move the next check up
                       to its time */
                    next_tmo_check = curr->tmo_tick;
                }
                UNLOCK_THREAD(curr, state);  /* Unlock thread slot */
            }
            else
            {
                /* Sleep timeout has been reached so bring the thread back to
                 * life again. */
                if (state == STATE_BLOCKED_W_TMO)
                {
                    remove_from_list_l_locked(curr->bqp, curr);
                }

                remove_from_list_tmo(curr);
                add_to_list_l(&cores[core].running, curr);
                UNLOCK_THREAD_SET_STATE(curr, STATE_RUNNING);
            }

            /* Break the loop once we have walked through the list of all
             * sleeping processes or have removed them all. */
        }
        while (next != NULL);
    }

    cores[core].next_tmo_check = next_tmo_check;
}

/*---------------------------------------------------------------------------
 * Performs operations that must be done before blocking a thread but after
 * the state is saved - follows reverse of locking order. blk_ops.flags is
 * assumed to be nonzero.
 *---------------------------------------------------------------------------
 */
#if NUM_CORES > 1
static inline void run_blocking_ops(
    unsigned int core, struct thread_entry *thread)
{
    struct thread_blk_ops *ops = &cores[IF_COP_CORE(core)].blk_ops;
    const unsigned flags = ops->flags;

    if (flags == 0)
        return;

    if (flags & TBOP_SWITCH_CORE)
    {
        core_switch_blk_op(core, thread);
    }

#if CONFIG_CORELOCK == SW_CORELOCK
    if (flags & TBOP_UNLOCK_LIST)
    {
        UNLOCK_LIST(ops->list_p, NULL);
    }

    if (flags & TBOP_UNLOCK_CORELOCK)
    {
        corelock_unlock(ops->cl_p);
    }

    if (flags & TBOP_UNLOCK_THREAD)
    {
        UNLOCK_THREAD(ops->thread, 0);
    }
#elif CONFIG_CORELOCK == CORELOCK_SWAP
    /* Write updated variable value into memory location */
    switch (flags & TBOP_VAR_TYPE_MASK)
    {
    case TBOP_UNLOCK_LIST:
        UNLOCK_LIST(ops->list_p, ops->list_v);
        break;
    case TBOP_SET_VARi:
        *ops->var_ip = ops->var_iv;
        break;
    case TBOP_SET_VARu8:
        *ops->var_u8p = ops->var_u8v;
        break;
    }
#endif /* CONFIG_CORELOCK == */

    /* Unlock thread's slot */
    if (flags & TBOP_UNLOCK_CURRENT)
    {
        UNLOCK_THREAD(thread, ops->state);
    }

    ops->flags = 0;
}
#endif /* NUM_CORES > 1 */


/*---------------------------------------------------------------------------
 * Runs any operations that may cause threads to be ready to run and then
 * sleeps the processor core until the next interrupt if none are.
 *---------------------------------------------------------------------------
 */
static inline struct thread_entry * sleep_core(IF_COP_VOID(unsigned int core))
{
    for (;;)
    {
        set_irq_level(HIGHEST_IRQ_LEVEL);
        /* We want to do these ASAP as it may change the decision to sleep
         * the core or a core has woken because an interrupt occurred
         * and posted a message to a queue. */
        if (cores[IF_COP_CORE(core)].waking.queue != NULL)
        {
            core_perform_wakeup(IF_COP(core));
        }

        /* If there are threads on a timeout and the earliest wakeup is due,
         * check the list and wake any threads that need to start running
         * again. */
        if (!TIME_BEFORE(current_tick, cores[IF_COP_CORE(core)].next_tmo_check))
        {
            check_tmo_threads();
        }

        /* If there is a ready to run task, return its ID and keep core
         * awake. */
        if (cores[IF_COP_CORE(core)].running == NULL)
        {
            /* Enter sleep mode to reduce power usage - woken up on interrupt
             * or wakeup request from another core - expected to enable all
             * interrupts. */
            core_sleep(IF_COP(core));
            continue;
        }

        set_irq_level(0);
        return cores[IF_COP_CORE(core)].running;
    }
}

#ifdef RB_PROFILE
void profile_thread(void)
{
    profstart(cores[CURRENT_CORE].running - threads);
}
#endif

/*---------------------------------------------------------------------------
 * Prepares a thread to block on an object's list and/or for a specified
 * duration - expects object and slot to be appropriately locked if needed.
 *---------------------------------------------------------------------------
 */
static inline void _block_thread_on_l(struct thread_queue *list,
                                               struct thread_entry *thread,
                                               unsigned state
                                               IF_SWCL(, const bool nolock))
{
    /* If inlined, unreachable branches will be pruned with no size penalty
       because constant params are used for state and nolock. */
    const unsigned int core = IF_COP_CORE(thread->core);

    /* Remove the thread from the list of running threads. */
    remove_from_list_l(&cores[core].running, thread);

    /* Add a timeout to the block if not infinite */
    switch (state)
    {
    case STATE_BLOCKED:
        /* Put the thread into a new list of inactive threads. */
#if CONFIG_CORELOCK == SW_CORELOCK
        if (nolock)
        {
            thread->bqp = NULL; /* Indicate nolock list */
            thread->bqnlp = (struct thread_entry **)list;
            add_to_list_l((struct thread_entry **)list, thread);
        }
        else
#endif
        {
            thread->bqp = list;
            add_to_list_l_locked(list, thread);
        }
        break;
    case STATE_BLOCKED_W_TMO:
        /* Put the thread into a new list of inactive threads. */
#if CONFIG_CORELOCK == SW_CORELOCK
        if (nolock)
        {
            thread->bqp = NULL; /* Indicate nolock list */
            thread->bqnlp = (struct thread_entry **)list;
            add_to_list_l((struct thread_entry **)list, thread);
        }
        else
#endif
        {
            thread->bqp = list;
            add_to_list_l_locked(list, thread);
        }
        /* Fall-through */
    case STATE_SLEEPING:
        /* If this thread times out sooner than any other thread, update
           next_tmo_check to its timeout */
        if (TIME_BEFORE(thread->tmo_tick, cores[core].next_tmo_check))
        {
            cores[core].next_tmo_check = thread->tmo_tick;
        }

        if (thread->tmo.prev == NULL)
        {
            add_to_list_tmo(thread);
        }
        /* else thread was never removed from list - just keep it there */
        break;
    }

#ifdef HAVE_PRIORITY_SCHEDULING
    /* Reset priorities */
    if (thread->priority == cores[core].highest_priority)
        cores[core].highest_priority = LOWEST_PRIORITY;
#endif

#if NUM_CORES == 1 || CONFIG_CORELOCK == SW_CORELOCK
    /* Safe to set state now */
    thread->state = state;
#elif CONFIG_CORELOCK == CORELOCK_SWAP
    cores[core].blk_ops.state = state;
#endif

#if NUM_CORES > 1
    /* Delay slot unlock until task switch */
    cores[core].blk_ops.flags |= TBOP_UNLOCK_CURRENT;
#endif
}

static inline void block_thread_on_l(
    struct thread_queue *list, struct thread_entry *thread, unsigned state)
{
    _block_thread_on_l(list, thread, state IF_SWCL(, false));
}

static inline void block_thread_on_l_no_listlock(
    struct thread_entry **list, struct thread_entry *thread, unsigned state)
{
    _block_thread_on_l((struct thread_queue *)list, thread, state IF_SWCL(, true));
}

/*---------------------------------------------------------------------------
 * Switch thread in round robin fashion for any given priority. Any thread
 * that removed itself from the running list first must specify itself in
 * the paramter.
 *
 * INTERNAL: Intended for use by kernel and not for programs.
 *---------------------------------------------------------------------------
 */
void switch_thread(struct thread_entry *old)
{
    const unsigned int core = CURRENT_CORE;
    struct thread_entry *thread = cores[core].running;
    struct thread_entry *block = old;

    if (block == NULL)
        old = thread;

#ifdef RB_PROFILE
    profile_thread_stopped(old - threads);
#endif

    /* Begin task switching by saving our current context so that we can
     * restore the state of the current thread later to the point prior
     * to this call. */
    store_context(&old->context);

    /* Check if the current thread stack is overflown */
    if(((unsigned int *)old->stack)[0] != DEADBEEF)
        thread_stkov(old);

#if NUM_CORES > 1
    /* Run any blocking operations requested before switching/sleeping */
    run_blocking_ops(core, old);
#endif

    /* Go through the list of sleeping task to check if we need to wake up
     * any of them due to timeout. Also puts core into sleep state until
     * there is at least one running process again. */
    thread = sleep_core(IF_COP(core));

#ifdef HAVE_PRIORITY_SCHEDULING
    /* Select the new task based on priorities and the last time a process
     * got CPU time. */
    if (block == NULL)
        thread = thread->l.next;

    for (;;)
    {
        int priority = thread->priority;

        if (priority < cores[core].highest_priority)
            cores[core].highest_priority = priority;

        if (priority == cores[core].highest_priority ||
            thread->priority_x < cores[core].highest_priority ||
            (current_tick - thread->last_run > priority * 8))
        {
            cores[core].running = thread;
            break;
        }

        thread = thread->l.next;
    }
    
    /* Reset the value of thread's last running time to the current time. */
    thread->last_run = current_tick;
#else
    if (block == NULL)
    {
        thread = thread->l.next;
        cores[core].running = thread;
    }
#endif /* HAVE_PRIORITY_SCHEDULING */

    /* And finally give control to the next thread. */
    load_context(&thread->context);

#ifdef RB_PROFILE
    profile_thread_started(thread - threads);
#endif
}

/*---------------------------------------------------------------------------
 * Change the boost state of a thread boosting or unboosting the CPU
 * as required. Require thread slot to be locked first.
 *---------------------------------------------------------------------------
 */
static inline void boost_thread(struct thread_entry *thread, bool boost)
{
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    if ((thread->boosted != 0) != boost)
    {
        thread->boosted = boost;
        cpu_boost(boost);
    }
#endif
    (void)thread; (void)boost;
}

/*---------------------------------------------------------------------------
 * Sleeps a thread for a specified number of ticks and unboost the thread if
 * if it is boosted. If ticks is zero, it does not delay but instead switches
 * tasks.
 *
 * INTERNAL: Intended for use by kernel and not for programs.
 *---------------------------------------------------------------------------
 */
void sleep_thread(int ticks)
{
    /* Get the entry for the current running thread. */
    struct thread_entry *current = cores[CURRENT_CORE].running;

#if NUM_CORES > 1
    /* Lock thread slot */
    GET_THREAD_STATE(current);
#endif

    /* Set our timeout, change lists, and finally switch threads.
     * Unlock during switch on mulicore. */
    current->tmo_tick = current_tick + ticks + 1;
    block_thread_on_l(NULL, current, STATE_SLEEPING);
    switch_thread(current);

    /* Our status should be STATE_RUNNING */
    THREAD_ASSERT(peek_thread_state(current) == STATE_RUNNING,
                  "S:R->!*R", current);
}

/*---------------------------------------------------------------------------
 * Indefinitely block a thread on a blocking queue for explicit wakeup.
 * Caller with interrupt-accessible lists should disable interrupts first
 * and request a BOP_IRQ_LEVEL blocking operation to reset it.
 *
 * INTERNAL: Intended for use by kernel objects and not for programs.
 *---------------------------------------------------------------------------
 */
IF_SWCL(static inline) void _block_thread(struct thread_queue *list
                                          IF_SWCL(, const bool nolock))
{
    /* Get the entry for the current running thread. */
    struct thread_entry *current = cores[CURRENT_CORE].running;

    /* Set the state to blocked and ask the scheduler to switch tasks,
     * this takes us off of the run queue until we are explicitly woken */

#if NUM_CORES > 1
    /* Lock thread slot */
    GET_THREAD_STATE(current);
#endif

#if CONFIG_CORELOCK == SW_CORELOCK
    /* One branch optimized away during inlining */
    if (nolock)
    {
        block_thread_on_l_no_listlock((struct thread_entry **)list,
                                      current, STATE_BLOCKED);
    }
    else
#endif
    {
        block_thread_on_l(list, current, STATE_BLOCKED);
    }

    switch_thread(current);

    /* Our status should be STATE_RUNNING */
    THREAD_ASSERT(peek_thread_state(current) == STATE_RUNNING,
                  "B:R->!*R", current);
}

#if CONFIG_CORELOCK == SW_CORELOCK
/* Inline lock/nolock version of _block_thread into these functions */
void block_thread(struct thread_queue *tq)
{
    _block_thread(tq, false);
}

void block_thread_no_listlock(struct thread_entry **list)
{
    _block_thread((struct thread_queue *)list, true);
}
#endif /* CONFIG_CORELOCK */

/*---------------------------------------------------------------------------
 * Block a thread on a blocking queue for a specified time interval or until
 * explicitly woken - whichever happens first.
 * Caller with interrupt-accessible lists should disable interrupts first
 * and request that interrupt level be restored after switching out the
 * current thread.
 *
 * INTERNAL: Intended for use by kernel objects and not for programs.
 *---------------------------------------------------------------------------
 */
void block_thread_w_tmo(struct thread_queue *list, int timeout)
{
    /* Get the entry for the current running thread. */
    struct thread_entry *current = cores[CURRENT_CORE].running;

#if NUM_CORES > 1
    /* Lock thread slot */
    GET_THREAD_STATE(current);
#endif

    /* Set the state to blocked with the specified timeout */
    current->tmo_tick = current_tick + timeout;
    /* Set the list for explicit wakeup */
    block_thread_on_l(list, current, STATE_BLOCKED_W_TMO);

    /* Now force a task switch and block until we have been woken up
     * by another thread or timeout is reached - whichever happens first */
    switch_thread(current);

    /* Our status should be STATE_RUNNING */
    THREAD_ASSERT(peek_thread_state(current) == STATE_RUNNING,
                  "T:R->!*R", current);
}

/*---------------------------------------------------------------------------
 * Explicitly wakeup a thread on a blocking queue. Has no effect on threads
 * that called sleep().
 * Caller with interrupt-accessible lists should disable interrupts first.
 * This code should be considered a critical section by the caller.
 *
 * INTERNAL: Intended for use by kernel objects and not for programs.
 *---------------------------------------------------------------------------
 */
IF_SWCL(static inline) struct thread_entry * _wakeup_thread(
    struct thread_queue *list IF_SWCL(, const bool nolock))
{
    struct thread_entry *t;
    struct thread_entry *thread;
    unsigned state;

    /* Wake up the last thread first. */
#if CONFIG_CORELOCK == SW_CORELOCK
    /* One branch optimized away during inlining */
    if (nolock)
    {
        t = list->queue;
    }
    else
#endif
    {    
        t = LOCK_LIST(list);
    }

    /* Check if there is a blocked thread at all. */
    if (t == NULL)
    {
#if CONFIG_CORELOCK == SW_CORELOCK
        if (!nolock)
#endif
        {
            UNLOCK_LIST(list, NULL);
        }
        return NULL;
    }

    thread = t;

#if NUM_CORES > 1
#if CONFIG_CORELOCK == SW_CORELOCK
    if (nolock)
    {
        /* Lock thread only, not list */
        state = GET_THREAD_STATE(thread);
    }
    else
#endif
    {
        /* This locks in reverse order from other routines so a retry in the
           correct order may be needed */
        state = TRY_GET_THREAD_STATE(thread);
        if (state == STATE_BUSY)
        {
            /* Unlock list and retry slot, then list */
            UNLOCK_LIST(list, t);
            state = GET_THREAD_STATE(thread);
            t = LOCK_LIST(list);
            /* Be sure thread still exists here - it couldn't have re-added
               itself if it was woken elsewhere because this function is
               serialized within the object that owns the list. */
            if (thread != t)
            {
                /* Thread disappeared :( */
                UNLOCK_LIST(list, t);
                UNLOCK_THREAD(thread, state);
                return THREAD_WAKEUP_MISSING; /* Indicate disappearance */
            }
        }
    }
#else /* NUM_CORES == 1 */
    state = GET_THREAD_STATE(thread);
#endif /* NUM_CORES */

    /* Determine thread's current state. */
    switch (state)
    {
    case STATE_BLOCKED:
    case STATE_BLOCKED_W_TMO:
        /* Remove thread from object's blocked list - select t or list depending
           on locking type at compile time */
        REMOVE_FROM_LIST_L_SELECT(t, list, thread);
#if CONFIG_CORELOCK == SW_CORELOCK
         /* Statment optimized away during inlining if nolock != false */
        if (!nolock)
#endif
        {
            UNLOCK_LIST(list, t); /* Unlock list - removal complete */
        }

#ifdef HAVE_PRIORITY_SCHEDULING
        /* Give the task a kick to avoid a stall after wakeup.
           Not really proper treatment - TODO later. */
        thread->last_run = current_tick - 8*LOWEST_PRIORITY;
#endif
        core_schedule_wakeup(thread);
        UNLOCK_THREAD_SET_STATE(thread, STATE_RUNNING);
        return thread;
    default:
        /* Nothing to do. State is not blocked. */
#if THREAD_EXTRA_CHECKS
        THREAD_PANICF("wakeup_thread->block invalid", thread);
    case STATE_RUNNING:
    case STATE_KILLED:
#endif
#if CONFIG_CORELOCK == SW_CORELOCK
        /* Statement optimized away during inlining if nolock != false */
        if (!nolock)
#endif
        {
            UNLOCK_LIST(list, t);  /* Unlock the object's list */
        }
        UNLOCK_THREAD(thread, state);  /* Unlock thread slot */
        return NULL;
    }
}

#if CONFIG_CORELOCK == SW_CORELOCK
/* Inline lock/nolock version of _wakeup_thread into these functions */
struct thread_entry * wakeup_thread(struct thread_queue *tq)
{
    return _wakeup_thread(tq, false);
}

struct thread_entry * wakeup_thread_no_listlock(struct thread_entry **list)
{
    return _wakeup_thread((struct thread_queue *)list, true);
}
#endif /* CONFIG_CORELOCK */

/*---------------------------------------------------------------------------
 * Find an empty thread slot or MAXTHREADS if none found. The slot returned
 * will be locked on multicore.
 *---------------------------------------------------------------------------
 */
static int find_empty_thread_slot(void)
{
#if NUM_CORES > 1
    /* Any slot could be on an IRQ-accessible list */
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
#endif
    /* Thread slots are not locked on single core */

    int n;

    for (n = 0; n < MAXTHREADS; n++)
    {
        /* Obtain current slot state - lock it on multicore */
        unsigned state = GET_THREAD_STATE(&threads[n]);

        if (state == STATE_KILLED
#if NUM_CORES > 1
            && threads[n].name != THREAD_DESTRUCT
#endif
        )
        {
            /* Slot is empty - leave it locked and caller will unlock */
            break;
        }

        /* Finished examining slot - no longer busy - unlock on multicore */
        UNLOCK_THREAD(&threads[n], state);
    }

#if NUM_CORES > 1
    set_irq_level(oldlevel); /* Reenable interrups - this slot is
                                not accesible to them yet */
#endif

    return n;
}


/*---------------------------------------------------------------------------
 * Place the current core in idle mode - woken up on interrupt or wake
 * request from another core.
 *---------------------------------------------------------------------------
 */
void core_idle(void)
{
#if NUM_CORES > 1
    const unsigned int core = CURRENT_CORE;
#endif
    set_irq_level(HIGHEST_IRQ_LEVEL);
    core_sleep(IF_COP(core));
}

/*---------------------------------------------------------------------------
 * Create a thread
 * If using a dual core architecture, specify which core to start the thread
 * on, and whether to fall back to the other core if it can't be created
 * Return ID if context area could be allocated, else NULL.
 *---------------------------------------------------------------------------
 */
struct thread_entry* 
    create_thread(void (*function)(void), void* stack, int stack_size,
                  unsigned flags, const char *name
                  IF_PRIO(, int priority)
                  IF_COP(, unsigned int core))
{
    unsigned int i;
    unsigned int stacklen;
    unsigned int *stackptr;
    int slot;
    struct thread_entry *thread;
    unsigned state;

    slot = find_empty_thread_slot();
    if (slot >= MAXTHREADS)
    {
        return NULL;
    }

    /* Munge the stack to make it easy to spot stack overflows */
    stacklen = stack_size / sizeof(int);
    stackptr = stack;
    for(i = 0;i < stacklen;i++)
    {
        stackptr[i] = DEADBEEF;
    }

    /* Store interesting information */
    thread = &threads[slot];
    thread->name = name;
    thread->stack = stack;
    thread->stack_size = stack_size;
    thread->bqp = NULL;
#if CONFIG_CORELOCK == SW_CORELOCK
    thread->bqnlp = NULL;
#endif
    thread->queue = NULL;
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    thread->boosted = 0;
#endif
#ifdef HAVE_PRIORITY_SCHEDULING
    thread->priority_x = LOWEST_PRIORITY;
    thread->priority = priority;
    thread->last_run = current_tick - priority * 8;
    cores[IF_COP_CORE(core)].highest_priority = LOWEST_PRIORITY;
#endif

#if NUM_CORES > 1
    thread->core = core;

    /* Writeback stack munging or anything else before starting */
    if (core != CURRENT_CORE)
    {
        flush_icache();
    }
#endif

    /* Thread is not on any timeout list but be a bit paranoid */
    thread->tmo.prev = NULL;

    state = (flags & CREATE_THREAD_FROZEN) ?
        STATE_FROZEN : STATE_RUNNING;
    
    /* Align stack to an even 32 bit boundary */
    thread->context.sp = (void*)(((unsigned int)stack + stack_size) & ~3);

    /* Load the thread's context structure with needed startup information */
    THREAD_STARTUP_INIT(core, thread, function);

    if (state == STATE_RUNNING)
    {
#if NUM_CORES > 1
        if (core != CURRENT_CORE)
        {
            /* Next task switch on other core moves thread to running list */
            core_schedule_wakeup(thread);
        }
        else
#endif
        {
            /* Place on running list immediately */
            add_to_list_l(&cores[IF_COP_CORE(core)].running, thread);
        }
    }

    /* remove lock and set state */ 
    UNLOCK_THREAD_SET_STATE(thread, state);
 
    return thread;
}

#ifdef HAVE_SCHEDULER_BOOSTCTRL
void trigger_cpu_boost(void)
{
    /* No IRQ disable nescessary since the current thread cannot be blocked
       on an IRQ-accessible list */
    struct thread_entry *current = cores[CURRENT_CORE].running;
    unsigned state;

    state = GET_THREAD_STATE(current);
    boost_thread(current, true);
    UNLOCK_THREAD(current, state);

    (void)state;
}

void cancel_cpu_boost(void)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;
    unsigned state;

    state = GET_THREAD_STATE(current);
    boost_thread(current, false);
    UNLOCK_THREAD(current, state);

    (void)state;
}
#endif /* HAVE_SCHEDULER_BOOSTCTRL */

/*---------------------------------------------------------------------------
 * Remove a thread from the scheduler.
 * Parameter is the ID as returned from create_thread().
 *
 * Use with care on threads that are not under careful control as this may
 * leave various objects in an undefined state. When trying to kill a thread
 * on another processor, be sure you know what it's doing and won't be
 * switching around itself.
 *---------------------------------------------------------------------------
 */
void remove_thread(struct thread_entry *thread)
{
#if NUM_CORES > 1
    /* core is not constant here because of core switching */
    unsigned int core = CURRENT_CORE;
    unsigned int old_core = NUM_CORES;
#else
    const unsigned int core = CURRENT_CORE;
#endif
    unsigned state;
    int oldlevel;

    if (thread == NULL)
        thread = cores[core].running;

    oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    state = GET_THREAD_STATE(thread);

    if (state == STATE_KILLED)
    {
        goto thread_killed;
    }

#if NUM_CORES > 1
    if (thread->core != core)
    {
        /* Switch cores and safely extract the thread there */
        /* Slot HAS to be unlocked or a deadlock could occur - potential livelock
           condition if the thread runs away to another processor. */
        unsigned int new_core = thread->core;
        const char *old_name = thread->name;

        thread->name = THREAD_DESTRUCT; /* Slot can't be used for now */
        UNLOCK_THREAD(thread, state);
        set_irq_level(oldlevel);

        old_core = switch_core(new_core);

        oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
        state = GET_THREAD_STATE(thread);

        core = new_core;

        if (state == STATE_KILLED)
        {
            /* Thread suicided before we could kill it */
            goto thread_killed;
        }

        /* Reopen slot - it's locked again anyway */
        thread->name = old_name;

        if (thread->core != core)
        {
            /* We won't play thread tag - just forget it */
            UNLOCK_THREAD(thread, state);
            set_irq_level(oldlevel);
            goto thread_kill_abort;
        }

        /* Perform the extraction and switch ourselves back to the original
           processor */
    }
#endif /* NUM_CORES > 1 */

#ifdef HAVE_PRIORITY_SCHEDULING
    cores[IF_COP_CORE(core)].highest_priority = LOWEST_PRIORITY;
#endif
    if (thread->tmo.prev != NULL)
    {
        /* Clean thread off the timeout list if a timeout check hasn't
         * run yet */
        remove_from_list_tmo(thread);
    }

    boost_thread(thread, false);

    if (thread == cores[core].running)
    {
        /* Suicide - thread has unconditional rights to do this */
        /* Maintain locks until switch-out */
        block_thread_on_l(NULL, thread, STATE_KILLED);

#if NUM_CORES > 1
        /* Switch to the idle stack if not on the main core (where "main"
         * runs) */
        if (core != CPU)
        {
            switch_to_idle_stack(core);
        }

        flush_icache();
#endif
        /* Signal this thread */
        thread_queue_wake_no_listlock(&thread->queue);
        /* Switch tasks and never return */
        switch_thread(thread);
        /* This should never and must never be reached - if it is, the
         * state is corrupted */
        THREAD_PANICF("remove_thread->K:*R", thread);
    }

#if NUM_CORES > 1
    if (thread->name == THREAD_DESTRUCT)
    {
        /* Another core is doing this operation already */
        UNLOCK_THREAD(thread, state);
        set_irq_level(oldlevel);
        return;
    }
#endif
    if (cores[core].waking.queue != NULL)
    {
        /* Get any threads off the waking list and onto the running
         * list first - waking and running cannot be distinguished by
         * state */
        core_perform_wakeup(IF_COP(core));
    }

    switch (state)
    {
    case STATE_RUNNING:
        /* Remove thread from ready to run tasks */
        remove_from_list_l(&cores[core].running, thread);
        break;
    case STATE_BLOCKED:
    case STATE_BLOCKED_W_TMO:
        /* Remove thread from the queue it's blocked on - including its
         * own if waiting there */
#if CONFIG_CORELOCK == SW_CORELOCK
        /* One or the other will be valid */
        if (thread->bqp == NULL)
        {
            remove_from_list_l(thread->bqnlp, thread);
        }
        else
#endif /* CONFIG_CORELOCK */
        {
            remove_from_list_l_locked(thread->bqp, thread);
        }
        break;
    /* Otherwise thread is killed or is frozen and hasn't run yet */
    }

    /* If thread was waiting on itself, it will have been removed above.
     * The wrong order would result in waking the thread first and deadlocking
     * since the slot is already locked. */
    thread_queue_wake_no_listlock(&thread->queue);

thread_killed: /* Thread was already killed */
    /* Removal complete - safe to unlock state and reenable interrupts */
    UNLOCK_THREAD_SET_STATE(thread, STATE_KILLED);
    set_irq_level(oldlevel);

#if NUM_CORES > 1
thread_kill_abort: /* Something stopped us from killing the thread */
    if (old_core < NUM_CORES)
    {
        /* Did a removal on another processor's thread - switch back to
           native core */
        switch_core(old_core);
    }
#endif
}

/*---------------------------------------------------------------------------
 * Block the current thread until another thread terminates. A thread may
 * wait on itself to terminate which prevents it from running again and it
 * will need to be killed externally.
 * Parameter is the ID as returned from create_thread().
 *---------------------------------------------------------------------------
 */
void thread_wait(struct thread_entry *thread)
{
    const unsigned int core = CURRENT_CORE;
    struct thread_entry *current = cores[core].running;
    unsigned thread_state;
#if NUM_CORES > 1
    int oldlevel;
    unsigned current_state;
#endif

    if (thread == NULL)
        thread = current;

#if NUM_CORES > 1
    oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
#endif

    thread_state = GET_THREAD_STATE(thread);

#if NUM_CORES > 1
    /* We can't lock the same slot twice. The waitee will also lock itself
       first then the thread slots that will be locked and woken in turn.
       The same order must be observed here as well. */
    if (thread == current)
    {
        current_state = thread_state;
    }
    else
    {
        current_state = GET_THREAD_STATE(current);
    }
#endif

    if (thread_state != STATE_KILLED)
    {
        /* Unlock the waitee state at task switch - not done for self-wait
           because the would double-unlock the state and potentially
           corrupt another's busy assert on the slot */
        if (thread != current)
        {
#if CONFIG_CORELOCK == SW_CORELOCK
            cores[core].blk_ops.flags |= TBOP_UNLOCK_THREAD;
            cores[core].blk_ops.thread = thread;
#elif CONFIG_CORELOCK == CORELOCK_SWAP
            cores[core].blk_ops.flags |= TBOP_SET_VARu8;
            cores[core].blk_ops.var_u8p = &thread->state;
            cores[core].blk_ops.var_u8v = thread_state;
#endif
        }
        block_thread_on_l_no_listlock(&thread->queue, current, STATE_BLOCKED);
        switch_thread(current);
        return;
    }

    /* Unlock both slots - obviously the current thread can't have
       STATE_KILLED so the above if clause will always catch a thread
       waiting on itself  */
#if NUM_CORES > 1
    UNLOCK_THREAD(current, current_state);
    UNLOCK_THREAD(thread, thread_state);
    set_irq_level(oldlevel);    
#endif
}

#ifdef HAVE_PRIORITY_SCHEDULING
/*---------------------------------------------------------------------------
 * Sets the thread's relative priority for the core it runs on.
 *---------------------------------------------------------------------------
 */
int thread_set_priority(struct thread_entry *thread, int priority)
{
    unsigned old_priority = (unsigned)-1;
    
    if (thread == NULL)
        thread = cores[CURRENT_CORE].running;

#if NUM_CORES > 1
    /* Thread could be on any list and therefore on an interrupt accessible
       one - disable interrupts */
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
#endif
    unsigned state = GET_THREAD_STATE(thread);

    /* Make sure it's not killed */
    if (state != STATE_KILLED)
    {
        old_priority = thread->priority;
        thread->priority = priority;
        cores[IF_COP_CORE(thread->core)].highest_priority = LOWEST_PRIORITY;
    }

#if NUM_CORES > 1
    UNLOCK_THREAD(thread, state);
    set_irq_level(oldlevel);
#endif
    return old_priority;
}

/*---------------------------------------------------------------------------
 * Returns the current priority for a thread.
 *---------------------------------------------------------------------------
 */
int thread_get_priority(struct thread_entry *thread)
{
    /* Simple, quick probe. */
    if (thread == NULL)
        thread = cores[CURRENT_CORE].running;

    return (unsigned)thread->priority;
}

/*---------------------------------------------------------------------------
 * Yield that guarantees thread execution once per round regardless of
 * thread's scheduler priority - basically a transient realtime boost
 * without altering the scheduler's thread precedence.
 *
 * HACK ALERT! Search for "priority inheritance" for proper treatment.
 *---------------------------------------------------------------------------
 */
void priority_yield(void)
{
    const unsigned int core = CURRENT_CORE;
    struct thread_entry *thread = cores[core].running;
    thread->priority_x = HIGHEST_PRIORITY;
    switch_thread(NULL);
    thread->priority_x = LOWEST_PRIORITY;
}
#endif /* HAVE_PRIORITY_SCHEDULING */

/* Resumes a frozen thread - similar logic to wakeup_thread except that
   the thread is on no scheduler list at all. It exists simply by virtue of
   the slot having a state of STATE_FROZEN. */  
void thread_thaw(struct thread_entry *thread)
{
#if NUM_CORES > 1
    /* Thread could be on any list and therefore on an interrupt accessible
       one - disable interrupts */
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
#endif
    unsigned state = GET_THREAD_STATE(thread);

    if (state == STATE_FROZEN)
    {
        const unsigned int core = CURRENT_CORE;
#if NUM_CORES > 1
        if (thread->core != core)
        {
            core_schedule_wakeup(thread);
        }
        else
#endif
        {
            add_to_list_l(&cores[core].running, thread);
        }

        UNLOCK_THREAD_SET_STATE(thread, STATE_RUNNING);
#if NUM_CORES > 1
        set_irq_level(oldlevel);
#endif
        return;
    }

#if NUM_CORES > 1
    UNLOCK_THREAD(thread, state);
    set_irq_level(oldlevel);
#endif
}

/*---------------------------------------------------------------------------
 * Return the ID of the currently executing thread.
 *---------------------------------------------------------------------------
 */
struct thread_entry * thread_get_current(void)
{
    return cores[CURRENT_CORE].running;
}

#if NUM_CORES > 1
/*---------------------------------------------------------------------------
 * Switch the processor that the currently executing thread runs on.
 *---------------------------------------------------------------------------
 */
unsigned int switch_core(unsigned int new_core)
{
    const unsigned int core = CURRENT_CORE;
    struct thread_entry *current = cores[core].running;
    struct thread_entry *w;
    int oldlevel;

    /* Interrupts can access the lists that will be used - disable them */
    unsigned state = GET_THREAD_STATE(current);

    if (core == new_core)
    {
        /* No change - just unlock everything and return same core */
        UNLOCK_THREAD(current, state);
        return core;
    }

    /* Get us off the running list for the current core */
    remove_from_list_l(&cores[core].running, current);

    /* Stash return value (old core) in a safe place */
    current->retval = core;

    /* If a timeout hadn't yet been cleaned-up it must be removed now or
     * the other core will likely attempt a removal from the wrong list! */
    if (current->tmo.prev != NULL)
    {
        remove_from_list_tmo(current);
    }

    /* Change the core number for this thread slot */
    current->core = new_core;

    /* Do not use core_schedule_wakeup here since this will result in
     * the thread starting to run on the other core before being finished on
     * this one. Delay the wakeup list unlock to keep the other core stuck
     * until this thread is ready. */
    oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    w = LOCK_LIST(&cores[new_core].waking);
    ADD_TO_LIST_L_SELECT(w, &cores[new_core].waking, current);

    /* Make a callback into device-specific code, unlock the wakeup list so
     * that execution may resume on the new core, unlock our slot and finally
     * restore the interrupt level */
    cores[core].blk_ops.flags = TBOP_SWITCH_CORE | TBOP_UNLOCK_CURRENT |
                                TBOP_UNLOCK_LIST;
    cores[core].blk_ops.list_p = &cores[new_core].waking;
#if CONFIG_CORELOCK == CORELOCK_SWAP
    cores[core].blk_ops.state = STATE_RUNNING;
    cores[core].blk_ops.list_v = w;
#endif

#ifdef HAVE_PRIORITY_SCHEDULING
    current->priority_x = HIGHEST_PRIORITY;
    cores[core].highest_priority = LOWEST_PRIORITY;
#endif
    /* Do the stack switching, cache_maintenence and switch_thread call -
       requires native code */
    switch_thread_core(core, current);

#ifdef HAVE_PRIORITY_SCHEDULING
    current->priority_x = LOWEST_PRIORITY;
    cores[current->core].highest_priority = LOWEST_PRIORITY;
#endif

    /* Finally return the old core to caller */
    return current->retval;
    (void)state;
}
#endif /* NUM_CORES > 1 */

/*---------------------------------------------------------------------------
 * Initialize threading API. This assumes interrupts are not yet enabled. On
 * multicore setups, no core is allowed to proceed until create_thread calls
 * are safe to perform.
 *---------------------------------------------------------------------------
 */
void init_threads(void)
{
    const unsigned int core = CURRENT_CORE;
    struct thread_entry *thread;
    int slot;

    /* CPU will initialize first and then sleep */
    slot = find_empty_thread_slot();

    if (slot >= MAXTHREADS)
    {
        /* WTF? There really must be a slot available at this stage.
         * This can fail if, for example, .bss isn't zero'ed out by the loader
         * or threads is in the wrong section. */
        THREAD_PANICF("init_threads->no slot", NULL);
    }

    /* Initialize initially non-zero members of core */
    thread_queue_init(&cores[core].waking);
    cores[core].next_tmo_check = current_tick; /* Something not in the past */
#ifdef HAVE_PRIORITY_SCHEDULING
    cores[core].highest_priority = LOWEST_PRIORITY;
#endif

    /* Initialize initially non-zero members of slot */
    thread = &threads[slot];
    thread->name = main_thread_name;
    UNLOCK_THREAD_SET_STATE(thread, STATE_RUNNING); /* No sync worries yet */
#if NUM_CORES > 1
    thread->core = core;
#endif
#ifdef HAVE_PRIORITY_SCHEDULING
    thread->priority = PRIORITY_USER_INTERFACE;
    thread->priority_x = LOWEST_PRIORITY;
#endif
#if CONFIG_CORELOCK == SW_CORELOCK
    corelock_init(&thread->cl);
#endif

    add_to_list_l(&cores[core].running, thread);

    if (core == CPU)
    {
        thread->stack = stackbegin;
        thread->stack_size = (int)stackend - (int)stackbegin;
#if NUM_CORES > 1  /* This code path will not be run on single core targets */
        /* TODO: HAL interface for this */
        /* Wake up coprocessor and let it initialize kernel and threads */
#ifdef CPU_PP502x
        MBX_MSG_CLR = 0x3f;
#endif
        COP_CTL = PROC_WAKE;
        /* Sleep until finished */
        CPU_CTL = PROC_SLEEP;
        nop; nop; nop; nop;
    } 
    else
    {
        /* Initial stack is the COP idle stack */
        thread->stack = cop_idlestackbegin;
        thread->stack_size = IDLE_STACK_SIZE;
        /* Get COP safely primed inside switch_thread where it will remain
         * until a thread actually exists on it */
        CPU_CTL = PROC_WAKE;
        remove_thread(NULL);
#endif /* NUM_CORES */
    }
}

/*---------------------------------------------------------------------------
 * Returns the maximum percentage of stack a thread ever used while running.
 * NOTE: Some large buffer allocations that don't use enough the buffer to
 * overwrite stackptr[0] will not be seen.
 *---------------------------------------------------------------------------
 */
int thread_stack_usage(const struct thread_entry *thread)
{
    unsigned int *stackptr = thread->stack;
    int stack_words = thread->stack_size / sizeof (int);
    int i, usage = 0;

    for (i = 0; i < stack_words; i++)
    {
        if (stackptr[i] != DEADBEEF)
        {
            usage = ((stack_words - i) * 100) / stack_words;
            break;
        }
    }

    return usage;
}

#if NUM_CORES > 1
/*---------------------------------------------------------------------------
 * Returns the maximum percentage of the core's idle stack ever used during
 * runtime.
 *---------------------------------------------------------------------------
 */
int idle_stack_usage(unsigned int core)
{
    unsigned int *stackptr = idle_stacks[core];
    int i, usage = 0;

    for (i = 0; i < IDLE_STACK_WORDS; i++)
    {
        if (stackptr[i] != DEADBEEF)
        {
            usage = ((IDLE_STACK_WORDS - i) * 100) / IDLE_STACK_WORDS;
            break;
        }
    }

    return usage;
}
#endif

/*---------------------------------------------------------------------------
 * Fills in the buffer with the specified thread's name. If the name is NULL,
 * empty, or the thread is in destruct state a formatted ID is written
 * instead.
 *---------------------------------------------------------------------------
 */
void thread_get_name(char *buffer, int size,
                     struct thread_entry *thread)
{
    if (size <= 0)
        return;

    *buffer = '\0';

    if (thread)
    {
        /* Display thread name if one or ID if none */
        const char *name = thread->name;
        const char *fmt = "%s";
        if (name == NULL IF_COP(|| name == THREAD_DESTRUCT) || *name == '\0')
        {
            name = (const char *)thread;
            fmt = "%08lX";
        }
        snprintf(buffer, size, fmt, name);
    }
}
