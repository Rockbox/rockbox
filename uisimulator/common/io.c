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
#include <sys/stat.h>
#ifdef __FreeBSD__
#include <sys/param.h>
#include <sys/mount.h>
#elif !defined(WIN32)
#include <sys/vfs.h>
#endif

#ifndef _MSC_VER
#include <dirent.h>
#include <unistd.h>
#else
#include "dir-win32.h"
#endif

#include <fcntl.h>
#include "debug.h"

#define DIRFUNCTIONS_DEFINED /* prevent those prototypes */
#define dirent sim_dirent
#define DIR SIMDIR
#include "../../firmware/include/dir.h"
#undef dirent
#undef DIR

#define SIMULATOR_ARCHOS_ROOT "archos"

struct mydir {
    DIR *dir;
    char *name;
};

typedef struct mydir MYDIR;

#ifndef WIN32
static unsigned int rockbox2sim(int opt)
{
    int newopt = 0;
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

    strcpy(secret.d_name, x11->d_name);

    /* build file name */
    sprintf(buffer, SIMULATOR_ARCHOS_ROOT "%s/%s",
            dir->name, x11->d_name);
    stat(buffer, &s); /* get info */

    secret.attribute = S_ISDIR(s.st_mode)?ATTR_DIRECTORY:0;
    secret.size = s.st_size;
    secret.wrtdate = (unsigned short)(s.st_mtime >> 16); 
    secret.wrttime = (unsigned short)(s.st_mtime & 0xFFFF); 

    return &secret;
}

void sim_closedir(MYDIR *dir)
{
    free(dir->name);
    (closedir)(dir->dir);

    free(dir);
}


int sim_open(const char *name, int o)
{
    char buffer[256]; /* sufficiently big */
#ifndef WIN32
    int opts = rockbox2sim(o);
#endif

    if(name[0] == '/') {
        sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);
        
        debugf("We open the real file '%s'\n", buffer);
#ifdef WIN32
        return (open)(buffer, o);
#else
        return (open)(buffer, opts, 0666);
#endif
    }
    fprintf(stderr, "WARNING, bad file name lacks slash: %s\n",
            name);
    return -1;
}

int sim_close(int fd)
{
    return (close)(fd);
}

int sim_creat(const char *name, mode_t mode)
{
    char buffer[256]; /* sufficiently big */
    (void)mode;
    if(name[0] == '/') {
        sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);
        
        debugf("We create the real file '%s'\n", buffer);
        return (creat)(buffer, 0666);
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
    return (mkdir)(buffer);
#else
    return (mkdir)(buffer, 0666);
#endif
}

int sim_rmdir(const char *name)
{
    char buffer[256]; /* sufficiently big */
    if(name[0] == '/') {
        sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);
        
        debugf("We remove the real directory '%s'\n", buffer);
        return (rmdir)(buffer);
    }
    return (rmdir)(name);
}

int sim_remove(const char *name)
{
    char buffer[256]; /* sufficiently big */

    if(name[0] == '/') {
        sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);

        debugf("We remove the real file '%s'\n", buffer);
        return (remove)(buffer);
    }
    return (remove)(name);
}

int sim_rename(const char *oldpath, const char* newpath)
{
    char buffer1[256];
    char buffer2[256];

    if(oldpath[0] == '/') {
        sprintf(buffer1, "%s%s", SIMULATOR_ARCHOS_ROOT, oldpath);
        sprintf(buffer2, "%s%s", SIMULATOR_ARCHOS_ROOT, newpath);

        debugf("We rename the real file '%s' to '%s'\n", buffer1, buffer2);
        return (rename)(buffer1, buffer2);
    }
    return -1;
}

off_t sim_filesize(int fd)
{
    int old = lseek(fd, 0, SEEK_CUR);
    int size = lseek(fd, 0, SEEK_END);
    lseek(fd, old, SEEK_SET);

    return(size);
}

void fat_size(unsigned int* size, unsigned int* free)
{
#ifdef WIN32
    *size = 2049;
    *free = 1037;
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
    else {
        if (size)
            *size = 0;
        if (free)
            *free = 0;
    }
#endif
}
