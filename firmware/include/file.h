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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _FILE_H_
#define _FILE_H_

#include <sys/types.h>
#include <stdbool.h>
#include <fcntl.h>
#ifdef WIN32
/* this has SEEK_SET et al */
#include <stdio.h>
#endif
#include "config.h"
#include "gcc_extensions.h"

#undef MAX_PATH /* this avoids problems when building simulator */
#define MAX_PATH 260

enum relate_result
{
                          /* < 0 == failure */
    RELATE_DIFFERENT = 0, /* the two paths are different objects */
    RELATE_SAME,          /* the two paths are the same object */
    RELATE_PREFIX,        /* the path2 contains path1 as a prefix */
};

#if defined(APPLICATION) || defined(CHECKWPS)
#include "filesystem-app.h"
#elif defined(SIMULATOR) || defined(DBTOOL)
#include "../../uisimulator/common/filesystem-sim.h"
#else
#include "filesystem-native.h"
#endif

#ifndef FILEFUNCTIONS_DECLARED
int fdprintf(int fildes, const char *fmt, ...) ATTRIBUTE_PRINTF(2, 3);
#endif /* FILEFUNCTIONS_DECLARED */

#ifndef FILEFUNCTIONS_DEFINED
#ifndef open
#define open            FS_PREFIX(open)
#endif
#ifndef creat
#define creat           FS_PREFIX(creat)
#endif
#ifndef close
#define close           FS_PREFIX(close)
#endif
#ifndef ftruncate
#define ftruncate       FS_PREFIX(ftruncate)
#endif
#ifndef fsync
#define fsync           FS_PREFIX(fsync)
#endif
#ifndef lseek
#define lseek           FS_PREFIX(lseek)
#endif
#ifndef read
#define read            FS_PREFIX(read)
#endif
#ifndef write
#define write           FS_PREFIX(write)
#endif
#ifndef remove
#define remove          FS_PREFIX(remove)
#endif
#ifndef rename
#define rename          FS_PREFIX(rename)
#endif
#ifndef filesize
#define filesize        FS_PREFIX(filesize)
#endif
#ifndef fsamefile
#define fsamefile       FS_PREFIX(fsamefile)
#endif
#ifndef file_exists
#define file_exists     FS_PREFIX(file_exists)
#endif
#ifndef relate
#define relate          FS_PREFIX(relate)
#endif
#ifndef readlink
#define readlink        FS_PREFIX(readlink)
#endif
#endif /* FILEFUNCTIONS_DEFINED */

#endif /* _FILE_H_ */
