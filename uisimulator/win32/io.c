/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include "file.h"
#include "debug.h"

#define SIMULATOR_ARCHOS_ROOT "archos"

int win32_rename(char *oldpath, char* newpath)
{
  char buffer1[256];
  char buffer2[256];

  if(oldpath[0] == '/') {
      sprintf(buffer1, "%s%s", SIMULATOR_ARCHOS_ROOT, oldpath);
      sprintf(buffer2, "%s%s", SIMULATOR_ARCHOS_ROOT, newpath);

      debugf("We rename the real file '%s' to '%s'\n", buffer1, buffer2);
      return rename(buffer1, buffer2);
  }
  return -1;
}

int win32_filesize(int fd)
{
    int old = lseek(fd, 0, SEEK_CUR);
    int size = lseek(fd, 0, SEEK_END);
    lseek(fd, old, SEEK_SET);

    return(size);
}

extern (mkdir)(const char *name);

int win32_mkdir(const char *name, int mode)
{
    char buffer[256]; /* sufficiently big */
    (void)mode;
    if(name[0] == '/') {
        sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);
        
        debugf("We create the real directory '%s'\n", buffer);
        return (mkdir)(buffer);
    }
    return (mkdir)(name);
}
