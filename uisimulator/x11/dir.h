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
#ifndef _X11_DIR_H_
#define _X11_DIR_H_

#include <sys/types.h>
typedef void DIR;

#define DIRFUNCTIONS_DEFINED /* prevent those prototypes */
#define dirent x11_dirent
#include "../../firmware/include/dir.h"
#undef dirent

typedef void * MYDIR;

extern MYDIR *x11_opendir(char *name);
extern struct x11_dirent* x11_readdir(MYDIR* dir);
extern int x11_closedir(MYDIR *dir);
extern int x11_mkdir(char *name, int mode);

#ifndef NO_REDEFINES_PLEASE

#define DIR MYDIR
#define dirent x11_dirent
#define opendir(x) x11_opendir(x)
#define readdir(x) x11_readdir(x)
#define closedir(x) x11_closedir(x)
#define mkdir(x, y) x11_mkdir(x, y)

#endif

#endif
