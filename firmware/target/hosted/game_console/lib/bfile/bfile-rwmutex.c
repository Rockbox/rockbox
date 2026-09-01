
#include "bfile.h"

/* Define LOGF_ENABLE to enable logf output in this file */
#if 0
#define LOGF_ENABLE
#endif
#include "logf.h"

/* rw mutex implementation */
void sync_RWMutexInit(sync_RWMutex *m) {
    sys_mutex_init(&m->shared);
    sys_cond_init(&m->reader_q);
    sys_cond_init(&m->writer_q);
    m->active_readers = 0;
    m->active_writers = 0;
    m->waiting_writers = 0;
}

static inline sysMutex *unique_lock(sysMutex *lk) {
    sys_mutex_lock(lk);
    return lk;
}

void sync_RWMutexRLock(sync_RWMutex *m) {
    sysMutex *lk = unique_lock(&m->shared);
    while (m->waiting_writers != 0) {
        sys_cond_wait(&m->reader_q, lk);
    }
    ++m->active_readers;
    sys_mutex_unlock(lk);
}

void sync_RWMutexRUnlock(sync_RWMutex *m) {
    sysMutex *lk = unique_lock(&m->shared);
    --m->active_readers;
    sys_mutex_unlock(lk);
    sys_cond_signal(&m->writer_q);
}

void sync_RWMutexLock(sync_RWMutex *m) {
    sysMutex *lk = unique_lock(&m->shared);
    ++m->waiting_writers;
    while ((m->active_readers != 0) || (m->active_writers != 0)) {
        sys_cond_wait(&m->writer_q, lk);
    }
    ++m->active_writers;
    sys_mutex_unlock(lk);
}

void sync_RWMutexUnlock(sync_RWMutex *m) {
    sysMutex *lk = unique_lock(&m->shared);
    --m->waiting_writers;
    --m->active_writers;
    if (m->waiting_writers > 0) {
        sys_cond_signal(&m->writer_q);
    } else {
        sys_cond_broadcast(&m->reader_q);
    }
    sys_mutex_unlock(lk);
}