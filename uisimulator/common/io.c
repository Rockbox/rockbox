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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#ifdef __FreeBSD__
#include <sys/param.h>
#include <sys/mount.h>
#elif !defined(WIN32)
#include <sys/vfs.h>
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

#define MAX_PATH 260

#include <fcntl.h>
#include "debug.h"

#define SIMULATOR_ARCHOS_ROOT "archos"

struct sim_dirent {
    unsigned char d_name[MAX_PATH];
    int attribute;
    int size;
    int startcluster;
    unsigned short wrtdate; /*  Last write date */ 
    unsigned short wrttime; /*  Last write time */
};

struct dirstruct {
    void *dir; /* actually a DIR* dir */
    char *name;    
} SIM_DIR;

struct mydir {
    DIR *dir;
    char *name;
};

typedef struct mydir MYDIR;

#if 1 /* maybe this needs disabling for MSVC... */
static unsigned int rockbox2sim(int opt)
{
#ifdef WIN32
    int newopt = O_BINARY;
#else
    int newopt = 0;
#endif
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
}
#endif

MYDIR *sim_opendir(const char *name)
{
    char buffer[256]; /* sufficiently big */
    DIR *dir;

    if(name[0] == '/') {
        sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);    
        dir=(DIR *)opendir(buffer);
    }
    else
        dir=(DIR *)opendir(name);
    
    if(dir) {
        MYDIR *my = (MYDIR *)malloc(sizeof(MYDIR));
        my->dir = dir;
        my->name = (char *)strdup(name);

        return my;
    }
    /* failed open, return NULL */
    return (MYDIR *)0;
}

struct sim_dirent *sim_readdir(MYDIR *dir)
{
    char buffer[512]; /* sufficiently big */
    static struct sim_dirent secret;
    struct stat s;
    struct dirent *x11 = (readdir)(dir->dir);

    if(!x11)
        return (struct sim_dirent *)0;

    strcpy((char *)secret.d_name, x11->d_name);

    /* build file name */
    sprintf(buffer, SIMULATOR_ARCHOS_ROOT "%s/%s",
            dir->name, x11->d_name);
    stat(buffer, &s); /* get info */

#define ATTR_DIRECTORY 0x10
    
    secret.attribute = S_ISDIR(s.st_mode)?ATTR_DIRECTORY:0;
    secret.size = s.st_size;
    secret.wrtdate = (unsigned short)(s.st_mtime >> 16); 
    secret.wrttime = (unsigned short)(s.st_mtime & 0xFFFF); 

    return &secret;
}

void sim_closedir(MYDIR *dir)
{
    free(dir->name);
    closedir(dir->dir);

    free(dir);
}

int sim_open(const char *name, int o)
{
    char buffer[256]; /* sufficiently big */
    int opts = rockbox2sim(o);

    if(name[0] == '/') {
        sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);

        debugf("We open the real file '%s'\n", buffer);
        return open(buffer, opts, 0666);
    }
    fprintf(stderr, "WARNING, bad file name lacks slash: %s\n",
            name);
    return -1;
}

int sim_creat(const char *name, mode_t mode)
{
    char buffer[256]; /* sufficiently big */
    (void)mode;
    if(name[0] == '/') {
        sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);
        
        debugf("We create the real file '%s'\n", buffer);
        return creat(buffer, 0666);
    }
    fprintf(stderr, "WARNING, bad file name lacks slash: %s\n",
            name);
    return -1;
}      

int sim_mkdir(const char *name, mode_t mode)
{
    char buffer[256]; /* sufficiently big */
    (void)mode;

    sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);
        
    debugf("We create the real directory '%s'\n", buffer);
#ifdef WIN32
    /* since we build with -DNOCYGWIN we have the plain win32 version */
    return mkdir(buffer);
#else
    return mkdir(buffer, 0777);
#endif
}

int sim_rmdir(const char *name)
{
    char buffer[256]; /* sufficiently big */
    if(name[0] == '/') {
        sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);
        
        debugf("We remove the real directory '%s'\n", buffer);
        return rmdir(buffer);
    }
    return rmdir(name);
}

int sim_remove(const char *name)
{
    char buffer[256]; /* sufficiently big */

    if(name[0] == '/') {
        sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);

        debugf("We remove the real file '%s'\n", buffer);
        return remove(buffer);
    }
    return remove(name);
}

int sim_rename(const char *oldpath, const char* newpath)
{
    char buffer1[256];
    char buffer2[256];

    if(oldpath[0] == '/') {
        sprintf(buffer1, "%s%s", SIMULATOR_ARCHOS_ROOT, oldpath);
        sprintf(buffer2, "%s%s", SIMULATOR_ARCHOS_ROOT, newpath);

        debugf("We rename the real file '%s' to '%s'\n", buffer1, buffer2);
        return rename(buffer1, buffer2);
    }
    return -1;
}

