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
 * by giving high priority threads more CPU time than less priority threads
 * when they need it.
 * 
 * If software playback codec pcm buffer is going down to critical, codec
 * can change it own priority to REALTIME to override user interface and
 * prevent playback skipping.
 */
#define HIGHEST_PRIORITY         1   /* The highest possible thread priority */
#define LOWEST_PRIORITY          100 /* The lowest possible thread priority */
#define PRIORITY_REALTIME        1
#define PRIORITY_USER_INTERFACE  4   /* The main thread */
#define PRIORITY_RECORDING       4   /* Recording thread */
#define PRIORITY_PLAYBACK        4   /* or REALTIME when needed */
#define PRIORITY_BUFFERING       4   /* Codec buffering thread */
#define PRIORITY_SYSTEM          6   /* All other firmware threads */
#define PRIORITY_BACKGROUND      8   /* Normal application threads */

/* TODO: Only a minor tweak to create_thread would be needed to let
 * thread slots be caller allocated - no essential threading functionality
 * depends upon an array */
#if CONFIG_CODEC == SWCODEC

#ifdef HAVE_RECORDING
#define MAXTHREADS  18
#else
#define MAXTHREADS  17
#endif

#else
#define MAXTHREADS  11
#endif /* CONFIG_CODE == * */

#define DEFAULT_STACK_SIZE 0x400 /* Bytes */

/**
 * "Busy" values that can be swapped into a variable to indicate
 * that the variable or object pointed to is in use by another processor
 * core. When accessed, the busy value is swapped-in while the current
 * value is atomically returned. If the swap returns the busy value,
 * the processor should retry the operation until some other value is
 * returned. When modification is finished, the new value should be
 * written which unlocks it and updates it atomically.
 *
 * Procedure:
 * while ((curr_value = swap(&variable, BUSY_VALUE)) == BUSY_VALUE);
 *
 * Modify/examine object at mem location or variable. Create "new_value"
 * as suitable.
 *
 * variable = new_value or curr_value;
 *
 * To check a value for busy and perform an operation if not:
 * curr_value = swap(&variable, BUSY_VALUE);
 *
 * if (curr_value != BUSY_VALUE)
 * {
 *     Modify/examine object at mem location or variable. Create "new_value"
 *     as suitable.
 *     variable = new_value or curr_value;
 * }
 * else
 * {
 *     Do nothing - already busy
 * }
 *
 * Only ever restore when an actual value is returned or else it could leave
 * the variable locked permanently if another processor unlocked in the
 * meantime. The next access attempt would deadlock for all processors since
 * an abandoned busy status would be left behind.
 */
#define STATE_BUSYuptr    ((void*)UINTPTR_MAX)
#define STATE_BUSYu8      UINT8_MAX
#define STATE_BUSYi       INT_MIN

#ifndef SIMULATOR
/* Need to keep structures inside the header file because debug_menu
 * needs them. */
#ifdef CPU_COLDFIRE
struct regs
{
    unsigned int macsr;  /*     0 - EMAC status register */
    unsigned int d[6];   /*  4-24 - d2-d7 */
    unsigned int a[5];   /* 28-44 - a2-a6 */
    void         *sp;    /*    48 - Stack pointer (a7) */
    void         *start; /*    52 - Thread start address, or NULL when started */
};
#elif CONFIG_CPU == SH7034
struct regs
{
    unsigned int r[7];   /*  0-24 - Registers r8 thru r14 */
    void         *sp;    /*    28 - Stack pointer (r15) */
    void         *pr;    /*    32 - Procedure register */
    void         *start; /*    36 - Thread start address, or NULL when started */
};
#elif defined(CPU_ARM)
struct regs
{
    unsigned int r[8];   /*  0-28 - Registers r4-r11 */
    void         *sp;    /*    32 - Stack pointer (r13) */
    unsigned int lr;     /*    36 - r14 (lr) */
    void         *start; /*    40 - Thread start address, or NULL when started */
};
#endif /* CONFIG_CPU */
#else
struct regs
{
    void *t;             /* Simulator OS thread */
    void *c;             /* Condition for blocking and sync */
    void (*start)(void); /* Start function */
};
#endif /* !SIMULATOR */

/* NOTE: The use of the word "queue" may also refer to a linked list of
   threads being maintainted that are normally dealt with in FIFO order
   and not nescessarily kernel event_queue */
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
#if NUM_CORES > 1
    STATE_BUSY = STATE_BUSYu8, /* Thread slot is being examined */
#endif
};

#if NUM_CORES > 1
#define THREAD_DESTRUCT ((const char *)0x84905617)
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
    unsigned char locked;
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

struct thread_queue
{
    struct thread_entry *queue; /* list of threads waiting -
                                   _must_ be first member */
#if CONFIG_CORELOCK == SW_CORELOCK
    struct corelock cl;         /* lock for atomic list operations */
#endif
};

