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
 * Copyright (C) 2014 Michael Sevakis
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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include "config.h"
#include "system.h"
#include "file.h"
#include "dir.h"
#include "file_internal.h"
#include "pathfuncs.h"
#include "string-extra.h"
#include "debug.h"
#include "mv.h"

#ifndef os_fstatat
#define USE_OSDIRNAME /* we need to remember the open directory path */
#endif

extern const char *sim_root_dir;

/* Windows (and potentially other OSes) distinguish binary and text files.
 * Define a dummy for the others. */
#ifndef O_BINARY
#define O_BINARY 0
#endif

struct filestr_desc
{
    int  osfd;              /* The host OS file descriptor */
    bool mounted;           /* Is host volume still mounted? */
#ifdef HAVE_MULTIVOLUME
    int  volume;            /* The virtual volume number */
#endif
} openfiles[MAX_OPEN_FILES] =
{
    [0 ... MAX_OPEN_FILES-1] = { .osfd = -1 }
};

static struct filestr_desc * alloc_filestr(int *fildesp)
{
    for (unsigned int i = 0; i < MAX_OPEN_FILES; i++)
    {
        struct filestr_desc *filestr = &openfiles[i];
        if (filestr->osfd < 0)
        {
            *fildesp = i;
            return filestr;
        }
    }

    return NULL;
}

static struct filestr_desc * get_filestr(int fildes)
{
    struct filestr_desc *filestr = &openfiles[fildes];

    if ((unsigned int)fildes >= MAX_OPEN_FILES || filestr->osfd < 0)
        filestr = NULL;
    else if (filestr->mounted)
        return filestr;

    errno = filestr ? ENXIO : EBADF;
    DEBUGF("fildes %d: %s\n", fildes, strerror(errno));
    return NULL;
}

struct dirstr_desc
{
    int               osfd;          /* Host OS directory file descriptor */
    bool              osfd_opened;   /* Host fd is another open file */
    OS_DIR_T          *osdirp;       /* Host OS directory stream */
#ifdef USE_OSDIRNAME
    char              *osdirname;    /* Host OS directory path */
#endif
    struct sim_dirent entry;         /* Rockbox directory entry */
#ifdef HAVE_MULTIVOLUME
    int               volume;        /* Virtual volume number */
    int               volumecounter; /* Counter for root volume entries */
#endif
    bool              mounted;       /* Is the virtual volume still mounted? */
} opendirs[MAX_OPEN_DIRS];

static struct dirstr_desc * alloc_dirstr(void)
{
    for (unsigned int i = 0; i < MAX_OPEN_DIRS; i++)
    {
        struct dirstr_desc *dirstr = &opendirs[i];
        if (dirstr->osdirp == NULL)
            return dirstr;
    }

    return NULL;
}

static struct dirstr_desc * get_dirstr(DIR *dirp)
{
    struct dirstr_desc *dirstr = (struct dirstr_desc *)dirp;

    if (!PTR_IN_ARRAY(opendirs, dirstr, MAX_OPEN_DIRS) || !dirstr->osdirp)
        dirstr = NULL;
    else if (dirstr->mounted)
        return dirstr;

    errno = dirstr ? ENXIO : EBADF;
    DEBUGF("dir #%d: %s\n", (int)(dirstr - opendirs), strerror(errno));
    return NULL;
}

static int close_dirstr(struct dirstr_desc *dirstr)
{
    OS_DIR_T *osdirp = dirstr->osdirp;

    dirstr->mounted = false;

#ifdef USE_OSDIRNAME
    free(dirstr->osdirname);
#endif
    if (dirstr->osfd_opened)
    {
        os_close(dirstr->osfd);
        dirstr->osfd_opened = false;
    }

    int rc = os_closedir(osdirp);
    dirstr->osdirp = NULL;

    return rc;
}

