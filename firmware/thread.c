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
#include "string.h"
#ifdef RB_PROFILE
#include <profile.h>
#endif

#if NUM_CORES > 1
# define IF_COP2(x) x
#else
# define IF_COP2(x) CURRENT_CORE
#endif

#define DEADBEEF ((unsigned int)0xdeadbeef)
/* Cast to the the machine int type, whose size could be < 4. */

struct core_entry cores[NUM_CORES] IBSS_ATTR;
struct thread_entry threads[MAXTHREADS] IBSS_ATTR;
#ifdef HAVE_SCHEDULER_BOOSTCTRL
static int boosted_threads IBSS_ATTR;
#endif

/* Define to enable additional checks for blocking violations etc. */
#define THREAD_EXTRA_CHECKS

static const char main_thread_name[] = "main";

extern int stackbegin[];
extern int stackend[];

#ifdef CPU_PP
#ifndef BOOTLOADER
extern int cop_stackbegin[];
extern int cop_stackend[];
#else
/* The coprocessor stack is not set up in the bootloader code, but the threading
 * is.  No threads are run on the coprocessor, so set up some dummy stack */
int *cop_stackbegin = stackbegin;
int *cop_stackend = stackend;
#endif
#endif

#if NUM_CORES > 1
static long cores_locked IBSS_ATTR;

#define LOCK(...) do { } while (test_and_set(&cores_locked, 1))
#define UNLOCK(...) cores_locked = 0

inline void lock_cores(void)
{
    if (!cores[CURRENT_CORE].lock_issued)
    {
        LOCK();
        cores[CURRENT_CORE].lock_issued = true;
    }
}

inline void unlock_cores(void)
{
    if (cores[CURRENT_CORE].lock_issued)
    {
        cores[CURRENT_CORE].lock_issued = false;
        UNLOCK();
    }
}
#endif

/* Conserve IRAM
static void add_to_list(struct thread_entry **list,
                        struct thread_entry *thread) ICODE_ATTR;
static void remove_from_list(struct thread_entry **list,
                             struct thread_entry *thread) ICODE_ATTR;
*/

void switch_thread(bool save_context, struct thread_entry **blocked_list) 
    ICODE_ATTR;

static inline void store_context(void* addr) __attribute__ ((always_inline));
static inline void load_context(const void* addr)
    __attribute__ ((always_inline));

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
        "beq.b   1f              \n"  /* NULL -> already running */
        "clr.l   (52,%0)         \n"  /* Clear start address.. */
        "move.l  %%d0,%0         \n"
        "jmp     (%0)            \n"  /* ..and start the thread */
    "1:                          \n"
        : : "a" (addr) : "d0" /* only! */
    );
}

/* Set EMAC unit to fractional mode with saturation for each new thread,
   since that's what'll be the most useful for most things which the dsp
   will do. Codecs should still initialize their preferred modes
   explicitly. */
#define THREAD_CPU_INIT(core, thread) \
    ({ (thread)->context.macsr = EMAC_FRACTIONAL | EMAC_SATURATE; })

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

#endif

#ifndef THREAD_CPU_INIT
/* No cpu specific init - make empty */
#define THREAD_CPU_INIT(core, thread)
#endif

static void add_to_list(struct thread_entry **list, struct thread_entry *thread)
{
    if (*list == NULL)
    {
        thread->next = thread;
        thread->prev = thread;
        *list = thread;
    }
    else
    {
        /* Insert last */
        thread->next = *list;
        thread->prev = (*list)->prev;
        thread->prev->next = thread;
        (*list)->prev = thread;
        
        /* Insert next
         thread->next = (*list)->next;
         thread->prev = *list;
         thread->next->prev = thread;
         (*list)->next = thread;
         */
    }
}

static void remove_from_list(struct thread_entry **list,
                      struct thread_entry *thread)
{
    if (list != NULL)
    {
        if (thread == thread->next)
        {
            *list = NULL;
            return;
        }
        
        if (thread == *list)
            *list = thread->next;
    }
    
