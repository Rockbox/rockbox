/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <io.h>
#include <malloc.h>
#include "file-win32.h"
#include "file.h"

// Directory operations
//

// opendir
// open directory for scanning
DIR *opendir (
              char *dirname // directory name
              )
{
    DIR                 *p = (DIR*)malloc(sizeof(DIR));
    struct _finddata_t  fd;
    if ((p->handle = _findfirst (dirname, &fd)) == -1)
    {
        free (p);
        return NULL;
    }
    return p;
}

// closedir
// close directory
int closedir (
              DIR *dir // previously opened dir search
              )
{
    free(dir);
    return 0;
}

// read dir
// read next entry in directory
dirent *readdir (
                 DIR *dir
                 )
{
    struct _finddata_t fd;
    if (_findnext (dir->handle, &fd) == -1)
        return NULL;
    memcpy (dir->fd.d_name, fd.name, 256);
    dir->fd.d_reclen = sizeof (dirent);
    return &dir->fd;
}