#ifdef HAVE_MULTIVOLUME
static int readdir_volume_inner(struct dirstr_desc *dirstr,
                                struct sim_dirent *entry)
{
    /* Volumes (secondary file systems) get inserted into the system root
     * directory. If the path specified volume 0, enumeration will not
     * include other volumes, but just its own files and directories.
     *
     * Fake special directories, which don't really exist, that will get
     * redirected upon opendir()
     */
    while (++dirstr->volumecounter < NUM_VOLUMES)
    {
        /* on the system root */
        if (!volume_present(dirstr->volumecounter))
            continue;

        get_volume_name(dirstr->volumecounter, entry->d_name);
        return 1;
    }

    /* do normal directory entry fetching */
    return 0;
}
#endif /* HAVE_MULTIVOLUME */

static inline int readdir_volume(struct dirstr_desc *dirstr,
                                 struct sim_dirent *entry)
{
#ifdef HAVE_MULTIVOLUME
    if (dirstr->volumecounter < NUM_VOLUMES)
        return readdir_volume_inner(dirstr, entry);
#endif /* HAVE_MULTIVOLUME */

    /* do normal directory entry fetching */
    return 0;
    (void)dirstr; (void)entry;
}


/** Internal functions **/

#ifdef HAVE_HOTSWAP
/**
 * Handle drive extraction by pretending the files' volumes no longer exist
 * and invalidating their I/O for the remainder of their lifetimes as would
 * happen on a native target
 */
void sim_ext_extracted(int drive)
{
    for (unsigned int i = 0; i < MAX_OPEN_FILES; i++)
    {
        struct filestr_desc *filestr = &openfiles[i];
        if (filestr->osfd >= 0 IF_MV(&& volume_drive(filestr->volume) == drive))
            filestr->mounted = false;
    }

    for (unsigned int i = 0; i < MAX_OPEN_DIRS; i++)
    {
        struct dirstr_desc *dirstr = &opendirs[i];
        if (dirstr->osdirp IF_MV(&& volume_drive(dirstr->volume) == drive))
            dirstr->mounted = false;
    }

    (void)drive;
}
#endif /* HAVE_HOTSWAP */

/**
 * Provides target-like path parsing behavior with single and multiple volumes
 * while performing minimal transforming of the input.
 *
 * Paths are sandboxed to the simulated namespace:
 *   e.g. "{simdir}/foo/../../../../bar" becomes "{simdir}/foo/../bar"
 */