    /* Fix links to jump over the removed entry. */
    thread->prev->next = thread->next;
    thread->next->prev = thread->prev;
}

/* Compiler trick: Don't declare as static to prevent putting 
 * function in IRAM. */
void check_sleepers(void)
{
    struct thread_entry *current, *next;
    
    /* Check sleeping threads. */
    current = cores[CURRENT_CORE].sleeping;
    if (current == NULL)
        return ;
    
    for (;;)
    {
        next = current->next;
        
        if ((unsigned)current_tick >= GET_STATE_ARG(current->statearg))
        {
            /* Sleep timeout has been reached so bring the thread
             * back to life again. */
            remove_from_list(&cores[CURRENT_CORE].sleeping, current);
            add_to_list(&cores[CURRENT_CORE].running, current);
            current->statearg = 0;
            
            /* If there is no more processes in the list, break the loop. */
            if (cores[CURRENT_CORE].sleeping == NULL)
                break;
            
            current = next;
            continue;
        }
        
        current = next;
        
        /* Break the loop once we have walked through the list of all
         * sleeping processes. */
        if (current == cores[CURRENT_CORE].sleeping)
            break;
    }
}

/* Safely finish waking all threads potentialy woken by interrupts -
 * statearg already zeroed in wakeup_thread. */
static void wake_list_awaken(void)
{
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);

    /* No need for another check in the IRQ lock since IRQs are allowed
       only to add threads to the waking list. They won't be adding more
       until we're done here though. */

    struct thread_entry *waking = cores[CURRENT_CORE].waking;
    struct thread_entry *running = cores[CURRENT_CORE].running;

    if (running != NULL)
    {
        /* Place waking threads at the end of the running list. */
        struct thread_entry *tmp;
        waking->prev->next  = running;
        running->prev->next = waking;
        tmp                 = running->prev;
        running->prev       = waking->prev;
        waking->prev        = tmp;
    }
    else
    {
        /* Just transfer the list as-is - just came out of a core
         * sleep. */
        cores[CURRENT_CORE].running = waking;
    }

    /* Done with waking list */
    cores[CURRENT_CORE].waking = NULL;
    set_irq_level(oldlevel);
}

static inline void sleep_core(void)
{
#if CONFIG_CPU == S3C2440
    int i;
#endif

    for (;;)
    {
        /* We want to do these ASAP as it may change the decision to sleep
           the core or the core has woken because an interrupt occurred
           and posted a message to a queue. */
        if (cores[CURRENT_CORE].waking != NULL)
            wake_list_awaken();

        if (cores[CURRENT_CORE].last_tick != current_tick)
        {
            check_sleepers();
            cores[CURRENT_CORE].last_tick = current_tick;
        }
        
        /* We must sleep until there is at least one process in the list
         * of running processes. */
        if (cores[CURRENT_CORE].running != NULL)
            break;

        /* Enter sleep mode to reduce power usage, woken up on interrupt */
#ifdef CPU_COLDFIRE
        asm volatile ("stop #0x2000");
#elif CONFIG_CPU == SH7034
        and_b(0x7F, &SBYCR);
        asm volatile ("sleep");
#elif defined (CPU_PP)
        unlock_cores();
        
        /* This should sleep the CPU. It appears to wake by itself on
           interrupts */
        if (CURRENT_CORE == CPU)
            CPU_CTL = PROC_SLEEP;
        else
            COP_CTL = PROC_SLEEP;
        
        lock_cores();
#elif CONFIG_CPU == S3C2440
        CLKCON |= (1 << 2); /* set IDLE bit */
        for(i=0; i<10; i++); /* wait for IDLE */
        CLKCON &= ~(1 << 2); /* reset IDLE bit when wake up */
#endif
        
    }
}

#ifdef RB_PROFILE
static int get_threadnum(struct thread_entry *thread)
{
    int i;
    
    for (i = 0; i < MAXTHREADS; i++)
    {
        if (&threads[i] == thread)
            return i;
    }
    
    return -1;
}

