/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Michael Sevakis
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
#include "config.h"
#include <errno.h>
#include "system.h"
#include "debug.h"
#include "panic.h"
#include "pathfuncs.h"
#include "file_internal.h"
#include "fileobj_mgr.h"
#include "dircache_redirect.h"
#include "string-extra.h"
#include "dir.h"
#include <kernel.h>

/** Internal common filesystem service functions  **/

struct mrsw_lock file_internal_mrsw SHAREDBSS_ATTR;

/* for internal functions' scanning use to save quite a bit of stack space -
   access must be serialized by the writer lock */
#if defined(CPU_SH) || defined(IAUDIO_M5) \
    || CONFIG_CPU == IMX233
/* otherwise, out of IRAM */
static struct dirent __dirent_g;
struct fileentry __fileentry_g;
#else
static struct dirent __dirent_g IBSS_ATTR;
struct fileentry __fileentry_g IBSS_ATTR;
#endif

#define STATIC_DIRENT (&__dirent_g)

int __internal_errno_g;

static unsigned int fd_callflags = ~0u;

/** Internal directory service functions **/

/* check if the directory is empty (ie. only "." and/or ".." entries
   exist at most) */
int test_dir_empty_internal(struct fileinfo *dirinfo)
{
    struct fileentry *entry = STATIC_FILEENTRY;
    int rc = get_fileentry_internal(dirinfo, NULL, 0, 0, entry);

    if (FSI_S_RDDIR_HAVEENT(rc))
        rc = -ENOTEMPTY;

    return rc;
}

int uncached_readdir_internal(struct filestr *dirstr,
                              struct dirscan *scanp,
                              struct dirent *entry)
{
    struct fileentry fileentry;
    fileentry.name = entry->d_name;
    int rc = __FILEOP(readdir)(dirstr, scanp, &fileentry);
    if (FSI_S_RDDIR_HAVEENT(rc))
    {
        int rc2 = __FILEOP(convert_entry)(entry, &fileentry,
                                          CVTENT_SRC_FILEENTRY |
                                          CVTENT_DST_DIRENT);
        if (rc2 < 0)
            rc = rc2;
    }

    return rc;
}

/** open_stream_internal() helpers and types **/

struct pathwalk
{
    const char                 *path;      /* current location in input path */
    unsigned int               callflags;  /* callflags parameter */
    struct path_component_info *compinfop; /* compinfo parameter */
    struct fileinfo            **infopp;   /* infopp parameter */
    unsigned int               pfxcnt;     /* prefix depth counter */
};

struct pathwalk_component
{
    struct fileinfo           info;        /* basic file information */
    const char                *name;       /* component name location in path */
    uint16_t                  namelen;     /* length of name of component */
    uint16_t                  attr;        /* attributes of this component */
    struct pathwalk_component *nextp;      /* parent if in use else next free */
};

#define WALK_RC_CONT_AT_ROOT (-__ELASTERROR)  /* continue at root level */

/* return another struct pathwalk_component from the pool, or NULL if the
   pool is completely used */
static void * pathwalk_comp_alloc_(struct pathwalk_component *parentp)
{
    /* static pool that goes to a depth of STATIC_COMP_NUM before allocating
       elements from the stack */
    static struct pathwalk_component aux_pathwalk[STATIC_PATHCOMP_NUM];
    struct pathwalk_component *compp = NULL;

    if (!parentp)
        compp = &aux_pathwalk[0]; /* root */
    else if (PTR_IN_ARRAY(aux_pathwalk, parentp, STATIC_PATHCOMP_NUM-1))
        compp = parentp + 1;

    return compp;
}

/* allocates components from the pool or stack depending upon the depth */
#define pathwalk_comp_alloc(parentp) \
    ({                                                        \
        void *__c = pathwalk_comp_alloc_(parentp);            \
        if (!__builtin_constant_p(parentp) && !__c)           \
            __c = alloca(sizeof (struct pathwalk_component)); \
        (struct pathwalk_component *)__c;                     \
    })

