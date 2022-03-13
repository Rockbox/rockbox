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

#include "rbpaths.h"
#include "pathfuncs.h"
#include "dir.h"
#include "dircache.h"

#if defined(HAVE_MULTIBOOT) && !defined(SIMULATOR) && !defined(BOOTLOADER)
#include "rb-loader.h"
#include "multiboot.h"
#include "bootdata.h"
#include "crc32.h"
#endif

#ifndef RB_ROOT_VOL_HIDDEN
#define RB_ROOT_VOL_HIDDEN(v)   (0 == 0)
#endif
#ifndef RB_ROOT_CONTENTS_DIR
#define RB_ROOT_CONTENTS_DIR    "/"
#endif
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
#if defined(HAVE_MULTIBOOT) && !defined(SIMULATOR) && !defined(BOOTLOADER)
    char path[VOL_MAX_LEN+2];
    char rtpath[MAX_PATH / 2];
    make_volume_root(volume, path);

    unsigned int crc = crc_32(boot_data.payload, boot_data.length, 0xffffffff);
    if (crc > 0 && crc == boot_data.crc)
    {
        /* we need to mount the drive before we can access it */
        root_mount_path(path, 0); /* root could be different folder don't hide */

        if (volume == boot_data.boot_volume) /* boot volume contained in uint8_t payload */
        {
            int rtlen = get_redirect_dir(rtpath, sizeof(rtpath), volume, "", "");
            while (rtlen > 0 && rtpath[--rtlen] == PATH_SEPCH)
                rtpath[rtlen] = '\0'; /* remove extra separators */

#if 0 /*removed, causes issues with playback for now?*/
            if (rtlen <= 0 || rtpath[rtlen] == VOL_END_TOK)
                root_unmount_volume(volume); /* unmount so root can be hidden*/
#endif
            if (rtlen <= 0) /* Error occurred, card removed? Set root to default */
            {
                root_unmount_volume(volume); /* unmount so root can be hidden*/
                goto standard_redirect;
            }
            root_mount_path(rtpath, NSITEM_CONTENTS);
        }

    } /*CRC OK*/
    else
    {
standard_redirect:
        root_mount_path(path, RB_ROOT_VOL_HIDDEN(volume) ? NSITEM_HIDDEN : 0);
        if (volume == path_strip_volume(RB_ROOT_CONTENTS_DIR, NULL, false))
            root_mount_path(RB_ROOT_CONTENTS_DIR, NSITEM_CONTENTS);
    }
#elif defined(HAVE_MULTIVOLUME)
    char path[VOL_MAX_LEN+2];
    make_volume_root(volume, path);
    root_mount_path(path, RB_ROOT_VOL_HIDDEN(volume) ? NSITEM_HIDDEN : 0);
    if (volume == path_strip_volume(RB_ROOT_CONTENTS_DIR, NULL, false))
        root_mount_path(RB_ROOT_CONTENTS_DIR, NSITEM_CONTENTS);
#else
    const char *path = PATH_ROOTSTR;
    root_mount_path(path, RB_ROOT_VOL_HIDDEN(volume) ? NSITEM_HIDDEN : 0);
    root_mount_path(RB_ROOT_CONTENTS_DIR, NSITEM_CONTENTS);
#endif /* HAVE_MULTIBOOT */

#ifdef HAVE_DIRCACHE
    dircache_mount();
#endif
}

static inline void volume_onunmount_internal(IF_MV_NONVOID(int volume))
{
#ifdef HAVE_DIRCACHE
    /* First, to avoid update of something about to be destroyed anyway */
    dircache_unmount(IF_MV(volume));
#endif
    root_unmount_volume(IF_MV(volume));
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
                                 struct DIRENT *entry)
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
