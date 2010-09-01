/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Maurus Cuelenaere
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __INCLUDE_FILEFUNCS_H_
#define __INCLUDE_FILEFUNCS_H_

#include <stdbool.h>
#include "config.h"
#include "file.h"
#include "dir.h"

#ifdef HAVE_MULTIVOLUME
int strip_volume(const char* name, char* namecopy);
#endif

#ifndef __PCTOOL__
bool file_exists(const char *file);
bool dir_exists(const char *path);
#endif
extern struct dirinfo dir_get_info(DIR* parent, struct dirent *entry);

#endif /* __INCLUDE_FILEFUNCS_H_ */
