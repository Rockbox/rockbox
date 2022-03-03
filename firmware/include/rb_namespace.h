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
#ifndef RB_NAMESPACE_H
#define RB_NAMESPACE_H

#include "file_internal.h"

enum ns_item_flags
{
    NSITEM_MOUNTED   = 0x01, /* item is mounted */
    NSITEM_HIDDEN    = 0x02, /* item is not enumerated */
    NSITEM_CONTENTS  = 0x04, /* contents enumerate */
};

struct ns_scan_info
{
    struct dirscan_info scan; /* dirscan info - first! */
    int                 item; /* current item in parent */
};

/* root functions */
int root_mount_path(const char *path, unsigned int flags);
void root_unmount_volume(IF_MV_NONVOID(int volume));
int root_readdir_dirent(struct filestr_base *stream,
                        struct ns_scan_info *scanp,
                        struct DIRENT *entry);

/* namespace functions */
int ns_parse_root(const char *path, const char **pathp, uint16_t *lenp);
int ns_open_root(IF_MV(int volume,) unsigned int *callflagsp,
                 struct file_base_info *infop, uint16_t *attrp);
int ns_open_stream(const char *path, unsigned int callflags,
                   struct filestr_base *stream, struct ns_scan_info *scanp);

/* closes the namespace stream */
static inline int ns_close_stream(struct filestr_base *stream)
{
    return close_stream_internal(stream);
}

#include "dircache_redirect.h"

static inline void ns_dirscan_rewind(struct ns_scan_info *scanp)
{
    rewinddir_dirent(&scanp->scan);
    if (scanp->item != -1)
        scanp->item = 0;
}

static inline int ns_readdir_dirent(struct filestr_base *stream,
                                    struct ns_scan_info *scanp,
                                    struct dirent *entry)

{
    if (scanp->item == -1)
        return readdir_dirent(stream, &scanp->scan, entry);
    else
        return root_readdir_dirent(stream, scanp, entry);
}

#endif /* RB_NAMESPACE_H */