/* Information kept in each thread slot
 * members are arranged according to size - largest first - in order
 * to ensure both alignment and packing at the same time.
 */
struct thread_entry
{
    struct regs context;       /* Register context at switch -
                                  _must_ be first member */
    void *stack;               /* Pointer to top of stack */
    const char *name;          /* Thread name */
    long tmo_tick;             /* Tick when thread should be woken from
                                  timeout */
    struct thread_list l;      /* Links for blocked/waking/running -
                                  circular linkage in both directions */
    struct thread_list tmo;    /* Links for timeout list -
                                  Self-pointer-terminated in reverse direction,
                                  NULL-terminated in forward direction */
    struct thread_queue *bqp;  /* Pointer to list variable in kernel
                                  object where thread is blocked - used
                                  for implicit unblock and explicit wake */
#if CONFIG_CORELOCK == SW_CORELOCK
    struct thread_entry **bqnlp; /* Pointer to list variable in kernel
                                    object where thread is blocked - non-locked
                                    operations will be used */
#endif
    struct thread_entry *queue; /* List of threads waiting for thread to be
                                  removed */
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    intptr_t retval;           /* Return value from a blocked operation */
#endif
#ifdef HAVE_PRIORITY_SCHEDULING
    long last_run;             /* Last tick when started */
#endif
    unsigned short stack_size; /* Size of stack in bytes */
#ifdef HAVE_PRIORITY_SCHEDULING
    unsigned char priority;    /* Current priority */
    unsigned char priority_x;  /* Inherited priority - right now just a
                                  runtime guarantee flag */
#endif
    unsigned char state;       /* Thread slot state (STATE_*) */
#if NUM_CORES > 1
    unsigned char core;        /* The core to which thread belongs */
#endif
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    unsigned char boosted;     /* CPU frequency boost flag */
#endif
#if CONFIG_CORELOCK == SW_CORELOCK
    struct corelock cl;        /* Corelock to lock thread slot */
#endif
};

#if NUM_CORES > 1
/* Operations to be performed just before stopping a thread and starting
   a new one if specified before calling switch_thread */
#define TBOP_UNLOCK_LIST     0x01 /* Set a pointer variable address var_ptrp */
#if CONFIG_CORELOCK == CORELOCK_SWAP
#define TBOP_SET_VARi        0x02 /* Set an int at address var_ip */
#define TBOP_SET_VARu8       0x03 /* Set an unsigned char at address var_u8p */
#define TBOP_VAR_TYPE_MASK   0x03 /* Mask for variable type*/
#endif /* CONFIG_CORELOCK */
#define TBOP_UNLOCK_CORELOCK 0x04
#define TBOP_UNLOCK_THREAD   0x08 /* Unlock a thread's slot */
#define TBOP_UNLOCK_CURRENT  0x10 /* Unlock the current thread's slot */
#define TBOP_IRQ_LEVEL       0x20 /* Set a new irq level */
#define TBOP_SWITCH_CORE     0x40 /* Call the core switch preparation routine */

struct thread_blk_ops
{
    int irq_level;                    /* new IRQ level to set */
#if CONFIG_CORELOCK != SW_CORELOCK
    union
    {
        int var_iv;                   /* int variable value to set */
        uint8_t var_u8v;              /* unsigned char valur to set */
        struct thread_entry *list_v;  /* list pointer queue value to set */
    };
#endif
    union
    {
#if CONFIG_CORELOCK != SW_CORELOCK
        int *var_ip;                  /* pointer to int variable */
        uint8_t *var_u8p;             /* pointer to unsigned char varuable */
#endif
        struct thread_queue *list_p;  /* pointer to list variable */
    };
#if CONFIG_CORELOCK == SW_CORELOCK
    struct corelock *cl_p;            /* corelock to unlock */
    struct thread_entry *thread;      /* thread to unlock */
#elif CONFIG_CORELOCK == CORELOCK_SWAP
    unsigned char state;              /* new thread state (performs unlock) */
#endif /* SOFTWARE_CORELOCK */
    unsigned char flags;    /* TBOP_* flags */
};
#endif /* NUM_CORES > 1 */

/* Information kept for each core
 * Member are arranged for the same reason as in thread_entry
 */
