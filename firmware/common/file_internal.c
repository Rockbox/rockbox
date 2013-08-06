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
#include "disk_cache.h"
#include "fileobj_mgr.h"
#include "dir.h"
#include "dircache_redirect.h"
#include "dircache.h"
#include "string-extra.h"
#include "rbunicode.h"

/** Internal common filesystem service functions  **/

/* for internal functions' scanning use to save quite a bit of stack space -
   access must be serialized by the writer/aux lock */
struct fat_direntry dir_fatent IBSS_ATTR;
struct ll_head open_files_internal[NUM_VOLUMES];

struct mrsw_lock file_internal_mrsw SHAREDBSS_ATTR;


/** File stream sector caching **/

/* initialize a new cache structure */
void file_cache_init(struct filestr_cache *cachep)
{
    cachep->buffer = NULL;
    cachep->sector = INVALID_SECNUM;
    cachep->flags  = 0;
}

/* discard and mark the cache buffer as unused */
void file_cache_reset(struct filestr_cache *cachep)
{
    cachep->sector = INVALID_SECNUM;
    cachep->flags  = 0;
}

/* allocate resources attached to the cache */
void file_cache_alloc(struct filestr_cache *cachep)
{
    /* if this fails, it is a bug; check for leaks and that the cache has
       enough buffers for the worst case */
    if (!cachep->buffer && !(cachep->buffer = dc_get_buffer()))
        panicf("file_cache_alloc - OOM");
}

/* free resources attached to the cache */
void file_cache_free(struct filestr_cache *cachep)
{
    if (cachep && cachep->buffer)
    {
        dc_release_buffer(cachep->buffer);
        cachep->buffer = NULL;
    }

    file_cache_reset(cachep);
}


/** Stream base APIs **/

static inline void filestr_clear(struct filestr_base *stream)
{
    stream->flags = 0;
    stream->bindp = NULL;
#if 0
    stream->mtx   = NULL;
#endif
}

/* actually late-allocate the assigned cache */
void filestr_alloc_cache(struct filestr_base *stream)
{
    file_cache_alloc(stream->cachep);
}

/* free the stream's cache buffer if it's its own */
void filestr_free_cache(struct filestr_base *stream)
{
    if (stream->cachep == &stream->cache)
        file_cache_free(stream->cachep);
}

/* assign a cache to the stream */
void filestr_assign_cache(struct filestr_base *stream,
                          struct filestr_cache *cachep)
{
    if (cachep)
    {
        filestr_free_cache(stream);
        stream->cachep = cachep;
    }
    else /* assign own cache */
    {
        file_cache_reset(&stream->cache);
        stream->cachep = &stream->cache;
    }
}

/* duplicate a cache into a stream's local cache */
void filestr_copy_cache(struct filestr_base *stream,
                        struct filestr_cache *cachep)
{
    stream->cachep = &stream->cache;
    stream->cache.sector = cachep->sector;
    stream->cache.flags  = cachep->flags;

    if (cachep->buffer)
    {
        file_cache_alloc(&stream->cache);
        memcpy(stream->cache.buffer, cachep->buffer, DC_CACHE_BUFSIZE);
    }
    else
    {
        file_cache_free(&stream->cache);
    }
}

/* discard cache contents and invalidate it */
void filestr_discard_cache(struct filestr_base *stream)
{
    file_cache_reset(stream->cachep);
}

/* Initialize the base descriptor */
void filestr_base_init(struct filestr_base *stream)
{
    filestr_clear(stream);
    file_cache_init(&stream->cache);
    stream->cachep = &stream->cache;
}

/* free base descriptor resources */
void filestr_base_destroy(struct filestr_base *stream)
{
    filestr_clear(stream);
    filestr_free_cache(stream);
}


/** Internal directory service functions **/

/* read the next directory entry and return its FS info */
int uncached_readdir_internal(struct filestr_base *stream,
                              struct file_base_info *infop,
                              struct fat_direntry *fatent)
{
    return fat_readdir(&stream->fatstr, &infop->fatfile.e,
                       filestr_get_cache(stream), fatent);
}

/* rewind the FS directory to the beginning */
void uncached_rewinddir_internal(struct file_base_info *infop)
{
    fat_rewinddir(&infop->fatfile.e);
}

/* check if the directory is empty (ie. only "." and/or ".." entries
   exist at most) */
