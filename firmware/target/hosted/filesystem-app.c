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
 * Copyright (C) 2020 Solomon Peachy
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
#define RB_FILESYSTEM_OS
#include <stdio.h> /* snprintf */
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include "config.h"
#include "system.h"
#include "file.h"
#include "dir.h"
#include "file_internal.h"
#include "pathfuncs.h"
#include "string-extra.h"
#include "rbpaths.h"
#include "logf.h"

/* PIVOT_ROOT adds a fixed prefix to all paths */
#if defined(BOOTLOADER) || defined(CHECKWPS) || defined(SIMULATOR) || defined(__PCTOOL__)
/* We don't want to use pivot root on these! */
#undef PIVOT_ROOT
#endif

#if defined(__PCTOOL__)
/* We don't want this for tools */
#undef HAVE_SPECIAL_DIRS
#endif

#if defined(HAVE_MULTIDRIVE) || defined(HAVE_SPECIAL_DIRS)
#if (CONFIG_PLATFORM & PLATFORM_ANDROID)
static const char rbhome[] = "/sdcard";
#elif (CONFIG_PLATFORM & (PLATFORM_SDL|PLATFORM_MAEMO|PLATFORM_PANDORA)) \
        && !defined(__PCTOOL__)
static const char *rbhome;
#elif defined(PIVOT_ROOT)
static const char rbhome[] = PIVOT_ROOT;
#else
/* Anything else? */
static const char rbhome[] = HOME_DIR;
#endif
#endif

#ifdef HAVE_MULTIDRIVE
/* This is to compare any opened directories with the home directory so that
   the special drive links may be returned for it only */
static int rbhome_fildes = -1;

/* A special link is created under e.g. HOME_DIR/<microSD1>, e.g. to access
 * external storage in a convenient location, much similar to the mount
 * point on our native targets. Here they are treated as symlink (one which
 * doesn't actually exist in the filesystem and therefore we have to override
 * readlink() */
static const char *handle_special_links(const char* link, unsigned flags,
                                char *buf, const size_t bufsize)
{
    (void) flags;
    char vol_string[VOL_MAX_LEN + 1];
    int len;

    for (int i = 1; i < NUM_VOLUMES; i++)
    {
        len = get_volume_name(i, vol_string);
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
    }

    return link;
}

/* we keep an open descriptor of the home directory to detect when it has been
   opened by opendir() so that its "symlinks" may be enumerated */
void cleanup_rbhome(void)
{
    os_close(rbhome_fildes);
    rbhome_fildes = -1;
}
void startup_rbhome(void)
{
    /* if this fails then alternate volumes will not work, but this function
       cannot return that fact */
    rbhome_fildes = os_opendirfd(rbhome);
    if (rbhome_fildes >= 0)
        atexit(cleanup_rbhome);
}

#endif /* HAVE_MULTIDRIVE */

void paths_init(void)
{
#ifdef HAVE_SPECIAL_DIRS
    /* make sure $HOME/.config/rockbox.org exists, it's needed for config.cfg */
#if (CONFIG_PLATFORM & PLATFORM_ANDROID)
    os_mkdir("/sdcard/rockbox" __MKDIR_MODE_ARG);
    os_mkdir("/sdcard/rockbox/rocks.data" __MKDIR_MODE_ARG);
    os_mkdir("/sdcard/rockbox/eqs" __MKDIR_MODE_ARG);
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
    os_mkdir(config_dir __MKDIR_MODE_ARG);
    snprintf(config_dir, sizeof(config_dir), "%s/.config/rockbox.org", home);
    os_mkdir(config_dir __MKDIR_MODE_ARG);
    /* Plugin data directory */
    snprintf(config_dir, sizeof(config_dir), "%s/.config/rockbox.org/rocks.data", home);
    os_mkdir(config_dir __MKDIR_MODE_ARG);
#endif
#endif /* HAVE_SPECIAL_DIRS */
#ifdef HAVE_MULTIDRIVE
    startup_rbhome();
#endif
}

#ifdef HAVE_SPECIAL_DIRS
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
    if (path_append(buf, "/sdcard/rockbox", pos, bufsize) >= bufsize)
        return NULL;
