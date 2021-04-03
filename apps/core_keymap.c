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

#if !defined(__PCTOOL__) || defined(CHECKWPS)
int core_load_key_remap(const char *filename)
{
    static int keymap_handle = -1;
    char *buf;
    int fd = -1;
    int count = 0;
    size_t fsize = 0;
    if (keymap_handle > 0) /* free old buffer */
    {
        action_set_keymap(NULL, -1);
        keymap_handle = core_free(keymap_handle);
    }
    if (filename != NULL)
        count = open_key_remap(filename, &fd, &fsize);
    while (count > 0)
    {

        keymap_handle = core_alloc_ex("key remap", fsize, &buflib_ops_locked);
        if (keymap_handle <= 0)
        {
            count = -30;
            break;
        }
        buf = core_get_data(keymap_handle);
        if (read(fd, buf, fsize) == (ssize_t) fsize)
        {
            count = action_set_keymap((struct button_mapping *) buf, count);
        }
        else
            count = -40;
        break;
    }
    close(fd);
    return count;
}

int open_key_remap(const char *filename, int *fd, size_t *fsize)
{
    int count = 0;

    while (filename && fd && fsize)
    {
        *fsize = 0;
        *fd = open(filename, O_RDONLY);
        if (*fd)
        {
            *fsize = filesize(*fd);

            count = *fsize / sizeof(struct button_mapping);

            if (count * sizeof(struct button_mapping) != *fsize)
            {
                count = -10;
                break;
            }

            if (count > 1)
            {
                struct button_mapping header = {0};
                read(*fd, &header, sizeof(struct button_mapping));
                if (KEYREMAP_VERSION == header.action_code &&
                    KEYREMAP_HEADERID == header.button_code &&
                    header.pre_button_code == count)
                {
                    count--;
                    *fsize -= sizeof(struct button_mapping);
                }
                else /* Header mismatch */
                {
                    count = -20;
                    break;
                }
            }
        }
        break;
    }
    return count;
}

#endif /* !defined(__PCTOOL__) */