int test_dir_empty_internal(struct filestr_base *stream)
{
    int rc;

    struct file_base_info info;
    fat_rewind(&stream->fatstr);
    rewinddir_internal(&info);

    while ((rc = readdir_internal(stream, &info, &dir_fatent)) > 0)
    {
        /* no OEM decoding is recessary for this simple check */
        if (!is_dotdir_name(dir_fatent.name))
        {
            DEBUGF("Directory not empty\n");
            FILE_ERROR_RETURN(ENOTEMPTY, -1);
        }
    }

    if (rc < 0)
    {
        DEBUGF("I/O error checking directory: %d\n", rc);
        FILE_ERROR_RETURN(EIO, rc * 10 - 2);
    }

    return 0;
}

/* iso decode the name to UTF-8 */
void iso_decode_d_name(char *d_name)
{
    if (is_dotdir_name(d_name))
        return;

    char shortname[13];
    size_t len = strlcpy(shortname, d_name, sizeof (shortname));
    /* This MUST be the default codepage thus not something that could be
       loaded on call */
    iso_decode(shortname, d_name, -1, len + 1);
}

#ifdef HAVE_DIRCACHE
/* nullify all the fields of the struct dirent */
void empty_dirent(struct dirent *entry)
{
    entry->d_name[0] = '\0';
    entry->info.attr    = 0;
    entry->info.size    = 0;
    entry->info.wrtdate = 0;
    entry->info.wrttime = 0;
}

/* fill the native dirinfo from the static dir_fatent */
void fill_dirinfo_native(struct dirinfo_native *dinp)
{
    struct fat_direntry *fatent = get_dir_fatent();
    dinp->attr    = fatent->attr;
    dinp->size    = fatent->filesize;
    dinp->wrtdate = fatent->wrtdate;
    dinp->wrttime = fatent->wrttime;
}
#endif /* HAVE_DIRCACHE */

int uncached_readdir_dirent(struct filestr_base *stream,
                            struct dirscan_info *scanp,
                            struct dirent *entry)
{
    struct fat_direntry fatent;
    int rc = fat_readdir(&stream->fatstr, &scanp->fatscan,
                         filestr_get_cache(stream), &fatent);

    /* FAT driver clears the struct fat_dirent if nothing is returned */
    strcpy(entry->d_name, fatent.name);
    entry->info.attr    = fatent.attr;
    entry->info.size    = fatent.filesize;
    entry->info.wrtdate = fatent.wrtdate;
    entry->info.wrttime = fatent.wrttime;

    return rc;
}

/* rewind the FS directory pointer */
void uncached_rewinddir_dirent(struct dirscan_info *scanp)
{
    fat_rewinddir(&scanp->fatscan);
}


/** open_stream_internal() helpers and types **/

struct pathwalk
{
    const char            *path;          /* current location in input path */
    unsigned int          callflags;      /* callflags parameter */
    struct filestr_base   *stream;        /* stream parameter */
    struct path_component_info *compinfo; /* compinfo parameter */
};

struct pathwalk_component
{
    struct file_base_info info;     /* basic file information */
    file_size_t           filesize; /* size of the file */
    const char            *name;    /* component name location in path */
    uint16_t              length;   /* length of name of component */
    uint16_t              attr;     /* attributes of this component */
};

/* return another struct pathwalk_component from the pool, or NULL if the
   pool is completely used */
static struct pathwalk_component *
pathwalk_comp_alloc_(struct pathwalk_component *parentp)
{
    /* static pool that goes to a depth of STATIC_COMP_NUM before allocating
       elements from the stack */
    static struct pathwalk_component aux_pathwalk[STATIC_PATHCOMP_NUM];

    if (!parentp)
        return &aux_pathwalk[0]; /* root */

    if (PTR_IN_ARRAY(aux_pathwalk, parentp, STATIC_PATHCOMP_NUM-1))
        return parentp + 1;

    return NULL;
}

/* allocates components from the pool or stack depending upon the depth */
#define pathwalk_comp_alloc(parentp) \
    ({  struct pathwalk_component *__c = pathwalk_comp_alloc_(parentp); \
        __builtin_constant_p(parentp) ?  __c :                          \
        (__c ?: (struct pathwalk_component *)alloca(sizeof(*(parentp)))); })