#else
    if (path_append(buf, rbhome, ".config/rockbox.org", bufsize) >= bufsize ||
        path_append(buf, PA_SEP_SOFT, pos, bufsize) >= bufsize)
        return NULL;
#endif

    /* always return the replacement buffer (pointing to $HOME) if
     * write access is needed */
    if (flags & NEED_WRITE)
        ret = buf;
    else if (os_file_exists(buf))
        ret = buf;

    if (ret != buf) /* not found in $HOME, try ROCKBOX_BASE_DIR, !NEED_WRITE only */
    {
        if (path_append(buf, ROCKBOX_SHARE_PATH, pos, bufsize) >= bufsize)
            return NULL;

        if (os_file_exists(buf))
            ret = buf;
    }

    return ret;
}

#endif

const char * handle_special_dirs(const char *dir, unsigned flags,
                                 char *buf, const size_t bufsize)
{
    (void) flags; (void) buf; (void) bufsize;
#ifdef HAVE_SPECIAL_DIRS
#define HOME_DIR_LEN (sizeof(HOME_DIR)-1)
    /* This replaces HOME_DIR (ie '<HOME'>') with runtime rbhome */
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
#define MULTIDRIVE_DIR_LEN (sizeof(MULTIDRIVE_DIR)-1)

    dir = handle_special_links(dir, flags, buf, bufsize);
#endif
#ifdef PIVOT_ROOT
#define PIVOT_ROOT_LEN (sizeof(PIVOT_ROOT)-1)
    /* Prepend root prefix to find actual path */
    if (strncmp(PIVOT_ROOT, dir, PIVOT_ROOT_LEN)
#ifdef MULTIDRIVE_DIR
	/* Unless it's a MULTIDRIVE dir, in which case use as-is */
	&& strncmp(MULTIDRIVE_DIR, dir, MULTIDRIVE_DIR_LEN)
#endif
       )
    {
        snprintf(buf, bufsize, "%s/%s", PIVOT_ROOT, dir);
        dir = buf;
    }
#endif
    return dir;
}

int app_open(const char *path, int oflag, ...)
{
    int flags = IS_FILE;
    if (oflag & O_ACCMODE)
        flags |= NEED_WRITE;

    char realpath[MAX_PATH];
    const char *fpath = handle_special_dirs(path, flags, realpath,
                                            sizeof (realpath));
    if (!fpath)
        FILE_ERROR_RETURN(ENAMETOOLONG, -1);

    oflag |= O_CLOEXEC;
    return os_open(fpath, oflag __OPEN_MODE_ARG);
}

int app_creat(const char *path, mode_t mode)
{
    return app_open(path, O_CREAT|O_WRONLY|O_TRUNC, mode);
}

int app_remove(const char *path)
{
    char realpath[MAX_PATH];
    const char *fpath = handle_special_dirs(path, NEED_WRITE, realpath,
                                            sizeof (realpath));
    if (!fpath)
        FILE_ERROR_RETURN(ENAMETOOLONG, -1);

    return os_remove(fpath);
}

int app_rename(const char *old, const char *new)
{
    char realpath_old[MAX_PATH], realpath_new[MAX_PATH];
    const char *fold = handle_special_dirs(old, NEED_WRITE, realpath_old,
                                           sizeof (realpath_old));
    const char *fnew = handle_special_dirs(new, NEED_WRITE, realpath_new,
                                           sizeof (realpath_new));
    if (!fold || !fnew)
        FILE_ERROR_RETURN(ENAMETOOLONG, -1);

    return os_rename(fold, fnew);
}

#ifdef HAVE_SDL_THREADS
ssize_t app_read(int fd, void *buf, size_t nbyte)
{
    return os_read(fd, buf, nbyte);
}

ssize_t app_write(int fd, const void *buf, size_t nbyte)
{
    return os_write(fd, buf, nbyte);
}
#endif /* HAVE_SDL_THREADS */

int app_relate(const char *path1, const char *path2)
{
    char realpath_1[MAX_PATH], realpath_2[MAX_PATH];
    const char *fpath1 = handle_special_dirs(path1, 0, realpath_1,
                                             sizeof (realpath_1));
    const char *fpath2 = handle_special_dirs(path2, 0, realpath_2,
                                             sizeof (realpath_2));

    if (!fpath1 || !fpath2)
        FILE_ERROR_RETURN(ENAMETOOLONG, -1);

    return os_relate(fpath1, fpath2);
}

