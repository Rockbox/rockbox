#include <pthread.h>
#include "kernel.h"

void mutex_init(struct mutex *m)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

void mutex_lock(struct mutex *m)
{
    pthread_mutex_lock(&m->mutex);
}

void mutex_unlock(struct mutex *m)
{
    pthread_mutex_unlock(&m->mutex);
}