/* fill in the details of the struct path_component_info for caller */
static int fill_path_compinfo(struct pathwalk *walkp,
                              struct pathwalk_component *compp,
                              int rc)
{
    /* still fail if writing is disabled and file doesn't exist */
    if (rc == -ENOENT)
    {
        /* this component wasn't found; see if more of them exist or path
           has trailing separators; if it does, this component should be
           interpreted as a directory even if it doesn't exist and it's the
           final one; also, this has to be the last part or it's an error */
        compp->attr = *walkp->path ? ATTR_DIRECTORY : 0;
        compp->info.size = 0;

        if (!*GOBBLE_PATH_SEPCH(walkp->path))
            rc = FSI_S_OPEN_NOENT; /* successfully not found */
    }

    if (rc >= 0)
    {
        struct path_component_info *compinfop = walkp->compinfop;
        compinfop->name      = compp->name;
        compinfop->namelen   = compp->namelen;
        compinfop->attr      = compp->attr;
        compinfop->filesize  = compp->info.size;
        compinfop->flags     = walkp->pfxcnt ? FF_PREFIX : 0;

        if (walkp->callflags & FF_INFO)
            fileinfo_copy_fsinfo(&compinfop->info, &compp->info);

        if (walkp->callflags & FF_PARENTINFO)
        {
            fileinfo_copy_fsinfo(&compinfop->parentinfo,
                                 &(compp->nextp ?: compp)->info);
        }
    }

    return rc;
}

/* open the final stream itself, if found */
static int walk_open_info(struct pathwalk *walkp,
                          struct pathwalk_component *compp,
                          int rc)
{
    /* this may make adjustments to things; do it first */
    if (walkp->compinfop)
        rc = fill_path_compinfo(walkp, compp, rc);

    if (rc < 0 || rc == FSI_S_OPEN_NOENT)
        return rc;

    unsigned int callflags = walkp->callflags;
    bool isdir = compp->attr & ATTR_DIRECTORY;

    /* type must match what is called for */
    switch (callflags & FF_TYPEMASK)
    {
    case FF_FILE:
        if (!isdir) break;
        DEBUGF("File is a directory\n");
        return -EISDIR;
    case FF_DIR:
        if (isdir) break;
        DEBUGF("File is not a directory\n");
        return -ENOTDIR;
    /* FF_ANYTYPE: basically, ignore FF_FILE/FF_DIR */
    }

    if (!walkp->infopp)
        return FSI_S_OPEN_OK;

    /* FO_DIRECTORY must match type */
    if (isdir)
        callflags |= FO_DIRECTORY;
    else
        callflags &= ~FO_DIRECTORY;

    if (compp->attr == ATTR_SYSTEM_ROOT)
        callflags |= FF_SYSROOT;
    else
        callflags &= ~FF_SYSROOT;

    /* make open official if not simply probing for presence - must do it here
       or compp->info on stack will get destroyed before it was copied */
    fileop_onopen(&compp->info, callflags, walkp->infopp);
    return FSI_S_OPEN_OK;
}

/* check the component against the prefix test info */
static inline void walk_check_prefix(const struct fileinfo *parentinfop,
                                     const struct fileinfo *pfxinfop,
                                     struct pathwalk *walkp)
{
    /* children inherit the prefix coloring from the parent */
    if (walkp->pfxcnt ||
        compare_fileinfo_internal(parentinfop, pfxinfop) == CMPFI_EQ)
    {
        walkp->pfxcnt++;
    }
}

/* opens the component named by 'comp' in the directory 'parent' */
static NO_INLINE int open_path_component(struct pathwalk *walkp,
                                         struct pathwalk_component *compp)
{
    int rc;

    /* create a null-terminated copy of the component name */
    unsigned int callflags = walkp->callflags;
    struct pathwalk_component *parentp = compp->nextp;
    struct fileentry *entry = STATIC_FILEENTRY;

