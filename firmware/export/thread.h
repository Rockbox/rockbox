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
#include <stddef.h>
#include <stdbool.h>

/* Priority scheduling (when enabled with HAVE_PRIORITY_SCHEDULING) works
 * by giving high priority threads more CPU time than lower priority threads
 * when they need it. Priority is differential such that the priority
 * difference between a lower priority runnable thread and the highest priority
 * runnable thread determines the amount of aging necessary for the lower
 * priority thread to be scheduled in order to prevent starvation.
 *
 * If software playback codec pcm buffer is going down to critical, codec
 * can gradually raise its own priority to override user interface and
 * prevent playback skipping.
 */
#define PRIORITY_RESERVED_HIGH   0   /* Reserved */
#define PRIORITY_RESERVED_LOW    32  /* Reserved */
#define HIGHEST_PRIORITY         1   /* The highest possible thread priority */
#define LOWEST_PRIORITY          31  /* The lowest possible thread priority */
/* Realtime range reserved for threads that will not allow threads of lower
 * priority to age and run (future expansion) */
#define PRIORITY_REALTIME_1      1
#define PRIORITY_REALTIME_2      2
#define PRIORITY_REALTIME_3      3
#define PRIORITY_REALTIME_4      4
#define PRIORITY_REALTIME        4   /* Lowest realtime range */
#define PRIORITY_BUFFERING       15  /* Codec buffering thread */
#define PRIORITY_USER_INTERFACE  16  /* The main thread */
#define PRIORITY_RECORDING       16  /* Recording thread */
#define PRIORITY_PLAYBACK        16  /* Variable between this and MAX */
#define PRIORITY_PLAYBACK_MAX    5   /* Maximum allowable playback priority */
#define PRIORITY_SYSTEM          18  /* All other firmware threads */
#define PRIORITY_BACKGROUND      20  /* Normal application threads */
#define NUM_PRIORITIES           32
#define PRIORITY_IDLE            32  /* Priority representative of no tasks */

/* TODO: Only a minor tweak to create_thread would be needed to let
 * thread slots be caller allocated - no essential threading functionality
 * depends upon an array */
#if CONFIG_CODEC == SWCODEC

#ifdef HAVE_RECORDING
#define BASETHREADS  18
#else
#define BASETHREADS  17
#endif

#else
#define BASETHREADS  11
#endif /* CONFIG_CODE == * */

#ifndef TARGET_EXTRA_THREADS
#define TARGET_EXTRA_THREADS 0
#endif

#define MAXTHREADS (BASETHREADS+TARGET_EXTRA_THREADS)

#define DEFAULT_STACK_SIZE 0x400 /* Bytes */

#ifndef SIMULATOR
/* Need to keep structures inside the header file because debug_menu
 * needs them. */
#ifdef CPU_COLDFIRE
struct regs
{
    uint32_t macsr; /*     0 - EMAC status register */
    uint32_t d[6];  /*  4-24 - d2-d7 */
    uint32_t a[5];  /* 28-44 - a2-a6 */
    uint32_t sp;    /*    48 - Stack pointer (a7) */
    uint32_t start; /*    52 - Thread start address, or NULL when started */
};
#elif CONFIG_CPU == SH7034
struct regs
{
    uint32_t r[7];  /*  0-24 - Registers r8 thru r14 */
    uint32_t sp;    /*    28 - Stack pointer (r15) */
    uint32_t pr;    /*    32 - Procedure register */
    uint32_t start; /*    36 - Thread start address, or NULL when started */
};
#elif defined(CPU_ARM)
struct regs
{
    uint32_t r[8];  /*  0-28 - Registers r4-r11 */
    uint32_t sp;    /*    32 - Stack pointer (r13) */
    uint32_t lr;    /*    36 - r14 (lr) */
    uint32_t start; /*    40 - Thread start address, or NULL when started */
};
#endif /* CONFIG_CPU */
#else
struct regs
{
    void *t;             /* Simulator OS thread */
    void *s;             /* Semaphore for blocking and wakeup */
    void (*start)(void); /* Start function */
};
#endif /* !SIMULATOR */

