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
#endif

#if !defined(SIMULATOR) || defined(__MINGW32__)
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
#ifdef WIN32
#include <io.h>
#include <stdio.h>
#endif /* WIN32 */
#endif /* SIMULATOR */

#endif
