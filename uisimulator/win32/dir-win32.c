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
#include <windows.h>
#include <malloc.h>
#include "dir-win32.h"

// Directory operations
//

// opendir
// open directory for scanning
DIR *opendir (
              const char *dirname // directory name
              )
{
    DIR                 *p = (DIR*)malloc(sizeof(DIR));
    struct _finddata_t  fd;
    unsigned int        i;
    char                *s = (char*)malloc(strlen(dirname) + 5);
    wsprintf (s, "%s", dirname);

    for (i = 0; i < strlen(s); i++)
        if (s[i] == '/')
            s[i] = '\\';

    if (s[i - 1] != '\\')
    {
        s[i] = '\\';
        s[++i] = '\0';
    }

    OutputDebugString (s);

    wsprintf (s, "%s*.*", s);

    if ((p->handle = _findfirst (s, &fd)) == -1)
    {
        free (s);
        free (p);
        return 0;
    }
    free (s);
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
struct dirent *readdir (
                        DIR *dir
                        )
{
    struct _finddata_t fd;
    if (_findnext (dir->handle, &fd) == -1)
        return 0;
    memcpy (dir->fd.d_name, fd.name, 256);
    
    dir->fd.attribute = fd.attrib & 0x3f;
    dir->fd.size = fd.size;
    dir->fd.startcluster = 0 ;

    
    return &dir->fd;
}
