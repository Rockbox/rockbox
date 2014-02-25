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
#include "crc32.h"
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
#undef readlink

#if (CONFIG_PLATFORM & PLATFORM_ANDROID)
static const char rbhome[] = "/sdcard";
#elif (CONFIG_PLATFORM & (PLATFORM_SDL|PLATFORM_MAEMO|PLATFORM_PANDORA)) && !defined(__PCTOOL__)
const char *rbhome;
#else
/* YPR0, YPR1 */
static const char rbhome[] = HOME_DIR;
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

#ifdef HAVE_MULTIDRIVE
static uint32_t rbhome_hash;
/* A special link is created under e.g. HOME_DIR/<microSD1>, e.g. to access
 * external storage in a convinient location, much similar to the mount
 * point on our native targets. Here they are treated as symlink (one which
 * doesn't actually exist in the filesystem and therefore we have to override
 * readlink() */
static const char *handle_special_links(const char* link, unsigned flags,
                                char *buf, const size_t bufsize)
{
    (void) flags;
    char vol_string[VOL_ENUM_POS + 8];
    int len = sprintf(vol_string, VOL_NAMES, 1);

    /* link might be passed with or without HOME_DIR expanded. To handle
     * both perform substring matching (VOL_NAMES is unique enough) */
    const char *begin = strstr(link, vol_string);
    if (begin)
    {
        /* begin now points to the start of vol_string within link,
         * we want to copy the remainder of the paths, prefixed by
         * the actual mount point (the remainder might be "") */
        snprintf(buf, bufsize, MULTIDRIVE_DIR"%s", begin + len);
        return buf;
    }

    return link;
}
#endif

void paths_init(void)
{
#ifdef HAVE_SPECIAL_DIRS
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
#endif /* HAVE_SPECIAL_DIRS */

#ifdef HAVE_MULTIDRIVE
    rbhome_hash = crc_32((const void *) rbhome, strlen(rbhome), 0xffffffff);
#endif /* HAVE_MULTIDRIVE */
}

#ifdef HAVE_SPECIAL_DIRS
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

#endif

static const char* handle_special_dirs(const char* dir, unsigned flags,
                                char *buf, const size_t bufsize)
{
    (void) flags; (void) buf; (void) bufsize;
#ifdef HAVE_SPECIAL_DIRS
    if (!strncmp(HOME_DIR, dir, HOME_DIR_LEN))
    {
        const char *p = dir + HOME_DIR_LEN;
        while (*p == '/') p++;
        snprintf(buf, bufsize, "%s/%s", rbhome, p);
        dir = buf;
    }
    else if (!strncmp(ROCKBOX_DIR, dir, ROCKBOX_DIR_LEN))
        dir = _get_user_file_path(dir, flags, buf, bufsize);
#endif
#ifdef HAVE_MULTIDRIVE
    dir = handle_special_links(dir, flags, buf, bufsize);
#endif
    return dir;
}

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
#ifdef HAVE_MULTIDRIVE
    int volumes_returned;
    /* A crc of rbhome is used to speed op the common case where
     * readdir()/get_dir_info() is called on non-rbhome paths, because
     * each call needs to check against rbhome for the virtual
     * mount point of the external storage */
    uint32_t path_hash;
#endif
    char path[];
};

struct dirinfo dir_get_info(DIR* _parent, struct dirent *dir)
{
    struct __dir *parent = (struct __dir*)_parent;
    struct stat s;
    struct tm *tm = NULL;
    struct dirinfo ret;
    char path[MAX_PATH];

    memset(&ret, 0, sizeof(ret));

#ifdef HAVE_MULTIDRIVE
    char vol_string[VOL_ENUM_POS + 8];
    sprintf(vol_string, VOL_NAMES, 1);
    if (UNLIKELY(rbhome_hash == parent->path_hash) &&
            /* compare path anyway because of possible hash collision */
            !strcmp(vol_string, dir->d_name))
    {
        ret.attribute = ATTR_LINK;
        strcpy(path, MULTIDRIVE_DIR);
    }
    else
#endif
        snprintf(path, sizeof(path), "%s/%s", parent->path, dir->d_name);


    if (!lstat(path, &s))
    {
        int err = 0;
        if (S_ISLNK(s.st_mode))
        {
            ret.attribute |= ATTR_LINK;
            err = stat(path, &s);
        }
        if (!err)
        {
            if (S_ISDIR(s.st_mode))
                ret.attribute |= ATTR_DIRECTORY;

            ret.size = s.st_size;
            if ((tm = localtime(&(s.st_mtime))))
            {
                ret.wrtdate = ((tm->tm_year - 80) << 9) |
                                  ((tm->tm_mon + 1) << 5) |
                                    tm->tm_mday;
                ret.wrttime = (tm->tm_hour << 11) |
                                  (tm->tm_min << 5) |
                                  (tm->tm_sec >> 1);
            }
        }
    }

    return ret;
}
   
DIR* app_opendir(const char *_name)
{
    size_t name_len;
    char realpath[MAX_PATH];
    const char *name = handle_special_dirs(_name, 0, realpath, sizeof(realpath));
    name_len = strlen(name);
    char *buf = malloc(sizeof(struct __dir) + name_len+1);
    if (!buf)
        return NULL;

    struct __dir *this = (struct __dir*)buf;
    /* carefully remove any trailing slash from the input, so that
     * hash/path matching in readdir() works properly */
    while (name[name_len-1] == '/')
        name_len -= 1;
    /* strcpy cannot be used because of trailing slashes */
    memcpy(this->path, name, name_len);
    this->path[name_len] = 0;
    this->dir = opendir(this->path);

    if (!this->dir)
    {
        free(buf);
        return NULL;
    }
#ifdef HAVE_MULTIDRIVE
    this->volumes_returned = 0;
    this->path_hash = crc_32((const void *)this->path, name_len, 0xffffffff);
#endif

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
#ifdef HAVE_MULTIDRIVE
    /* this is not MT-safe but OK according to man readdir */
    static struct dirent voldir;
    if (UNLIKELY(rbhome_hash == d->path_hash)
            && d->volumes_returned < (NUM_VOLUMES-1)
            && volume_present(d->volumes_returned+1)
            /* compare path anyway because of possible hash collision */
            && !strcmp(d->path, rbhome))
    {
        d->volumes_returned += 1;
        sprintf(voldir.d_name, VOL_NAMES, d->volumes_returned);
        return &voldir;
    }
#endif
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


/* On MD we create a virtual symlink for the external drive,
 * for this we need to override readlink(). */
ssize_t app_readlink(const char *path, char *buf, size_t bufsiz)
{
    char _buf[MAX_PATH];
    (void) path; (void) buf; (void) bufsiz;
    path = handle_special_dirs(path, 0, _buf, sizeof(_buf));
#ifdef HAVE_MULTIDRIVE
    /* if path == _buf then we can be sure handle_special_dir() did something
     * and path is not an ordinary directory */
    if (path == _buf && !strncmp(path, MULTIDRIVE_DIR, sizeof(MULTIDRIVE_DIR)-1))
    {
        /* copying NUL is not required as per readlink specification */
        ssize_t len = strlen(path);
        memcpy(buf, path, len);
        return len;
    }
#endif
    /* does not append NUL !! */
    return readlink(path, buf, bufsiz);
}