struct core_entry
{
    /* "Active" lists - core is constantly active on these and are never
       locked and interrupts do not access them */
    struct thread_entry *running;  /* threads that are running */
    struct thread_entry *timeout;  /* threads that are on a timeout before
                                      running again */
    /* "Shared" lists - cores interact in a synchronized manner - access
       is locked between cores and interrupts */
    struct thread_queue  waking;   /* intermediate locked list that
                                      hold threads other core should wake up
                                      on next task switch */
    long next_tmo_check;           /* soonest time to check tmo threads */
#if NUM_CORES > 1
    struct thread_blk_ops blk_ops; /* operations to perform when
                                      blocking a thread */
#else
    #define STAY_IRQ_LEVEL (-1)
    int irq_level;                 /* sets the irq level to irq_level */
#endif /* NUM_CORES */
#ifdef HAVE_PRIORITY_SCHEDULING
    unsigned char highest_priority;
#endif
};

#ifdef HAVE_PRIORITY_SCHEDULING
#define IF_PRIO(...)    __VA_ARGS__
#else
#define IF_PRIO(...)
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

#define CREATE_THREAD_FROZEN   0x00000001 /* Thread is frozen at create time */
struct thread_entry*
    create_thread(void (*function)(void), void* stack, int stack_size,
                  unsigned flags, const char *name
                  IF_PRIO(, int priority)
		          IF_COP(, unsigned int core));

#ifdef HAVE_SCHEDULER_BOOSTCTRL
void trigger_cpu_boost(void);
void cancel_cpu_boost(void);
#else
#define trigger_cpu_boost()
#define cancel_cpu_boost()
#endif
void thread_thaw(struct thread_entry *thread);
void thread_wait(struct thread_entry *thread);
void remove_thread(struct thread_entry *thread);
void switch_thread(struct thread_entry *old);
void sleep_thread(int ticks);

/**
 * Setup to allow using thread queues as locked or non-locked without speed
 * sacrifices in both core locking types.
 *
 * The blocking/waking function inline two different version of the real
 * function into the stubs when a software or other separate core locking
 * mechanism is employed.
 *
 * When a simple test-and-set or similar instruction is available, locking
 * has no cost and so one version is used and the internal worker is called
 * directly.
 *
 * CORELOCK_NONE is treated the same as when an atomic instruction can be
 * used.
 */

/* Blocks the current thread on a thread queue */
#if CONFIG_CORELOCK == SW_CORELOCK
void block_thread(struct thread_queue *tq);
void block_thread_no_listlock(struct thread_entry **list);
#else
void _block_thread(struct thread_queue *tq);
static inline void block_thread(struct thread_queue *tq)
    { _block_thread(tq); }
static inline void block_thread_no_listlock(struct thread_entry **list)
    { _block_thread((struct thread_queue *)list); }
#endif /* CONFIG_CORELOCK */

/* Blocks the current thread on a thread queue for a max amount of time
 * There is no "_no_listlock" version because timeout blocks without sync on
 * the blocking queues is not permitted since either core could access the
 * list at any time to do an implicit wake. In other words, objects with
 * timeout support require lockable queues. */
void block_thread_w_tmo(struct thread_queue *tq, int timeout);

/* Wakes up the thread at the head of the queue */
#define THREAD_WAKEUP_NONE    ((struct thread_entry *)NULL)
#define THREAD_WAKEUP_MISSING ((struct thread_entry *)(NULL+1))
#if CONFIG_CORELOCK == SW_CORELOCK
struct thread_entry * wakeup_thread(struct thread_queue *tq);
struct thread_entry * wakeup_thread_no_listlock(struct thread_entry **list);
#else
struct thread_entry * _wakeup_thread(struct thread_queue *list);
static inline struct thread_entry * wakeup_thread(struct thread_queue *tq)
    { return _wakeup_thread(tq); }
static inline struct thread_entry * wakeup_thread_no_listlock(struct thread_entry **list)
    { return _wakeup_thread((struct thread_queue *)list); }
#endif /* CONFIG_CORELOCK */

/* Initialize a thread_queue object. */
static inline void thread_queue_init(struct thread_queue *tq)
    { tq->queue = NULL; IF_SWCL(corelock_init(&tq->cl);) }
/* A convenience function for waking an entire queue of threads. */
static inline void thread_queue_wake(struct thread_queue *tq)
    { while (wakeup_thread(tq) != NULL); }
/* The no-listlock version of thread_queue_wake() */
static inline void thread_queue_wake_no_listlock(struct thread_entry **list)
    { while (wakeup_thread_no_listlock(list) != NULL); }

#ifdef HAVE_PRIORITY_SCHEDULING
int thread_set_priority(struct thread_entry *thread, int priority);
int thread_get_priority(struct thread_entry *thread);
/* Yield that guarantees thread execution once per round regardless of
   thread's scheduler priority - basically a transient realtime boost
   without altering the scheduler's thread precedence. */
void priority_yield(void);
#else
#define priority_yield  yield
#endif /* HAVE_PRIORITY_SCHEDULING */
#if NUM_CORES > 1
unsigned int switch_core(unsigned int new_core);
#endif
struct thread_entry * thread_get_current(void);
void init_threads(void);
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
