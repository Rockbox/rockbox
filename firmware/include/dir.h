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
#ifndef _DIR_H_
#define _DIR_H_

#include <stdbool.h>
#include <file.h>

#define ATTR_READ_ONLY   0x01
#define ATTR_HIDDEN      0x02
#define ATTR_SYSTEM      0x04
#define ATTR_VOLUME_ID   0x08
#define ATTR_DIRECTORY   0x10
#define ATTR_ARCHIVE     0x20
#define ATTR_VOLUME      0x40 /* this is a volume, not a real directory */

#ifndef DIRENT_DEFINED

struct dirent {
    unsigned char d_name[MAX_PATH];
    int attribute;
    int size;
    int startcluster;
    unsigned short wrtdate; /*  Last write date */ 
    unsigned short wrttime; /*  Last write time */
};
#endif


#ifndef SIMULATOR

#include "fat.h"

typedef struct {
    bool busy;
    int startcluster;
    struct fat_dir fatdir;
    struct fat_dir parent_dir;
    struct dirent theent;
#ifdef HAVE_MULTIVOLUME
    int volumecounter; /* running counter for faked volume entries */
#endif
} DIR;

#else /* SIMULATOR */

#ifdef WIN32
#ifndef __MINGW32__
#include <io.h>
#endif /* __MINGW32__ */

typedef struct DIRtag
{
    struct dirent   fd;
    int             handle;
} DIR;

#endif /* WIN32 */

#endif /* SIMULATOR */

#ifndef DIRFUNCTIONS_DEFINED

extern DIR* opendir(const char* name);
extern int closedir(DIR* dir);
extern int mkdir(const char* name, int mode);
extern int rmdir(const char* name);

extern struct dirent* readdir(DIR* dir);

extern int release_dirs(int volume);

#endif /* DIRFUNCTIONS_DEFINED */

#endif
