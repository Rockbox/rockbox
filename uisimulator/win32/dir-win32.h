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
#ifndef _DIR_WIN32_H_
#define _DIR_WIN32_H_

#include <stdbool.h>

struct dirent {
    unsigned char d_name[MAX_PATH];
    int attribute;
    int size;
    int startcluster;
};

typedef struct
{
    struct dirent   fd;
    int             handle;
} DIR;

extern DIR* opendir(const char* name);
extern int closedir(DIR* dir);
extern int mkdir(const char* name);
extern int rmdir(const char* name);

extern struct dirent* readdir(DIR* dir);

#define S_ISDIR(x) (((x) & _S_IFDIR) == _S_IFDIR)

#endif
