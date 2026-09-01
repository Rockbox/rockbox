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
#ifndef _FILESYSTEM_CONSOLE_H_
#define _FILESYSTEM_CONSOLE_H_

#if defined(PLUGIN) || defined(CODEC)
#define FILEFUNCTIONS_DECLARED
#define FILEFUNCTIONS_DEFINED
#define DIRFUNCTIONS_DECLARED
#define DIRFUNCTIONS_DEFINED
#define OSFUNCTIONS_DECLARED
#endif /* PLUGIN || CODEC */

#ifndef OSFUNCTIONS_DECLARED
#define FS_PREFIX(_x_) console_ ## _x_

void paths_init(void);
#endif /* !OSFUNCTIONS_DECLARED */
#endif /* _FILESYSTEM_CONSOLE_H_ */

#ifdef _FILE_H_
#include <unistd.h>

#ifndef _FILESYSTEM_CONSOLE__FILE_H_
#define _FILESYSTEM_CONSOLE__FILE_H_

#ifdef RB_FILESYSTEM_OS
#define FILEFUNCTIONS_DEFINED
#endif

#ifndef FILEFUNCTIONS_DECLARED
#define __OPEN_MODE_ARG
#define __CREAT_MODE_ARG \
    , mode

#include <time.h>

int     console_open(const char *name, int oflag, ...);
int     console_creat(const char *name, mode_t mode);
int     console_close(int fildes);
int     console_ftruncate(int fildes, off_t length);
int     console_fsync(int fildes);
off_t   console_lseek(int fildes, off_t offset, int whence);
ssize_t console_read(int fildes, void *buf, size_t nbyte);
ssize_t console_write(int fildes, const void *buf, size_t nbyte);
int     console_remove(const char *path);
int     console_rename(const char *old, const char *new);
int     console_modtime(const char *path, time_t modtime);
off_t   console_filesize(int fildes);
int     console_fsamefile(int fildes1, int fildes2);
int     console_relate(const char *path1, const char *path2);
bool    console_file_exists(const char *path);
ssize_t console_readlink(const char *path, char *buf, size_t bufsiz);

#endif /* !FILEFUNCTIONS_DECLARED */

#endif /* _FILESYSTEM_CONSOLE__FILE_H_ */
#endif /* _FILE_H_ */

#ifdef _DIR_H_
#ifndef _FILESYSTEM_CONSOLE__DIR_H_
#define _FILESYSTEM_CONSOLE__DIR_H_

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
#define __MKDIR_MODE_ARG \
    , 0777

#ifdef RB_FILESYSTEM_OS
#define DIRFUNCTIONS_DEFINED
#endif

DIR *  console_opendir(const char *dirname);
struct dirent * console_readdir(DIR *dirp);
int    console_readdir_r(DIR *dirp, struct dirent *entry,
                 struct dirent **result);
void   console_rewinddir(DIR *dirp);
int    console_closedir(DIR *dirp);
int    console_mkdir(const char *path);
int    console_rmdir(const char *path);
int    console_samedir(DIR *dirp1, DIR *dirp2);
bool   console_dir_exists(const char *dirname);
#endif /* !DIRFUNCTIONS_DECLARED */

#endif /* _FILESYSTEM_CONSOLE__DIR_H_ */
#endif /* _DIR_H_ */