/* NOTE: The use of the word "queue" may also refer to a linked list of
   threads being maintained that are normally dealt with in FIFO order
   and not necessarily kernel event_queue */
enum
{
    /* States without a timeout must be first */
    STATE_KILLED = 0,    /* Thread is killed (default) */
    STATE_RUNNING,       /* Thread is currently running */
    STATE_BLOCKED,       /* Thread is indefinitely blocked on a queue */
    /* These states involve adding the thread to the tmo list */
    STATE_SLEEPING,      /* Thread is sleeping with a timeout */
    STATE_BLOCKED_W_TMO, /* Thread is blocked on a queue with a timeout */
    /* Miscellaneous states */
    STATE_FROZEN,        /* Thread is suspended and will not run until
                            thread_thaw is called with its ID */
    THREAD_NUM_STATES,
    TIMEOUT_STATE_FIRST = STATE_SLEEPING,
};

#if NUM_CORES > 1
/* Pointer value for name field to indicate thread is being killed. Using
 * an alternate STATE_* won't work since that would interfere with operation
 * while the thread is still running. */
#define THREAD_DESTRUCT ((const char *)~(intptr_t)0)
#endif

/* Link information for lists thread is in */
struct thread_entry; /* forward */
struct thread_list
{
    struct thread_entry *prev; /* Previous thread in a list */
    struct thread_entry *next; /* Next thread in a list */
};

/* Small objects for core-wise mutual exclusion */
#if CONFIG_CORELOCK == SW_CORELOCK
/* No reliable atomic instruction available - use Peterson's algorithm */
struct corelock
{
    volatile unsigned char myl[NUM_CORES];
    volatile unsigned char turn;
} __attribute__((packed));

void corelock_init(struct corelock *cl);
void corelock_lock(struct corelock *cl);
int corelock_try_lock(struct corelock *cl);
void corelock_unlock(struct corelock *cl);
#elif CONFIG_CORELOCK == CORELOCK_SWAP
/* Use native atomic swap/exchange instruction */
struct corelock
{
    volatile unsigned char locked;
} __attribute__((packed));

#define corelock_init(cl) \
    ({ (cl)->locked = 0; })
#define corelock_lock(cl) \
    ({ while (test_and_set(&(cl)->locked, 1)); })
#define corelock_try_lock(cl) \
    ({ test_and_set(&(cl)->locked, 1) ? 0 : 1; })
#define corelock_unlock(cl) \
    ({ (cl)->locked = 0; })
#else
/* No atomic corelock op needed or just none defined */
#define corelock_init(cl)
#define corelock_lock(cl)
#define corelock_try_lock(cl)
#define corelock_unlock(cl)
#endif /* core locking selection */

#ifdef HAVE_PRIORITY_SCHEDULING
struct blocker
{
    struct thread_entry *thread;   /* thread blocking other threads
                                      (aka. object owner) */
    int priority;                  /* highest priority waiter */
    struct thread_entry * (*wakeup_protocol)(struct thread_entry *thread);
};

/* Choices of wakeup protocol */

/* For transfer of object ownership by one thread to another thread by
 * the owning thread itself (mutexes) */
struct thread_entry *
    wakeup_priority_protocol_transfer(struct thread_entry *thread);

/* For release by owner where ownership doesn't change - other threads,
 * interrupts, timeouts, etc. (mutex timeout, queues) */
struct thread_entry *
    wakeup_priority_protocol_release(struct thread_entry *thread);


struct priority_distribution
{
    uint8_t  hist[NUM_PRIORITIES]; /* Histogram: Frequency for each priority */
    uint32_t mask;                 /* Bitmask of hist entries that are not zero */
};

#endif /* HAVE_PRIORITY_SCHEDULING */

/* Information kept in each thread slot
 * members are arranged according to size - largest first - in order
 * to ensure both alignment and packing at the same time.
 */