void profile_thread(void) {
    profstart(get_threadnum(cores[CURRENT_CORE].running));
}
#endif

/* Compiler trick: Don't declare as static to prevent putting 
 * function in IRAM. */
void change_thread_state(struct thread_entry **blocked_list)
{
    struct thread_entry *old;
    unsigned long new_state;
    
    /* Remove the thread from the list of running threads. */
    old = cores[CURRENT_CORE].running;
    new_state = GET_STATE(old->statearg);

    /* Check if a thread state change has been requested. */
    if (new_state)
    {
        /* Change running thread state and switch to next thread. */
        remove_from_list(&cores[CURRENT_CORE].running, old);
        
        /* And put the thread into a new list of inactive threads. */
        if (new_state == STATE_BLOCKED)
            add_to_list(blocked_list, old);
        else
            add_to_list(&cores[CURRENT_CORE].sleeping, old);
        
#ifdef HAVE_PRIORITY_SCHEDULING
        /* Reset priorities */
        if (old->priority == cores[CURRENT_CORE].highest_priority)
            cores[CURRENT_CORE].highest_priority = 100;
#endif
    }
    else
        /* Switch to the next running thread. */
        cores[CURRENT_CORE].running = old->next;
}

/*---------------------------------------------------------------------------
 * Switch thread in round robin fashion.
 *---------------------------------------------------------------------------
 */
void switch_thread(bool save_context, struct thread_entry **blocked_list)
{
#ifdef RB_PROFILE
    profile_thread_stopped(get_threadnum(cores[CURRENT_CORE].running));
#endif
    unsigned int *stackptr;
    
#ifdef SIMULATOR
    /* Do nothing */
#else

    lock_cores();
    
    /* Begin task switching by saving our current context so that we can
     * restore the state of the current thread later to the point prior
     * to this call. */
    if (save_context)
    {
        store_context(&cores[CURRENT_CORE].running->context);

        /* Check if the current thread stack is overflown */
        stackptr = cores[CURRENT_CORE].running->stack;
        if(stackptr[0] != DEADBEEF)
            panicf("Stkov %s", cores[CURRENT_CORE].running->name);
    
        /* Rearrange thread lists as needed */
        change_thread_state(blocked_list);

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
        /* This has to be done after the scheduler is finished with the
           blocked_list pointer so that an IRQ can't kill us by attempting
           a wake but before attempting any core sleep. */
        if (cores[CURRENT_CORE].switch_to_irq_level != STAY_IRQ_LEVEL)
        {
            int level = cores[CURRENT_CORE].switch_to_irq_level;
            cores[CURRENT_CORE].switch_to_irq_level = STAY_IRQ_LEVEL;
            set_irq_level(level);
        }
#endif
    }
    
    /* Go through the list of sleeping task to check if we need to wake up
     * any of them due to timeout. Also puts core into sleep state until
     * there is at least one running process again. */
    sleep_core();
    
#ifdef HAVE_PRIORITY_SCHEDULING
    /* Select the new task based on priorities and the last time a process
     * got CPU time. */
    for (;;)
    {
        int priority = cores[CURRENT_CORE].running->priority;

        if (priority < cores[CURRENT_CORE].highest_priority)
            cores[CURRENT_CORE].highest_priority = priority;

        if (priority == cores[CURRENT_CORE].highest_priority ||
                (current_tick - cores[CURRENT_CORE].running->last_run >
                 priority * 8) ||
            cores[CURRENT_CORE].running->priority_x != 0)
        {
            break;
        }

        cores[CURRENT_CORE].running = cores[CURRENT_CORE].running->next;
    }
    
    /* Reset the value of thread's last running time to the current time. */
    cores[CURRENT_CORE].running->last_run = current_tick;
#endif
    
#endif
    unlock_cores();
    
    /* And finally give control to the next thread. */
    load_context(&cores[CURRENT_CORE].running->context);
    
#ifdef RB_PROFILE
    profile_thread_started(get_threadnum(cores[CURRENT_CORE].running));
#endif
}

