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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _FILE_H_
#define _FILE_H_

#define MAX_PATH 260

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#ifndef O_RDONLY
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#endif

#ifndef SIMULATOR
extern int open(char* pathname, int flags);
extern int close(int fd);
extern int read(int fd, void* buf, int count);
extern int lseek(int fd, int offset, int whence);

#ifdef DISK_WRITE
extern int write(int fd, void* buf, int count);
extern int remove(char* pathname);
extern int rename(char* oldname, char* newname);
#endif

#else
#if defined(WIN32) && !defined(__MINGW32__)
#include <io.h>
#include <stdio.h>
#endif // WIN32
#endif // SIMULATOR

#endif
