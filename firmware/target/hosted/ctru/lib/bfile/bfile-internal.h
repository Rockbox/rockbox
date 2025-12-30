#ifndef _BFILE_INTERNAL_H_
#define _BFILE_INTERNAL_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>

#include <3ds/gfx.h>
#include <3ds/svc.h>
#include <3ds/types.h>
#include <3ds/thread.h>
#include <3ds/result.h>
#include <3ds/services/fs.h>
#include <3ds/synchronization.h>

/* #define MALLOC_DEBUG
#include "rmalloc/rmalloc.h" */

/* in go functions can return two values */
#define two_type_value(type1, type2, name1, name2, type_name)   \
typedef struct {                                                \
    type1 name1;                                                \
    type2 name2;                                                \
} type_name##_t;

/* single mutex implementation */
#define sync_Mutex  LightLock
static inline void sync_MutexInit(sync_Mutex* m) {
    LightLock_Init(m);
}

/* mutex unlock */
static inline void sync_MutexLock(sync_Mutex* m) {
    LightLock_Lock(m);
}

/* mutex lock */
static inline void sync_MutexUnlock(sync_Mutex* m) {
    LightLock_Unlock(m);
}

/* read_write mutex implementation */
typedef struct {
    sync_Mutex      shared;
    CondVar         reader_q;
    CondVar         writer_q;
    int             active_readers;
    int             active_writers;
    int             waiting_writers;
} sync_RWMutex;

void sync_RWMutexInit(sync_RWMutex *m);
void sync_RWMutexRLock(sync_RWMutex *m);
void sync_RWMutexRUnlock(sync_RWMutex *m);
void sync_RWMutexLock(sync_RWMutex *m);
void sync_RWMutexUnlock(sync_RWMutex *m);

/* declare a two type value with name 'n_err_t' */
two_type_value(int, const char*, n, err, int_error);
two_type_value(struct stat, const char*, fi, err, stat_error);
typedef const char* file_error_t;

typedef struct {
    Handle  file;
    s64     start;
    s64     end;
    s64     size;
    bool    isLastPage;

    u8      *buffer;
} PageReader;

#endif /* _BFILE_INTERNAL_H_ */
