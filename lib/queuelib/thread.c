
#include <stdbool.h>
#include <pthread.h>
#include "thread.h"
#include "locking.h"

static int threads_initialized;

struct thread_init_data {
    void (*function)(void);
    bool start_frozen;
    struct corelock start_frozen_lock;
    struct thread_entry *entry;
};

struct thread_entry __thread *_current;


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
        corelock_init(&data->start_frozen_lock);
        corelock_lock(&data->start_frozen_lock);

        _current->lock = &data->start_frozen_lock;
        block_thread(_current);
        _current->lock = NULL;

        corelock_unlock(&data->start_frozen_lock);
    }

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

    if (pthread_create(&retval, NULL, trampoline, data) < 0)
        return -1;

    item = __find_thread_entry(0);
    item->thread_id = retval;
    item->entry = entry;

    pthread_setname_np(retval, name);
    

    return retval;
}

void yield(void) {}

void sleep(int ticks)
{
    struct timespec ts;

    ts.tv_sec = ticks/HZ;
    ts.tv_nsec = (ticks % HZ) * (NSEC_PER_SEC/HZ);

    nanosleep(&ts, NULL);
}
