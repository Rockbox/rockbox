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
#ifndef _FILESYSTEM_SIM_H_
#define _FILESYSTEM_SIM_H_

#if defined(PLUGIN) || defined(CODEC)
/* Prevent often-problematic plugin namespace pollution */
#define FILEFUNCTIONS_DECLARED
#define FILEFUNCTIONS_DEFINED
#define DIRFUNCTIONS_DECLARED
#define DIRFUNCTIONS_DEFINED
#define OSFUNCTIONS_DECLARED
#endif /* PLUGIN || CODEC */

#ifndef OSFUNCTIONS_DECLARED
#define FS_PREFIX(_x_) sim_ ## _x_

/* path sandboxing and volume redirector */
int sim_get_os_path(char *buffer, const char *path, size_t bufsize);
#define SIM_TMPBUF_MAX_PATH (MAX_PATH+1)

#endif /* !OSFUNCTIONS_DECLARED */

#endif /* _FILESYSTEM_SIM_H_ */

#include "filesystem-sdl.h"
#ifdef WIN32
#include "filesystem-win32.h"
#else /* !WIN32 */
#include "filesystem-unix.h"
#endif /* WIN32 */
#include "filesystem-hosted.h"

#ifdef _FILE_H_
#ifndef _FILESYSTEM_SIM_H__FILE_H_
#define _FILESYSTEM_SIM_H__FILE_H_

#ifdef RB_FILESYSTEM_OS
#define FILEFUNCTIONS_DEFINED
#endif

#ifndef FILEFUNCTIONS_DECLARED
int     sim_open(const char *name, int oflag, ...);
int     sim_creat(const char *name, mode_t mode);
int     sim_close(int fildes);
int     sim_ftruncate(int fildes, off_t length);
int     sim_fsync(int fildes);
off_t   sim_lseek(int fildes, off_t offset, int whence);
ssize_t sim_read(int fildes, void *buf, size_t nbyte);
ssize_t sim_write(int fildes, const void *buf, size_t nbyte);
int     sim_remove(const char *path);
int     sim_rename(const char *old, const char *new);
int     sim_modtime(const char *path, time_t modtime);
off_t   sim_filesize(int fildes);
int     sim_fsamefile(int fildes1, int fildes2);
int     sim_relate(const char *path1, const char *path2);
bool    sim_file_exists(const char *path);
#endif /* !FILEFUNCTIONS_DECLARED */

#endif /* _FILESYSTEM_SIM_H__FILE_H_ */
#endif /* _FILE_H_ */

#ifdef _DIR_H_
#ifndef _FILESYSTEM_SIM_H__DIR_H_
#define _FILESYSTEM_SIM_H__DIR_H_

#ifdef RB_FILESYSTEM_OS
#define DIRFUNCTIONS_DEFINED
#else
#define dirent DIRENT
#endif

#define DIRENT sim_dirent

struct dirinfo_native
{
    void *osdirent;
};

#ifndef DIRFUNCTIONS_DECLARED
DIR * sim_opendir(const char *dirname);
struct sim_dirent * sim_readdir(DIR *dirp);
int   sim_closedir(DIR *dirp);
int   sim_mkdir(const char *path);
int   sim_rmdir(const char *path);
int   sim_samedir(DIR *dirp1, DIR *dirp2);
bool  sim_dir_exists(const char *dirname);
#endif /* !DIRFUNCTIONS_DECLARED */

#endif /* _FILESYSTEM_SIM_H__DIR_H_ */
#endif /* _DIR_H_ */
