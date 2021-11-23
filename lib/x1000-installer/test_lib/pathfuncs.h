/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Michael Sevakis
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
#ifndef _PATHFUNCS_H_
#define _PATHFUNCS_H_

#include <stddef.h>
#include <stdbool.h>

#define PATH_SEPCH '/'

/* return true if path begins with a root '/' component and is not NULL */
static inline bool path_is_absolute(const char *path)
{
    return path && path[0] == PATH_SEPCH;
}

size_t path_dirname(const char *name, const char **nameptr);
size_t path_append(char *buf, const char *basepath,
                   const char *component, size_t bufsize);

#endif
