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
#include <stdio.h>
#include <string.h>
#include "fat.h"
#include "dir.h"
#include "debug.h"
#include "types.h"

static DIR thedir;
static struct dirent theent;
static bool busy=FALSE;

DIR* opendir(char* name)
{
    char* part;
    struct fat_direntry entry;
    struct fat_dir* dir = &(thedir.fatdir);

    if ( busy ) {
        DEBUGF("Only one open dir at a time\n");
        return NULL;
    }

    if ( name[0] != '/' ) {
        DEBUGF("Only absolute paths supported right now\n");
        return NULL;
    }

    if ( fat_opendir(dir, 0) < 0 ) {
        DEBUGF("Failed opening root dir\n");
        return NULL;
    }

    /* fixme: strtok() is not thread safe, and fat_getnext() calls yield() */
    for ( part = strtok(name, "/"); part;
          part = strtok(NULL, "/") ) {
        int partlen = strlen(part);
        /* scan dir for name */
        while (1) {
            if (fat_getnext(dir,&entry) < 0)
                return NULL;
            if ( !entry.name[0] )
                return NULL;
            if ( (entry.attr & FAT_ATTR_DIRECTORY) &&
                 (!strncmp(part, entry.name, partlen)) ) {
                if ( fat_opendir(dir, entry.firstcluster) < 0 ) {
                    DEBUGF("Failed opening dir '%s' (%d)\n",
                           part, entry.firstcluster);
                    return NULL;
                }
                break;
            }
        }
    }

    busy = TRUE;

    return &thedir;
}

int closedir(DIR* dir)
{
    busy=FALSE;
    return 0;
}

struct dirent* readdir(DIR* dir)
{
    struct fat_direntry entry;

    if (fat_getnext(&(dir->fatdir),&entry) < 0)
        return NULL;

    if ( !entry.name[0] )
        return NULL;

    strncpy(theent.d_name, entry.name, sizeof( theent.d_name ) );
    theent.attribute = entry.attr;
    theent.size = entry.filesize;

    return &theent;
}

/*
 * local variables:
 * eval: (load-file "../rockbox-mode.el")
 * end:
 */