void sleep_thread(int ticks)
{
    struct thread_entry *current;

    lock_cores();
    
    current = cores[CURRENT_CORE].running;

#ifdef HAVE_SCHEDULER_BOOSTCTRL
    if (STATE_IS_BOOSTED(current->statearg))
    {
        boosted_threads--;
        if (!boosted_threads)
        {
            cpu_boost(false);
        }
    }
#endif

    /* Set the thread's new state and timeout and finally force a task switch
     * so that scheduler removes thread from the list of running processes
     * and puts it in list of sleeping tasks. */
    SET_STATE(current->statearg, STATE_SLEEPING, current_tick + ticks + 1);
    
    switch_thread(true, NULL);
}

void block_thread(struct thread_entry **list)
{
    struct thread_entry *current;
    
    lock_cores();
    
    /* Get the entry for the current running thread. */
    current = cores[CURRENT_CORE].running;

#ifdef HAVE_SCHEDULER_BOOSTCTRL
    /* Keep the boosted state over indefinite block calls, because
     * we are waiting until the earliest time that someone else
     * completes an action */
    unsigned long boost_flag = STATE_IS_BOOSTED(current->statearg);
#endif

#ifdef THREAD_EXTRA_CHECKS
    /* We are not allowed to mix blocking types in one queue. */
    if (*list && GET_STATE((*list)->statearg) == STATE_BLOCKED_W_TMO)
        panicf("Blocking violation B->*T");
#endif
    
    /* Set the state to blocked and ask the scheduler to switch tasks,
     * this takes us off of the run queue until we are explicitly woken */
    SET_STATE(current->statearg, STATE_BLOCKED, 0);

    switch_thread(true, list);

#ifdef HAVE_SCHEDULER_BOOSTCTRL
    /* Reset only the boosted flag to indicate we are up and running again. */
    current->statearg = boost_flag;
#else
    /* Clear all flags to indicate we are up and running again. */
    current->statearg = 0;
#endif
}

void block_thread_w_tmo(struct thread_entry **list, int timeout)
{
    struct thread_entry *current;
    /* Get the entry for the current running thread. */
    current = cores[CURRENT_CORE].running;
    
    lock_cores();
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    /* A block with a timeout is a sleep situation, whatever we are waiting
     * for _may or may not_ happen, regardless of boost state, (user input
     * for instance), so this thread no longer needs to boost */
    if (STATE_IS_BOOSTED(current->statearg))
    {
        boosted_threads--;
        if (!boosted_threads)
        {
            cpu_boost(false);
        }
    }
#endif

#ifdef THREAD_EXTRA_CHECKS
    /* We can store only one thread to the "list" if thread is used
     * in other list (such as core's list for sleeping tasks). */
    if (*list)
        panicf("Blocking violation T->*B");
#endif
    
    /* Set the state to blocked with the specified timeout */
    SET_STATE(current->statearg, STATE_BLOCKED_W_TMO, current_tick + timeout);

    /* Set the "list" for explicit wakeup */
    *list = current;

    /* Now force a task switch and block until we have been woken up
     * by another thread or timeout is reached. */
    switch_thread(true, NULL);

    /* It is now safe for another thread to block on this "list" */
    *list = NULL;
}

#if defined(HAVE_EXTENDED_MESSAGING_AND_NAME) && !defined(SIMULATOR)
void set_irq_level_and_block_thread(struct thread_entry **list, int level)
{
    cores[CURRENT_CORE].switch_to_irq_level = level;
    block_thread(list);
}

#if 0
void set_irq_level_and_block_thread_w_tmo(struct thread_entry **list,
                                          int timeout, int level)
{
    cores[CURRENT_CORE].switch_to_irq_level = level;
    block_thread_w_tmo(list, timeout);
}
#endif
#endif /* HAVE_EXTENDED_MESSAGING_AND_NAME */

