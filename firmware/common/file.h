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

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2

#ifndef SIMULATOR
extern int open(char* pathname, int flags);
extern int close(int fd);

extern int read(int fd, void* buf, int count);
extern int write(int fd, void* buf, int count);

extern int lseek(int fd, int offset, int whence);

extern int remove(char* pathname);
extern int rename(char* oldname, char* newname);
#else
#ifdef WIN32
#include <io.h>
#include <stdio.h>
#endif // WIN32
#endif // SIMULATOR

#endif
