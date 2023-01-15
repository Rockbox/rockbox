/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2023 Christian Soffke
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
#include "plugin.h"

bool retrieve_id3(struct mp3entry *id3, const char* file, bool try_tagcache)
{
    bool ret = false;
    int fd;

#if defined (HAVE_TAGCACHE) && defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
    if (try_tagcache && rb->tagcache_fill_tags(id3, file))
    {
        rb->strlcpy(id3->path, file, sizeof(id3->path));
        return true;
    }
#else
    (void) try_tagcache;
#endif

    fd = rb->open(file, O_RDONLY);
    if (fd >= 0)
    {
        if (rb->get_metadata(id3, fd, file))
            ret = true;
        rb->close(fd);
    }
    return ret;
}