struct thread_entry
{
    struct regs context;       /* Register context at switch -
                                  _must_ be first member */
    uintptr_t *stack;          /* Pointer to top of stack */
    const char *name;          /* Thread name */
    long tmo_tick;             /* Tick when thread should be woken from
                                  timeout -
                                  states: STATE_SLEEPING/STATE_BLOCKED_W_TMO */
    struct thread_list l;      /* Links for blocked/waking/running -
                                  circular linkage in both directions */
    struct thread_list tmo;    /* Links for timeout list -
                                  Circular in reverse direction, NULL-terminated in
                                  forward direction -
                                  states: STATE_SLEEPING/STATE_BLOCKED_W_TMO */
    struct thread_entry **bqp; /* Pointer to list variable in kernel
                                  object where thread is blocked - used
                                  for implicit unblock and explicit wake
                                  states: STATE_BLOCKED/STATE_BLOCKED_W_TMO  */
#if NUM_CORES > 1
    struct corelock *obj_cl;   /* Object corelock where thead is blocked -
                                  states: STATE_BLOCKED/STATE_BLOCKED_W_TMO */
#endif
    struct thread_entry *queue; /* List of threads waiting for thread to be
                                  removed */
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    #define HAVE_WAKEUP_EXT_CB
    void (*wakeup_ext_cb)(struct thread_entry *thread); /* Callback that
                                  performs special steps needed when being
                                  forced off of an object's wait queue that
                                  go beyond the standard wait queue removal
                                  and priority disinheritance */
    /* Only enabled when using queue_send for now */
#endif
#if defined(HAVE_EXTENDED_MESSAGING_AND_NAME) || NUM_CORES > 1
    intptr_t retval;           /* Return value from a blocked operation/
                                  misc. use */
#endif
#ifdef HAVE_PRIORITY_SCHEDULING
    /* Priority summary of owned objects that support inheritance */
    struct blocker *blocker;   /* Pointer to blocker when this thread is blocked
                                  on an object that supports PIP -
                                  states: STATE_BLOCKED/STATE_BLOCKED_W_TMO  */
    struct priority_distribution pdist; /* Priority summary of owned objects
                                  that have blocked threads and thread's own
                                  base priority */
    int skip_count;            /* Number of times skipped if higher priority
                                  thread was running */
#endif
    unsigned short stack_size; /* Size of stack in bytes */
#ifdef HAVE_PRIORITY_SCHEDULING
    unsigned char base_priority; /* Base priority (set explicitly during
                                  creation or thread_set_priority) */
    unsigned char priority;    /* Scheduled priority (higher of base or
                                  all threads blocked by this one) */
#endif
    unsigned char state;       /* Thread slot state (STATE_*) */
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    unsigned char cpu_boost;   /* CPU frequency boost flag */
#endif
#if NUM_CORES > 1
    unsigned char core;        /* The core to which thread belongs */
    struct corelock waiter_cl; /* Corelock for thread_wait */
    struct corelock slot_cl;   /* Corelock to lock thread slot */
#endif
};

#if NUM_CORES > 1
/* Operations to be performed just before stopping a thread and starting
   a new one if specified before calling switch_thread */
enum
{
    TBOP_CLEAR = 0,       /* No operation to do */
    TBOP_UNLOCK_CORELOCK, /* Unlock a corelock variable */
    TBOP_SWITCH_CORE,     /* Call the core switch preparation routine */
};

struct thread_blk_ops
{
    struct corelock *cl_p;    /* pointer to corelock */
    unsigned char    flags;   /* TBOP_* flags */
};
#endif /* NUM_CORES > 1 */

/* Information kept for each core
 * Members are arranged for the same reason as in thread_entry
 */