int sim_get_os_path(char *buffer, const char *path, size_t bufsize)
{
    #define ADVBUF(amt) \
        ({ buffer += (amt); bufsize -= (amt); })

    #define PPP_SHOWPATHS 0

    if (!path_is_absolute(path))
    {
        DEBUGF("ERROR: path is not absolute: \"%s\"\n", path);
        errno = ENOENT;
        return -1;
    }

#if PPP_SHOWPATHS
    const char *const buffer0 = buffer;
    DEBUGF("PPP (pre): \"%s\"\n", path);
#endif

    /* Prepend sim root */
    size_t size = strlcpy(buffer, sim_root_dir, bufsize);
    if (size >= bufsize)
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    ADVBUF(size);

#ifdef HAVE_MULTIVOLUME
    /* Track the last valid volume spec encountered */
    int volume = -1;
    bool sysroot = true;

    /* Basename of sim dir to switch back to simdisk from ext */
    #define DIRBASE_FMT  ".." PATH_SEPSTR "%s"
    ssize_t dirbase_len = 0;
    char dirbase[size + 3 + 1];

    /* Basename of ext directory to switch to alternate volume */
    #define SIMEXT_FMT   ".." PATH_SEPSTR "simext%d"
    char extbuf[sizeof (SIMEXT_FMT) + 20 + 1];
#endif /* HAVE_MULTIVOLUME */

    int level = 0;
    bool done = false;
    while (!done)
    {
        const char *p;
        ssize_t len = parse_path_component(&path, &p);


        switch (len)
        {
        case 0:
            done = true;
            if (path[-1] != PATH_SEPCH)
                continue;

            /* Path ends with a separator; preserve that */
            p = &path[-1];
            len = 1;
            break;

        case 1:
        case 2:
            if (p[0] == '.')
            {
                if (len == 1)
                    break;

                if (p[1] == '.')
                    goto is_dot_dot;
            }

        default:
            level++;

        #ifdef HAVE_MULTIVOLUME
            if (level != 1)
                break; /* Volume spec only valid @ root level */
            if (p[-1] != PATH_SEPCH)
                break;

            const char *next;
            volume = path_strip_volume(p-1, &next, true);

            if (volume == ROOT_VOLUME)
                volume = 0; /* FIXME: root no longer implies volume 0 */

            if (next > p)
            {
                /* Feign failure if the volume isn't "mounted" */
                if (!volume_present(volume))
                {
                    errno = ENXIO;
                    return -1;
                }

                sysroot = false;

                if (volume == 0)
                    continue;

                p = extbuf;
                len = snprintf(extbuf, sizeof (extbuf), SIMEXT_FMT, volume);
            }
        #endif /* HAVE_MULTIVOLUME */
            break;

        is_dot_dot:
            if (level <= 0)
                continue; /* Can't go above root; erase */

            level--;

        #ifdef HAVE_MULTIVOLUME
            if (level == 0)
            {
                int v = volume;
                bool sr = sysroot;
                volume = -1;
                sysroot = true;

                if (v <= 0)
                {
                    if (sr)
                        break;

                    continue;
                }

                /* Going up from a volume root and back down to the sys root */
                if (dirbase_len == 0)
                {
                    /* Get the main simdisk directory so it can be reentered */
                    char tmpbuf[sizeof (dirbase)];
                #ifdef WIN32
                    path_correct_separators(tmpbuf, sim_root_dir);
                    path_strip_drive(tmpbuf, &p, false);
                #else
                    p = tmpbuf;
                    strcpy(tmpbuf, sim_root_dir);
                #endif
                    size = path_basename(p, &p);
                    ((char *)p)[size] = '\0';

                    if (size == 0 || is_dotdir_name(p))
                    {
                        /* This is nonsense and won't work */
                        DEBUGF("ERROR: sim root dir basname is dotdir or"
                               " empty: \"%s\"\n", sim_root_dir);
                        errno = ENOENT;
                        return -1;
                    }

                    dirbase_len = snprintf(dirbase, sizeof (dirbase),
                                           DIRBASE_FMT, p);
                }

                p = dirbase;
                len = dirbase_len;
                break;
            }
        #endif /* HAVE_MULTIVOLUME */
            break;
        } /* end switch */

        char compname[len + 1];
        strmemcpy(compname, p, len);

        size = path_append(buffer, PA_SEP_HARD, compname, bufsize);
        if (size >= bufsize)
        {
            errno = ENAMETOOLONG;
            return -1;
        }
        ADVBUF(size);
    }

#if PPP_SHOWPATHS
    DEBUGF("PPP (post): \"%s\"" IF_MV(" vol:%d") "\n",
           buffer0 IF_MV(, volume));
#endif

    return IF_MV(volume) +1;
}


/** File functions **/

int sim_open(const char *path, int oflag, ...)
{
    int fildes;
    struct filestr_desc *filestr = alloc_filestr(&fildes);
    if (!filestr)
    {
        errno = EMFILE;
        return -1;
    }

    char ospath[SIM_TMPBUF_MAX_PATH];
    int pprc = sim_get_os_path(ospath, path, sizeof (ospath));
    if (pprc < 0)
        return -2;

    filestr->osfd = os_open(ospath, oflag | O_BINARY __OPEN_MODE_ARG);
    if (filestr->osfd < 0)
        return -3;

#ifdef HAVE_MULTIVOLUME
    filestr->volume  = MAX(pprc - 1, 0);
#endif
    filestr->mounted = true;
    return fildes;
}

