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

#define dirent x11_dirent
#define readdir(x) x11_readdir(x)
#define opendir(x) x11_opendir(x)

/*
 * The defines above should let us use the readdir() and opendir() in target
 * code just as they're defined to work in target. They will then call our
 * x11_* versions of the functions that'll work as wrappers for the actual
 * host functions.
 */

#include <sys/types.h>
#include <dirent.h>

#undef dirent


#define DIRFUNCTIONS_DEFINED /* prevent those prototypes */

#include "../../firmware/common/dir.h"

extern DIR *x11_opendir(char *name);