bool app_file_exists(const char *path)
{
    char realpath[MAX_PATH];
    const char *fpath = handle_special_dirs(path, NEED_WRITE, realpath,
                                            sizeof (realpath));
    if (!fpath)
        FILE_ERROR_RETURN(ENAMETOOLONG, false);

    return os_file_exists(fpath);
}

/* need to wrap around DIR* because we need to save the parent's directory
 * path in order to determine dirinfo for volumes or convert the path to UTF-8;
 * also is required to implement get_dir_info() */
struct __dir
{
    OS_DIR_T *osdirp;
#ifdef HAVE_MULTIDRIVE
    int volumes_returned;
#endif
    int osfd;
    bool osfd_is_opened;
#if defined(OS_DIRENT_CONVERT) || defined (HAVE_MULTIDRIVE)
    #define USE_DIRENTP
    struct dirent *direntp;
    size_t d_name_size;
#endif
    char path[];
};

static void __dir_free(struct __dir *this)
{
    if (!this)
        return;

#ifdef USE_DIRENTP
    free(this->direntp);
#endif

    if (this->osfd_is_opened)
        os_close(this->osfd);

    free(this);
}

DIR * app_opendir(const char *dirname)
{
    int rc;
    char realpath[MAX_PATH];
    const char *fname = handle_special_dirs(dirname, 0, realpath,
                                            sizeof (realpath));
    if (!fname)
        FILE_ERROR_RETURN(ENAMETOOLONG, NULL);

    size_t name_len = path_strip_trailing_separators(fname, &fname);
    struct __dir *this = calloc(1, sizeof (*this) + name_len + 1);
    if (!this)
        FILE_ERROR(ENOMEM, RC);

#ifdef USE_DIRENTP
    /* allocate what we're really going to return to callers, making certain
       it has at least the d_name size we want */
    this->d_name_size = MAX(MAX_PATH, sizeof (this->direntp->d_name));
    this->direntp = calloc(1, offsetof(typeof (*this->direntp), d_name) +
                                 this->d_name_size);
    if (!this->direntp)
        FILE_ERROR(ENOMEM, RC);

    /* only the d_name field will be valid but that is all that anyone may
       truely count on portably existing */
#endif /* USE_DIRENTP */

    strmemcpy(this->path, fname, name_len);

    rc = os_opendir_and_fd(this->path, &this->osdirp, &this->osfd);
    if (rc < 0)
        FILE_ERROR(ERRNO, RC);

    this->osfd_is_opened = rc > 0;

#ifdef HAVE_MULTIDRIVE
    this->volumes_returned = INT_MAX; /* assume NOT $HOME */
    if (rbhome_fildes >= 0 && os_fsamefile(rbhome_fildes, this->osfd) > 0)
        this->volumes_returned = 0; /* there's no place like $HOME */
#endif /* HAVE_MULTIDRIVE */

    return (DIR *)this;
file_error:
    __dir_free(this);
    return NULL;
}

int app_closedir(DIR *dirp)
{
    struct __dir *this = (struct __dir *)dirp;
    if (!this)
        FILE_ERROR_RETURN(EBADF, -1);

    OS_DIR_T *osdirp = this->osdirp;
    __dir_free(this);

    return os_closedir(osdirp);
}

struct dirent * app_readdir(DIR *dirp)
{
    struct __dir *this = (struct __dir *)dirp;
    if (!this)
        FILE_ERROR_RETURN(EBADF, NULL);

#ifdef HAVE_MULTIDRIVE
    if (this->volumes_returned < NUM_VOLUMES)
    {
        while (++this->volumes_returned < NUM_VOLUMES)
        {
            if (!volume_present(this->volumes_returned))
                continue;

            get_volume_name(this->volumes_returned, this->direntp->d_name);
            return this->direntp;
        }
    }
    /* do normal directory reads */
#endif /* HAVE_MULTIDRIVE */

    OS_DIRENT_T *osdirent = os_readdir(this->osdirp);

#ifdef OS_DIRENT_CONVERT
    if (strlcpy_from_os(this->direntp->d_name, osdirent->d_name,
                        this->d_name_size) >= this->d_name_size)
    {
        this->direntp->d_name[0] = '\0';
        errno = EOVERFLOW;
        return NULL;
    }

