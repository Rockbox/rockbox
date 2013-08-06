/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Daniel Stenberg
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
#define DIR_DEFINED
#define DIRFUNCTIONS_DEFINED
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include "config.h"
#include "system.h"
#include "ata_idle_notify.h"
#include "dir.h"
#include "file_internal.h"

#define HAVE_STATVFS (!defined(WIN32))
#define HAVE_LSTAT   (!defined(WIN32))

#if HAVE_STATVFS
#include <sys/statvfs.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#ifndef _MSC_VER
#include <dirent.h>
#include <unistd.h>
#else
#include "dir-win32.h"
#endif

#include <fcntl.h>
#ifdef HAVE_SDL_THREADS
#include "thread-sdl.h"
#else
#define sim_thread_unlock() NULL
#define sim_thread_lock(a)
#endif
#include "thread.h"
#include "kernel.h"
#include "debug.h"
#include "ata.h" /* for IF_MV et al. */
#include "rbpaths.h"
#include "load_code.h"

/* keep this in sync with file.h! */
#undef MAX_PATH /* this avoids problems when building simulator */
#define MAX_PATH 260
#define MAX_OPEN_FILES 11

/* Windows (and potentially other OSes) distinguish binary and text files.
 * Define a dummy for the others. */
#ifndef O_BINARY
#define O_BINARY 0
#endif

/* Unicode compatibility for win32 */
#if defined __MINGW32__
/* Rockbox unicode functions */
extern const unsigned char* utf8decode(const unsigned char *utf8,
                                       unsigned short *ucs);
extern unsigned char* utf8encode(unsigned long ucs, unsigned char *utf8);

/* Static buffers for the conversion results. This isn't thread safe,
 * but it's sufficient for rockbox. */
static unsigned char convbuf1[3*MAX_PATH];
static unsigned char convbuf2[3*MAX_PATH];

static wchar_t* utf8_to_ucs2(const unsigned char *utf8, void *buffer)
{
    wchar_t *ucs = buffer;

    while (*utf8)
        utf8 = utf8decode(utf8, ucs++);

    *ucs = 0;
    return buffer;
}
static unsigned char *ucs2_to_utf8(const wchar_t *ucs, unsigned char *buffer)
{
    unsigned char *utf8 = buffer;

    while (*ucs)
        utf8 = utf8encode(*ucs++, utf8);

    *utf8 = 0;
    return buffer;
}

#define UTF8_TO_OS(a)  utf8_to_ucs2(a,convbuf1)
#define OS_TO_UTF8(a)  ucs2_to_utf8(a,convbuf1)
#define DIR_T       _WDIR
#define DIRENT_T    struct _wdirent
#define STAT_T      struct _stat
extern int _wmkdir(const wchar_t*);
extern int _wrmdir(const wchar_t*);
#define MKDIR(a,b)  (_wmkdir)(UTF8_TO_OS(a))
#define RMDIR(a)    (_wrmdir)(UTF8_TO_OS(a))
#define OPENDIR(a)  (_wopendir)(UTF8_TO_OS(a))
#define READDIR(a)  (_wreaddir)(a)
#define CLOSEDIR(a) (_wclosedir)(a)
#define STAT(a,b)   (_wstat)(UTF8_TO_OS(a),b)
/* empty variable parameter list doesn't work for variadic macros,
 * so pretend the second parameter is variable too */
#define OPEN(a,...) (_wopen)(UTF8_TO_OS(a), __VA_ARGS__)
#define CLOSE(a)    (close)(a)
#define REMOVE(a)   (_wremove)(UTF8_TO_OS(a))
#define RENAME(a,b) (_wrename)(UTF8_TO_OS(a),utf8_to_ucs2(b,convbuf2))
/* readlink isn't used in the sim yet (FIXME) */
#define READLINK(a,b,c) ({ fprintf(stderr, "no readlink on windows yet"); abort(); })
#else  /* !__MINGW32__ */

