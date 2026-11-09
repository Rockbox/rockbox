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

/*
 * Asynchronous libctru filesystem calls.
 *
 * Rockbox threads all run on the main libctru thread, so a blocking FS IPC
 * call would stall every Rockbox thread. Including this header (after the
 * libctru headers) redirects the FS calls to wrappers that hand the request
 * to a worker thread and block only the calling Rockbox thread on a kernel
 * semaphore, like waiting for a DMA completion interrupt on native targets.
 *
 * Before sys_fs_async_init() (e.g. from system_init) calls run synchronously.
 */

#ifndef _SYS_FS_ASYNC_3DS_H_
#define _SYS_FS_ASYNC_3DS_H_

#include <3ds/types.h>
#include <3ds/services/fs.h>

void sys_fs_async_init(void);

Result ctru_FSUSER_OpenFile(Handle* out, FS_Archive archive, FS_Path path, u32 openFlags, u32 attributes);
Result ctru_FSUSER_OpenDirectory(Handle *out, FS_Archive archive, FS_Path path);
Result ctru_FSUSER_DeleteFile(FS_Archive archive, FS_Path path);
Result ctru_FSUSER_DeleteDirectory(FS_Archive archive, FS_Path path);
Result ctru_FSUSER_CreateDirectory(FS_Archive archive, FS_Path path, u32 attributes);
Result ctru_FSUSER_RenameFile(FS_Archive srcArchive, FS_Path srcPath, FS_Archive dstArchive, FS_Path dstPath);
Result ctru_FSUSER_RenameDirectory(FS_Archive srcArchive, FS_Path srcPath, FS_Archive dstArchive, FS_Path dstPath);
Result ctru_FSUSER_GetSdmcArchiveResource(FS_ArchiveResource* archiveResource);
Result ctru_FSFILE_Read(Handle handle, u32* bytesRead, u64 offset, void* buffer, u32 size);
Result ctru_FSFILE_Write(Handle handle, u32* bytesWritten, u64 offset, const void* buffer, u32 size, u32 flags);
Result ctru_FSFILE_GetSize(Handle handle, u64* size);
Result ctru_FSFILE_SetSize(Handle handle, u64 size);
Result ctru_FSFILE_Close(Handle handle);
Result ctru_FSFILE_Flush(Handle handle);
Result ctru_FSDIR_Read(Handle handle, u32* entriesRead, u32 entryCount, FS_DirectoryEntry* entries);
Result ctru_FSDIR_Close(Handle handle);
Result ctru_archive_getmtime(const char *name, u64 *mtime); /* <3ds/archive.h> */

#ifndef SYS_FS_ASYNC_IMPL
#define FSUSER_OpenFile                 ctru_FSUSER_OpenFile
#define FSUSER_OpenDirectory            ctru_FSUSER_OpenDirectory
#define FSUSER_DeleteFile               ctru_FSUSER_DeleteFile
#define FSUSER_DeleteDirectory          ctru_FSUSER_DeleteDirectory
#define FSUSER_CreateDirectory          ctru_FSUSER_CreateDirectory
#define FSUSER_RenameFile               ctru_FSUSER_RenameFile
#define FSUSER_RenameDirectory          ctru_FSUSER_RenameDirectory
#define FSUSER_GetSdmcArchiveResource   ctru_FSUSER_GetSdmcArchiveResource
#define FSFILE_Read                     ctru_FSFILE_Read
#define FSFILE_Write                    ctru_FSFILE_Write
#define FSFILE_GetSize                  ctru_FSFILE_GetSize
#define FSFILE_SetSize                  ctru_FSFILE_SetSize
#define FSFILE_Close                    ctru_FSFILE_Close
#define FSFILE_Flush                    ctru_FSFILE_Flush
#define FSDIR_Read                      ctru_FSDIR_Read
#define FSDIR_Close                     ctru_FSDIR_Close
#define archive_getmtime                ctru_archive_getmtime
#endif /* SYS_FS_ASYNC_IMPL */

#endif /* _SYS_FS_ASYNC_3DS_H_ */
