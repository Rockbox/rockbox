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
#include "mv.h"
#include "file.h"
#include "dir.h"

#ifdef HAVE_MULTIVOLUME
int strip_volume(const char *name, const char **nameptr, bool greedy);
int get_volume_name(int volume, char *name);
#endif

size_t path_basename(const char *path, const char **basename);
size_t append_to_path(char *buf, const char *basepath, const char *component,
                      size_t bufsize);
ssize_t parse_path_component(const char **pathp, const char **namep);

struct dirinfo dir_get_info(DIR *dirp, struct dirent *entry);
bool file_exists(const char *path);
bool dir_exists(const char *dirname);

#endif /* __INCLUDE_FILEFUNCS_H_ */
