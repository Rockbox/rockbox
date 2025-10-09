/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 by Mauricio G.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _SYS_FILE_H
#define _SYS_FILE_H

#include "bfile.h"

/* Include for file.h and dir.h because mkdir and friends may be here */
#include <sys/stat.h>

#define strlcpy_from_os strlcpy

#define AtomicLoad(ptr) __atomic_load_n((u32*)(ptr), __ATOMIC_SEQ_CST)
#define AtomicAdd(ptr, value) __atomic_add_fetch((u32*)(ptr), value, __ATOMIC_SEQ_CST)

struct filestr_base {
    Pager*      cache;            /* buffer IO implementation (cache) */
    Handle      handle;           /* file handle */
    u64         size;             /* file size */
    int         flags;            /* stream flags */
    char        path[MAX_PATH+1];

    LightLock   mtx;              /* serialization for this stream */
};

static inline void filestr_lock(struct filestr_base *stream)
{
    LightLock_Lock(&stream->mtx);
}

static inline void filestr_unlock(struct filestr_base *stream)
{
    LightLock_Unlock(&stream->mtx);
}

/* stream lock doesn't have to be used if getting RW lock writer access */
#define FILESTR_WRITER 0
#define FILESTR_READER 1

#define FILESTR_LOCK(type, stream) \
    ({ if (FILESTR_##type) filestr_lock(stream); })

#define FILESTR_UNLOCK(type, stream) \
    ({ if (FILESTR_##type) filestr_unlock(stream); })

/** Synchronization used throughout **/

/* acquire the filesystem lock as READER */
static inline void file_internal_lock_READER(void)
{
    extern sync_RWMutex file_internal_mrsw;
    sync_RWMutexRLock(&file_internal_mrsw);
}

/* release the filesystem lock as READER */
static inline void file_internal_unlock_READER(void)
{
    extern sync_RWMutex file_internal_mrsw;
    sync_RWMutexRUnlock(&file_internal_mrsw);
}

/* acquire the filesystem lock as WRITER */
static inline void file_internal_lock_WRITER(void)
{
    extern sync_RWMutex file_internal_mrsw;
    sync_RWMutexLock(&file_internal_mrsw);
}

/* release the filesystem lock as WRITER */
static inline void file_internal_unlock_WRITER(void)
{
    extern sync_RWMutex file_internal_mrsw;
    sync_RWMutexUnlock(&file_internal_mrsw);
}

#define ERRNO 0 /* maintain errno value */
#define _RC   0 /* maintain rc value */

/* NOTES: if _errno is a non-constant expression, it must set an error
 *        number and not return the ERRNO constant which will merely set
 *        errno to zero, not preserve the current value; if you must set
 *        errno to zero, set it explicitly, not in the macro
 *
 *        if _rc is constant-expression evaluation to 'RC', then rc will
 *        NOT be altered; i.e. if you must set rc to zero, set it explicitly,
 *        not in the macro
 */

#define FILE_SET_CODE(_name, _keepcode, _value) \
    ({  __builtin_constant_p(_value) ?                             \
            ({ if ((_value) != (_keepcode)) _name = (_value); }) : \
            ({ _name = (_value); }); })

/* set errno and rc and proceed to the "file_error:" label */
#define FILE_ERROR(_errno, _rc) \
    ({  FILE_SET_CODE(errno, ERRNO, (_errno)); \
        FILE_SET_CODE(rc, _RC, (_rc));          \
        goto file_error; })

/* set errno and return a value at the point of invocation */
#define FILE_ERROR_RETURN(_errno, _rc...) \
    ({ FILE_SET_CODE(errno, ERRNO, _errno); \
       return _rc; })

/* set errno and return code, no branching */
#define FILE_ERROR_SET(_errno, _rc) \
    ({ FILE_SET_CODE(errno, ERRNO, (_errno)); \
       FILE_SET_CODE(rc, _RC, (_rc)); })

#endif /* _SYS_FILE_H */