struct core_entry
{
    /* "Active" lists - core is constantly active on these and are never
       locked and interrupts do not access them */
    struct thread_entry *running;  /* threads that are running (RTR) */
    struct thread_entry *timeout;  /* threads that are on a timeout before
                                      running again */
    struct thread_entry *block_task; /* Task going off running list */
#ifdef HAVE_PRIORITY_SCHEDULING
    struct priority_distribution rtr; /* Summary of running and ready-to-run
                                         threads */
#endif
    long next_tmo_check;           /* soonest time to check tmo threads */
#if NUM_CORES > 1
    struct thread_blk_ops blk_ops; /* operations to perform when
                                      blocking a thread */
    struct corelock rtr_cl;        /* Lock for rtr list */
#endif /* NUM_CORES */
};

#ifdef HAVE_PRIORITY_SCHEDULING
#define IF_PRIO(...)    __VA_ARGS__
#define IFN_PRIO(...)
#else
#define IF_PRIO(...)
#define IFN_PRIO(...)   __VA_ARGS__
#endif

/* Macros generate better code than an inline function is this case */
#if (defined (CPU_PP) || defined (CPU_ARM))
/* atomic */
#if CONFIG_CORELOCK == SW_CORELOCK
#define test_and_set(a, v, cl) \
    xchg8((a), (v), (cl))
/* atomic */
#define xchg8(a, v, cl) \
({  uint32_t o;            \
    corelock_lock(cl);     \
    o = *(uint8_t *)(a);   \
    *(uint8_t *)(a) = (v); \
    corelock_unlock(cl);   \
    o; })
#define xchg32(a, v, cl) \
({  uint32_t o;             \
    corelock_lock(cl);      \
    o = *(uint32_t *)(a);   \
    *(uint32_t *)(a) = (v); \
    corelock_unlock(cl);    \
    o; })
#define xchgptr(a, v, cl) \
({  typeof (*(a)) o;     \
    corelock_lock(cl);   \
    o = *(a);            \
    *(a) = (v);          \
    corelock_unlock(cl); \
    o; })
#elif CONFIG_CORELOCK == CORELOCK_SWAP
/* atomic */
#define test_and_set(a, v, ...) \
    xchg8((a), (v))
#define xchg8(a, v, ...) \
({  uint32_t o;                \
    asm volatile(              \
        "swpb %0, %1, [%2]"    \
        : "=&r"(o)             \
        : "r"(v),              \
          "r"((uint8_t*)(a))); \
    o; })
/* atomic */
#define xchg32(a, v, ...) \
({  uint32_t o;                 \
    asm volatile(               \
        "swp %0, %1, [%2]"      \
        : "=&r"(o)              \
        : "r"((uint32_t)(v)),   \
          "r"((uint32_t*)(a))); \
    o; })
/* atomic */
#define xchgptr(a, v, ...) \
({  typeof (*(a)) o;        \
    asm volatile(           \
        "swp %0, %1, [%2]"  \
        : "=&r"(o)          \
        : "r"(v), "r"(a));  \
    o; })
#endif /* locking selection */
#elif defined (CPU_COLDFIRE)
/* atomic */
/* one branch will be optimized away if v is a constant expression */
#define test_and_set(a, v, ...) \
({  uint32_t o = 0;                \
    if (v) {                       \
        asm volatile (             \
            "bset.b #0, (%0)"      \
            : : "a"((uint8_t*)(a)) \
            : "cc");               \
    } else {                       \
        asm volatile (             \
            "bclr.b #0, (%0)"      \
            : : "a"((uint8_t*)(a)) \
            : "cc");               \
    }                              \
    asm volatile ("sne.b %0"       \
        : "+d"(o));                \
    o; })
#elif CONFIG_CPU == SH7034
/* atomic */
#define test_and_set(a, v, ...) \
({  uint32_t o;                 \
    asm volatile (              \
        "tas.b  @%2     \n"     \
        "mov    #-1, %0 \n"     \
        "negc   %0, %0  \n"     \
        : "=r"(o)               \
        : "M"((uint32_t)(v)),   /* Value of_v must be 1 */ \
          "r"((uint8_t *)(a))); \
    o; })
#endif /* CONFIG_CPU == */