    osdirent = (OS_DIRENT_T *)this->direntp;
#endif /* OS_DIRENT_CONVERT */

    return (struct dirent *)osdirent;
}

int app_mkdir(const char *path)
{
    char realpath[MAX_PATH];
    const char *fname = handle_special_dirs(path, NEED_WRITE, realpath,
                                            sizeof (realpath));
    if (!fname)
        FILE_ERROR_RETURN(ENAMETOOLONG, -1);

    return os_mkdir(fname __MKDIR_MODE_ARG);
}

int app_rmdir(const char *path)
{
    char realpath[MAX_PATH];
    const char *fname = handle_special_dirs(path, NEED_WRITE, realpath,
                                            sizeof (realpath));
    if (!fname)
        FILE_ERROR_RETURN(ENAMETOOLONG, -1);

    return os_rmdir(fname);
}

int app_samedir(DIR *dirp1, DIR *dirp2)
{
    struct __dir *this1 = (struct __dir *)dirp1;
    struct __dir *this2 = (struct __dir *)dirp2;

    if (!this1 || !this2)
    {
        errno = EBADF;
        return -1;
    }

    return os_fsamefile(this1->osfd, this2->osfd);
}

bool app_dir_exists(const char *dirname)
{
    char realpath[MAX_PATH];
    const char *fname = handle_special_dirs(dirname, 0, realpath,
                                            sizeof (realpath));
    if (!fname)
        FILE_ERROR_RETURN(ENAMETOOLONG, false);

    OS_DIR_T *osdirp = os_opendir(fname);
    if (!osdirp)
        return false;

    os_closedir(osdirp);
    return true;
}

struct dirinfo dir_get_info(DIR *dirp, struct dirent *entry)
{
    struct __dir *this = (struct __dir *)dirp;
    struct dirinfo ret = { .attribute = 0,
                           .mtime = 0 };

    if (!this)
        FILE_ERROR_RETURN(EBADF, ret);

    if (!entry || entry->d_name[0] == '\0')
        FILE_ERROR_RETURN(ENOENT, ret);

    char path[MAX_PATH];

#ifdef HAVE_MULTIDRIVE
    if (this->volumes_returned < NUM_VOLUMES)
    {
        /* last thing read was a "symlink" */
        ret.attribute = ATTR_LINK;
        strcpy(path, MULTIDRIVE_DIR);
    }
    else
#endif
    if (path_append(path, this->path, entry->d_name, sizeof (path))
            >= sizeof (path))
    {
        FILE_ERROR_RETURN(ENAMETOOLONG, ret);
    }

    struct stat s;
    if (os_lstat(path, &s) < 0)
        FILE_ERROR_RETURN(ERRNO, ret);

    int err = 0;
    if (S_ISLNK(s.st_mode))
    {
        ret.attribute |= ATTR_LINK;
        err = os_stat(path, &s);
    }

    if (err < 0)
        FILE_ERROR_RETURN(ERRNO, ret);

    if (S_ISDIR(s.st_mode))
        ret.attribute |= ATTR_DIRECTORY;

    ret.size = s.st_size;

    struct tm tm;
    if (!localtime_r(&s.st_mtime, &tm))
        FILE_ERROR_RETURN(ERRNO, ret);

    ret.mtime = mktime(&tm);
    return ret;
}

/* On MD we create a virtual symlink for the external drive,
 * for this we need to override readlink(). */
ssize_t app_readlink(const char *path, char *buf, size_t bufsiz)
{
    char _buf[MAX_PATH];
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
    return os_readlink(path, buf, bufsiz);
    (void) path; (void) buf; (void) bufsiz;
}

int os_volume_path(IF_MV(int volume, ) char *buffer, size_t bufsize)
{
#ifdef HAVE_MULTIVOLUME
    char volname[VOL_MAX_LEN + 1];
    get_volume_name(volume, volname);
#else
    const char *volname = "/";
#endif

    if (!handle_special_dirs(volname, NEED_WRITE, buffer, bufsize))
    {
        errno = ENAMETOOLONG;
        return -1;
    }

    return 0;
}