#define UTF8_TO_OS(a) (a)
#define OS_TO_UTF8(a) (a)
#define DIR_T       DIR
#define DIRENT_T    struct dirent
#define STAT_T      struct stat
#define MKDIR(a,b)  (mkdir)(a,b)
#define RMDIR(a)    (rmdir)(a)
#define OPENDIR(a)  (opendir)(a)
#define READDIR(a)  (readdir)(a)
#define CLOSEDIR(a) (closedir)(a)
#define STAT(a,b)   (stat)(a,b)
/* empty variable parameter list doesn't work for variadic macros,
 * so pretend the second parameter is variable too */
#define OPEN(a, ...) (open)(a, __VA_ARGS__)
#define CLOSE(x)    (close)(x)
#define REMOVE(a)   (remove)(a)
#define RENAME(a,b) (rename)(a,b)
#define READLINK(a,b,c) (readlink)(a,b,c)

#endif /* !__MINGW32__ */

#ifndef APPLICATION
extern const char *sim_root_dir;
static int num_openfiles = 0;
static int num_opendirs  = 0;

struct dirstr_desc
{
    DIR_T             *dir;
    char              *name;
    struct sim_dirent entry;
#ifdef HAVE_MULTIVOLUME
    int               volumecounter;
#endif
};
#endif /* APPLICATION */

/** Simulator I/O engine routines **/
#define IO_YIELD_THRESHOLD 512
static struct mutex sim_io_mutex;

int ata_init(void)
{
    /* initialize the rockbox kernel objects on a rockbox thread */
    mutex_init(&sim_io_mutex);
    return 1;
}

int ata_spinup_time(void)
{
    return HZ;
}

static ssize_t sim_readwrite(int fildes, void *buf, size_t nbyte, bool writeop)
{
    void *mythread = NULL;

    if (nbyte > IO_YIELD_THRESHOLD)
        mythread = sim_thread_unlock();

    ssize_t rc = writeop ? write(fildes, buf, nbyte) :
                           read(fildes, buf, nbyte);

    if (mythread)
        sim_thread_lock(mythread);

    if (rc >= 0)
    {
        /* don't let callback mess up errno (it shouldn't be altered when
           successful) */
        int myerrno = errno;
        call_storage_idle_notifys(false);
        errno = myerrno;
    }

    return rc;
}

ssize_t sim_read(int fildes, void *buf, size_t nbyte)
{
    mutex_lock(&sim_io_mutex);
    ssize_t rc = sim_readwrite(fildes, buf, nbyte, false);
    mutex_unlock(&sim_io_mutex);
    return rc;
}

ssize_t sim_write(int fildes, const void *buf, size_t nbyte)
{
    mutex_lock(&sim_io_mutex);
    ssize_t rc = sim_readwrite(fildes, (void *)buf, nbyte, true);
    mutex_unlock(&sim_io_mutex);
    return rc;
}

#if !defined(APPLICATION)

static const char * handle_special_links(const char *link)
{
#ifdef HAVE_MULTIDRIVE
################################
    static char buffer[MAX_PATH]; /* sufficiently big */
    char vol_string[VOL_MAX_LEN + 1];
    int len = get_volume_name(-1, vol_string);

    /* link might be passed with or without HOME_DIR expanded. To handle
     * both perform substring matching (VOL_NAMES is unique enough) */
    const char *begin = strstr(link, vol_string);
    if (begin)
    {
        /* begin now points to the start of vol_string within link,
         * we want to copy the remainder of the paths, prefixed by
         * the actual mount point (the remainder might be "") */
        snprintf(buffer, sizeof(buffer), "%s/../simext/%s",
                 sim_root_dir, begin + len);
        return buffer;
    }
    else
#endif
        return link;
}


static const char * get_sim_pathname(const char *name)
{
    static char buffer[MAX_PATH]; /* sufficiently big */

    if (!PATH_IS_ABSOLUTE(name))
    {
        DEBUGF("WARNING, bad file name lacks slash: \"%s\"\n", name);
        errno = ENOENT;
        return NULL;
    }

    size_t size = append_to_path(buffer, sim_root_dir, name, sizeof (buffer));
    if (size >= sizeof (buffer))
    {
        errno = ENAMETOOLONG;
        return NULL;
    }

    return handle_special_links(buffer);
}

