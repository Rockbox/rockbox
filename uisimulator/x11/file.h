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

#ifndef ROCKBOX_FILE_H
#define ROCKBOX_FILE_H

#include <stdio.h>
#include <sys/types.h>

int x11_open(char *name, int opts);
int x11_close(int fd);
int x11_creat(char *name, int mode);
int x11_remove(char *name);
int x11_rename(char *oldpath, char *newpath);

#define open(x,y) x11_open(x,y)
#define close(x) x11_close(x)
#define creat(x,y) x11_creat(x,y)
#define remove(x) x11_remove(x)
#define rename(x,y) x11_rename(x,y)

#include "../../firmware/include/file.h"

extern int open(char* pathname, int flags);
extern int close(int fd);
extern int printf(const char *format, ...);

off_t lseek(int fildes, off_t offset, int whence);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);

#endif