/* fill in the details of the struct path_component_info for caller */
static inline void fill_path_compinfo(struct pathwalk *walkp,
                                      struct pathwalk_component *parentp,
                                      struct pathwalk_component *compp,
                                      int rc)
{
    if (rc == RC_NOT_FOUND)
    {
        /* this component wasn't found; see if more of them exist or path
           has trailing separators; if it does, this component should be
           interpreted as a directory even if it doesn't exist and it's the
           final one */
        const char *p = GOBBLE_PATH_SEPCH(walkp->path);
        if (p > walkp->path)
            compp->attr |= ATTR_DIRECTORY;

        /* if it was just trailing separators, p will point at the null */
        walkp->path = p;
    }

    struct path_component_info *compinfo = walkp->compinfo;

    compinfo->name       = compp->name;
    compinfo->length     = compp->length;
    compinfo->attr       = compp->attr;
    compinfo->filesize   = compp->filesize;
    compinfo->parentinfo = parentp->info;
}

/* open the final stream itself, if found */
static int walk_open_info(struct pathwalk *walkp,
                          struct pathwalk_component *parentp,
                          struct pathwalk_component *compp,
                          int rc)
{
    /* this may make adjustments to things; do it first */
    if (walkp->compinfo)
        fill_path_compinfo(walkp, parentp, compp, rc);

    if (rc != RC_FOUND)
        return rc;

    unsigned int callflags = walkp->callflags;
    bool isdir = compp->attr & ATTR_DIRECTORY;

    /* type must match what is called for */
    switch (callflags & FF_TYPEMASK)
    {
    case FF_FILE:
        if (!isdir) break;
        DEBUGF("File is a directory\n");
        return EISDIR;
    case FF_DIR:
        if (isdir) break;
        DEBUGF("File is not a directory\n");
        return ENOTDIR;
    /* FF_ANYTYPE: basically, ignore FF_FILE/FF_DIR */
    }

    /* FO_DIRECTORY must match type */
    if (isdir)
        callflags |= FO_DIRECTORY;
    else
        callflags &= ~FO_DIRECTORY;

    fileop_onopen_internal(walkp->stream, &compp->info, callflags);
    return RC_FOUND;
}

/* check the component against the prefix test info */
static void walk_check_prefix(struct pathwalk *walkp,
                              struct pathwalk_component *compp)
{
    if (compp->attr & ATTR_PREFIX)
        return;

    if (!fat_file_is_same(&compp->info.fatfile,
                          &walkp->compinfo->prefixp->fatfile))
        return;

    compp->attr |= ATTR_PREFIX;
}

/* opens the component named by 'comp' in the directory 'parent' */
static NO_INLINE int open_path_component(struct pathwalk *walkp,
                                         struct pathwalk_component *parentp,
                                         struct pathwalk_component *compp)
{
    int rc;

    /* create a null-terminated copy of the component name */
    char * compname = strlendupa(compp->name, compp->length);

    struct filestr_base *stream = walkp->stream;
    unsigned int callflags = walkp->callflags;

    /* scan parent for name; stream is converted to this parent */
    file_cache_reset(stream->cachep);
    stream->infop = &parentp->info;
    fat_filestr_init(&stream->fatstr, &parentp->info.fatfile);
    rewinddir_internal(&compp->info);

    while ((rc = readdir_internal(stream, &compp->info, &dir_fatent)) > 0)
    {
        if (rc > 1 && !(callflags & FF_NOISO))
            iso_decode_d_name(dir_fatent.name);

        if (!strcasecmp(compname, dir_fatent.name))
            break;
    }

    /* children inherit the prefix coloring from the parent */
    compp->attr = parentp->attr & ATTR_PREFIX;

    if (rc == 0)
    {
        compp->filesize = 0;
        DEBUGF("File/directory not found\n");
        return RC_OPEN_ENOENT;
    }
    else if (rc < 0)
    {
        DEBUGF("I/O error reading directory: %d\n", rc);
        return EIO;
    }

    rc = fat_open(stream->fatstr.fatfilep, dir_fatent.firstcluster,
                  &compp->info.fatfile);
    if (rc < 0)
    {
        DEBUGF("I/O error opening file/directory: %d (%ld)\n",
               rc, dir_fatent.firstcluster);
        return EIO;
    }

    compp->filesize = dir_fatent.filesize;
    compp->attr    |= dir_fatent.attr;

    if (callflags & FF_CHECKPREFIX)
        walk_check_prefix(walkp, compp);

    return RC_CONTINUE;
}

/* parse a path component, open it and process the next */
static NO_INLINE int
process_path_component(struct pathwalk *walkp,
                       struct pathwalk_component *parentp)
{
    int rc;

    struct pathwalk_component *compp = pathwalk_comp_alloc(parentp);

    rc = parse_path_component(&walkp->path, &compp->name, false);
    if (rc <= 0)
        return rc;
    else if (rc >= MAX_PATH)
        return ENAMETOOLONG;    /* that's just unpossible, man */

