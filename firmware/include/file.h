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

#if !defined(PLUGIN) && !defined(CODEC)

#if defined(APPLICATION) && !defined(__PCTOOL__)
#include "rbpaths.h"
#  define open(path, mode...)           app_open(path, mode)
#  define creat(path, mode)             app_creat(path, mode)
#  define remove(path)                  app_remove(path)
#  define rename(old, new)              app_rename(old, new)
#  define readlink(path, buf, bufsize)  app_readlink(path, buf, bufsize)
#if (CONFIG_PLATFORM & (PLATFORM_SDL|PLATFORM_MAEMO|PLATFORM_PANDORA))
/* SDL overrides a few more */
#  define read(fildes, buf, nbyte)      sim_read(fildes, buf, nbyte)
#  define write(fildes, buf, nbyte)     sim_write(fildes, buf, nbyte)
#endif
#elif defined(SIMULATOR) || defined(DBTOOL)
#  define open(path, mode...)           sim_open(path, mode)
#  define creat(path, mode)             sim_creat(path, mode)
#  define close(fildes)                 sim_close(fildes)
#  define ftruncate(fildes, length)     sim_ftruncate(fildes, length)
#  define fsync(fildes)                 sim_fsync(fildes)
#  define lseek(fildes, offset, whench) sim_lseek(fildes, offset, whence)
#  define read(fildes, buf, nbyte)      sim_read(fildes, buf, nbyte)
#  define write(fildes, buf, nbyte)     sim_write(fildes, buf, nbyte)
#  define remove(path)                  sim_remove(path)
#  define rename(old, new)              sim_rename(old, new)

/* readlink() not used in the sim yet */
int sim_open(const char *name, int o, ...);
int sim_creat(const char *name, mode_t mode);

#else /* Native */

/** POSIX **/
int open(const char *path, int oflag);
int creat(const char *path);
int close(int fildes);
int ftruncate(int fildes, off_t length);
int fsync(int fildes);
off_t lseek(int fildes, off_t offset, int whence);
ssize_t read(int fildes, void *buf, size_t nbyte);
ssize_t write(int fildes, const void *buf, size_t nbyte);
int remove(const char *path);
int rename(const char *old, const char *new);

/** Extensions **/
off_t filesize(int fildes);
int fsamefile(int fildes1, int fildes2);
int fdprintf(int fildes, const char *fmt, ...) ATTRIBUTE_PRINTF(2, 3);

#ifndef _FILE_C_
#  define open(path, oflag, mode...)  open(path, oflag)
#  define creat(path, mode)           creat(path)
#endif /* _FILE_C_ */

#endif /* CONFIG_PLATFORM */

#endif /* !CODEC && !PLUGIN */

#endif /* _FILE_H_ */
