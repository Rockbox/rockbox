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

struct dirent {
    unsigned char d_name[256];
    int attribute;
    int size;
};

#ifndef SIMULATOR
typedef struct {
    int offset;
} DIR;
#else // SIMULATOR
#ifdef _WIN32
typedef struct DIRtag
{
    struct dirent   fd;
    intptr_t        handle;
} DIR;
#endif //   _WIN32
#endif // SIMULATOR

extern DIR* opendir(char* name);
extern int closedir(DIR* dir);

extern struct dirent* readdir(DIR* dir);

#endif