/* rockbox off_t may be different from system off_t */
long sim_lseek(int fildes, long offset, int whence)
{
    return lseek(fildes, offset, whence);
}

long sim_filesize(int fd)
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

void fat_size(unsigned int* size, unsigned int* free)
{
#ifdef WIN32
    long secperclus, bytespersec, free_clusters, num_clusters;

    if (GetDiskFreeSpace(NULL, &secperclus, &bytespersec, &free_clusters,
                     &num_clusters)) {
        if (size)
            *size = num_clusters * secperclus / 2 * (bytespersec / 512);
        if (free)
            *free = free_clusters * secperclus / 2 * (bytespersec / 512);
    }
#else
    struct statfs fs;

    if (!statfs(".", &fs)) {
        DEBUGF("statfs: bsize=%d blocks=%d free=%d\n",
               fs.f_bsize, fs.f_blocks, fs.f_bfree);
        if (size)
            *size = fs.f_blocks * (fs.f_bsize / 1024);
        if (free)
            *free = fs.f_bfree * (fs.f_bsize / 1024);
    }
#endif
    else {
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

#ifdef WIN32
/* sim-win32 */
#define dlopen(_x_, _y_) LoadLibrary(_x_)
#define dlsym(_x_, _y_) (void *)GetProcAddress(_x_, _y_)
#define dlclose(_x_) FreeLibrary(_x_)
#else
/* sim-x11 */
#include <dlfcn.h>
#endif

void *sim_codec_load_ram(char* codecptr, int size,
        void* ptr2, int bufwrap, int *pd_fd)
{
    void *pd, *hdr;
    const char *path = "archos/_temp_codec.dll";
    int fd;
    int copy_n;
#ifdef WIN32
    char buf[256];
#endif

    *pd_fd = 0;

    /* We have to create the dynamic link library file from ram
       so we could simulate the codec loading. */

#ifdef WIN32
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IRWXU);
#else
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
#endif
    if (fd < 0) {
        DEBUGF("failed to open for write: %s\n", path);
        return NULL;
    }

    if (bufwrap == 0)
        bufwrap = size;
        
    copy_n = bufwrap < size ? bufwrap : size;
    if (write(fd, codecptr, copy_n) != copy_n) {
        DEBUGF("write failed");
        return NULL;
    }
    size -= copy_n;
    if (size > 0) {
        if (write(fd, ptr2, size) != size) {
            DEBUGF("write failed [2]");
            return NULL;
        }
    }
    close(fd);

    /* Now load the library. */
    pd = dlopen(path, RTLD_NOW);
    if (!pd) {
        DEBUGF("failed to load %s\n", path); 
#ifdef WIN32
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0,
                      buf, sizeof buf, NULL);
        DEBUGF("dlopen(%s): %s\n", path, buf);
#else
        DEBUGF("dlopen(%s): %s\n", path, dlerror());
#endif
        dlclose(pd);
        return NULL;
    }

    hdr = dlsym(pd, "__header");
    if (!hdr)
        hdr = dlsym(pd, "___header");

    *pd_fd = (int)pd;
    return hdr;       /* maybe NULL if symbol not present */
}

void sim_codec_close(int pd)
{
    dlclose((void *)pd);
}

void *sim_plugin_load(char *plugin, int *fd)
{
    void *pd, *hdr;
    char path[256];
#ifdef WIN32
    char buf[256];
#endif

    snprintf(path, sizeof path, "archos%s", plugin);
    
    *fd = 0;

    pd = dlopen(path, RTLD_NOW);
    if (!pd) {
        DEBUGF("failed to load %s\n", plugin);
#ifdef WIN32
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0,
                      buf, sizeof buf, NULL);
        DEBUGF("dlopen(%s): %s\n", path, buf);
#else
        DEBUGF("dlopen(%s): %s\n", path, dlerror());
#endif
        dlclose(pd);
        return NULL;
    }

    hdr = dlsym(pd, "__header");
    if (!hdr)
        hdr = dlsym(pd, "___header");

    *fd = (int)pd; /* success */
    return hdr;    /* maybe NULL if symbol not present */
}

void sim_plugin_close(int pd)
{
    dlclose((void *)pd);
}

#if !defined(WIN32) || defined(SDL)
/* the win32 version is in debug-win32.c */

void debug_init(void)
{
    /* nothing to be done */
}

void debugf(const char *fmt, ...)
{
    va_list ap;
    va_start( ap, fmt );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
}

void ldebugf(const char* file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start( ap, fmt );
    fprintf( stderr, "%s:%d ", file, line );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
}

#endif

/* rockbox off_t may be different from system off_t */
int sim_ftruncate(int fd, long length)
{
#ifdef WIN32
    return _chsize(fd, length);
#else
    return ftruncate(fd, length);
#endif
}
