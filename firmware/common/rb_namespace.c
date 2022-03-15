/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 by Michael Sevakis
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
#include "fileobj_mgr.h"
#include "rb_namespace.h"
#include "file_internal.h"

/* Define LOGF_ENABLE to enable logf output in this file */
//#define LOGF_ENABLE
#include "logf.h"

#if !defined(HAVE_MULTIVOLUME) && defined(LOGF_ENABLE)
    int volume = 0;
#endif


#define ROOT_CONTENTS_INDEX  (NUM_VOLUMES)
#define NUM_ROOT_ITEMS   (NUM_VOLUMES+1)

static uint8_t root_entry_flags[NUM_VOLUMES+1];
static struct file_base_binding *root_bindp;

static inline unsigned int get_root_item_state(int item)
{
    return root_entry_flags[item];
}

static inline void set_root_item_state(int item, unsigned int state)
{
    root_entry_flags[item] = state;
}

static void get_mount_point_entry(IF_MV(int volume,) struct DIRENT *entry)
{
#ifdef HAVE_MULTIVOLUME
    get_volume_name(volume, entry->d_name);
#else /* */
    strcpy(entry->d_name, PATH_ROOTSTR);
#endif /* HAVE_MULTIVOLUME */
#if defined(_FILESYSTEM_NATIVE_H_)
    entry->info.attr    = ATTR_MOUNT_POINT;
    entry->info.size    = 0;
    entry->info.wrtdate = 0;
    entry->info.wrttime = 0;
#endif /* is dirinfo_native */
    logf("%s: vol:%d, %s", __func__, volume, entry->d_name);
}

/* unmount the directory that enumerates into the root namespace */
static void unmount_item(int item)
{
    unsigned int state = get_root_item_state(item);
    logf("%s: state: %u", __func__, state);
    if (!state)
        return;

    if (state & NSITEM_CONTENTS)
    {
        fileobj_unmount(root_bindp);
        root_bindp = NULL;
    }

    set_root_item_state(item, 0);
}

/* mount the directory that enumerates into the root namespace */
int root_mount_path(const char *path, unsigned int flags)
{
#ifdef HAVE_MULTIVOLUME
    int volume = path_strip_volume(path, NULL, false);
    if (volume == ROOT_VOLUME)
        return -EINVAL;

    if (!CHECK_VOL(volume))
        return -ENOENT;
#else
    if (!path_is_absolute(path))
    {
        logf("Path not absolute %s", path);
        return -ENOENT;
    }
#endif /* HAVE_MULTIVOLUME */

    bool contents = flags & NSITEM_CONTENTS;
    int item = contents ? ROOT_CONTENTS_INDEX : IF_MV_VOL(volume);
    unsigned int state = get_root_item_state(item);

    logf("%s: item:%d, st:%u, %s", __func__, item, state, path);

    if (state)
        return -EBUSY;

    if (contents)
    {
        /* cache information about the target */
        struct filestr_base stream;
        struct path_component_info compinfo;

        int e = errno;
        int rc = open_stream_internal(path, FF_DIR | FF_PROBE | FF_INFO |
                                      FF_DEVPATH, &stream, &compinfo);
        if (rc <= 0)
        {
            rc = rc ? -errno : -ENOENT;
            errno = e;
            return rc;
        }

        if (!fileobj_mount(&compinfo.info, FO_DIRECTORY, &root_bindp))
            return -EBUSY;
    }

    state = NSITEM_MOUNTED | (flags & (NSITEM_HIDDEN|NSITEM_CONTENTS));
    set_root_item_state(item, state);

    return 0;
}

/* inform root that an entire volume is being unmounted */
void root_unmount_volume(IF_MV_NONVOID(int volume))
{
    logf("%s: vol: %d", __func__, volume);
    FOR_EACH_VOLUME(volume, item)
    {
    #ifdef HAVE_MULTIVOLUME
        uint32_t state = get_root_item_state(item);
        if (state && (volume < 0 || item == volume))
    #endif /* HAVE_MULTIVOLUME */
            unmount_item(item);
    }

    /* if the volume unmounted contains the root directory contents then
       the contents must also be unmounted */
#ifdef HAVE_MULTIVOLUME
    uint32_t state = get_root_item_state(ROOT_CONTENTS_INDEX);
    if (state && (volume < 0 || BASEINFO_VOL(&root_bindp->info) == volume))
#endif
        unmount_item(ROOT_CONTENTS_INDEX);
}

