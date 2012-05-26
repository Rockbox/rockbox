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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include "config.h"

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
#include "ata.h" /* for IF_MV2 et al. */
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

#endif /* !__MINGW32__ */


#ifdef HAVE_DIRCACHE
int dircache_get_entry_id(const char *filename);
void dircache_add_file(const char *name, long startcluster);
void dircache_remove(const char *name);
void dircache_rename(const char *oldname, const char *newname);
#endif


#define SIMULATOR_DEFAULT_ROOT "simdisk"
extern const char *sim_root_dir;

static int num_openfiles = 0;

/* from dir.h */
struct dirinfo {
    int attribute;
    long size;
    unsigned short wrtdate;
    unsigned short wrttime;
};

struct sim_dirent {
    unsigned char d_name[MAX_PATH];
    struct dirinfo info;
    long startcluster;
};

struct dirstruct {
    void *dir; /* actually a DIR* dir */
    char *name;
} SIM_DIR;

struct mydir {
    DIR_T *dir;
    char *name;
};

typedef struct mydir MYDIR;

static unsigned int rockbox2sim(int opt)
{
#if 0
/* this shouldn't be needed since we use the host's versions */
    int newopt = O_BINARY;

    if(opt & 1)
        newopt |= O_WRONLY;
    if(opt & 2)
        newopt |= O_RDWR;
    if(opt & 4)
        newopt |= O_CREAT;
    if(opt & 8)
        newopt |= O_APPEND;
    if(opt & 0x10)
        newopt |= O_TRUNC;

    return newopt;
#else
    return opt|O_BINARY;
#endif
}

/** Simulator I/O engine routines **/
#define IO_YIELD_THRESHOLD 512

enum io_dir
{
    IO_READ,
    IO_WRITE,
};

struct sim_io
{
    struct mutex sim_mutex; /* Rockbox mutex */
    int cmd; /* The command to perform */
    int ready; /* I/O ready flag - 1= ready */
    int fd; /* The file to read/write */
    void *buf; /* The buffer to read/write */
    size_t count; /* Number of bytes to read/write */
    size_t accum; /* Acculated bytes transferred */
};

static struct sim_io io;

int ata_init(void)
{
    /* Initialize the rockbox kernel objects on a rockbox thread */
    mutex_init(&io.sim_mutex);
    io.accum = 0;
    return 1;
}

int ata_spinup_time(void)
{
    return HZ;
}

static ssize_t io_trigger_and_wait(enum io_dir cmd)
{
    void *mythread = NULL;
    ssize_t result;

    if (io.count > IO_YIELD_THRESHOLD ||
        (io.accum += io.count) >= IO_YIELD_THRESHOLD)
    {
        /* Allow other rockbox threads to run */
        io.accum = 0;
        mythread = sim_thread_unlock();
    }

    switch (cmd)
    {
    case IO_READ:
        result = read(io.fd, io.buf, io.count);
        break;
    case IO_WRITE:
        result = write(io.fd, io.buf, io.count);
        break;
        /* shut up gcc */
    default:
        result = -1;
    }

    /* Regain our status as current */
    if (mythread != NULL)
    {
        sim_thread_lock(mythread);
    }

    return result;
}

#if !defined(__PCTOOL__) && !defined(APPLICATION)
static const char *get_sim_pathname(const char *name)
{
    static char buffer[MAX_PATH]; /* sufficiently big */

    if(name[0] == '/')
    {
        snprintf(buffer, sizeof(buffer), "%s%s", 
            sim_root_dir != NULL ? sim_root_dir : SIMULATOR_DEFAULT_ROOT, name);
        return buffer;
    }
    fprintf(stderr, "WARNING, bad file name lacks slash: %s\n", name);
    return name;
}
#else
#define get_sim_pathname(name) name
#endif

MYDIR *sim_opendir(const char *name)
{
    DIR_T *dir;
    dir = (DIR_T *) OPENDIR(get_sim_pathname(name));

    if (dir)
    {
        MYDIR *my = (MYDIR *)malloc(sizeof(MYDIR));
        my->dir = dir;
        my->name = (char *)malloc(strlen(name)+1);
        strcpy(my->name, name);

        return my;
    }
    /* failed open, return NULL */
    return (MYDIR *)0;
}

#if defined(WIN32)
static inline struct tm* localtime_r (const time_t *clock, struct tm *result) {
       if (!clock || !result) return NULL;
       memcpy(result,localtime(clock),sizeof(*result));
       return result;
}
#endif

struct sim_dirent *sim_readdir(MYDIR *dir)
{
    char buffer[MAX_PATH]; /* sufficiently big */
    static struct sim_dirent secret;
    STAT_T s;
    struct tm tm;
    DIRENT_T *x11;

#ifdef EOVERFLOW
read_next:
#endif
    x11 = READDIR(dir->dir);

    if(!x11)
        return (struct sim_dirent *)0;

    strcpy((char *)secret.d_name, OS_TO_UTF8(x11->d_name));

    /* build file name */
    snprintf(buffer, sizeof(buffer), "%s/%s", 
        get_sim_pathname(dir->name), secret.d_name);

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

#define ATTR_DIRECTORY 0x10

    secret.info.attribute = 0;

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
#define ATTR_LINK      0x80
    if (!lstat(buffer, &s) && S_ISLNK(s.st_mode))
    {
        secret.info.attribute |= ATTR_LINK;
    }
#endif

    return &secret;
}

void sim_closedir(MYDIR *dir)
{
    free(dir->name);
    CLOSEDIR(dir->dir);

    free(dir);
}

int sim_open(const char *name, int o, ...)
{
    int opts = rockbox2sim(o);
    int ret;
    if (num_openfiles >= MAX_OPEN_FILES)
        return -2;

    if (opts & O_CREAT)
    {
        va_list ap;
        va_start(ap, o);
        mode_t mode = va_arg(ap, unsigned int);
        ret = OPEN(get_sim_pathname(name), opts, mode);
#ifdef HAVE_DIRCACHE
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
    int ret;
    ret = CLOSE(fd);
    if (ret == 0)
        num_openfiles--;
    return ret;
}

int sim_creat(const char *name, mode_t mode)
{
    int ret = OPEN(get_sim_pathname(name),
                   O_BINARY | O_WRONLY | O_CREAT | O_TRUNC, mode);
#ifdef HAVE_DIRCACHE
    if (ret >= 0 && (dircache_get_entry_id(name) < 0))
        dircache_add_file(name, 0);
#endif
    return ret;
}

ssize_t sim_read(int fd, void *buf, size_t count)
{
    ssize_t result;

    mutex_lock(&io.sim_mutex);

    /* Setup parameters */
    io.fd = fd;
    io.buf = buf;
    io.count = count;

    result = io_trigger_and_wait(IO_READ);

    mutex_unlock(&io.sim_mutex);

    return result;
}

ssize_t sim_write(int fd, const void *buf, size_t count)
{
    ssize_t result;

    mutex_lock(&io.sim_mutex);

    io.fd = fd;
    io.buf = (void*)buf;
    io.count = count;

    result = io_trigger_and_wait(IO_WRITE);

    mutex_unlock(&io.sim_mutex);

    return result;
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

void fat_size(IF_MV2(int volume,) unsigned long* size, unsigned long* free)
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
    (void)buf;
    (void)buf_size;
    void *handle = SDL_LoadObject(get_sim_pathname(filename));
    if (handle == NULL)
    {
        DEBUGF("failed to load %s\n", filename);
        DEBUGF("lc_open(%s): %s\n", filename, SDL_GetError());
    }
    return handle;
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
