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
#include "config.h"
#include "gcc_extensions.h"
#include <fcntl.h>
#ifdef WIN32
/* this has SEEK_SET et al */
#include <stdio.h>
#endif


#undef MAX_PATH /* this avoids problems when building simulator */
#define MAX_PATH 260
#define MAX_OPEN_FILES 11

#if !defined(PLUGIN) && !defined(CODEC)
#if defined(APPLICATION) && !defined(__PCTOOL__)
#   define open(x, ...)    app_open(x, __VA_ARGS__)
#   define creat(x,m)      app_creat(x, m)
#   define remove(x)       app_remove(x)
#   define rename(x,y)     app_rename(x,y)
extern int app_open(const char *name, int o, ...);
extern int app_creat(const char *name, mode_t mode);
extern int app_remove(const char* pathname);
extern int app_rename(const char* path, const char* newname);
#   if (CONFIG_PLATFORM & (PLATFORM_SDL|PLATFORM_MAEMO|PLATFORM_PANDORA))
#   define fsync(x) sim_fsync(x)
#   define ftruncate(x,y) sim_ftruncate(x,y)
#   define lseek(x,y,z) sim_lseek(x,y,z)
#   define read(x,y,z) sim_read(x,y,z)
#   define write(x,y,z) sim_write(x,y,z)
#   define close(x) sim_close(x)
#   endif
#elif defined(SIMULATOR)
#   define open(x, ...) sim_open(x, __VA_ARGS__)
#   define creat(x,m) sim_creat(x,m)
#   define remove(x) sim_remove(x)
#   define rename(x,y) sim_rename(x,y)
#   define fsync(x) sim_fsync(x)
#   define ftruncate(x,y) sim_ftruncate(x,y)
#   define lseek(x,y,z) sim_lseek(x,y,z)
#   define read(x,y,z) sim_read(x,y,z)
#   define write(x,y,z) sim_write(x,y,z)
#   define close(x) sim_close(x)
extern int sim_open(const char *name, int o, ...);
extern int sim_creat(const char *name, mode_t mode);
#endif

typedef int (*open_func)(const char* pathname, int flags, ...);
typedef ssize_t (*read_func)(int fd, void *buf, size_t count);
typedef int (*creat_func)(const char *pathname, mode_t mode);
typedef ssize_t (*write_func)(int fd, const void *buf, size_t count);
typedef void (*qsort_func)(void *base, size_t nmemb,  size_t size,
                           int(*_compar)(const void *, const void *));

extern int file_open(const char* pathname, int flags);
extern int close(int fd);
extern int fsync(int fd);
extern ssize_t read(int fd, void *buf, size_t count);
extern off_t lseek(int fildes, off_t offset, int whence);
extern int file_creat(const char *pathname);
#if ((CONFIG_PLATFORM & PLATFORM_NATIVE) && !defined(__PCTOOL__)) || \
    defined(TEST_FAT)
#define creat(x, y) file_creat(x)

#if !defined(CODEC) && !defined(PLUGIN)
#define open(x, y, ...) file_open(x,y)
#endif
#endif

extern ssize_t write(int fd, const void *buf, size_t count);
extern int remove(const char* pathname);
extern int rename(const char* path, const char* newname);
extern int ftruncate(int fd, off_t length);
extern off_t filesize(int fd);
extern int release_files(int volume);
int fdprintf (int fd, const char *fmt, ...) ATTRIBUTE_PRINTF(2, 3);
#endif /* !CODEC && !PLUGIN */
#endif
