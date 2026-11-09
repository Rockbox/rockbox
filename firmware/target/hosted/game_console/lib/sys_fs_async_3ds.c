/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

/* See sys_fs_async_3ds.h */

#include <3ds.h>
#include <3ds/archive.h>

#define SYS_FS_ASYNC_IMPL
#include "sys_fs_async_3ds.h"

#include "config.h"
#include "system.h"
#include "kernel.h"
#include "debug.h"

enum fs_op
{
    OP_OPEN_FILE,
    OP_OPEN_DIR,
    OP_DELETE_FILE,
    OP_DELETE_DIR,
    OP_CREATE_DIR,
    OP_RENAME_FILE,
    OP_RENAME_DIR,
    OP_ARCHIVE_RESOURCE,
    OP_FILE_READ,
    OP_FILE_WRITE,
    OP_FILE_GET_SIZE,
    OP_FILE_SET_SIZE,
    OP_FILE_CLOSE,
    OP_FILE_FLUSH,
    OP_DIR_READ,
    OP_DIR_CLOSE,
    OP_ARCHIVE_GETMTIME,
};

/* Arguments live on the requesting thread's stack, which stays valid
 * while it is blocked waiting for the result */
struct fs_request
{
    enum fs_op  op;
    Handle      handle;
    Handle     *handle_out;
    FS_Archive  archive, archive2;
    FS_Path     path, path2;
    const char *name;
    u32         u0, u1;
    u32        *u32_out;
    u64         offset;
    u64        *u64_out;
    void       *buffer;
    const void *cbuffer;
    void       *out;
    Result      res;
};

#define FS_WORKER_STACK_SIZE  (16 * 1024)

extern s32 main_thread_priority; /* system-3ds.c, from svcGetThreadPriority */

static struct mutex       fs_req_mtx;   /* one request in flight */
static struct semaphore   fs_req_done;  /* released by the worker */
static struct fs_request *volatile fs_pending;
static LightEvent         fs_req_event; /* wakes the worker */
static Thread             fs_worker;
static bool               fs_async_ready = false;

static Result fs_execute(struct fs_request *r)
{
    switch (r->op)
    {
    case OP_OPEN_FILE:
        return FSUSER_OpenFile(r->handle_out, r->archive, r->path, r->u0, r->u1);
    case OP_OPEN_DIR:
        return FSUSER_OpenDirectory(r->handle_out, r->archive, r->path);
    case OP_DELETE_FILE:
        return FSUSER_DeleteFile(r->archive, r->path);
    case OP_DELETE_DIR:
        return FSUSER_DeleteDirectory(r->archive, r->path);
    case OP_CREATE_DIR:
        return FSUSER_CreateDirectory(r->archive, r->path, r->u0);
    case OP_RENAME_FILE:
        return FSUSER_RenameFile(r->archive, r->path, r->archive2, r->path2);
    case OP_RENAME_DIR:
        return FSUSER_RenameDirectory(r->archive, r->path, r->archive2, r->path2);
    case OP_ARCHIVE_RESOURCE:
        return FSUSER_GetSdmcArchiveResource(r->out);
    case OP_FILE_READ:
        return FSFILE_Read(r->handle, r->u32_out, r->offset, r->buffer, r->u0);
    case OP_FILE_WRITE:
        return FSFILE_Write(r->handle, r->u32_out, r->offset, r->cbuffer, r->u0, r->u1);
    case OP_FILE_GET_SIZE:
        return FSFILE_GetSize(r->handle, r->u64_out);
    case OP_FILE_SET_SIZE:
        return FSFILE_SetSize(r->handle, r->offset);
    case OP_FILE_CLOSE:
        return FSFILE_Close(r->handle);
    case OP_FILE_FLUSH:
        return FSFILE_Flush(r->handle);
    case OP_DIR_READ:
        return FSDIR_Read(r->handle, r->u32_out, r->u0, r->out);
    case OP_DIR_CLOSE:
        return FSDIR_Close(r->handle);
    case OP_ARCHIVE_GETMTIME:
        return archive_getmtime(r->name, r->u64_out);
    }
    return -1;
}

static void fs_worker_run(void *arg)
{
    (void)arg;

    while (1)
    {
        LightEvent_Wait(&fs_req_event);

        struct fs_request *r = fs_pending;
        if (r == NULL)
            continue;

        r->res = fs_execute(r);
        fs_pending = NULL;

        /* Completion "interrupt": wake the waiting Rockbox thread */
        sim_enter_irq_handler();
        semaphore_release(&fs_req_done);
        sim_exit_irq_handler();
    }
}

void sys_fs_async_init(void)
{
    if (fs_async_ready)
        return;

    mutex_init(&fs_req_mtx);
    semaphore_init(&fs_req_done, 1, 0);
    LightEvent_Init(&fs_req_event, RESET_ONESHOT);

    /* Slightly higher priority than the main thread so completions are
     * handled promptly (libctru: userland range is [0x18;0x3F]) */
    int prio = main_thread_priority - 1;
    if (prio < 0x18)
        prio = 0x18;

    /* App core (-2: exheader default). The worker only waits on FS IPC, so
     * it needs no CPU time of its own. Core 1 (syscore) would require
     * APT_SetAppCpuTimeLimit(), which caps app threads there to a fraction
     * of the core's time and delays every request. */
    int core = -2;
    fs_worker = threadCreate(fs_worker_run, NULL, FS_WORKER_STACK_SIZE,
                             prio, core, true);

    if (fs_worker == NULL)
    {
        DEBUGF("sys_fs_async_init: worker creation failed, using sync FS\n");
        return;
    }

    DEBUGF("sys_fs_async_init: worker on core %d, priority 0x%x\n", core, prio);
    fs_async_ready = true;
}

