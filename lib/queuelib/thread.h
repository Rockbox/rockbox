
#ifndef THREAD_H
#define THREAD_H

#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>

#define IF_COP(...) __VA_ARGS__
#define IF_PRIO(...)

struct corelock;

struct thread_entry {
    struct corelock *lock;
    pthread_cond_t   cond;
    bool             runnable;
    intptr_t         retval;
};

static inline unsigned int thread_self(void)
{
    return (unsigned) pthread_self();
}

static inline struct thread_entry *thread_self_entry(void)
{
    extern __thread struct thread_entry *_current;
    //~ static struct thread_entry entry;

    return _current;
}

void thread_thaw(unsigned int thread_id);

#define CREATE_THREAD_FROZEN   0x00000001 /* Thread is frozen at create time */
unsigned int create_thread(void (*function)(void),
                           void* stack, size_t stack_size,
                           unsigned flags, const char *name
                           IF_PRIO(, int priority)
                           IF_COP(, unsigned int core));

void init_threads(void);


#endif /* THREAD_H */
