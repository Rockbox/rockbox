/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2020 by William Wilgus
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

#include "action.h"
#include "core_alloc.h"
#include "core_keymap.h"

/*#define LOGF_ENABLE*/
#include "logf.h"

#if !defined(__PCTOOL__) || defined(CHECKWPS)
int core_set_keyremap(struct button_mapping* core_keymap, int count)
{
    return action_set_keymap(core_keymap, count);
}

static int open_key_remap(const char *filename, int *countp)
{
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
        return fd;

    size_t fsize = filesize(fd);
    int count = fsize / sizeof(struct button_mapping);
    if (count == 0 || (size_t)(count * sizeof(struct button_mapping)) != fsize)
    {
        logf("core_keyremap: bad filesize %d / %lu", count, (unsigned long)fsize);
        goto error;
    }

    struct button_mapping header;
    if(read(fd, &header, sizeof(header)) != (ssize_t)sizeof(header))
    {
        logf("core_keyremap: read error");
        goto error;
    }

    if (header.action_code != KEYREMAP_VERSION ||
        header.button_code != KEYREMAP_HEADERID ||
        header.pre_button_code != count)
    {
        logf("core_keyremap: bad header %d", count);
        goto error;
    }

    *countp = count - 1;
    return fd;

  error:
    close(fd);
    return -1;
}

int INIT_ATTR core_load_key_remap(const char *filename)
{
    int count = 0; /* gcc falsely believes this may be used uninitialized */
    int fd = open_key_remap(filename, &count);
    if (fd < 0)
        return -1;

    size_t bufsize = count * sizeof(struct button_mapping);
    int handle = core_alloc(bufsize);
    if (handle > 0)
    {
        void *data = core_get_data_pinned(handle);

        if (read(fd, data, bufsize) == (ssize_t)bufsize)
            count = action_set_keymap_handle(handle, count);

        core_put_data_pinned(data);
    }

    close(fd);
    return count;
}

#endif /* !defined(__PCTOOL__) */