    if (callflags & FF_PREFIX)
        walk_check_prefix(&parentp->info, walkp->compinfop->prefixp, walkp);

    rc = get_fileentry_internal(&parentp->info, compp->name, compp->namelen,
                                callflags, entry);
    if (rc == FSI_S_RDDIR_EOD)
    {
        DEBUGF("%s:File/directory not found\n", __func__);
        return -ENOENT;
    }
    else if (rc < 0)
    {
        DEBUGF("%s:I/O error reading directory:%d\n", __func__, rc);
        return rc;
    }

    rc = fileop_open_fileinfo(&parentp->info, entry, &compp->info);
    if (rc < 0)
    {
        DEBUGF("%s:I/O error opening file/directory:%d\n", __func__, rc);
        return rc;
    }

    rc = __FILEOP(convert_entry)(STATIC_DIRENT, entry,
                                 CVTENT_SRC_FILEENTRY | CVTENT_DST_DIRENT);
    if (rc < 0)
    {
        DEBUGF("%s:I/O error converting directory entry:%d\n", __func__, rc);
        return -EIO;
    }

    compp->attr = STATIC_DIRENT->info.attr;

    return FSI_S_OPEN_OK;
}

/* parse a path component, open it and process the next */
static NO_INLINE int
walk_path(struct pathwalk *walkp, struct pathwalk_component *compp)
{
    int rc = FSI_S_OPEN_OK;

    /* alloca is used in a loop, but we reuse any blocks previously allocated
       if we went up then back down; if the path takes us back to the root, then
       everything is cleaned automatically */
    struct pathwalk_component *freep = NULL;

    const char *name;
    ssize_t len;

    while ((len = parse_path_component(&walkp->path, &name)))
    {
        if (len > MAX_COMPNAME)
        {
            rc = -ENAMETOOLONG;
            break;
        }

        /* whatever is to be a parent must be a directory */
        if (!(compp->attr & ATTR_DIRECTORY))
        {
            rc = -ENOTDIR;
            break;
        }

        /* check for "." and ".." */
        if (name[0] == '.')
        {
            if (len == 1)
                continue; /* is "." */

            if (len == 2 && name[1] == '.')
            {
                /* is ".." */
                struct pathwalk_component *parentp = compp->nextp;

                if (walkp->pfxcnt)
                    walkp->pfxcnt--;

                if (!parentp IF_MV( || parentp->attr == ATTR_SYSTEM_ROOT ))
                    return WALK_RC_CONT_AT_ROOT;

                compp->nextp = freep;
                freep = compp;
                compp = parentp;
                continue;
            }
        }

        struct pathwalk_component *newp = freep;
        if (!newp)
            newp = pathwalk_comp_alloc(compp);
        else
            freep = freep->nextp;

        newp->nextp = compp;
        compp = newp;

        compp->name    = name;
        compp->namelen = len;

        rc = open_path_component(walkp, compp);
        if (rc < 0)
            break; /* return info below */
    }

    return walk_open_info(walkp, compp, rc);
}

/* open a stream given a path to the resource */
int open_file_internal(const char *path,
                       unsigned int callflags,
                       struct path_component_info *compinfop,
                       struct fileinfo **infopp)
{
    DEBUGF("%s(path=\"%s\",flg=%X,str=%p,compinfo=%p)\n", __func__,
           path, callflags, str, compinfo);
    int rc;

    if (infopp)
        *infopp = NULL;

    if (!path_is_absolute(path))
    {
        /* while this supports relative components, there is currently no
           current working directory concept at this level by which to
           fully qualify the path (though that would not be excessively
           difficult to add) */
        DEBUGF("\"%s\" is not an absolute path\n"
               "Only absolute paths currently supported.\n", path);
        FILE_ERROR_RC(path ? -ENOENT : -EFAULT);
    }

    /* if !compinfo then these cannot be returned anyway */
    if (!compinfop)
        callflags &= ~(FF_INFO | FF_PARENTINFO | FF_PREFIX);

