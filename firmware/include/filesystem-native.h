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
#ifndef _FILESYSTEM_NATIVE_H_
#define _FILESYSTEM_NATIVE_H_

#if defined(PLUGIN) || defined(CODEC)
#define FILEFUNCTIONS_DECLARED
#define FILEFUNCTIONS_DEFINED
#define DIRFUNCTIONS_DECLARED
#define DIRFUNCTIONS_DEFINED
#endif /* PLUGIN || CODEC */

#define FS_PREFIX(_x_) _x_
#endif /* _FILESYSTEM_NATIVE_H_ */

#ifdef _FILE_H_
#ifndef _FILESYSTEM_NATIVE__FILE_H_
#define _FILESYSTEM_NATIVE__FILE_H_

#ifdef RB_FILESYSTEM_OS
#define FILEFUNCTIONS_DEFINED
#endif

#ifndef FILEFUNCTIONS_DECLARED
#define __OPEN_MODE_ARG
#define __CREAT_MODE_ARG

#include <time.h>

int     open(const char *name, int oflag);
int     creat(const char *name);
int     close(int fildes);
int     ftruncate(int fildes, off_t length);
int     fsync(int fildes);
off_t   lseek(int fildes, off_t offset, int whence);
ssize_t read(int fildes, void *buf, size_t nbyte);
ssize_t write(int fildes, const void *buf, size_t nbyte);
int     remove(const char *path);
int     rename(const char *old, const char *new);
int     modtime(const char *path, time_t modtime);
off_t   filesize(int fildes);
int     fsamefile(int fildes1, int fildes2);
int     relate(const char *path1, const char *path2);
bool    file_exists(const char *path);
#endif /* !FILEFUNCTIONS_DECLARED */

#if !defined(RB_FILESYSTEM_OS) && !defined (FILEFUNCTIONS_DEFINED)
#define open(path, oflag, ...) open(path, oflag)
#define creat(path, mode)      creat(path)
#endif /* FILEFUNCTIONS_DEFINED */

#endif /* _FILESYSTEM_NATIVE__FILE_H_ */
#endif /* _FILE_H_ */

#ifdef _DIR_H_
#ifndef _FILESYSTEM_NATIVE__DIR_H_
#define _FILESYSTEM_NATIVE__DIR_H_

#define DIRENT dirent
struct dirent;

struct dirinfo_native
{
    unsigned int attr;
    off_t        size;
    uint16_t     wrtdate;
    uint16_t     wrttime;
};

typedef struct {} DIR;

#ifndef DIRFUNCTIONS_DECLARED
#define __MKDIR_MODE_ARG

#ifdef RB_FILESYSTEM_OS
#define DIRFUNCTIONS_DEFINED
#endif

DIR *  opendir(const char *dirname);
struct dirent * readdir(DIR *dirp);
int    readdir_r(DIR *dirp, struct dirent *entry,
                 struct dirent **result);
void   rewinddir(DIR *dirp);
int    closedir(DIR *dirp);
int    mkdir(const char *path);
int    rmdir(const char *path);
int    samedir(DIR *dirp1, DIR *dirp2);
bool   dir_exists(const char *dirname);
#endif /* !DIRFUNCTIONS_DECLARED */

#endif /* _FILESYSTEM_NATIVE__DIR_H_ */
#endif /* _DIR_H_ */
