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

#undef MAX_PATH /* this avoids problems when building simulator */
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
#define O_CREAT  4
#define O_APPEND 8
#define O_TRUNC  0x10
#endif

#if defined(__MINGW32__) && defined(SIMULATOR)
extern int open(const char*, int, ...);
extern int close(int fd);
extern int read(int, void*, unsigned int);
extern long lseek(int, long, int);
extern int creat(const char *, int);
extern int write(int, const void*, unsigned int);
extern int remove(const char*);

#else

#ifndef SIMULATOR
extern int open(const char* pathname, int flags);
extern int close(int fd);
extern int fsync(int fd);
extern int read(int fd, void* buf, int count);
extern int lseek(int fd, int offset, int whence);
extern int creat(const char *pathname, int mode);
extern int write(int fd, void* buf, int count);
extern int remove(const char* pathname);
extern int rename(const char* path, const char* newname);
extern int ftruncate(int fd, unsigned int size);
extern int filesize(int fd);
#endif /* SIMULATOR */
#endif /* __MINGW32__ */

#endif
