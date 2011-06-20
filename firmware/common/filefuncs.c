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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "dir.h"
#include "stdlib.h"
#include "string.h"
#include "debug.h"
#include "file.h"
#include "filefuncs.h"

#ifndef __PCTOOL__
#ifdef HAVE_MULTIVOLUME
/* returns on which volume this is, and copies the reduced name
   (sortof a preprocessor for volume-decorated pathnames) */
int strip_volume(const char* name, char* namecopy)
{
    int volume = 0;
    const char *temp = name;

    while (*temp == '/')          /* skip all leading slashes */
        ++temp;

    if (*temp && !strncmp(temp, VOL_NAMES, VOL_ENUM_POS))
    {
        temp += VOL_ENUM_POS;     /* behind special name */
        volume = atoi(temp);      /* number is following */
        temp = strchr(temp, '/'); /* search for slash behind */
        if (temp != NULL)
            name = temp;          /* use the part behind the volume */
        else
            name = "/";           /* else this must be the root dir */
    }

    strlcpy(namecopy, name, MAX_PATH);

    return volume;
}
#endif /* #ifdef HAVE_MULTIVOLUME */

/* Test file existence, using dircache of possible */
bool file_exists(const char *file)
{
    int fd;

#ifdef DEBUG
    if (!file || !*file)
    {
        DEBUGF("%s(%p): Invalid parameter!\n", __func__, file);
        return false;
    }
#endif

#ifdef HAVE_DIRCACHE
    if (dircache_is_enabled())
        return (dircache_get_entry_id(file) >= 0);
#endif

    fd = open(file, O_RDONLY);
    if (fd < 0)
        return false;
    close(fd);
    return true;
}

bool dir_exists(const char *path)
{
    DIR* d = opendir(path);
    if (!d)
        return false;
    closedir(d);
    return true;
}

#endif /* __PCTOOL__ */

#if (CONFIG_PLATFORM & (PLATFORM_NATIVE|PLATFORM_SDL|PLATFORM_MAEMO|PLATFORM_PANDORA))
struct dirinfo dir_get_info(DIR* parent, struct dirent *entry)
{
    (void)parent;
    return entry->info;
}
#endif