#ifdef HAVE_MULTIVOLUME
static int readdir_volume_inner(struct dirstr_desc *dirstr, struct sim_dirent *entry)
{
    /* Volumes (secondary file systems) get inserted into the system root
     * directory. If the path specified volume 0, enumeration will not
     * include other volumes, but just its own files and directories.
     *
     * Fake special directories, which don't really exist, that will get
     * redirected upon opendir()
     */
    while (++dir->volumecounter < NUM_VOLUMES)
    {
        /* on the system root */
        if (!volume_ismounted(dir->volumecounter))
            continue;

        get_volume_name(dirstr->volumecounter, entry->d_name);
        entry->info.attribute = ATTR_DIRECTORY | ATTR_VOLUME | ATTR_LINK;
        entry->info.size      = 0;
        entry->info.wrtdate   = 0;
        entry->info.wrttime   = 0;

        return 1;
    }

    /* do normal directory entry fetching */
    return 0;
}
#endif /* HAVE_MULTIVOLUME */

static inline int readdir_volume(struct dirstr_desc *dirstr, struct sim_dirent *entry)
{
#ifdef HAVE_MULTIVOLUME
    if (dir->volumecounter < NUM_VOLUMES && dir->volumecounter >= 0)
        return readdir_volume_inner(dir, entry);
#endif /* HAVE_MULTIVOLUME */

    /* do normal directory entry fetching */
    return 0;
    (void)dirstr; (void)entry;

#if 0
    if (dir->name[0] == '/' && dir->name[1] == '\0'
            && dir->volumes_returned++ < (NUM_VOLUMES-1)
            && volume_present(dir->volumes_returned))
    {
        sprintf((char *)secret.d_name, VOL_NAMES, dir->volumes_returned);
        secret.info.attribute = ATTR_LINK;
        /* build file name for stat() which is the actual mount point */
        snprintf(buffer, sizeof(buffer), "%s/../simext", sim_root_dir);
    }
    else
#endif
}


DIR * sim_opendir(const char *dirname)
{
    const char *simpath = get_sim_pathname(dirname);
    if (!simpath)
        return NULL;

    DIR_T *hostdirp = (DIR_T *)OPENDIR(simpath);
    if (!hostdirp)
        return NULL;

    struct dirstr_desc *dirstr = malloc(sizeof (*dirstr));
    if (dirstr)
    {
        dirstr->hostdirp = hostdirp;
        dirstr->name     = strdup(name);

        if (dirstr->name)
        {
        #ifdef HAVE_MULTIVOLUME
            const char *basename;
            size_t length = path_basename(name, &basename);
            dirstr->volumecounter = basename[0] == PATH_SEPCH ? 0 : -1;
        #endif

            return (DIR *)dirstr; /* A-Okay */
        }

        free(dirstr);
    }

    closedir(hostdirp);

    /* failed open, return NULL */
    return NULL;
}

#if defined(WIN32)
static inline struct tm * localtime_r(const time_t *clock, struct tm *result)
{
    if (!clock || !result)
        return NULL;

    memcpy(result, localtime(clock), sizeof(*result));
    return result;
}
#endif



struct sim_dirent * sim_readdir(DIR *dirp)
{
    struct dirstr_desc *dirstr = GET_DIRSTR(dirp);
    if (!dirstr)
        return NULL;

    char buffer[MAX_PATH]; /* sufficiently big */
    STAT_T s;
    struct tm tm;
#ifdef EOVERFLOW
read_next:
#endif

    dirstr->entry.info.attribute = 0;

    if (!readdir_volume(dirstr, &dirstr->entry))
    {
        DIRENT_T *dirent = READDIR(dirstr->dir);
        if (!dirent)
            return NULL;

        strcpy((char *)secret.d_name, OS_TO_UTF8(x11->d_name));
        /* build file name for stat() */
        snprintf(buffer, sizeof(buffer), "%s/%s",
                get_sim_pathname(dir->name), secret.d_name);
    }

