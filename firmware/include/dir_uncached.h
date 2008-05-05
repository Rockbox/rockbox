/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: dir.h 13741 2007-06-30 02:08:27Z jethead71 $
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
#ifndef _DIR_UNCACHED_H_
#define _DIR_UNCACHED_H_

#include <stdbool.h>
#include "file.h"

#define ATTR_READ_ONLY   0x01
#define ATTR_HIDDEN      0x02
#define ATTR_SYSTEM      0x04
#define ATTR_VOLUME_ID   0x08
#define ATTR_DIRECTORY   0x10
#define ATTR_ARCHIVE     0x20
#define ATTR_VOLUME      0x40 /* this is a volume, not a real directory */

#ifdef SIMULATOR
#define dirent_uncached sim_dirent
#define DIR_UNCACHED SIM_DIR
#define opendir_uncached sim_opendir
#define readdir_uncached sim_readdir
#define closedir_uncached sim_closedir
#define mkdir_uncached sim_mkdir
#define rmdir_uncached sim_rmdir
#endif

#ifndef DIRENT_DEFINED

struct dirent_uncached {
    unsigned char d_name[MAX_PATH];
    int attribute;
    long size;
    long startcluster;
    unsigned short wrtdate; /*  Last write date */ 
    unsigned short wrttime; /*  Last write time */
};
#endif

#include "fat.h"

typedef struct {
#ifndef SIMULATOR
    bool busy;
    long startcluster;
    struct fat_dir fatdir;
    struct fat_dir parent_dir;
    struct dirent_uncached theent;
#ifdef HAVE_MULTIVOLUME
    int volumecounter; /* running counter for faked volume entries */
#endif
#else
    /* simulator: */
    void *dir; /* actually a DIR* dir */
    char *name;
#endif
} DIR_UNCACHED;

#ifdef HAVE_HOTSWAP
char *get_volume_name(int volume);
#endif

#ifdef HAVE_MULTIVOLUME
    int strip_volume(const char*, char*);
#endif

#ifndef DIRFUNCTIONS_DEFINED

extern DIR_UNCACHED* opendir_uncached(const char* name);
extern int closedir_uncached(DIR_UNCACHED* dir);
extern int mkdir_uncached(const char* name);
extern int rmdir_uncached(const char* name);

extern struct dirent_uncached* readdir_uncached(DIR_UNCACHED* dir);

extern int release_dirs(int volume);

#endif /* DIRFUNCTIONS_DEFINED */

#endif
