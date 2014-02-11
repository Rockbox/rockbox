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
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "config.h"
#include "rbpaths.h"
#include "file.h" /* MAX_PATH */
#include "logf.h"
#include "gcc_extensions.h"
#include "string-extra.h"
#include "filefuncs.h"

/* In this file we need the actual OS library functions, not the shadowed
 * wrapper used within Rockbox' application code (except SDL adds
 * another layer) */
#undef open
#undef creat
#undef remove
#undef rename
#undef opendir
#undef closedir
#undef readdir
#undef mkdir
#undef rmdir
#undef dirent
#undef DIR

#if (CONFIG_PLATFORM & PLATFORM_ANDROID)
static const char rbhome[] = "/sdcard";
#endif
#if (CONFIG_PLATFORM & (PLATFORM_SDL|PLATFORM_MAEMO|PLATFORM_PANDORA)) && !defined(__PCTOOL__)
const char *rbhome;
#endif

#if !(defined(SAMSUNG_YPR0) || defined(SAMSUNG_YPR1)) && !defined(__PCTOOL__)
/* Special dirs are user-accessible (and user-writable) dirs which take priority
 * over the ones where Rockbox is installed to. Classic example would be
 * $HOME/.config/rockbox.org vs /usr/share/rockbox */
#define HAVE_SPECIAL_DIRS
#endif

/* flags for get_user_file_path() */
/* whether you need write access to that file/dir, especially true
 * for runtime generated files (config.cfg) */
#define NEED_WRITE          (1<<0)
/* file or directory? */
#define IS_FILE             (1<<1)

#ifdef HAVE_SPECIAL_DIRS
void paths_init(void)
{
    /* make sure $HOME/.config/rockbox.org exists, it's needed for config.cfg */
#if (CONFIG_PLATFORM & PLATFORM_ANDROID)
    mkdir("/sdcard/rockbox", 0777);
    mkdir("/sdcard/rockbox/rocks.data", 0777);
#else
    char config_dir[MAX_PATH];

    const char *home = getenv("RBROOT");
    if (!home)
    {
        home = getenv("HOME");
    }
    if (!home)
    {
      logf("HOME environment var not set. Can't write config");
      return;
    }

    rbhome = home;
    snprintf(config_dir, sizeof(config_dir), "%s/.config", home);
    mkdir(config_dir, 0777);
    snprintf(config_dir, sizeof(config_dir), "%s/.config/rockbox.org", home);
    mkdir(config_dir, 0777);
    /* Plugin data directory */
    snprintf(config_dir, sizeof(config_dir), "%s/.config/rockbox.org/rocks.data", home);
    mkdir(config_dir, 0777);
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
    if (snprintf(buf, bufsize, "%s/.config/rockbox.org/%s", rbhome, pos)
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


static const char* handle_special_dirs(const char* dir, unsigned flags,
                                char *buf, const size_t bufsize)
{
    if (!strncmp(HOME_DIR, dir, HOME_DIR_LEN))
    {
        const char *p = dir + HOME_DIR_LEN;
        while (*p == '/') p++;
        snprintf(buf, bufsize, "%s/%s", rbhome, p);
        return buf;
    }
    else if (!strncmp(ROCKBOX_DIR, dir, ROCKBOX_DIR_LEN))
        return _get_user_file_path(dir, flags, buf, bufsize);

    return dir;
}

#else /* !HAVE_SPECIAL_DIRS */

#ifndef paths_init
void paths_init(void) { }
#endif

static const char* handle_special_dirs(const char* dir, unsigned flags,
                                char *buf, const size_t bufsize)
{
    (void) flags; (void) buf; (void) bufsize;
    return dir;
}

#endif

int app_open(const char *name, int o, ...)
{
    char realpath[MAX_PATH];
    va_list ap;
    int fd;
    int flags = IS_FILE;
    if (o & (O_CREAT|O_RDWR|O_TRUNC|O_WRONLY))
        flags |= NEED_WRITE;
    
    name = handle_special_dirs(name, flags, realpath, sizeof(realpath));

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
    const char *fname = handle_special_dirs(name, NEED_WRITE, realpath, sizeof(realpath));

    return remove(fname);
}

int app_rename(const char *old, const char *new)
{
    char realpath_old[MAX_PATH], realpath_new[MAX_PATH];
    const char *final_old, *final_new;

    final_old = handle_special_dirs(old, NEED_WRITE, realpath_old, sizeof(realpath_old));
    final_new = handle_special_dirs(new, NEED_WRITE, realpath_new, sizeof(realpath_new));

    return rename(final_old, final_new);
}

/* need to wrap around DIR* because we need to save the parent's
 * directory path in order to determine dirinfo, required to implement
 * get_dir_info() */
struct __dir {
    DIR *dir;
    char path[];
};

struct dirinfo dir_get_info(DIR* _parent, struct dirent *dir)
{
    struct __dir *parent = (struct __dir*)_parent;
    struct stat s;
    struct tm *tm = NULL;
    struct dirinfo ret;
    char path[MAX_PATH];

    snprintf(path, sizeof(path), "%s/%s", parent->path, dir->d_name);
    memset(&ret, 0, sizeof(ret));

    if (!stat(path, &s))
    {
        if (S_ISDIR(s.st_mode))
        {
            ret.attribute = ATTR_DIRECTORY;
        }
        ret.size = s.st_size;
        tm = localtime(&(s.st_mtime));
    }

    if (!lstat(path, &s) && S_ISLNK(s.st_mode))
    {
        ret.attribute |= ATTR_LINK;
    }

    if (tm)
    {
        ret.wrtdate = ((tm->tm_year - 80) << 9) |
                            ((tm->tm_mon + 1) << 5) |
                            tm->tm_mday;
        ret.wrttime = (tm->tm_hour << 11) |
                            (tm->tm_min << 5) |
                            (tm->tm_sec >> 1);
    }

    return ret;
}
   
DIR* app_opendir(const char *_name)
{
    char realpath[MAX_PATH];
    const char *name = handle_special_dirs(_name, 0, realpath, sizeof(realpath));
    char *buf = malloc(sizeof(struct __dir) + strlen(name)+1);
    if (!buf)
        return NULL;

    struct __dir *this = (struct __dir*)buf;
    /* definitely fits due to strlen() */
    strcpy(this->path, name);

    this->dir = opendir(name);

    if (!this->dir)
    {
        free(buf);
        return NULL;
    }
    return (DIR*)this;
}

int app_closedir(DIR *dir)
{
    struct __dir *this = (struct __dir*)dir;
    int ret = closedir(this->dir);
    free(this);
    return ret;
}


struct dirent* app_readdir(DIR* dir)
{
    struct __dir *d = (struct __dir*)dir;
    return readdir(d->dir);
}


int app_mkdir(const char* name)
{
    char realpath[MAX_PATH];
    const char *fname = handle_special_dirs(name, NEED_WRITE, realpath, sizeof(realpath));
    return mkdir(fname, 0777);
}


int app_rmdir(const char* name)
{
    char realpath[MAX_PATH];
    const char *fname = handle_special_dirs(name, NEED_WRITE, realpath, sizeof(realpath));
    return rmdir(fname);
}
