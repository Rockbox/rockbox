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

#include "dircache.h"

/***
 ** Internal redirects that depend upon whether or not dircache is made
 **
 ** Some stuff deals with it, some doesn't right now. This is a nexus point..
 **/

/** File binding **/

static inline void fileobj_bind_file(struct fileinfo *infop)
{
#ifdef HAVE_DIRCACHE
    dircache_bind_file(infop);
#else
    file_binding_insert_last(infop);
#endif
}

static inline void fileobj_unbind_file(struct fileinfo *infop)
{
#ifdef HAVE_DIRCACHE
    dircache_unbind_file(infop);
#else
    file_binding_remove(infop);
#endif
}

/** File ops that combine with dircache **/
static inline enum cmpfi_result
    compare_fileinfo_internal(const struct fileinfo *info1,
                              const struct fileinfo *info2)
{
    return __FILEOP(compare_fileinfo)(info1, info2);
}

static inline int get_fileentry_internal(struct fileinfo *dirinfo,
                                         const char *name,
                                         size_t length,
                                         unsigned int callflags,
                                         struct fileentry *entry)
{
#ifdef HAVE_DIRCACHE
    return dircache_get_fileentry(dirinfo, name, length, callflags, entry);
#else
    return __FILEOP(get_fileentry)(dirinfo, name, length, entry);
    (void)callflags;
#endif
}

static inline int fileop_open_volume_fileinfo(struct bpb *bpb,
                                              struct fileinfo *infop)
{
    int rc = __FILEOP(open_volume)(bpb, infop);
#ifdef HAVE_DIRCACHE
    if (rc >= 0)
        dircache_get_rootinfo(bpb, infop);
#endif
    return rc;
}

static inline int fileop_open_fileinfo(struct fileinfo *dirinfop,
                                       struct fileentry *entry,
                                       struct fileinfo *infop)
{
    int rc = __FILEOP(open)(dirinfop, entry, infop);
#ifdef HAVE_DIRCACHE
    if (rc >= 0)
        dircache_get_entryinfo(entry, infop);
#endif
    return rc;
}

/** Fileop event handlers **/
static inline void fileop_onopen(struct fileinfo *srcinfop,
                                 unsigned int callflags,
                                 struct fileinfo **infopp)
{
    fileobj_fileop_open(srcinfop, callflags, infopp);
}

static inline void fileop_oncreate(struct fileinfo *dirinfop,
                                   struct fileentry *entry)
{
#ifdef HAVE_DIRCACHE
    dircache_fileop_create(dirinfop, entry);
#endif
    (void)dirinfop; (void)entry;
}

static inline void fileop_onclose(struct fileinfo *infop)
{
    fileobj_fileop_close(infop);
}

static inline void fileop_onopen_filestr(unsigned int callflags,
                                         struct filestr *str)
{
    fileobj_fileop_open_filestr(callflags, str);
}

static inline void fileop_onclose_filestr(struct filestr *str)
{
    fileobj_fileop_close_filestr(str);
}

static inline void fileop_onsync_filestr(struct fileinfo *infop,
                                         struct fileentry *entry)
{
#ifdef HAVE_DIRCACHE
    dircache_fileop_sync(infop, entry);
#endif
    (void)infop; (void)entry;
}

static inline void fileop_onremove(struct fileinfo *infop)
{
#ifdef HAVE_DIRCACHE
    dircache_fileop_remove(infop);
#endif
    (void)infop;
}

static inline void fileop_onrename(struct fileinfo *dirinfop,
                                   struct fileinfo *infop,
                                   struct fileentry *entry)
{
#ifdef HAVE_DIRCACHE
    dircache_fileop_rename(dirinfop, infop, entry);
#endif
    (void)dirinfop; (void)infop; (void)entry;
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


/** Directory reading **/

static inline int readdir_internal(struct filestr *dirstr,
                                   struct dirscan *scanp,
                                   struct dirent *entry)
{
#ifdef HAVE_DIRCACHE
    return dircache_readdir(dirstr, scanp, entry);
#else
    return uncached_readdir_internal(dirstr, scanp, entry);
#endif
}

static inline void rewinddir_internal(struct filestr *dirstr,
                                      struct dirscan *scanp)
{
#ifdef HAVE_DIRCACHE
    dircache_rewinddir(dirstr, scanp);
#else
    __FILEOP(rewinddir)(dirstr, scanp);
#endif
}

#endif /* _DIRCACHE_REDIRECT_H_ */
