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
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include "fat.h"
#include "dir.h"
#include "debug.h"

#define MAX_OPEN_DIRS 8

static DIR opendirs[MAX_OPEN_DIRS];

DIR* opendir(char* name)
{
    char namecopy[MAX_PATH];
    char* part;
    char* end;
    struct fat_direntry entry;
    int dd;

    /* find a free dir descriptor */
    for ( dd=0; dd<MAX_OPEN_DIRS; dd++ )
        if ( !opendirs[dd].busy )
            break;

    if ( dd == MAX_OPEN_DIRS ) {
        DEBUGF("Too many dirs open\n");
        errno = EMFILE;
        return NULL;
    }

    opendirs[dd].busy = true;

    if ( name[0] != '/' ) {
        DEBUGF("Only absolute paths supported right now\n");
        opendirs[dd].busy = false;
        return NULL;
    }

    if ( fat_opendir(&(opendirs[dd].fatdir), 0) < 0 ) {
        DEBUGF("Failed opening root dir\n");
        opendirs[dd].busy = false;
        return NULL;
    }

    strncpy(namecopy,name,sizeof(namecopy));
    namecopy[sizeof(namecopy)-1] = 0;

    for ( part = strtok_r(namecopy, "/", &end); part;
          part = strtok_r(NULL, "/", &end)) {
        /* scan dir for name */
        while (1) {
            if ((fat_getnext(&(opendirs[dd].fatdir),&entry) < 0) ||
                (!entry.name[0])) {
                opendirs[dd].busy = false;
                return NULL;
            }
            if ( (entry.attr & FAT_ATTR_DIRECTORY) &&
                 (!strcasecmp(part, entry.name)) ) {
                if ( fat_opendir(&(opendirs[dd].fatdir),
                                 entry.firstcluster) < 0 ) {
                    DEBUGF("Failed opening dir '%s' (%d)\n",
                           part, entry.firstcluster);
                    opendirs[dd].busy = false;
                    return NULL;
                }
                break;
            }
        }
    }

    return &opendirs[dd];
}

int closedir(DIR* dir)
{
    dir->busy=false;
    return 0;
}

struct dirent* readdir(DIR* dir)
{
    struct fat_direntry entry;
    struct dirent* theent = &(dir->theent);

    if (fat_getnext(&(dir->fatdir),&entry) < 0)
        return NULL;

    if ( !entry.name[0] )
        return NULL;	

    strncpy(theent->d_name, entry.name, sizeof( theent->d_name ) );
    theent->attribute = entry.attr;
    theent->size = entry.filesize;
    theent->startcluster = entry.firstcluster;

    return theent;
}

int mkdir(char *name)
{
    DIR *dir;
    char namecopy[MAX_PATH];
    char* end;
    char *basename;
    char *parent;
    struct dirent *entry;
    struct fat_dir newdir;
    int rc;
    
    if ( name[0] != '/' ) {
        DEBUGF("Only absolute paths supported right now\n");
        return -1;
    }

    strncpy(namecopy,name,sizeof(namecopy));
    namecopy[sizeof(namecopy)-1] = 0;

    /* Split the base name and the path */
    end = strrchr(namecopy, '/');
    *end = 0;
    basename = end+1;

    if(namecopy == end) /* Root dir? */
        parent = "/";
    else
        parent = namecopy;
        
    DEBUGF("mkdir: parent: %s, name: %s\n", parent, basename);

    dir = opendir(parent);
    
    if(!dir) {
        DEBUGF("mkdir: can't open parent dir\n");
        return -2;
    }    

    /* Now check if the name already exists */
    while ((entry = readdir(dir))) {
        if ( !strcasecmp(basename, entry->d_name) ) {
            DEBUGF("mkdir error: file exists\n");
            errno = EEXIST;
            closedir(dir);
            return - 3;
        }
    }

    closedir(dir);
    
    rc = fat_create_dir(basename, &newdir, &(dir->fatdir));
    
    return rc;
}
