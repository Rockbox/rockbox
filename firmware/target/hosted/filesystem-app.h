/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Michael Sevakis
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

#ifndef _FILESYSTEM_APP_H_
#define _FILESYSTEM_APP_H_

#if defined(PLUGIN) || defined(CODEC)
/* Prevent often-problematic plugin namespace pollution */
#define FILEFUNCTIONS_DECLARED
#define FILEFUNCTIONS_DEFINED
#define DIRFUNCTIONS_DECLARED
#define DIRFUNCTIONS_DEFINED
#define OSFUNCTIONS_DECLARED
#endif /* PLUGIN || CODEC */

/* flags for get_user_file_path() */
/* whether you need write access to that file/dir, especially true
 * for runtime generated files (config.cfg) */
#define NEED_WRITE          (1<<0)
/* file or directory? */
#define IS_FILE             (1<<1)

#ifndef OSFUNCTIONS_DECLARED
#define FS_PREFIX(_x_) app_ ## _x_

void paths_init(void);
const char * handle_special_dirs(const char *dir, unsigned flags,
                                 char *buf, const size_t bufsize);

#endif /* !OSFUNCTIONS_DECLARED */
#endif /* _FILESYSTEM_APP_H_ */

#ifdef HAVE_SDL
#include "filesystem-sdl.h"
#endif /* HAVE_SDL */
#ifdef WIN32
#include "filesystem-win32.h"
#else /* !WIN32 */
#include "filesystem-unix.h"
#endif /* WIN32 */
#include "filesystem-hosted.h"

#ifdef _FILE_H_
#ifndef _FILESYSTEM_APP__FILE_H_
#define _FILESYSTEM_APP__FILE_H_

#ifdef RB_FILESYSTEM_OS
#define FILEFUNCTIONS_DEFINED
#endif

#ifndef FILEFUNCTIONS_DECLARED
int     app_open(const char *name, int oflag, ...);
int     app_creat(const char *name, mode_t mode);
#define app_close       os_close
#define app_ftruncate   os_ftruncate
#define app_fsync       os_fsync
#define app_lseek       os_lseek
#ifdef HAVE_SDL_THREADS
ssize_t app_read(int fildes, void *buf, size_t nbyte);
ssize_t app_write(int fildes, const void *buf, size_t nbyte);
#else
#define app_read        os_read
#define app_write       os_write
#endif /* HAVE_SDL_THREADS */
int     app_remove(const char *path);
int     app_rename(const char *old, const char *new);
#define app_modtime     os_modtime
#define app_filesize    os_filesize
#define app_fsamefile   os_fsamefile
int     app_relate(const char *path1, const char *path2);
bool    app_file_exists(const char *path);
ssize_t app_readlink(const char *path, char *buf, size_t bufsize);
#endif /* !FILEFUNCTIONS_DECLARED */

#endif /* _FILESYSTEM_APP__FILE_H_ */
#endif /* _FILE_H_ */

#ifdef _DIR_H_
#ifndef _FILESYSTEM_APP__DIR_H_
#define _FILESYSTEM_APP__DIR_H_

#ifdef RB_FILESYSTEM_OS
#define DIRFUNCTIONS_DEFINED
#endif

#define DIRENT dirent
#define DIRENT_DEFINED

#ifndef DIRFUNCTIONS_DECLARED
DIR * app_opendir(const char *dirname);
struct dirent * app_readdir(DIR *dirp);
int   app_closedir(DIR *dirp);
int   app_mkdir(const char *path);
int   app_rmdir(const char *path);
int   app_samedir(DIR *dirp1, DIR *dirp2);
bool  app_dir_exists(const char *dirname);
#endif /* DIRFUNCTIONS_DECLARED */

#endif /* _FILESYSTEM_APP__DIR_H_ */
#endif /* _DIR_H_ */
