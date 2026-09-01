/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#ifndef _SYS_FILE_INTERNAL_H
#define _SYS_FILE_INTERNAL_H

/* structure used for open file descriptors */
struct filestr_base;
struct filestr_desc
{
    struct filestr_base stream; /* basic stream info (first!) */
    file_size_t         offset; /* current offset for stream */
    u64                 *sizep; /* shortcut to file size in fileobj */
};

extern struct filestr_desc open_streams[MAX_OPEN_FILES];

int test_stream_exists_internal(const char *path);
int alloc_filestr(struct filestr_desc **filep);
struct filestr_desc * get_filestr(int fildes);
off_t lseek_internal(struct filestr_desc *file, off_t offset,
                            int whence);
ssize_t readwrite(struct filestr_desc *file, void *buf, size_t nbyte,
                         bool write);
void filestr_base_init(struct filestr_base *stream);
int open_internal_locked(const char *path, int oflag);

#define GET_FILESTR(type, fildes) \
    ({                                                     \
        file_internal_lock_##type();                       \
        struct filestr_desc * _file = get_filestr(fildes); \
        if (_file)                                         \
            FILESTR_LOCK(type, &_file->stream);            \
        else   {                                            \
            file_internal_unlock_##type();                 \
  }\
        _file;                                             \
    })

/* release the lock on the filestr_desc* */
#define RELEASE_FILESTR(type, file) \
    ({                                         \
        FILESTR_UNLOCK(type, &(file)->stream); \
        file_internal_unlock_##type();         \
    })
#endif /* _SYS_FILE_INTERNAL_H */