    if (STAT(buffer, &s)) /* get info */
    {
#ifdef EOVERFLOW
        /* File size larger than 2 GB? */
        if (errno == EOVERFLOW)
        {
            DEBUGF("stat() overflow for %s. Skipping\n", buffer);
            goto read_next;
        }
#endif

        return NULL;
    }

    if (S_ISDIR(s.st_mode))
        secret.info.attribute = ATTR_DIRECTORY;

    secret.info.size = s.st_size;
    
    if (localtime_r(&(s.st_mtime), &tm) == NULL)
        return NULL;

    secret.info.wrtdate = ((tm.tm_year - 80) << 9) |
                        ((tm.tm_mon + 1) << 5) |
                        tm.tm_mday;
    secret.info.wrttime = (tm.tm_hour << 11) |
                        (tm.tm_min << 5) |
                        (tm.tm_sec >> 1);

#if HAVE_LSTAT
    if (!lstat(buffer, &s) && S_ISLNK(s.st_mode))
    {
        secret.info.attribute |= ATTR_LINK;
    }
#endif

    return &secret;
}

int sim_closedir(DIR *dirp)
{
    struct dirstr_desc *dirstr = GET_DIRSTR(dirp);
    if (!dirstr)
        return -1;

    DIR_T *dir = dirstr->dir;
    free(dirstr->name);
    free(dirstr);

    return CLOSEDIR(dir);
}

int sim_open(const char *path, int oflag, ...)
{
    int fildes;

    int rboflag = oflag | O_BINARY;
    if (num_openfiles >= MAX_OPEN_FILES)
        return -2;

    if (rboflag & O_CREAT)
    {
        /* read the mode argument */
        va_list ap;
        va_start(ap, o);
        mode_t mode = va_arg(ap, mode_t);
        ret = OPEN(get_sim_pathname(name), opts, mode);
#ifdef HAVE_DIRCACHE
############################
        if (ret >= 0 && (dircache_get_entry_id(name) < 0))
            dircache_add_file(name, 0);
#endif
        va_end(ap);
    }
    else
        ret = OPEN(get_sim_pathname(name), opts);

    if (ret >= 0)
        num_openfiles++;

    return ret;
}

int sim_close(int fd)
{
    int ret = CLOSE(fd);
    if (ret == 0)
        num_openfiles--;

    return ret;
}

int sim_creat(const char *name, mode_t mode)
{
    return sim_open(name, O_WRONLY|O_CREAT|O_TRUNC, mode);
}

int sim_mkdir(const char *name)
{
    return MKDIR(get_sim_pathname(name), 0777);
}

int sim_rmdir(const char *name)
{
    return RMDIR(get_sim_pathname(name));
}

int sim_remove(const char *name)
{
    int ret = REMOVE(get_sim_pathname(name));
#ifdef HAVE_DIRCACHE
    ################################################
    if (ret >= 0)
        dircache_remove(name);
#endif
    return ret;
}

int sim_rename(const char *oldname, const char *newname)
{
    char sim_old[MAX_PATH];
    char sim_new[MAX_PATH];
#ifdef HAVE_DIRCACHE
    dircache_rename(oldname, newname);
#endif
    // This is needed as get_sim_pathname() has a static buffer
    strncpy(sim_old, get_sim_pathname(oldname), MAX_PATH);
    strncpy(sim_new, get_sim_pathname(newname), MAX_PATH);
    return RENAME(sim_old, sim_new);
}

/* rockbox off_t may be different from system off_t */
long sim_lseek(int fildes, long offset, int whence)
{
    return lseek(fildes, offset, whence);
}

#else
#define get_sim_pathname(x) x
#endif

long filesize(int fd)
{
#ifdef WIN32
    return _filelength(fd);
#else
    struct stat buf;

    if (!fstat(fd, &buf))
        return buf.st_size;
    else
        return -1;
#endif
}

