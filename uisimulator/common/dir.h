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
#ifndef _SIM_DIR_H_
#define _SIM_DIR_H_

#include <sys/types.h>

#define DIRFUNCTIONS_DEFINED /* prevent those prototypes */
#define dirent sim_dirent
#include "../../firmware/include/dir.h"
#undef dirent

typedef void * MYDIR;

extern MYDIR *sim_opendir(const char *name);
extern struct sim_dirent* sim_readdir(MYDIR* dir);
extern int sim_closedir(MYDIR *dir);
extern int sim_mkdir(const char *name, int mode);
extern int sim_rmdir(char *name);

#define DIR MYDIR
#define dirent sim_dirent
#define opendir(x) sim_opendir(x)
#define readdir(x) sim_readdir(x)
#define closedir(x) sim_closedir(x)
#define mkdir(x, y) sim_mkdir(x, y)
#define rmdir(x) sim_rmdir(x)

#endif