void wakeup_thread(struct thread_entry **list)
{
    struct thread_entry *thread;
    
    /* Check if there is a blocked thread at all. */
    if (*list == NULL)
    {
        return ;
    }
    
    /* Wake up the last thread first. */
    thread = *list;
    
    /* Determine thread's current state. */
    switch (GET_STATE(thread->statearg)) 
    {
        case STATE_BLOCKED:
            /* Remove thread from the list of blocked threads and add it
             * to the scheduler's list of running processes. List removal
             * is safe since each object maintains it's own list of
             * sleepers and queues protect against reentrancy. */
            remove_from_list(list, thread);
            add_to_list(cores[IF_COP2(thread->core)].wakeup_list, thread);

        case STATE_BLOCKED_W_TMO:
            /* Just remove the timeout to cause scheduler to immediately
             * wake up the thread. */
            thread->statearg = 0;
            break;
        
        default:
            /* Nothing to do. Thread has already been woken up
             * or it's state is not blocked or blocked with timeout. */
            return ;
    }
}

inline static int find_empty_thread_slot(void)
{
    int n;
    
    for (n = 0; n < MAXTHREADS; n++)
    {
        if (threads[n].name == NULL)
            return n;
    }
    
    return -1;
}

/* Like wakeup_thread but safe against IRQ corruption when IRQs are disabled
   before calling. */
