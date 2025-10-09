#ifndef _BFILE_INTERNAL_H_
#define _BFILE_INTERNAL_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
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

#include "cslice.h"
#include "cmap.h"

#define nil           NULL

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

typedef struct page {
    s64           num;
    struct page*  prev;
    struct page*  next;
    u8*           data;
} page;

/* the two map types used by this library */
cmap_declare(page, s64, struct page*);
cmap_declare(bool, s64, bool);

typedef struct shard {
    sync_Mutex    mu;
    cmap(page)    pages;
    cmap(bool)    dirty;
    struct page*  head;
    struct page*  tail;
} shard;

typedef struct Pager {
    Handle        file;
    s64           pgsize;
    s64           pgmax;
    /* sync_RWMutex  mu; */
    s64           size;
    cslice(shard) shards;
} Pager;

#endif /* _BFILE_INTERNAL_H_ */
