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
#include <time.h>
#ifdef __FreeBSD__
#include <sys/param.h>
#include <sys/mount.h>
#elif defined(__APPLE__)
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
#define MAX_OPEN_FILES 11

#include <fcntl.h>
#include <SDL.h>
#include <SDL_thread.h>
#include "thread.h"
#include "kernel.h"
#include "debug.h"
#include "config.h"
#include "ata.h" /* for IF_MV2 et al. */

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
#define OPEN(a,b,c) (_wopen)(UTF8_TO_OS(a),b,c)
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
#define OPEN(a,b,c) (open)(a,b,c)
#define CLOSE(x)    (close)(x)
#define REMOVE(a)   (remove)(a)
#define RENAME(a,b) (rename)(a,b)

#endif /* !__MINGW32__ */


#ifdef HAVE_DIRCACHE
void dircache_remove(const char *name);
void dircache_rename(const char *oldpath, const char *newpath);
#endif


#define SIMULATOR_DEFAULT_ROOT "archos"
extern const char *sim_root_dir;

static int num_openfiles = 0;

struct sim_dirent {
    unsigned char d_name[MAX_PATH];
    int attribute;
    long size;
    long startcluster;
    unsigned short wrtdate; /*  Last write date */ 
    unsigned short wrttime; /*  Last write time */
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

#if 1 /* maybe this needs disabling for MSVC... */
static unsigned int rockbox2sim(int opt)
{
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
}
#endif

/** Simulator I/O engine routines **/
enum
{
    IO_QUIT = -1,
    IO_OPEN,
    IO_CLOSE,
    IO_READ,
    IO_WRITE,
};

struct sim_io
{
    SDL_mutex *m; /* Mutex for condition */
    SDL_cond *c;  /* Condition for synchronizing threads */
    SDL_Thread *t; /* The I/O thread */
    struct mutex sim_mutex; /* Rockbox mutex */
    volatile int cmd; /* The command to perform */
    volatile int ready; /* I/O ready flag - 1= ready */
    volatile int fd; /* The file to read/write */
    void* volatile buf; /* The buffer to read/write */
    volatile size_t count; /* Number of bytes to read/write */
    ssize_t result; /* Result of operation */
};

static struct sim_io io;

static int io_thread(void *data)
{
    SDL_LockMutex(io.m);

    io.ready = 1; /* Indication mutex has been locked */

    for (;;)
    {
        SDL_CondWait(io.c, io.m); /* unlock mutex and wait */

        switch (io.cmd)
        {
        case IO_READ:
            io.result = read(io.fd, io.buf, io.count);
            io.ready = 1;
            break;
        case IO_WRITE:
            io.result = write(io.fd, io.buf, io.count);
            io.ready = 1;
            break;
        case IO_QUIT:
            SDL_UnlockMutex(io.m);
            return 0;
        }
    }

    (void)data;
}

bool sim_io_init(void)
{
    io.ready = 0;

    io.m = SDL_CreateMutex();
    if (io.m == NULL)
    {
        fprintf(stderr, "Failed to create IO mutex\n");
        return false;
    }

    io.c = SDL_CreateCond();
    if (io.c == NULL)
    {
        fprintf(stderr, "Failed to create IO cond\n");
        return false;
    }

    io.t = SDL_CreateThread(io_thread, NULL);
    if (io.t == NULL)
    {
        fprintf(stderr, "Failed to create IO thread\n");
        return false;
    }

    /* Wait for IO thread to lock mutex */
    while (!io.ready)
        SDL_Delay(0);

    /* Wait for it to unlock */
    SDL_LockMutex(io.m);
    /* Free it for another thread */
    SDL_UnlockMutex(io.m);

    return true;
}

int ata_init(void)
{
    /* Initialize the rockbox kernel objects on a rockbox thread */
    mutex_init(&io.sim_mutex);
    return 1;
}

void sim_io_shutdown(void)
{
    SDL_LockMutex(io.m);

    io.cmd = IO_QUIT;

    SDL_CondSignal(io.c);
    SDL_UnlockMutex(io.m);

    SDL_WaitThread(io.t, NULL);

    SDL_DestroyMutex(io.m);
    SDL_DestroyCond(io.c);
}

static void io_trigger_and_wait(int cmd)
{
    /* Lock mutex before setting up new params and signaling condition */
    SDL_LockMutex(io.m);

    io.cmd = cmd;
    io.ready = 0;

    /* Get thread started */
    SDL_CondSignal(io.c);

    /* Let it run */
    SDL_UnlockMutex(io.m);

    /* Wait for IO to complete */
    while (!io.ready)
        yield();
}

static const char *get_sim_rootdir()
{
    if (sim_root_dir != NULL)
        return sim_root_dir;
    return SIMULATOR_DEFAULT_ROOT;
}

MYDIR *sim_opendir(const char *name)
{
    char buffer[MAX_PATH]; /* sufficiently big */
    DIR_T *dir;

#ifndef __PCTOOL__
    if(name[0] == '/')
    {
        snprintf(buffer, sizeof(buffer), "%s%s", get_sim_rootdir(), name);
        dir=(DIR_T *)OPENDIR(buffer);
    }
    else
#endif
        dir=(DIR_T *)OPENDIR(name);

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
    STAT_T s;
    DIRENT_T *x11 = READDIR(dir->dir);
    struct tm* tm;

    if(!x11)
        return (struct sim_dirent *)0;

    strcpy((char *)secret.d_name, OS_TO_UTF8(x11->d_name));

    /* build file name */
#ifdef __PCTOOL__
    snprintf(buffer, sizeof(buffer), "%s/%s", dir->name, secret.d_name);
#else
    snprintf(buffer, sizeof(buffer), "%s/%s/%s",
            get_sim_rootdir(), dir->name, secret.d_name);
#endif
    STAT(buffer, &s); /* get info */

#define ATTR_DIRECTORY 0x10
    
    secret.attribute = S_ISDIR(s.st_mode)?ATTR_DIRECTORY:0;
    secret.size = s.st_size;

    tm = localtime(&(s.st_mtime));
    secret.wrtdate = ((tm->tm_year - 80) << 9) |
                        ((tm->tm_mon + 1) << 5) |
                        tm->tm_mday;
    secret.wrttime = (tm->tm_hour << 11) |
                        (tm->tm_min << 5) |
                        (tm->tm_sec >> 1);
    return &secret;
}

void sim_closedir(MYDIR *dir)
{
    free(dir->name);
    CLOSEDIR(dir->dir);

    free(dir);
}

int sim_open(const char *name, int o)
{
    char buffer[MAX_PATH]; /* sufficiently big */
    int opts = rockbox2sim(o);
    int ret;

    if (num_openfiles >= MAX_OPEN_FILES)
        return -2;

#ifndef __PCTOOL__
    if(name[0] == '/')
    {
        snprintf(buffer, sizeof(buffer), "%s%s", get_sim_rootdir(), name);

        debugf("We open the real file '%s'\n", buffer);
        if (num_openfiles < MAX_OPEN_FILES)
        {
            ret = OPEN(buffer, opts, 0666);
            if (ret >= 0) num_openfiles++;
            return ret;
        }
    }

    fprintf(stderr, "WARNING, bad file name lacks slash: %s\n",
            name);
    return -1;
#else
    if (num_openfiles < MAX_OPEN_FILES)
    {
        ret = OPEN(buffer, opts, 0666);
        if (ret >= 0) num_openfiles++;
        return ret;
    }
#endif
}

int sim_close(int fd)
{
    int ret;
    ret = CLOSE(fd);
    if (ret == 0) num_openfiles--;
    return ret;
}

int sim_creat(const char *name)
{
#ifndef __PCTOOL__
    char buffer[MAX_PATH]; /* sufficiently big */
    if(name[0] == '/')
    {
        snprintf(buffer, sizeof(buffer), "%s%s", get_sim_rootdir(), name);
        
        debugf("We create the real file '%s'\n", buffer);
        return OPEN(buffer, O_BINARY | O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }
    fprintf(stderr, "WARNING, bad file name lacks slash: %s\n", name);
    return -1;
#else
    return OPEN(name, O_BINARY | O_WRONLY | O_CREAT | O_TRUNC, 0666);
#endif
}      

ssize_t sim_read(int fd, void *buf, size_t count)
{
    ssize_t result;

    mutex_lock(&io.sim_mutex);

    /* Setup parameters */
    io.fd = fd;
    io.buf = buf;
    io.count = count;

    io_trigger_and_wait(IO_READ);

    result = io.result;

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

    io_trigger_and_wait(IO_WRITE);

    result = io.result;

    mutex_unlock(&io.sim_mutex);

    return result;
}

int sim_mkdir(const char *name)
{
#ifdef __PCTOOL__
    return MKDIR(name, 0777);
#else
    char buffer[MAX_PATH]; /* sufficiently big */

    snprintf(buffer, sizeof(buffer), "%s%s", get_sim_rootdir(), name);

    debugf("We create the real directory '%s'\n", buffer);
    return MKDIR(buffer, 0777);
#endif
}

int sim_rmdir(const char *name)
{
#ifdef __PCTOOL__
    return RMDIR(name);
#else
    char buffer[MAX_PATH]; /* sufficiently big */
    if(name[0] == '/') 
    {
        snprintf(buffer, sizeof(buffer), "%s%s", get_sim_rootdir(), name);

        debugf("We remove the real directory '%s'\n", buffer);
        return RMDIR(buffer);
    }
    return RMDIR(name);
#endif
}

int sim_remove(const char *name)
{
#ifdef __PCTOOL__
    return REMOVE(name);
#else
    char buffer[MAX_PATH]; /* sufficiently big */

#ifdef HAVE_DIRCACHE
    dircache_remove(name);
#endif

    if(name[0] == '/') {
        snprintf(buffer, sizeof(buffer), "%s%s", get_sim_rootdir(), name);

        debugf("We remove the real file '%s'\n", buffer);
        return REMOVE(buffer);
    }
    return REMOVE(name);
#endif
}

int sim_rename(const char *oldpath, const char* newpath)
{
#ifdef __PCTOOL__
    return RENAME(oldpath, newpath);
#else
    char buffer1[MAX_PATH];
    char buffer2[MAX_PATH];

#ifdef HAVE_DIRCACHE
    dircache_rename(oldpath, newpath);
#endif

    if(oldpath[0] == '/') {
        snprintf(buffer1, sizeof(buffer1), "%s%s", get_sim_rootdir(),
                                                   oldpath);
        snprintf(buffer2, sizeof(buffer2), "%s%s", get_sim_rootdir(),
                                                   newpath);

        debugf("We rename the real file '%s' to '%s'\n", buffer1, buffer2);
        return RENAME(buffer1, buffer2);
    }
    return -1;
#endif
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

void fat_size(IF_MV2(int volume,) unsigned long* size, unsigned long* free)
{
#ifdef HAVE_MULTIVOLUME
    if (volume != 0) {
        debugf("io.c: fat_size(volume=%d); simulator only supports volume 0\n",
            volume);
        
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
    }
#else
    struct statfs fs;

    if (!statfs(".", &fs)) {
        DEBUGF("statfs: bsize=%d blocks=%ld free=%ld\n",
               (int)fs.f_bsize, fs.f_blocks, fs.f_bfree);
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
#define dlopen(_x_, _y_) LoadLibraryW(UTF8_TO_OS(_x_))
#define dlsym(_x_, _y_) (void *)GetProcAddress(_x_, _y_)
#define dlclose(_x_) FreeLibrary(_x_)
#else
/* sim-x11 */
#include <dlfcn.h>
#endif

#define TEMP_CODEC_FILE "archos/_temp_codec%d.dll"

void *sim_codec_load_ram(char* codecptr, int size, void **pd)
{
    void *hdr;
    char path[MAX_PATH];
    int fd;
    int codec_count;
#ifdef WIN32
    char buf[MAX_PATH];
#endif

    *pd = NULL;

    /* We have to create the dynamic link library file from ram so we 
       can simulate the codec loading. With voice and crossfade,
       multiple codecs may be loaded at the same time, so we need 
       to find an unused filename */
    for (codec_count = 0; codec_count < 10; codec_count++)
    {
        snprintf(path, sizeof(path), TEMP_CODEC_FILE, codec_count);

        fd = OPEN(path, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IRWXU);
        if (fd >= 0)
            break;  /* Created a file ok */
    }
    if (fd < 0)
    {
        DEBUGF("failed to open for write: %s\n", path);
        return NULL;
    }

    if (write(fd, codecptr, size) != size) {
        DEBUGF("write failed");
        return NULL;
    }
    close(fd);

    /* Now load the library. */
    *pd = dlopen(path, RTLD_NOW);
    if (*pd == NULL) {
        DEBUGF("failed to load %s\n", path); 
#ifdef WIN32
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0,
                       buf, sizeof buf, NULL);
        DEBUGF("dlopen(%s): %s\n", path, buf);
#else
        DEBUGF("dlopen(%s): %s\n", path, dlerror());
#endif
        return NULL;
    }

    hdr = dlsym(*pd, "__header");
    if (!hdr)
        hdr = dlsym(*pd, "___header");

    return hdr;       /* maybe NULL if symbol not present */
}

void sim_codec_close(void *pd)
{
    dlclose(pd);
}

void *sim_plugin_load(char *plugin, void **pd)
{
    void *hdr;
    char path[MAX_PATH];
#ifdef WIN32
    char buf[MAX_PATH];
#endif

    snprintf(path, sizeof(path), "archos%s", plugin);

    *pd = NULL;

    *pd = dlopen(path, RTLD_NOW);
    if (*pd == NULL) {
        DEBUGF("failed to load %s\n", plugin);
#ifdef WIN32
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0,
                       buf, sizeof(buf), NULL);
        DEBUGF("dlopen(%s): %s\n", path, buf);
#else
        DEBUGF("dlopen(%s): %s\n", path, dlerror());
#endif
        return NULL;
    }

    hdr = dlsym(*pd, "__header");
    if (!hdr)
        hdr = dlsym(*pd, "___header");

    return hdr;    /* maybe NULL if symbol not present */
}

void sim_plugin_close(void *pd)
{
    dlclose(pd);
}

#ifdef WIN32
static unsigned old_cp;

void debug_exit(void)
{
    /* Reset console output codepage */
    SetConsoleOutputCP(old_cp);
}

void debug_init(void)
{
    old_cp = GetConsoleOutputCP();
    /* Set console output codepage to UTF8. Only works
     * correctly when the console uses a truetype font. */
    SetConsoleOutputCP(65001);
    atexit(debug_exit);
}
#else
void debug_init(void)
{
    /* nothing to be done */
}
#endif

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

/* rockbox off_t may be different from system off_t */
int sim_ftruncate(int fd, long length)
{
#ifdef WIN32
    return _chsize(fd, length);
#else
    return ftruncate(fd, length);
#endif
}
