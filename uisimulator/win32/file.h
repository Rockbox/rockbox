/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg <daniel@haxx.se>
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _FILE_H_

#ifndef __MINGW32__
#include <io.h>
#include <fcntl.h>
#endif

#ifndef _commit
extern int _commit( int handle );
#endif

int win32_rename(char *oldpath, char *newpath);
int win32_filesize(int fd);

#define rename win32_rename
#define filesize win32_filesize
#define fsync _commit

#include "../../firmware/include/file.h"

#undef rename
#define mkdir(x,y) win32_mkdir(x,y)

#endif
