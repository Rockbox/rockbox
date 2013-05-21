#ifndef LOCKING_H
#define LOCKING_H

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include "thread.h"
 
#define NSEC_PER_SEC 1000000000L
static inline void timespec_add_ns(struct timespec *a, uint64_t ns)
{
    lldiv_t q = lldiv(a->tv_nsec + ns, NSEC_PER_SEC);
    a->tv_sec += q.quot;
    a->tv_nsec = q.rem;
}

#ifndef HZ
#define HZ 100
#endif

#ifndef IF_COP
#define IF_COP(...)         __VA_ARGS__
#endif

#define HAVE_CORELOCK_OBJECT

//~ #define NSEC_PER_SEC ((int)1e9)
struct corelock {
    pthread_mutex_t mutex;
};

static inline void corelock_init(struct corelock *lk)
{
    lk->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
}

static inline void corelock_lock(struct corelock *lk)
{
    pthread_mutex_lock(&lk->mutex);
}


static inline void corelock_unlock(struct corelock *lk)
{
    pthread_mutex_unlock(&lk->mutex);
}

/* for block_thread(), _w_tmp() and wakeup_thread() t->lock must point
 * to a corelock instance, and this corelock must be held by the caller */
static inline void block_thread(struct thread_entry *t)
{
    t->runnable = false;
    while(!t->runnable)
        pthread_cond_wait(&t->cond, &t->lock->mutex);
}

static inline void block_thread_w_tmo(struct thread_entry *t, long ticks)
{
    int err = 0;
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    timespec_add_ns(&ts, ticks * (NSEC_PER_SEC/HZ));

    t->runnable = false;
    while(!t->runnable && !err)
        err = pthread_cond_timedwait(&t->cond, &t->lock->mutex, &ts);
}

static inline void wakeup_thread(struct thread_entry *t)
{
    if (!t) return;

    t->runnable = true;
    pthread_cond_signal(&t->cond);
}



//~ #define disable_irq_save() 0
//~ #define restore_irq(x) ((void)x)

#endif
