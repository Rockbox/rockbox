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
#include <stdarg.h>
#include "rbpaths.h"
#include "file.h" /* MAX_PATH */
#include "logf.h"
#include "gcc_extensions.h"
#include "string-extra.h"
#include "filefuncs.h"

#undef open
#undef creat
#undef remove
#undef rename
#undef opendir
#undef mkdir
#undef rmdir

#if (CONFIG_PLATFORM & PLATFORM_ANDROID)
#include "dir-target.h"
#define opendir opendir_android
#define mkdir   mkdir_android
#define rmdir   rmdir_android
#elif (CONFIG_PLATFORM & (PLATFORM_SDL|PLATFORM_MAEMO|PLATFORM_PANDORA))
#define open    sim_open
#define remove  sim_remove
#define rename  sim_rename
#define opendir sim_opendir
#define mkdir   sim_mkdir
#define rmdir   sim_rmdir
extern int sim_open(const char* name, int o, ...);
extern int sim_remove(const char* name);
extern int sim_rename(const char* old, const char* new);
extern DIR* sim_opendir(const char* name);
extern int sim_mkdir(const char* name);
extern int sim_rmdir(const char* name);
#endif

/* flags for get_user_file_path() */
/* whether you need write access to that file/dir, especially true
 * for runtime generated files (config.cfg) */
#define NEED_WRITE          (1<<0)
/* file or directory? */
#define IS_FILE             (1<<1)

void paths_init(void)
{
    /* make sure $HOME/.config/rockbox.org exists, it's needed for config.cfg */
#if (CONFIG_PLATFORM & PLATFORM_ANDROID)
    mkdir("/sdcard/rockbox");
#else
    char config_dir[MAX_PATH];

    const char *home = getenv("HOME");
    if (!home)
    {
      logf("HOME environment var not set. Can't write config");
      return;
    }

    snprintf(config_dir, sizeof(config_dir), "%s/.config", home);
    mkdir(config_dir);
    snprintf(config_dir, sizeof(config_dir), "%s/.config/rockbox.org", home);
    mkdir(config_dir);
#endif
}

static bool try_path(const char* filename, unsigned flags)
{
    if (flags & IS_FILE)
    {
        if (file_exists(filename))
            return true;
    }
    else
    {
        if (dir_exists(filename))
            return true;
    }
    return false;
}

static const char* _get_user_file_path(const char *path,
                                       unsigned flags,
                                       char* buf,
                                       const size_t bufsize)
{
    const char *ret = path;
    const char *pos = path;
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
    else if (try_path(buf, flags))
        ret = buf;
        
    if (ret != buf) /* not found in $HOME, try ROCKBOX_BASE_DIR, !NEED_WRITE only */
    {
        if (snprintf(buf, bufsize, ROCKBOX_SHARE_PATH "/%s", pos) >= (int)bufsize)
            return NULL;

        if (try_path(buf, flags))
            ret = buf;
    }

    return ret;
}

int app_open(const char *name, int o, ...)
{
    char realpath[MAX_PATH];
    va_list ap;
    int fd;
    
    if (!strncmp(ROCKBOX_DIR, name, ROCKBOX_DIR_LEN))
    {
        int flags = IS_FILE;
        if (o & (O_CREAT|O_RDWR|O_TRUNC|O_WRONLY))
            flags |= NEED_WRITE;
        name = _get_user_file_path(name, flags, realpath, sizeof(realpath));
    }
    va_start(ap, o);
    fd = open(name, o, va_arg(ap, unsigned int));
    va_end(ap);
    
    return fd;
}

int app_creat(const char* name, mode_t mode)
{
    return app_open(name, O_CREAT|O_WRONLY|O_TRUNC, mode);
}

int app_remove(const char *name)
{
    char realpath[MAX_PATH];
    const char *fname = name;
    if (!strncmp(ROCKBOX_DIR, name, ROCKBOX_DIR_LEN))
    {
        fname = _get_user_file_path(name, NEED_WRITE, realpath, sizeof(realpath));
    }

    return remove(fname);
}

int app_rename(const char *old, const char *new)
{
    char realpath_old[MAX_PATH], realpath_new[MAX_PATH];

    const char *final_old = old;
    if (!strncmp(ROCKBOX_DIR, old, ROCKBOX_DIR_LEN))
    {
        final_old = _get_user_file_path(old, NEED_WRITE, realpath_old, sizeof(realpath_old));
    }

    const char *final_new = new;
    if (!strncmp(ROCKBOX_DIR, new, ROCKBOX_DIR_LEN))
    {
        final_new = _get_user_file_path(new, NEED_WRITE, realpath_new, sizeof(realpath_new));
    }

    return rename(final_old, final_new);
}

DIR *app_opendir(const char *name)
{
    char realpath[MAX_PATH];
    const char *fname = name;
    if (!strncmp(ROCKBOX_DIR, name, ROCKBOX_DIR_LEN))
    {
        fname = _get_user_file_path(name, 0, realpath, sizeof(realpath));
    }
    return opendir(fname);
}

int app_mkdir(const char* name)
{
    char realpath[MAX_PATH];
    const char *fname = name;
    if (!strncmp(ROCKBOX_DIR, name, ROCKBOX_DIR_LEN))
    {
        fname = _get_user_file_path(name, NEED_WRITE, realpath, sizeof(realpath));
    }
    return mkdir(fname);
}

int app_rmdir(const char* name)
{
    char realpath[MAX_PATH];
    const char *fname = name;
    if (!strncmp(ROCKBOX_DIR, name, ROCKBOX_DIR_LEN))
    {
        fname = _get_user_file_path(name, NEED_WRITE, realpath, sizeof(realpath));
    }
    return rmdir(fname);
}
