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
#ifndef _FILESYSTEM_UNIX_H_
#define _FILESYSTEM_UNIX_H_

/* Include for file.h and dir.h because mkdir and friends may be here */
#include <sys/stat.h>

#define strlcpy_from_os strlcpy
#endif

#ifdef _FILE_H_
#ifndef _FILESYSTEM_UNIX__FILE_H_
#define _FILESYSTEM_UNIX__FILE_H_

#include <unistd.h>

#define OS_STAT_T       struct stat

#ifndef OSFUNCTIONS_DECLARED
#define os_open         open
#define os_creat        creat
#define os_close        close
#define os_lseek        lseek
#define os_stat         stat
#define os_fstat        fstat
#define os_fstatat      fstatat
#define os_lstat        lstat
#define os_fsync        fsync
#define os_ftruncate    ftruncate
#define os_remove       remove
#define os_rename       rename
#define os_readlink     readlink
#ifndef os_read
#define os_read         read
#endif
#ifndef os_write
#define os_write        write
#endif
#endif /* !OSFUNCTIONS_DECLARED */

#endif /* _FILESYSTEM_UNIX__FILE_H_ */
#endif /* _FILE_H_ */

#ifdef _DIR_H_
#ifndef _FILESYSTEM_UNIX__DIR_H_ 
#define _FILESYSTEM_UNIX__DIR_H_

#include <dirent.h>

#define OS_DIR_T        DIR
#define OS_DIRENT_T     struct dirent

#ifndef OSFUNCTIONS_DECLARED
#define os_opendir      opendir
#define os_readdir      readdir
#define os_closedir     closedir
#define os_mkdir        mkdir
#define os_rmdir        rmdir
#define os_dirfd        dirfd /* NOTE: might have to wrap on some platforms */
#endif /* !OSFUNCTIONS_DECLARED */

#endif /* _FILESYSTEM_UNIX__DIR_H_ */
#endif /* _DIR_H_ */
