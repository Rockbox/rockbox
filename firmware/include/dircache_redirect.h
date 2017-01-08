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
#ifndef _DIRCACHE_REDIRECT_H_

#include "dir.h"

/***
 ** Internal redirects that depend upon whether or not dircache is made
 **
 ** Some stuff deals with it, some doesn't right now. This is a nexus point..
 **/

/** File binding **/

static inline void get_rootinfo_internal(struct file_base_info *infop)
{
#ifdef HAVE_DIRCACHE
    dircache_get_rootinfo(infop);
#else
    (void)infop;
#endif
}

static inline void fileobj_bind_file(struct file_base_binding *bindp)
{
#ifdef HAVE_DIRCACHE
    dircache_bind_file(bindp);
#else
    file_binding_insert_last(bindp);
#endif
}

static inline void fileobj_unbind_file(struct file_base_binding *bindp)
{
#ifdef HAVE_DIRCACHE
    dircache_unbind_file(bindp);
#else
    file_binding_remove(bindp);
#endif
}


/** File event handlers **/

static inline void fileop_onopen_internal(struct filestr_base *stream,
                                          struct file_base_info *srcinfop,
                                          unsigned int callflags)
{
    fileobj_fileop_open(stream, srcinfop, callflags);
}

static inline void fileop_onclose_internal(struct filestr_base *stream)
{
    fileobj_fileop_close(stream);
}

static inline void fileop_oncreate_internal(struct filestr_base *stream,
                                            struct file_base_info *srcinfop,
                                            unsigned int callflags,
                                            struct file_base_info *dirinfop,
                                            const char *basename)
{
#ifdef HAVE_DIRCACHE
    dircache_dcfile_init(&srcinfop->dcfile);
#endif
    fileobj_fileop_create(stream, srcinfop, callflags);
#ifdef HAVE_DIRCACHE
    struct dirinfo_native din;
    fill_dirinfo_native(&din);
    dircache_fileop_create(dirinfop, stream->bindp, basename, &din);
#endif
    (void)dirinfop; (void)basename;
}

static inline void fileop_onremove_internal(struct filestr_base *stream,
                                            struct file_base_info *oldinfop)
{
    fileobj_fileop_remove(stream, oldinfop);
#ifdef HAVE_DIRCACHE
    dircache_fileop_remove(stream->bindp);
#endif
}

static inline void fileop_onrename_internal(struct filestr_base *stream,
                                            struct file_base_info *oldinfop,
                                            struct file_base_info *dirinfop,
                                            const char *basename)
{
    fileobj_fileop_rename(stream, oldinfop);
#ifdef HAVE_DIRCACHE
    dircache_fileop_rename(dirinfop, stream->bindp, basename);
#endif
    (void)dirinfop; (void)basename;
}

static inline void fileop_onsync_internal(struct filestr_base *stream)
{
    fileobj_fileop_sync(stream);
#ifdef HAVE_DIRCACHE
    struct dirinfo_native din;
    fill_dirinfo_native(&din);
    dircache_fileop_sync(stream->bindp, &din);
#endif
}

static inline void volume_onmount_internal(IF_MV_NONVOID(int volume))
{
#ifdef HAVE_DIRCACHE
    dircache_mount();
#endif
    IF_MV( (void)volume; )
}

static inline void volume_onunmount_internal(IF_MV_NONVOID(int volume))
{
#ifdef HAVE_DIRCACHE
    /* First, to avoid update of something about to be destroyed anyway */
    dircache_unmount(IF_MV(volume));
#endif
    fileobj_mgr_unmount(IF_MV(volume));
}

static inline void fileop_onunmount_internal(struct filestr_base *stream)
{

    if (stream->flags & FD_WRITE)
        force_close_writer_internal(stream); /* try to save stuff */
    else
        fileop_onclose_internal(stream);     /* just readers, bye */
}


/** Directory reading **/

static inline int readdir_dirent(struct filestr_base *stream,
                                 struct dirscan_info *scanp,
                                 struct dirent *entry)
{
#ifdef HAVE_DIRCACHE
    return dircache_readdir_dirent(stream, scanp, entry);
#else
    return uncached_readdir_dirent(stream, scanp, entry);
#endif
}

static inline void rewinddir_dirent(struct dirscan_info *scanp)
{
#ifdef HAVE_DIRCACHE
    dircache_rewinddir_dirent(scanp);
#else
    uncached_rewinddir_dirent(scanp);
#endif
}

static inline int readdir_internal(struct filestr_base *stream,
                                   struct file_base_info *infop,
                                   struct fat_direntry *fatent)
{
#ifdef HAVE_DIRCACHE
    return dircache_readdir_internal(stream, infop, fatent);
#else
    return uncached_readdir_internal(stream, infop, fatent);
#endif
}

static inline void rewinddir_internal(struct file_base_info *infop)
{
#ifdef HAVE_DIRCACHE
    dircache_rewinddir_internal(infop);
#else
    uncached_rewinddir_internal(infop);
#endif
}


/** Misc. stuff **/

static inline struct fat_direntry *get_dir_fatent_dircache(void)
{
#ifdef HAVE_DIRCACHE
    return get_dir_fatent();
#else
    return NULL;
#endif
}

#endif /* _DIRCACHE_REDIRECT_H_ */
