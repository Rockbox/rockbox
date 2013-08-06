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

#include "config.h"
#include <sys/types.h>
#include "fs_attr.h"

#ifndef DIRFUNCTIONS_DEFINED
#if defined (SIMULATOR) || defined (__PCTOOL__)
#   define dirent       sim_dirent
#   define opendir      sim_opendir
#   define readdir      sim_readdir
#   define closedir     sim_closedir
#   define mkdir        sim_mkdir
#   define rmdir        sim_rmdir
#elif defined (APPLICATION)
#   define DIRENT_DEFINED
#   include "rbpaths.h"
#   define opendir      app_opendir
#   define readdir      app_readdir
#   define closedir     app_closedir
#   define mkdir        app_mkdir
#   define rmdir        app_rmdir
#endif

#endif /* DIRFUNCTIONS_DEFINED */

#ifndef DIR_DEFINED
typedef struct {} DIR;
#endif /* DIR_DEFINED */

struct dirinfo
{
    int            attribute;
    off_t          size;
    unsigned short wrtdate;
    unsigned short wrttime;
};

#ifndef DIRENT_DEFINED
struct dirent
{
    unsigned char d_name[MAX_PATH]; /* UTF-8 */
    struct dirinfo info;
};
#endif /* DIRENT_DEFINED */

#ifndef DIRFUNCIONS_DEFINED
DIR * opendir(const char *dirname);
int closedir(DIR *dirp);
struct dirent * readdir(DIR *dirp);
#if 0 /* not included right now but probably should be */
int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result);
void rewinddir(DIR *dirp);
#endif
int samedir(DIR *dirp1, DIR *dirp2);
int mkdir(const char *path);
int rmdir(const char *path);
#endif /* DIRFUNCTIONS_DEFINED */

#endif /* _DIR_H_ */