    struct pathwalk walk;
    walk.path      = path;
    walk.callflags = callflags;
    walk.compinfop = compinfop;
    walk.infopp    = infopp;
    walk.pfxcnt    = 0;

    struct pathwalk_component *rootp = pathwalk_comp_alloc(NULL);
    rootp->nextp = NULL;
    rootp->attr  = ATTR_SYSTEM_ROOT;

    while (1)
    {
        const char *pathptr = walk.path;
 
    #ifdef HAVE_MULTIVOLUME
        /* this seamlessly integrates secondary filesystems into the
           root namespace (e.g. "/<0>/../../<1>/../foo/." :<=> "/foo") */
        const char *p;
        int volume = path_strip_volume(pathptr, &p, false);
        if (!CHECK_VOL(volume))
        {
            DEBUGF("No such device or address: %d\n", volume);
            FILE_ERROR_RC(-ENXIO);
        }

        rootp->attr = p == pathptr ? ATTR_SYSTEM_ROOT : ATTR_MOUNT_POINT;
        walk.path = p;
    #endif /* HAVE_MULTIVOLUME */

        /* set name to start at last leading separator; names of volume
           specifiers will be returned as "/<fooN>" */
        rootp->name    = GOBBLE_PATH_SEPCH(pathptr) - 1;
        rootp->namelen = IF_MV( p != pathptr ? p - rootp->name : ) 1;

        struct bpb *bpb = get_vol_bpb(IF_MV_VOL(volume));
        rc = fileop_open_volume_fileinfo(bpb, &rootp->info);
        if (rc < 0)
        {
            /* not mounted */
            DEBUGF("No such device or address: %d\n", IF_MV_VOL(volume));
            FILE_ERROR_RC(rc);
        }

        rc = walk_path(&walk, rootp);
        if (rc != WALK_RC_CONT_AT_ROOT)
            break;
    }

file_error:
    return rc;
}

/* create a new file in the parent directory */
int create_file_internal(struct fileinfo *dirinfop, const char *basename,
                         size_t namelen, unsigned int callflags,
                         struct fileinfo **infopp)
{
    /* assumes open_file_internal was called first and failed to find the
       basename in the parent directory */
    DEBUGF("%s:dirip=%p:name='%.*s':attr=%X:cf=%X:infopp=%p\n",
           __func__, dirinfop, (int)namelen, basename, attr,
           callflags, infopp);

    if (infopp)
        *infopp = NULL;

    if (namelen > MAX_COMPNAME)
    {
        DEBUGF("Name too long\n");
        return -ENAMETOOLONG;
    }

    struct fileentry *entry = STATIC_FILEENTRY;

    strmemcpy(entry->name, basename, namelen);
    entry->namelen = namelen;

    /* create the file or directory entity */
    int rc = __FILEOP(create)(dirinfop, callflags, entry);
    if (rc < 0)
    {
        DEBUGF("Create failed:%d\n", rc);
        FILE_ERROR_RC(RC);
    }

    fileop_oncreate(dirinfop, entry);

    if (infopp)
    {
        /* caller wants to open it */
        struct fileinfo info;
        rc = fileop_open_fileinfo(dirinfop, entry, &info);
        if (rc < 0)
        {
            DEBUGF("Open failed:%d\n", rc);
            FILE_ERROR_RC(RC);
        }

        /* can't remove the above creation if this failed
           because it needs the file info to do it */
        fileop_onopen(&info, callflags, infopp);
    }

    rc = FSI_S_OPEN_OK;
file_error:
    return rc;
}

int close_file_internal(struct fileinfo *infop)
{
    DEBUGF("%s:ip=%p\n", __func__, infop);

    /* close things no matter what; state of fd is unspecified upon error */
    int rc = 0;

    if (fileobj_get_refcount(infop) == 1)
    {
        /* last reference to file is being closed */
        if (infop->flags & FO_SYNC)
            rc = sync_file_internal(infop);

        int rc2 = __FILEOP(close)(infop);
        if (rc >= 0 && rc2 < 0)
            rc = rc2;
    }

    fileop_onclose(infop);
    return rc;
}

