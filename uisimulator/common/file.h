/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg <daniel@haxx.se>
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _SIM_FILE_H_
#define _SIM_FILE_H_

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <stdio.h>
#endif

#include <sys/types.h>

#ifdef WIN32
#ifndef _commit
extern int _commit( int handle );

#ifdef _MSC_VER
typedef unsigned int mode_t;
#endif

#endif
#endif

int sim_open(const char *name, int opts);
int sim_close(int fd);
int sim_rename(const char *oldpath, const char *newpath);
off_t sim_filesize(int fd);
int sim_creat(const char *name, mode_t mode);
int sim_remove(const char *name);

#ifndef NO_REDEFINES_PLEASE
#define open(x,y) sim_open(x,y)
#define close(x) sim_close(x)
#define filesize(x) sim_filesize(x)
#define creat(x,y) sim_creat(x,y)
#define remove(x) sim_remove(x)
#define rename(x,y) sim_rename(x,y)
#ifdef WIN32
#define fsync _commit
#endif
#endif

#include "../../firmware/include/file.h"

#ifndef WIN32
int open(const char* pathname, int flags);
int close(int fd);
int printf(const char *format, ...);
int ftruncate(int fd, off_t length);
int fsync(int fd);

off_t lseek(int fildes, off_t offset, int whence);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
#endif

#endif
