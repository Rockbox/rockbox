#include <pthread.h>
#include "kernel.h"

void corelock_init(struct corelock *lk)
{
    lk->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
}

void corelock_lock(struct corelock *lk)
{
    pthread_mutex_lock(&lk->mutex);
}


void corelock_unlock(struct corelock *lk)
{
    pthread_mutex_unlock(&lk->mutex);
}