    compp->length = rc;

    rc = open_path_component(walkp, parentp, compp);
    if (rc == RC_CONTINUE)
    {
        if (!*walkp->path)
            rc = RC_PATH_ENDED; /* done */
        else if (!(compp->attr & ATTR_DIRECTORY))
            rc = ENOTDIR;       /* a parent component must be a directory */
        else
            while ((rc = process_path_component(walkp, compp)) == RC_CONTINUE);
    }

    switch (rc)
    {
    case RC_GO_UP:              /* hit ".."; drop to previous level */
        rc = RC_CONTINUE;
        break;
    case RC_PATH_ENDED:         /* success! */
    case RC_OPEN_ENOENT:        /* component not found */
        rc += DONE_RC_ADJUST;
        rc = walk_open_info(walkp, parentp, compp, rc);
        break;
    }

    return rc;
}

/* open a stream given a path to the resource */
int open_stream_internal(const char *path, unsigned int callflags,
                         struct filestr_base *stream,
                         struct path_component_info *compinfo)
{
    DEBUGF("%s(path=\"%s\",flg=%X,str=%p,compinfo=%p)\n", path, callflags,
           stream, compinfo);
    int rc;

    filestr_base_init(stream);

    if (!path_is_absolute(path))
    {
        /* while this supports relative components, there is currently no
           current working directory concept at this level by which to
           fully qualify the path (though that would not be excessively
           difficult to add) */
        DEBUGF("\"%s\" is not an absolute path\n"
               "Only absolute paths currently supported.\n", path);
        FILE_ERROR(ENOENT, -1);
    }

    /* if !compinfo, then the result of this check is not visible anyway */
    if (!compinfo)
        callflags &= ~FF_CHECKPREFIX;

    struct pathwalk walk;
    walk.path      = path;
    walk.callflags = callflags;
    walk.stream    = stream;
    walk.compinfo  = compinfo;

    struct pathwalk_component *rootp = pathwalk_comp_alloc(NULL);

#ifdef HAVE_MULTIVOLUME
    int volume = 0, sysroot = 0;
#endif /* HAVE_MULTIVOLUME */
    rc = RC_GO_UP;

    do
    {
        if (rc == RC_GO_UP)
        {
            const char *pathptr = walk.path;

        #ifdef HAVE_MULTIVOLUME
            /* this seamlessly integrates secondary filesystems into the
               root namespace (e.g. "/<0>/../../<1>/../foo/." :<=> "/foo") */
            const char *p;
            volume = path_strip_volume(pathptr, &p, false);
            if (!CHECK_VOL(volume))
            {
                DEBUGF("No such device or address: %d\n", volume);
                FILE_ERROR(ENXIO, -2);
            }

            /* the root of this subpath is the system root? */
            sysroot = p == pathptr ? 1 : 0;
            walk.path = p;
        #endif /* HAVE_MULTIVOLUME */

            /* set name to start at last leading separator; names of volume
               specifiers will be returned as "/<fooN>" */
            rootp->name   = GOBBLE_PATH_SEPCH(pathptr) - 1;
            rootp->length = IF_MV( !sysroot ? p - rootp->name : ) 1;
        }

        rc = fat_open_rootdir(IF_MV(volume,) &rootp->info.fatfile);
        if (rc < 0)
        {
            /* not mounted */
            DEBUGF("No such device or address: %d\n", IF_MV_VOL(volume));
            FILE_ERROR(ENXIO, rc * 10 - 3);
        }

        rootp->attr = ATTR_DIRECTORY;
        if (callflags & FF_CHECKPREFIX)
            walk_check_prefix(&walk, rootp);

        get_rootinfo_internal(&rootp->info);
        rc = process_path_component(&walk, rootp);
    }
    while (rc == RC_CONTINUE || rc == RC_GO_UP);

    switch (rc)
    {
    case RC_PATH_ENDED: /* ended up at a root */
        rc = walk_open_info(&walk, rootp, rootp, RC_FOUND);
        if (rc != RC_FOUND)
            FILE_ERROR(rc, -4);

        rc = IF_MV(sysroot) +1;
        break;

    case RC_FOUND:      /* every component opened successfully */
        rc = 1;
        break;

    case RC_NOT_FOUND:  /* some component not found */
        /* this has to be the last part or it's an error; no 'info' means it's
           only probing the namespace and it may be assumed the caller has no
           intention of creating anything as a result of 'not found' */
        if (!compinfo || *walk.path)
        {
            DEBUGF("File/directory not found\n");
            FILE_ERROR(ENOENT, -5);
        }

        rc = 0;
        break;

    default:            /* other sorts of trouble from within */
        FILE_ERROR(rc, -6);
    }

file_error:
    file_cache_reset(stream->cachep);

