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
#else
#include <sys/vfs.h>
#endif
#include <dirent.h>
#include <unistd.h>

#include <fcntl.h>
#include "debug.h"

#define DIRFUNCTIONS_DEFINED /* prevent those prototypes */
#define dirent x11_dirent
#include "../../firmware/include/dir.h"
#undef dirent

#define SIMULATOR_ARCHOS_ROOT "archos"

struct mydir {
    DIR *dir;
    char *name;
};

typedef struct mydir MYDIR;

MYDIR *x11_opendir(const char *name)
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

struct x11_dirent *x11_readdir(MYDIR *dir)
{
    char buffer[512]; /* sufficiently big */
    static struct x11_dirent secret;
    struct stat s;
    struct dirent *x11 = (readdir)(dir->dir);

    if(!x11)
        return (struct x11_dirent *)0;

    strcpy(secret.d_name, x11->d_name);

    /* build file name */
    sprintf(buffer, SIMULATOR_ARCHOS_ROOT "%s/%s",
            dir->name, x11->d_name);
    stat(buffer, &s); /* get info */

    secret.attribute = S_ISDIR(s.st_mode)?ATTR_DIRECTORY:0;
    secret.size = s.st_size;

    return &secret;
}

void x11_closedir(MYDIR *dir)
{
    free(dir->name);
    (closedir)(dir->dir);

    free(dir);
}


int x11_open(const char *name, int opts)
{
    char buffer[256]; /* sufficiently big */

    if(name[0] == '/') {
        sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);
        
        debugf("We open the real file '%s'\n", buffer);
        return (open)(buffer, opts);
    }
    return (open)(name, opts);
}

int x11_close(int fd)
{
    return (close)(fd);
}

int x11_creat(const char *name, mode_t mode)
{
    char buffer[256]; /* sufficiently big */
    (void)mode;
    if(name[0] == '/') {
        sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);
        
        debugf("We create the real file '%s'\n", buffer);
        return (creat)(buffer, 0666);
    }
    return (creat)(name, 0666);
}

int x11_mkdir(const char *name, mode_t mode)
{
    char buffer[256]; /* sufficiently big */
    (void)mode;
    if(name[0] == '/') {
        sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);
        
        debugf("We create the real directory '%s'\n", buffer);
        return (mkdir)(buffer, 0666);
    }
    return (mkdir)(name, 0666);
}

int x11_rmdir(const char *name)
{
    char buffer[256]; /* sufficiently big */
    if(name[0] == '/') {
        sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);
        
        debugf("We remove the real directory '%s'\n", buffer);
        return (rmdir)(buffer);
    }
    return (rmdir)(name);
}

int x11_remove(char *name)
{
    char buffer[256]; /* sufficiently big */

    if(name[0] == '/') {
        sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);

        debugf("We remove the real file '%s'\n", buffer);
        return (remove)(buffer);
    }
    return (remove)(name);
}

int x11_rename(char *oldpath, char* newpath)
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

int x11_filesize(int fd)
{
    int old = lseek(fd, 0, SEEK_CUR);
    int size = lseek(fd, 0, SEEK_END);
    lseek(fd, old, SEEK_SET);

    return(size);
}

void fat_size(unsigned int* size, unsigned int* free)
{
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
}