int sim_creat(const char *path, mode_t mode)
{
    return sim_open(path, O_WRONLY|O_CREAT|O_TRUNC, mode);
}

int sim_close(int fildes)
{
    struct filestr_desc *filestr = &openfiles[fildes];
    if ((unsigned int)fildes >= MAX_OPEN_FILES || filestr->osfd < 0)
    {
        errno = EBADF;
        return -1;
    }

    int osfd = filestr->osfd;
    filestr->osfd = -1;
    return os_close(osfd);
}

int sim_ftruncate(int fildes, off_t length)
{
    struct filestr_desc *filestr = get_filestr(fildes);
    if (!filestr)
        return -1;

    off_t size = os_filesize(filestr->osfd);
    if (size < 0)
        return -1;

    if (length >= size)
        return 0;

    int rc = os_ftruncate(filestr->osfd, length);

#ifdef HAVE_DIRCACHE
    if (rc >= 0)
        dircache_ftruncate(xxxxxx);
#endif

    return rc;
}

int sim_fsync(int fildes)
{
    struct filestr_desc *filestr = get_filestr(fildes);
    if (!filestr)
        return -1;

    int rc = os_fsync(filestr->osfd);

#ifdef HAVE_DIRCACHE
    if (rc >= 0)
        dircache_fsync(xxxxxx);
#endif

    return rc;
}

off_t sim_lseek(int fildes, off_t offset, int whence)
{
    struct filestr_desc *filestr = get_filestr(fildes);
    if (!filestr)
        return -1;

    return os_lseek(filestr->osfd, offset, whence);
}

ssize_t sim_read(int fildes, void *buf, size_t nbyte)
{
    struct filestr_desc *filestr = get_filestr(fildes);
    if (!filestr)
        return -1;

    return os_read(filestr->osfd, buf, nbyte);
}

ssize_t sim_write(int fildes, const void *buf, size_t nbyte)
{
    struct filestr_desc *filestr = get_filestr(fildes);
    if (!filestr)
        return -1;

    return os_write(filestr->osfd, buf, nbyte);
}

int sim_remove(const char *path)
{
    char ospath[SIM_TMPBUF_MAX_PATH];
    if (sim_get_os_path(ospath, path, sizeof (ospath)) < 0)
        return -1;

    int rc = os_remove(ospath);

#ifdef HAVE_DIRCACHE
    if (rc >= 0)
        dircache_remove(xxxxxx);
#endif

    return rc;
}

int sim_rename(const char *old, const char *new)
{
    char osold[SIM_TMPBUF_MAX_PATH];
    int pprc1 = sim_get_os_path(osold, old, sizeof (osold));
    if (pprc1 < 0)
        return -1;

    char osnew[SIM_TMPBUF_MAX_PATH];
    int pprc2 = sim_get_os_path(osnew, new, sizeof (osnew));
    if (pprc2 < 0)
        return -1;

    if (MAX(pprc1 - 1, 0) != MAX(pprc2 - 1, 0))
    {
        /* Pretend they're different devices */
        errno = EXDEV;
        return -1;
    }

    int rc = os_rename(osold, osnew);

#ifdef HAVE_DIRCACHE
    if (rc >= 0)
        dircache_rename(xxxxxx);
#endif

    return rc;
}

int sim_modtime(const char *path, time_t modtime)
{
    char ospath[SIM_TMPBUF_MAX_PATH];

    if (sim_get_os_path(ospath, path, sizeof (ospath)) < 0)
        return false;

    return os_modtime(ospath, modtime);
}

off_t sim_filesize(int fildes)
{
    struct filestr_desc *filestr = get_filestr(fildes);
    if (!filestr)
        return -1;

    return os_filesize(filestr->osfd);
}

