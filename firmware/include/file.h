/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
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

#ifndef _FILE_H_
#define _FILE_H_

#include <sys/types.h>

#undef MAX_PATH /* this avoids problems when building simulator */
#define MAX_PATH 260

#define MAX_OPEN_FILES 11

#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

#ifndef O_RDONLY
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  4
#define O_APPEND 8
#define O_TRUNC  0x10
#endif

#ifdef SIMULATOR
#define open(x,y) sim_open(x,y)
#define creat(x) sim_creat(x)
#define remove(x) sim_remove(x)
#define rename(x,y) sim_rename(x,y)
#define filesize(x) sim_filesize(x)
#define fsync(x) sim_fsync(x)
#define ftruncate(x,y) sim_ftruncate(x,y)
#define lseek(x,y,z) sim_lseek(x,y,z)
#define read(x,y,z) sim_read(x,y,z)
#define write(x,y,z) sim_write(x,y,z)
#define close(x) sim_close(x)
#endif

typedef int (*open_func)(const char* pathname, int flags);
typedef ssize_t (*read_func)(int fd, void *buf, size_t count);
typedef int (*creat_func)(const char *pathname);
typedef ssize_t (*write_func)(int fd, const void *buf, size_t count);
typedef void (*qsort_func)(void *base, size_t nmemb,  size_t size,
                           int(*_compar)(const void *, const void *));

extern int open(const char* pathname, int flags);
extern int close(int fd);
extern int fsync(int fd);
extern ssize_t read(int fd, void *buf, size_t count);
extern off_t lseek(int fildes, off_t offset, int whence);
extern int creat(const char *pathname);
extern ssize_t write(int fd, const void *buf, size_t count);
extern int remove(const char* pathname);
extern int rename(const char* path, const char* newname);
extern int ftruncate(int fd, off_t length);
extern off_t filesize(int fd);
extern int release_files(int volume);

#endif
