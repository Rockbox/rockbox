
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Dan Everton (safetydan)
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

#ifndef _ROCKLIBC_H_
#define _ROCKLIBC_H_

#include <ctype.h>
#include "plugin.h"

#ifndef LUA_LIB

#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
#include <errno.h>
#define PREFIX(_x_) sim_ ## _x_
#else
extern int errno;
#define EINVAL          22      /* Invalid argument */
#define ERANGE          34      /* Math result not representable */
#define PREFIX
#endif
#endif

#define __likely   LIKELY
#define __unlikely UNLIKELY

#ifdef PLUGIN
/* Simple substitutions */
#define memcmp rb->memcmp
#define strlen rb->strlen
#define strrchr rb->strrchr

#define write(fd,buf,n) rb->write(fd,buf,n)
#define read(fd,buf,n)  rb->read(fd,buf,n)
#define open(...)       rb->open(__VA_ARGS__)
#define close(fd)       rb->close(fd)
#define lseek(fd,off,w) rb->lseek(fd,off,w)
#define filesize(fd)    rb->filesize(fd)
#define ftruncate(fd,l) rb->ftruncate(fd,l)
#define file_exists(p)  rb->file_exists(p)
#define remove(f)       rb->remove(f)
#define rename(old,new) rb->rename(old,new)

#define mkdir(p)          rb->mkdir(p)
#define rmdir(p)          rb->rmdir(p)
#define opendir(p)        rb->opendir(p)
#define closedir(d)       rb->closedir(d)
#define readdir(d)        rb->readdir(d)
#define dir_get_info(d,e) rb->dir_get_info(d,e)

#else /*ndef PLUGIN*/
#define write(fd,buf,n) PREFIX(write)(fd,buf,n)
#define read(fd,buf,n)  PREFIX(read)(fd,buf,n)
#define open(...)       PREFIX(open)(__VA_ARGS__)
#define close(fd)       PREFIX(close)(fd)
#define lseek(fd,off,w) PREFIX(lseek)(fd,off,w)
#define filesize(fd)    PREFIX(filesize)(fd)
#define ftruncate(fd,l) PREFIX(ftruncate)(fd,l)
#define file_exists(p)  PREFIX(file_exists)(p)
#define remove(f)       PREFIX(remove)(f)
#define rename(old,new) PREFIX(rename)(old,new)

#endif /* def PLUGIN */
#endif /* _ROCKLIBC_H_ */