int sim_fsamefile(int fildes1, int fildes2)
{
    struct filestr_desc *filestr1 = get_filestr(fildes1);
    if (!filestr1)
        return -1;

    struct filestr_desc *filestr2 = get_filestr(fildes2);
    if (!filestr2)
        return -1;

    if (filestr1 == filestr2)
        return 1;

    return os_fsamefile(filestr1->osfd, filestr2->osfd);
}

int sim_relate(const char *path1, const char *path2)
{
    char ospath1[SIM_TMPBUF_MAX_PATH];
    if (sim_get_os_path(ospath1, path1, sizeof (ospath1)) < 0)
        return -1;

    char ospath2[SIM_TMPBUF_MAX_PATH];
    if (sim_get_os_path(ospath2, path2, sizeof (ospath2)) < 0)
        return -1;

    return os_relate(ospath1, ospath2);
}

bool sim_file_exists(const char *path)
{
    char ospath[SIM_TMPBUF_MAX_PATH];
    if (sim_get_os_path(ospath, path, sizeof (ospath)) < 0)
        return false;

    return os_file_exists(ospath);
}


/** Directory functions **/
DIR * sim_opendir(const char *dirname)
{
    struct dirstr_desc *dirstr = alloc_dirstr();
    if (!dirstr)
    {
        errno = EMFILE;
        return NULL;
    }

    char osdirname[SIM_TMPBUF_MAX_PATH];
    int pprc = sim_get_os_path(osdirname, dirname, sizeof (osdirname));
    if (pprc < 0)
        return NULL;

    int rc = os_opendir_and_fd(osdirname, &dirstr->osdirp, &dirstr->osfd);
    if (rc < 0)
        return NULL;

    dirstr->osfd_opened = rc > 0;

#ifdef USE_OSDIRNAME
    dirstr->osdirname = strdup(osdirname);
    if (!dirstr->osdirname)
    {
        close_dirstr(dirstr);
        return NULL;
    }
#endif

    dirstr->entry.d_name[0] = 0; /* Mark as invalid */
#ifdef HAVE_MULTIVOLUME
    dirstr->volume = MAX(pprc - 1, 0);
    dirstr->volumecounter = pprc == 0 ? 0 : INT_MAX;
#endif
    dirstr->mounted = true;
    return (DIR *)dirstr; /* A-Okay */
}

int sim_closedir(DIR *dirp)
{
    struct dirstr_desc *dirstr = (struct dirstr_desc *)dirp;
    if (!PTR_IN_ARRAY(opendirs, dirstr, MAX_OPEN_DIRS) || !dirstr->osdirp)
    {
        errno = EBADF;
        return -1;
    }

    return close_dirstr(dirstr);
}

struct sim_dirent * sim_readdir(DIR *dirp)
{
    struct dirstr_desc *dirstr = get_dirstr(dirp);
    if (!dirstr)
        return NULL;

    struct sim_dirent *entry = &dirstr->entry;
    entry->info.osdirent = NULL;

    if (readdir_volume(dirstr, entry))
        return entry;

    OS_DIRENT_T *osdirent = os_readdir(dirstr->osdirp);
    if (!osdirent)
        return NULL;

    size_t size = sizeof (entry->d_name);
    if (strlcpy_from_os(entry->d_name, osdirent->d_name, size) >= size)
        FILE_ERROR_RETURN(ENAMETOOLONG, NULL);

    entry->info.osdirent = osdirent;
    return entry;
}

int sim_mkdir(const char *path)
{
    char ospath[SIM_TMPBUF_MAX_PATH];
    if (sim_get_os_path(ospath, path, sizeof (ospath)) < 0)
        return -1;

    int rc = os_mkdir(ospath __MKDIR_MODE_ARG);

#ifdef HAVE_DIRCACHE
    if (rc >= 0)
        dircache_mkdir(xxxxxx);
#endif

    return rc;
}

