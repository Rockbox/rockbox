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
#if !defined(__PCTOOL__) || defined(CHECKWPS)
/* load a remap file to allow buttons for actions to be remapped
 * entries consist of 3 int [action, button, prebtn]
 * context look up table is at the beginning
 * action_code contains the (context | CONTEXT_REMAPPED)
 * button_code contains the index of the first remapped action for the matched context
 * [0] CORE_CONTEXT_REMAP(ctx1) offset1=(3)
 * [1] CORE_CONTEXT_REMAP(ctx2, offset2=(5)
 * [2] sentinel, 0, 0
 * [3] act0, btn, 0
 * [4] sentinel 0, 0
 * [5] act1, btn, 0
 * [6] sentinel, 0, 0
 *
 * Note:
 * last entry of each group is always the sentinel [CONTEXT_STOPSEARCHING, BUTTON_NONE, BUTTON_NONE]
 * contexts must match exactly -- re-mapped contexts run before the built in w/ fall through contexts
 * ie. you can't remap std_context and expect it to match std_context actions from the WPS context.
 */

int core_load_key_remap(const char *filename)
{
    /* dummy ops with no callbacks, needed because by
     * default buflib buffers can be moved around which must be avoided */
    static struct buflib_callbacks dummy_ops;
    static int keymap_handle = -1;
    int count = 0;
    int fd;
    char *buf;
    size_t fsize;
    if (keymap_handle > 0) /* free old buffer */
    {
        action_set_keymap(NULL, -1);
        keymap_handle = core_free(keymap_handle);
    }
    while (filename)
    {
        fd = open(filename, O_RDONLY);
        if (fd)
        {
            fsize = filesize(fd);
            count = fsize / sizeof(struct button_mapping);
            if (count * sizeof(struct button_mapping) != fsize)
            {
                count = -10;
                break;
            }
            keymap_handle = core_alloc_ex("key remap", fsize, &dummy_ops);
            if (keymap_handle <= 0)
            {
                count = -20;
                break;
            }
            buf = core_get_data(keymap_handle);
            if (read(fd, buf, fsize) == (ssize_t) fsize)
            {
                count = action_set_keymap((struct button_mapping *) buf, count);
            }
            else
                count = -30;

            close(fd);
        }
        break;
    }
    return count;
}
#endif /* !defined(__PCTOOL__) */
