#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include "/usr/include/semaphore.h"
#include "thread-internal.h"
#include "kernel.h"

#define NSEC_PER_SEC 1000000000L
static inline void timespec_add_ns(struct timespec *a, uint64_t ns)
{
    lldiv_t q = lldiv(a->tv_nsec + ns, NSEC_PER_SEC);
    a->tv_sec += q.quot;
    a->tv_nsec = q.rem;
}

static int threads_initialized;

struct thread_init_data {
    void (*function)(void);
    bool start_frozen;
    sem_t init_sem;
    struct thread_entry *entry;
};

__thread struct thread_entry *_current;

unsigned int thread_self(void)
{
    return (unsigned) pthread_self();
}

static struct thread_entry_item {
    unsigned thread_id;
    struct thread_entry *entry;
} entry_lookup[32];



static struct thread_entry_item *__find_thread_entry(unsigned thread_id)
{
    int i;
    
    for (i = 0; i < 32; i++)
    {
        if (entry_lookup[i].thread_id == thread_id)
            return &entry_lookup[i];
    }
    return NULL;
}

static struct thread_entry *find_thread_entry(unsigned thread_id)
{
    return __find_thread_entry(thread_id)->entry;
}

static void *trampoline(void *arg)
{
    struct thread_init_data *data = arg;

    void (*thread_fn)(void) = data->function;

    _current = data->entry;

    if (data->start_frozen)
    {
        struct corelock thaw_lock;
        corelock_init(&thaw_lock);
        corelock_lock(&thaw_lock);

        _current->lock = &thaw_lock;
        sem_post(&data->init_sem);
        block_thread_switch(_current, _current->lock);
        _current->lock = NULL;

        corelock_unlock(&thaw_lock);
    }
    else
        sem_post(&data->init_sem);

    free(data);
    thread_fn();

    return NULL;
}

void thread_thaw(unsigned int thread_id)
{
    struct thread_entry *e = find_thread_entry(thread_id);
    if (e->lock)
    {
        corelock_lock(e->lock);
        wakeup_thread(e);
        corelock_unlock(e->lock);
    }
    /* else: no lock. must be running already */
}

void init_threads(void)
{
    struct thread_entry_item *item0 = &entry_lookup[0];
    item0->entry = calloc(1, sizeof(struct thread_entry));
    item0->thread_id = pthread_self();

    _current = item0->entry;
    pthread_cond_init(&item0->entry->cond, NULL);
    threads_initialized = 1;
}


unsigned int create_thread(void (*function)(void),
                           void* stack, size_t stack_size,
                           unsigned flags, const char *name
                           //~ IF_PRIO(, int priority)
                           IF_COP(, unsigned int core))
{
    pthread_t retval;

    struct thread_init_data *data = calloc(1, sizeof(struct thread_init_data));
    struct thread_entry *entry = calloc(1, sizeof(struct thread_entry));
    struct thread_entry_item *item;

    if (!threads_initialized)
        abort();

    data->function = function;
    data->start_frozen = flags & CREATE_THREAD_FROZEN;
    data->entry = entry;
    pthread_cond_init(&entry->cond, NULL);         
    entry->runnable = true;

    sem_init(&data->init_sem, 0, 0);

    if (pthread_create(&retval, NULL, trampoline, data) < 0)
        return -1;

    sem_wait(&data->init_sem);

    item = __find_thread_entry(0);
    item->thread_id = retval;
    item->entry = entry;

    pthread_setname_np(retval, name);
    

    return retval;
}

/* for block_thread(), _w_tmp() and wakeup_thread() t->lock must point
 * to a corelock instance, and this corelock must be held by the caller */
void block_thread_switch(struct thread_entry *t, struct corelock *cl)
{
    t->runnable = false;
    if (wait_queue_ptr(t))
        wait_queue_register(t);
    while(!t->runnable)
        pthread_cond_wait(&t->cond, &cl->mutex);
}

void block_thread_switch_w_tmo(struct thread_entry *t, int timeout,
                               struct corelock *cl)
{
    int err = 0;
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    timespec_add_ns(&ts, timeout * (NSEC_PER_SEC/HZ));

    t->runnable = false;
    wait_queue_register(t->wqp, t);
    while(!t->runnable && !err)
        err = pthread_cond_timedwait(&t->cond, &cl->mutex, &ts);

    if (err == ETIMEDOUT)
    {   /* the thread timed out and was not explicitely woken up.
         * we need to do this now to mark it runnable again */
        t->runnable = true;
        /* NOTE: objects do their own removal upon timer expiration */
    }
}

unsigned int wakeup_thread(struct thread_entry *t)
{
    if (t->wqp)
        wait_queue_remove(t);
    t->runnable = true;
    pthread_cond_signal(&t->cond);
    return THREAD_OK;
}


void yield(void) {}

unsigned sleep(unsigned ticks)
{
    struct timespec ts;

    ts.tv_sec = ticks/HZ;
    ts.tv_nsec = (ticks % HZ) * (NSEC_PER_SEC/HZ);

    nanosleep(&ts, NULL);

    return 0;
}