/* removes files and directories - back-end to remove() and rmdir() */
int remove_file_internal(const char *path, unsigned int callflags,
                         struct fileinfo *infop)
{
    DEBUGF("%s:path='%s':cf=%X:ip=%p", __func__, path, callflags, infop);

    /* Only FF_* flags should be in callflags */
    int rc = 0, locrc = -1;

    if (!infop)
    {
        /* open it locally */
        locrc = open_file_internal(path, callflags, NULL, &infop);
        if (!FSI_S_OPENED(locrc))
        {
            DEBUGF("Failed opening path:%d\n", rc);
            FILE_ERROR_RC(locrc);
        }
    }
    /* else ignore the 'path' argument */

    if (infop->flags & FO_DIRECTORY)
    {
        /* directory to be removed must be empty */
        rc = test_dir_empty_internal(infop);
        if (rc < 0)
        {
            if (rc == -ENOTEMPTY)
                DEBUGF("Directory not empty\n");

            FILE_ERROR_RC(RC);
        }
    }

    /* remove the file's entry only */
    rc = __FILEOP(remove)(infop);
    if (rc < 0)
    {
        DEBUGF("Remove entry error:%d\n", rc);
        FILE_ERROR_RC(RC);
    }

    fileop_onremove(infop);

file_error:
    if (FSI_S_OPENED(locrc))
    {
        /* close and remove data */
        int rc2 = close_file_internal(infop);
        if (rc2 < 0 && rc >= 0)
        {
            rc = rc2;
            DEBUGF("Success but failed closing stream: %d\n", rc);
        }
    }
    /* else data will be removed when caller closes it */

    return rc;
}

/* flush all changes to storage */
int sync_file_internal(struct fileinfo *infop)
{
    int rc;

    struct fileentry *entry = STATIC_FILEENTRY;

    rc = __FILEOP(sync)(infop, entry);
    if (rc < 0)
        FILE_ERROR_RC(RC);

    infop->flags &= ~FO_SYNC;
    fileop_onsync_filestr(infop, entry);

    rc = 0;
file_error:
    return rc;
}

/* opens the stream using the info provided by open/create_file_internal */
int open_filestr_internal(struct fileinfo *infop, unsigned int callflags,
                          struct filestr *str)
{
    int rc;

    rc = __FILEOP(open_filestr)(infop, str);
    if (rc < 0)
        FILE_ERROR_RC(rc);

    fileop_onopen_filestr(callflags & fd_callflags, str);

    rc = FSI_S_OPEN_OK;
file_error:
    return rc;
}

/* close the stream referenced by 'str' */
int close_filestr_internal(struct filestr *str)
{
    int rc;

    if (!(str->flags & FD_NONEXIST))
    {
        if (str->flags & FD_WRITE)
            str->infop->flags |= FO_SYNC;

        rc = __FILEOP(close_filestr)(str);
        if (rc < 0)
            FILE_ERROR_RC(RC);
    }

    rc = 0;
file_error:
    /* close no matter what */
    fileop_onclose_filestr(str);
    return rc;
}


/** Filesystem **/

/* one-time init at startup */
void filesystem_init(void)
{
    STATIC_FILEENTRY->name = STATIC_DIRENT->d_name;
    mrsw_init(&file_internal_mrsw);
    iocache_init();
    fileobj_mgr_init();
}

/* flush outstanding writes for all volumes and prevent further writing */
void filesystem_close(void)
{
    file_internal_lock_WRITER();
    file_fileop_filesystem_close();
    fd_callflags = ~FD_WRITE;
    file_internal_unlock_WRITER();

    /* files descriptors may still be opened with read or write access if the
       underlying files exist but writes or creates will fail */
}