/* defaults for no asm version */
#ifndef test_and_set
/* not atomic */
#define test_and_set(a, v, ...) \
({  uint32_t o = *(uint8_t *)(a); \
    *(uint8_t *)(a) = (v);        \
    o; })
#endif /* test_and_set */
#ifndef xchg8
/* not atomic */
#define xchg8(a, v, ...) \
({  uint32_t o = *(uint8_t *)(a); \
    *(uint8_t *)(a) = (v);        \
    o; })
#endif /* xchg8 */
#ifndef xchg32
/* not atomic */
#define xchg32(a, v, ...) \
({  uint32_t o = *(uint32_t *)(a); \
    *(uint32_t *)(a) = (v);        \
    o; })
#endif /* xchg32 */
#ifndef xchgptr
/* not atomic */
#define xchgptr(a, v, ...) \
({  typeof (*(a)) o = *(a); \
    *(a) = (v);             \
    o; })
#endif /* xchgptr */

void core_idle(void);
void core_wake(IF_COP_VOID(unsigned int core));

/* Initialize the scheduler */
void init_threads(void);

/* Allocate a thread in the scheduler */
#define CREATE_THREAD_FROZEN   0x00000001 /* Thread is frozen at create time */
struct thread_entry*
    create_thread(void (*function)(void), void* stack, size_t stack_size,
                  unsigned flags, const char *name
                  IF_PRIO(, int priority)
		          IF_COP(, unsigned int core));

/* Set and clear the CPU frequency boost flag for the calling thread */
#ifdef HAVE_SCHEDULER_BOOSTCTRL
void trigger_cpu_boost(void);
void cancel_cpu_boost(void);
#else
#define trigger_cpu_boost()
#define cancel_cpu_boost()
#endif
/* Make a frozed thread runnable (when started with CREATE_THREAD_FROZEN).
 * Has no effect on a thread not frozen. */
void thread_thaw(struct thread_entry *thread);
/* Wait for a thread to exit */
void thread_wait(struct thread_entry *thread);
/* Exit the current thread */
void thread_exit(void);
#if defined(DEBUG) || defined(ROCKBOX_HAS_LOGF)
#define ALLOW_REMOVE_THREAD
/* Remove a thread from the scheduler */
void remove_thread(struct thread_entry *thread);
#endif

/* Switch to next runnable thread */
void switch_thread(void);
/* Blocks a thread for at least the specified number of ticks (0 = wait until
 * next tick) */
void sleep_thread(int ticks);
/* Indefinitely blocks the current thread on a thread queue */
void block_thread(struct thread_entry *current);
/* Blocks the current thread on a thread queue until explicitely woken or
 * the timeout is reached */
void block_thread_w_tmo(struct thread_entry *current, int timeout);

/* Return bit flags for thread wakeup */
#define THREAD_NONE     0x0 /* No thread woken up (exclusive) */
#define THREAD_OK       0x1 /* A thread was woken up */
#define THREAD_SWITCH   0x2 /* Task switch recommended (one or more of
                               higher priority than current were woken) */

/* A convenience function for waking an entire queue of threads. */
unsigned int thread_queue_wake(struct thread_entry **list);

/* Wakeup a thread at the head of a list */
unsigned int wakeup_thread(struct thread_entry **list);

#ifdef HAVE_PRIORITY_SCHEDULING
int thread_set_priority(struct thread_entry *thread, int priority);
int thread_get_priority(struct thread_entry *thread);
#endif /* HAVE_PRIORITY_SCHEDULING */
#if NUM_CORES > 1
unsigned int switch_core(unsigned int new_core);
#endif
struct thread_entry * thread_get_current(void);

/* Debugging info - only! */
int thread_stack_usage(const struct thread_entry *thread);
#if NUM_CORES > 1
int idle_stack_usage(unsigned int core);
#endif
void thread_get_name(char *buffer, int size,
                     struct thread_entry *thread);
#ifdef RB_PROFILE
void profile_thread(void);
#endif

#endif /* THREAD_H */
