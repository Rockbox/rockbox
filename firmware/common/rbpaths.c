/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Thomas Martitz
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


#include <stdio.h> /* snprintf */
#include <stdlib.h>
#include "rbpaths.h"
#include "file.h" /* MAX_PATH */
#include "dir.h"
#include "gcc_extensions.h"
#include "string-extra.h"
#include "filefuncs.h"


void paths_init(void)
{
    /* make sure $HOME/.config/rockbox.org exists, it's needed for config.cfg */
#if (CONFIG_PLATFORM & PLATFORM_ANDROID)
    mkdir("/sdcard/rockbox");
#else
    char home_path[MAX_PATH];
    snprintf(home_path, sizeof(home_path), "%s/.config/rockbox.org", getenv("HOME"));
    mkdir(home_path);
#endif
}

const char* get_user_file_path(const char *path,
                               unsigned flags,
                               char* buf,
                               const size_t bufsize)
{
    const char *ret = path;
    const char *pos = path;
    printf("%s(): looking for %s\n", __func__, path);
    /* replace ROCKBOX_DIR in path with $HOME/.config/rockbox.org */
    pos += ROCKBOX_DIR_LEN;
    if (*pos == '/') pos += 1;

#if (CONFIG_PLATFORM & PLATFORM_ANDROID)
    if (snprintf(buf, bufsize, "/sdcard/rockbox/%s", pos)
#else
    if (snprintf(buf, bufsize, "%s/.config/rockbox.org/%s", getenv("HOME"), pos)
#endif
            >= (int)bufsize)
        return NULL;

    /* always return the replacement buffer (pointing to $HOME) if
     * write access is needed */
    if (flags & NEED_WRITE)
        ret = buf;
    else
    {
        if (flags & IS_FILE)
        {
            if (file_exists(buf))
                ret = buf;
        }
        else
        {
            if (dir_exists(buf))
                ret = buf;
        }
    }

    /* make a copy if we're about to return the path*/
    if (UNLIKELY((flags & FORCE_BUFFER_COPY) && (ret != buf)))
    {
        strlcpy(buf, ret, bufsize);
        ret = buf;
    }

    printf("%s(): %s\n", __func__, ret);
    return ret;
}