static Result fs_call(struct fs_request *r)
{
    if (!fs_async_ready)
        return fs_execute(r);

    mutex_lock(&fs_req_mtx);
    fs_pending = r;
    LightEvent_Signal(&fs_req_event);
    semaphore_wait(&fs_req_done, TIMEOUT_BLOCK);
    mutex_unlock(&fs_req_mtx);

    return r->res;
}

Result ctru_FSUSER_OpenFile(Handle* out, FS_Archive archive, FS_Path path, u32 openFlags, u32 attributes)
{
    struct fs_request r = { .op = OP_OPEN_FILE, .handle_out = out, .archive = archive,
                            .path = path, .u0 = openFlags, .u1 = attributes };
    return fs_call(&r);
}

Result ctru_FSUSER_OpenDirectory(Handle *out, FS_Archive archive, FS_Path path)
{
    struct fs_request r = { .op = OP_OPEN_DIR, .handle_out = out, .archive = archive,
                            .path = path };
    return fs_call(&r);
}

Result ctru_FSUSER_DeleteFile(FS_Archive archive, FS_Path path)
{
    struct fs_request r = { .op = OP_DELETE_FILE, .archive = archive, .path = path };
    return fs_call(&r);
}

Result ctru_FSUSER_DeleteDirectory(FS_Archive archive, FS_Path path)
{
    struct fs_request r = { .op = OP_DELETE_DIR, .archive = archive, .path = path };
    return fs_call(&r);
}

Result ctru_FSUSER_CreateDirectory(FS_Archive archive, FS_Path path, u32 attributes)
{
    struct fs_request r = { .op = OP_CREATE_DIR, .archive = archive, .path = path,
                            .u0 = attributes };
    return fs_call(&r);
}

Result ctru_FSUSER_RenameFile(FS_Archive srcArchive, FS_Path srcPath, FS_Archive dstArchive, FS_Path dstPath)
{
    struct fs_request r = { .op = OP_RENAME_FILE, .archive = srcArchive, .path = srcPath,
                            .archive2 = dstArchive, .path2 = dstPath };
    return fs_call(&r);
}

Result ctru_FSUSER_RenameDirectory(FS_Archive srcArchive, FS_Path srcPath, FS_Archive dstArchive, FS_Path dstPath)
{
    struct fs_request r = { .op = OP_RENAME_DIR, .archive = srcArchive, .path = srcPath,
                            .archive2 = dstArchive, .path2 = dstPath };
    return fs_call(&r);
}

Result ctru_FSUSER_GetSdmcArchiveResource(FS_ArchiveResource* archiveResource)
{
    struct fs_request r = { .op = OP_ARCHIVE_RESOURCE, .out = archiveResource };
    return fs_call(&r);
}

Result ctru_FSFILE_Read(Handle handle, u32* bytesRead, u64 offset, void* buffer, u32 size)
{
    struct fs_request r = { .op = OP_FILE_READ, .handle = handle, .u32_out = bytesRead,
                            .offset = offset, .buffer = buffer, .u0 = size };
    return fs_call(&r);
}

Result ctru_FSFILE_Write(Handle handle, u32* bytesWritten, u64 offset, const void* buffer, u32 size, u32 flags)
{
    struct fs_request r = { .op = OP_FILE_WRITE, .handle = handle, .u32_out = bytesWritten,
                            .offset = offset, .cbuffer = buffer, .u0 = size, .u1 = flags };
    return fs_call(&r);
}

Result ctru_FSFILE_GetSize(Handle handle, u64* size)
{
    struct fs_request r = { .op = OP_FILE_GET_SIZE, .handle = handle, .u64_out = size };
    return fs_call(&r);
}

Result ctru_FSFILE_SetSize(Handle handle, u64 size)
{
    struct fs_request r = { .op = OP_FILE_SET_SIZE, .handle = handle, .offset = size };
    return fs_call(&r);
}

Result ctru_FSFILE_Close(Handle handle)
{
    struct fs_request r = { .op = OP_FILE_CLOSE, .handle = handle };
    return fs_call(&r);
}

Result ctru_FSFILE_Flush(Handle handle)
{
    struct fs_request r = { .op = OP_FILE_FLUSH, .handle = handle };
    return fs_call(&r);
}

Result ctru_FSDIR_Read(Handle handle, u32* entriesRead, u32 entryCount, FS_DirectoryEntry* entries)
{
    struct fs_request r = { .op = OP_DIR_READ, .handle = handle, .u32_out = entriesRead,
                            .u0 = entryCount, .out = entries };
    return fs_call(&r);
}

Result ctru_FSDIR_Close(Handle handle)
{
    struct fs_request r = { .op = OP_DIR_CLOSE, .handle = handle };
    return fs_call(&r);
}

Result ctru_archive_getmtime(const char *name, u64 *mtime)
{
    struct fs_request r = { .op = OP_ARCHIVE_GETMTIME, .name = name, .u64_out = mtime };
    return fs_call(&r);
}