int sim_rmdir(const char *path)
{
    char ospath[SIM_TMPBUF_MAX_PATH];
    if (sim_get_os_path(ospath, path, sizeof (ospath)) < 0)
        return -1;

    int rc = os_rmdir(ospath);

#ifdef HAVE_DIRCACHE
    if (rc >= 0)
        dircache_rmdir(xxxxxx);
#endif

    return rc;
}

int sim_samedir(DIR *dirp1, DIR *dirp2)
{
    struct dirstr_desc *dirstr1 = get_dirstr(dirp1);
    if (!dirstr1)
        return -1;

    struct dirstr_desc *dirstr2 = get_dirstr(dirp2);
    if (!dirstr2)
        return -1;

    return os_fsamefile(dirstr1->osfd, dirstr2->osfd);
}

bool sim_dir_exists(const char *dirname)
{
    char osdirname[SIM_TMPBUF_MAX_PATH];
    if (sim_get_os_path(osdirname, dirname, sizeof (osdirname)) < 0)
        return false;

    OS_DIR_T *dirp = os_opendir(osdirname);
    if (!dirp)
        return false;

    os_closedir(dirp);
    return true;
}

struct dirinfo dir_get_info(DIR *dirp, struct sim_dirent *entry)
{
    int rc;
    struct dirstr_desc *dirstr = get_dirstr(dirp);
    if (!dirstr)
        FILE_ERROR(ERRNO, RC);

    if (entry->d_name[0] == 0)
        FILE_ERROR(ENOENT, RC);

    OS_DIRENT_T *osdirent = entry->info.osdirent;
    if (osdirent == NULL)
        return (struct dirinfo){ .attribute = ATTR_MOUNT_POINT };

    struct dirinfo info;
    info.attribute = 0;

    OS_STAT_T s;

#ifdef os_fstatat
    if (os_fstatat(dirstr->osfd, entry->d_name, &s, 0))
#else /* no fstatat; build file name for stat() */
    char statpath[SIM_TMPBUF_MAX_PATH];
    if (path_append(statpath, dirstr->osdirname, entry->d_name,
                    sizeof (statpath)) >= sizeof (statpath))
    {
        FILE_ERROR(ENAMETOOLONG, RC);
    }

    if (os_stat(statpath, &s)) /* get info */
#endif /* os_fstatat */
    {
        /* File size larger than 2 GB? */
    #ifdef EOVERFLOW
        if (errno == EOVERFLOW)
            DEBUGF("stat overflow for \"%s\"\n", entry->d_name);
    #endif
        FILE_ERROR(ERRNO, RC);
    }

    if (S_ISDIR(s.st_mode))
        info.attribute |= ATTR_DIRECTORY;

    info.size = s.st_size;

    struct tm tm;
    if (localtime_r(&s.st_mtime, &tm) == NULL)
        FILE_ERROR(ERRNO, RC);

    info.mtime = mktime(&tm);

    return info;

file_error:
    return (struct dirinfo){ .size = 0 };
}

int os_volume_path(IF_MV(int volume, ) char *buffer, size_t bufsize)
{
    if (!buffer || !bufsize IF_MV( || !volume_present(volume) ))
        return -1;

    char tmpbuf[SIM_TMPBUF_MAX_PATH];
    tmpbuf[0] = '\0';

#ifdef HAVE_MULTIVOLUME
    char volname[VOL_MAX_LEN + 1];
    get_volume_name(volume, volname);

    if (path_append(tmpbuf, PA_SEP_HARD, volname, sizeof (tmpbuf))
            >= sizeof (volname))
        return -1;
#endif /* HAVE_MULTIVOLUME */

    if (path_append(tmpbuf, PA_SEP_HARD, ".", sizeof (tmpbuf))
        >= sizeof (tmpbuf))
        return -1;

    return sim_get_os_path(buffer, tmpbuf, bufsize);
}

const char* sim_root_realpath(void)
{
    return PATH_ROOTSTR;
}