    if (rc < 0)
    {
        /* utter, abject failure :`( */
        DEBUGF("Open failed: rc=%d, errno=%d\n", rc, errno);
        filestr_base_destroy(stream);
    }

    return rc;
}

/* close the stream referenced by 'stream' */
int close_stream_internal(struct filestr_base *stream)
{
    int rc;
    unsigned int foflags = fileobj_get_flags(stream);

    if ((foflags & (FO_SINGLE|FO_REMOVED)) == (FO_SINGLE|FO_REMOVED))
    {
        /* nothing is referencing it so now remove the file's data */
        rc = fat_remove(&stream->infop->fatfile, FAT_RM_DATA);
        if (rc < 0)
        {
            DEBUGF("I/O error removing file data: %d\n", rc);
            FILE_ERROR(EIO, rc * 10 - 1);
        }
    }

    rc = 0;
file_error:
    /* close no matter what */
    fileop_onclose_internal(stream);
    return rc;
}

/* create a new stream in the parent directory */
int create_stream_internal(struct file_base_info *parentinfop,
                           const char *basename, size_t length,
                           unsigned int attr, unsigned int callflags,
                           struct filestr_base *stream)
{
    /* assumes an attempt was made beforehand to open *stream with
       open_stream_internal() which returned zero (successfully not found),
       so does not initialize it here */
    const char * const name = strlendupa(basename, length);
    DEBUGF("Creating \"%s\"\n", name);

    struct file_base_info info;
    int rc = fat_create_file(&parentinfop->fatfile, name, attr,
                             &info.fatfile, get_dir_fatent_dircache());
    if (rc < 0)
    {
        DEBUGF("Create failed: %d\n", rc);
        FILE_ERROR(rc == FAT_RC_ENOSPC ? ENOSPC : EIO, rc * 10 - 1);
    }

    /* dir_fatent is implicit arg */
    fileop_oncreate_internal(stream, &info, callflags, parentinfop, name);
    rc = 0;
file_error:
    return rc;
}

/* removes files and directories - back-end to remove() and rmdir() */
int remove_stream_internal(const char *path, struct filestr_base *stream,
                           unsigned int callflags)
{
    /* Only FF_* flags should be in callflags */
    int rc;

    struct filestr_base opened_stream;
    if (!stream)
        stream = &opened_stream;

    if (stream == &opened_stream)
    {
        /* no stream provided so open local one */
        rc = open_stream_internal(path, callflags, stream, NULL);
        if (rc < 0)
        {
            DEBUGF("Failed opening path: %d\n", rc);
            FILE_ERROR(ERRNO, rc * 10 - 1);
        }
    }
    /* else ignore the 'path' argument */

    if (callflags & FF_DIR)
    {
        /* directory to be removed must be empty */
        rc = test_dir_empty_internal(stream);
        if (rc < 0)
            FILE_ERROR(ERRNO, rc * 10 - 2);
    }

    /* save old info since fat_remove() will destroy the dir info */
    struct file_base_info oldinfo = *stream->infop;
    rc = fat_remove(&stream->infop->fatfile, FAT_RM_DIRENTRIES);
    if (rc < 0)
    {
        DEBUGF("I/O error removing dir entries: %d\n", rc);
        FILE_ERROR(EIO, rc * 10 - 3);
    }

    fileop_onremove_internal(stream, &oldinfo);

    rc = 0;
file_error:
    if (stream == &opened_stream)
    {
        /* will do removal of data below if this is the only reference */
        int rc2 = close_stream_internal(stream);
        if (rc2 < 0 && rc >= 0)
        {
            rc = rc2 * 10 - 4;
            DEBUGF("Success but failed closing stream: %d\n", rc);
        }
    }

    return rc;
}

/* test file/directory existence with constraints */
int test_stream_exists_internal(const char *path, unsigned int callflags)
{
    /* only FF_* flags should be in callflags */
    struct filestr_base stream;
    int rc = open_stream_internal(path, callflags, &stream, NULL);
    if (rc > 0)
        close_stream_internal(&stream);

    return rc;
}

/* one-time init at startup */
void filesystem_init(void)
{
    mrsw_init(&file_internal_mrsw);
    dc_init();
    fileobj_mgr_init();
}