void fat_size(IF_MV(int volume,) unsigned long* size, unsigned long* free)
{
#ifdef HAVE_MULTIVOLUME
    if (volume != 0) {
        /* debugf("io.c: fat_size(volume=%d); simulator only supports volume 0\n",volume); */

        if (size) *size = 0;
        if (free) *free = 0;
        return;
    }
#endif

#ifdef WIN32
    long secperclus, bytespersec, free_clusters, num_clusters;

    if (GetDiskFreeSpace(NULL, &secperclus, &bytespersec, &free_clusters,
                     &num_clusters)) {
        if (size)
            *size = num_clusters * secperclus / 2 * (bytespersec / 512);
        if (free)
            *free = free_clusters * secperclus / 2 * (bytespersec / 512);
    } else
#elif HAVE_STATVFS
    struct statvfs vfs;

    if (!statvfs(".", &vfs)) {
        DEBUGF("statvfs: frsize=%d blocks=%ld free=%ld\n",
               (int)vfs.f_frsize, (long)vfs.f_blocks, (long)vfs.f_bfree);
        if (size)
            *size = vfs.f_blocks / 2 * (vfs.f_frsize / 512);
        if (free)
            *free = vfs.f_bfree / 2 * (vfs.f_frsize / 512);
    } else
#endif
    {
        if (size)
            *size = 0;
        if (free)
            *free = 0;
    }
}

int sim_fsync(int fd)
{
#ifdef WIN32
    return _commit(fd);
#else
    return fsync(fd);
#endif
}

#ifndef __PCTOOL__

#include <SDL_loadso.h>
void *lc_open(const char *filename, unsigned char *buf, size_t buf_size)
{
    filename = get_sim_pathname(filename);
    if (!filename)
        return NULL;

    void *handle = SDL_LoadObject(filename);
    if (handle == NULL)
    {
        DEBUGF("failed to load %s\n", filename);
        DEBUGF("lc_open(%s): %s\n", filename, SDL_GetError());
    }

    return handle;
    (void)buf; (void)buf_size;
}

void *lc_get_header(void *handle)
{
    char *ret = SDL_LoadFunction(handle, "__header");
    if (ret == NULL)
        ret = SDL_LoadFunction(handle, "___header");

    return ret;
}

void lc_close(void *handle)
{
    SDL_UnloadObject(handle);
}

void *lc_open_from_mem(void *addr, size_t blob_size)
{
#ifndef SIMULATOR
    (void)addr;
    (void)blob_size;
    /* we don't support loading code from memory on application builds,
     * it doesn't make sense (since it means writing the blob to disk again and
     * then falling back to load from disk) and requires the ability to write
     * to an executable directory */
    return NULL;
#else
    /* support it in the sim for the sake of simulating */
    int fd, i;
    char temp_filename[MAX_PATH];

    /* We have to create the dynamic link library file from ram so we
       can simulate the codec loading. With voice and crossfade,
       multiple codecs may be loaded at the same time, so we need
       to find an unused filename */
    for (i = 0; i < 10; i++)
    {
        snprintf(temp_filename, sizeof(temp_filename),
                 ROCKBOX_DIR "/libtemp_binary_%d.dll", i);
        fd = open(temp_filename, O_WRONLY|O_CREAT|O_TRUNC, 0700);
        if (fd >= 0)
            break;  /* Created a file ok */
    }

    if (fd < 0)
    {
        DEBUGF("open failed\n");
        return NULL;
    }

    if (write(fd, addr, blob_size) < (ssize_t)blob_size)
    {
        DEBUGF("Write failed\n");
        close(fd);
        remove(temp_filename);
        return NULL;
    }

    close(fd);
    return lc_open(temp_filename, NULL, 0);
#endif
}

#endif /* __PCTOOL__ */

/* rockbox off_t may be different from system off_t */
int sim_ftruncate(int fd, long length)
{
#ifdef WIN32
    return _chsize(fd, length);
#else
    return ftruncate(fd, length);
#endif
}