/* parse the root part of a path */
int ns_parse_root(const char *path, const char **pathp, uint16_t *lenp)
{
    logf("%s: path: %s", __func__, path);
    int volume = ROOT_VOLUME;

#ifdef HAVE_MULTIVOLUME
    /* this seamlessly integrates secondary filesystems into the
       root namespace (e.g. "/<0>/../../<1>/../foo/." :<=> "/foo") */
    const char *p;
    volume = path_strip_volume(path, &p, false);
    if (volume != ROOT_VOLUME && !CHECK_VOL(volume))
    {
        logf("vol: %d is not root", volume);
        return -ENOENT;
    }
#endif /* HAVE_MULTIVOLUME */

    /* set name to start at last leading separator; name of root will
     * be returned as "/", volume specifiers as "/<fooN>" */
    *pathp = GOBBLE_PATH_SEPCH(path) - 1;
    *lenp  = IF_MV( volume < NUM_VOLUMES ? p - *pathp : ) 1;
#ifdef LOGF_ENABLE
    if (volume == INT_MAX)
        logf("vol: ROOT(%d) %s", volume, *pathp);
    else
        logf("vol: %d %s", volume, *pathp);
#endif
#ifdef HAVE_MULTIVOLUME
    if (*lenp > MAX_COMPNAME+1)
    {
        logf("%s: path too long %s", __func__, path);
        return -ENAMETOOLONG;
    }
#endif
#ifdef LOGF_ENABLE
    if (volume == INT_MAX)
        logf("%s: vol: ROOT(%d) path: %s", __func__, volume, path);
    else
        logf("%s: vol: %d path: %s", __func__, volume, path);
#endif
    return volume;
}

/* open one of the items in the root */
int ns_open_root(IF_MV(int volume,) unsigned int *callflagsp,
                 struct file_base_info *infop, uint16_t *attrp)
{
    unsigned int callflags = *callflagsp;
    bool devpath = !!(callflags & FF_DEVPATH);
#ifdef HAVE_MULTIVOLUME
    bool sysroot = volume == ROOT_VOLUME;
    if (devpath && sysroot)
        return -ENOENT;         /* devpath needs volume spec */
#else
    bool sysroot = !devpath;    /* always sysroot unless devpath */
#endif

    int item = sysroot ? ROOT_CONTENTS_INDEX : IF_MV_VOL(volume);
    unsigned int state = get_root_item_state(item);
    logf("%s: Vol:%d St:%d", __func__, item, state);
    if (sysroot)
    {
        *attrp = ATTR_SYSTEM_ROOT;

        if (state)
            *infop = root_bindp->info;
        else
        {
            logf("%s: SysRoot Vol:%d St:%d NOT mounted", __func__, item, state);
            *callflagsp = callflags | FF_NOFS;  /* contents not mounted */
        }
    }
    else
    {
        *attrp = ATTR_MOUNT_POINT;

        if (!devpath && !state)
            return -ENOENT; /* regular open requires having been mounted */
#if CONFIG_PLATFORM & PLATFORM_NATIVE
        if (fat_open_rootdir(IF_MV(volume,) &infop->fatfile) < 0)
        {
            logf("%s: DevPath Vol:%d St:%d NOT mounted", __func__, item, state);
            return -ENOENT; /* not mounted */
        }
#endif
        get_rootinfo_internal(infop);
    }

    return 0;
}

/* read root directory entries */
int root_readdir_dirent(struct filestr_base *stream,
                        struct ns_scan_info *scanp, struct DIRENT *entry)
{
    int rc = 0;

    int item = scanp->item;
    logf("%s: item: %d", __func__, item);

    /* skip any not-mounted or hidden items */
    unsigned int state;
    while (1)
    {
        if (item >= NUM_ROOT_ITEMS)
            goto file_eod;

        state = get_root_item_state(item);
        if ((state & (NSITEM_MOUNTED|NSITEM_HIDDEN)) == NSITEM_MOUNTED)
        {
            logf("Found mounted item: %d %s", item, entry->d_name);
            break;
        }

        item++;
    }

    if (item == ROOT_CONTENTS_INDEX)
    {
        rc = readdir_dirent(stream, &scanp->scan, entry);
        if (rc < 0)
            FILE_ERROR(ERRNO, rc * 10 - 1);

        if (rc == 0)
        {
            logf("Found root item: %d %s", item, entry->d_name);
            item++;
        }
    }
    else
    {
        get_mount_point_entry(IF_MV(item,) entry);
        item++;
        rc = 1;
        logf("Found mp item:%d %s", item,  entry->d_name);
    }

    scanp->item = item;

file_eod:
#ifdef HAVE_DIRCACHE
    if (rc == 0)
        empty_dirent(entry);
#endif
file_error:
    logf("%s: status: %d", __func__, rc);
    return rc;
}

/* opens a stream to enumerate items in a namespace container */
int ns_open_stream(const char *path, unsigned int callflags,
                   struct filestr_base *stream, struct ns_scan_info *scanp)
{
    logf("%s: path: %s", __func__, path);
    /* stream still needs synchronization even if we don't have a stream */
    static struct mutex no_contents_mtx SHAREDBSS_ATTR;

    int rc = open_stream_internal(path, callflags, stream, NULL);
    if (rc < 0)
        FILE_ERROR(ERRNO, rc * 10 - 1);

    scanp->item = rc > 1 ? 0 : -1;

    if (stream->flags & FDO_BUSY)
    {
        /* root contents are mounted */
        fat_rewind(&stream->fatstr);
    }
    else
    {
        /* root contents not mounted */
        mutex_init(&no_contents_mtx);
        stream->mtx = &no_contents_mtx;
    }

    ns_dirscan_rewind(scanp);

    rc = 0;
file_error:
    return rc;
}
