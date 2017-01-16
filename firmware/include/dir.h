/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Kévin Ferrare
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
#ifndef _DIR_H_
#define _DIR_H_

#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include "config.h"
#include "fs_attr.h"

#if defined (APPLICATION) || defined(CHECKWPS)
#include "filesystem-app.h"
#elif defined(SIMULATOR) || defined(DBTOOL)
#include "../../uisimulator/common/filesystem-sim.h"
#else
#include "filesystem-native.h"
#endif

#ifndef DIRFUNCTIONS_DEFINED
#ifndef opendir
#define opendir         FS_PREFIX(opendir)
#endif
#ifndef readdir
#define readdir         FS_PREFIX(readdir)
#endif
#ifndef readdir_r
#define readdir_r       FS_PREFIX(readdir_r)
#endif
#ifndef rewinddir
#define rewinddir       FS_PREFIX(rewinddir)
#endif
#ifndef closedir
#define closedir        FS_PREFIX(closedir)
#endif
#ifndef mkdir
#define mkdir           FS_PREFIX(mkdir)
#endif
#ifndef rmdir
#define rmdir           FS_PREFIX(rmdir)
#endif
#ifndef samedir
#define samedir         FS_PREFIX(samedir)
#endif
#ifndef dir_exists
#define dir_exists      FS_PREFIX(dir_exists)
#endif
#endif /* !DIRFUNCTIONS_DEFINED */

#ifndef DIRENT_DEFINED
struct DIRENT
{
    struct dirinfo_native info; /* platform extra info */
    char d_name[MAX_PATH];      /* UTF-8 name of entry (last!) */
};
#endif /* DIRENT_DEFINED */

struct dirinfo
{
    unsigned int attribute; /* attribute bits of file */
    off_t        size;      /* binary size of file */
    time_t       mtime;     /* local file time */
};

#ifndef DIRFUNCTIONS_DECLARED
/* TIP: set errno to zero before calling to see if anything failed */
struct dirinfo dir_get_info(DIR *dirp, struct DIRENT *entry);
#endif /* !DIRFUNCTIONS_DECLARED */

#endif /* _DIR_H_ */