void wakeup_thread_irq_safe(struct thread_entry **list)
{
    struct core_entry *core = &cores[CURRENT_CORE];
    /* Switch wakeup lists and call wakeup_thread */
    core->wakeup_list = &core->waking;
    wakeup_thread(list);
    /* Switch back to normal running list */
    core->wakeup_list = &core->running;
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
                  const char *name IF_PRIO(, int priority)
		  IF_COP(, unsigned int core, bool fallback))
{
    unsigned int i;
    unsigned int stacklen;
    unsigned int *stackptr;
    int slot;
    struct regs *regs;
    struct thread_entry *thread;

/*****
 * Ugly code alert!
 * To prevent ifdef hell while keeping the binary size down, we define
 * core here if it hasn't been passed as a parameter
 *****/
#if NUM_CORES == 1
#define core CPU
#endif

#if NUM_CORES > 1
/* If the kernel hasn't initialised on the COP (most likely due to an old
 * bootloader) then refuse to start threads on the COP
 */
    if ((core == COP) && !cores[core].kernel_running)
    {
        if (fallback)
            return create_thread(function, stack, stack_size, name
	                         IF_PRIO(, priority) IF_COP(, CPU, false));
	else
            return NULL;
    }
#endif

    lock_cores();
    
    slot = find_empty_thread_slot();
    if (slot < 0)
    {
        unlock_cores();
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
    thread->statearg = 0;
#ifdef HAVE_PRIORITY_SCHEDULING
    thread->priority_x = 0;
    thread->priority = priority;
    cores[core].highest_priority = 100;
#endif

#if NUM_CORES > 1
    thread->core = core;
#endif
    
    regs = &thread->context;
    /* Align stack to an even 32 bit boundary */
    regs->sp = (void*)(((unsigned int)stack + stack_size) & ~3);
    regs->start = (void*)function;

    /* Do any CPU specific inits after initializing common items
       to have access to valid data */
    THREAD_CPU_INIT(core, thread);
    
    add_to_list(&cores[core].running, thread);
    unlock_cores();

    return thread;
#if NUM_CORES == 1
#undef core
#endif
}

#ifdef HAVE_SCHEDULER_BOOSTCTRL
void trigger_cpu_boost(void)
{
    lock_cores();
    
    if (!STATE_IS_BOOSTED(cores[CURRENT_CORE].running->statearg))
    {
        SET_BOOST_STATE(cores[CURRENT_CORE].running->statearg);
        if (!boosted_threads)
        {
            cpu_boost(true);
        }
        boosted_threads++;
    }
    
    unlock_cores();
}
#endif

/*---------------------------------------------------------------------------
 * Remove a thread on the current core from the scheduler.
 * Parameter is the ID as returned from create_thread().
 *---------------------------------------------------------------------------
 */
void remove_thread(struct thread_entry *thread)
{
    lock_cores();
    
    if (thread == NULL)
        thread = cores[IF_COP2(thread->core)].running;
    
    /* Free the entry by removing thread name. */
    thread->name = NULL;
#ifdef HAVE_PRIORITY_SCHEDULING
    cores[IF_COP2(thread->core)].highest_priority = 100;
#endif
    
    if (thread == cores[IF_COP2(thread->core)].running)
    {
        remove_from_list(&cores[IF_COP2(thread->core)].running, thread);
        switch_thread(false, NULL);
        return ;
    }
    
    if (thread == cores[IF_COP2(thread->core)].sleeping)
        remove_from_list(&cores[IF_COP2(thread->core)].sleeping, thread);
    else
        remove_from_list(NULL, thread);
    
    unlock_cores();
}

#ifdef HAVE_PRIORITY_SCHEDULING
int thread_set_priority(struct thread_entry *thread, int priority)
{
    int old_priority;
    
    lock_cores();
    if (thread == NULL)
        thread = cores[CURRENT_CORE].running;

    old_priority = thread->priority;
    thread->priority = priority;
    cores[IF_COP2(thread->core)].highest_priority = 100;
    unlock_cores();
    
    return old_priority;
}

int thread_get_priority(struct thread_entry *thread)
{
    if (thread == NULL)
        thread = cores[CURRENT_CORE].running;

    return thread->priority;
}

void priority_yield(void)
{
    struct thread_entry *thread = cores[CURRENT_CORE].running;
    thread->priority_x = 1;
    switch_thread(true, NULL);
    thread->priority_x = 0;
}
#endif /* HAVE_PRIORITY_SCHEDULING */

struct thread_entry * thread_get_current(void)
{
    return cores[CURRENT_CORE].running;
}

void init_threads(void)
{
    unsigned int core = CURRENT_CORE;
    int slot;

    /* Let main CPU initialize first. */
#if NUM_CORES > 1
    if (core != CPU)
    {
        while (!cores[CPU].kernel_running) ;
    }
#endif
    
    lock_cores();
    slot = find_empty_thread_slot();
    
    cores[core].sleeping = NULL;
    cores[core].running = NULL;
    cores[core].waking = NULL;
    cores[core].wakeup_list = &cores[core].running;
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    cores[core].switch_to_irq_level = STAY_IRQ_LEVEL;
#endif
    threads[slot].name = main_thread_name;
    threads[slot].statearg = 0;
    threads[slot].context.start = 0; /* core's main thread already running */
#if NUM_CORES > 1
    threads[slot].core = core;
#endif
#ifdef HAVE_PRIORITY_SCHEDULING
    threads[slot].priority = PRIORITY_USER_INTERFACE;
    threads[slot].priority_x = 0;
    cores[core].highest_priority = 100;
#endif
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    boosted_threads = 0;
#endif
    add_to_list(&cores[core].running, &threads[slot]);
    
    /* In multiple core setups, each core has a different stack.  There is
     * probably a much better way to do this. */
    if (core == CPU)
    {
        threads[slot].stack = stackbegin;
        threads[slot].stack_size = (int)stackend - (int)stackbegin;
    } 
#if NUM_CORES > 1  /* This code path will not be run on single core targets */
    else 
    {
        threads[slot].stack = cop_stackbegin;
        threads[slot].stack_size =
            (int)cop_stackend - (int)cop_stackbegin;
    }

    cores[core].kernel_running = true;
#endif
    
    unlock_cores();
}

int thread_stack_usage(const struct thread_entry *thread)
{
    unsigned int i;
    unsigned int *stackptr = thread->stack;

    for (i = 0;i < thread->stack_size/sizeof(int);i++)
    {
        if (stackptr[i] != DEADBEEF)
            break;
    }

    return ((thread->stack_size - i * sizeof(int)) * 100) /
        thread->stack_size;
}

int thread_get_status(const struct thread_entry *thread)
{
    return GET_STATE(thread->statearg);
}
