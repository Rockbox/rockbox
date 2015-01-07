/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 Franklin Wei
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include "plugin.h"
#include "util.h"

void *util_alloc(size_t sz)
{
    static void *plugin_buffer = NULL;
    static size_t bytes_left   = -1;
    if(!plugin_buffer)
    {
        plugin_buffer = rb->plugin_get_buffer(&bytes_left);
    }
    void *ret = plugin_buffer;
    plugin_buffer += sz;
    if((signed long) bytes_left - sz < 0)
    {
        LOGF("%s: OOM", __func__);
        return NULL;
    }
    bytes_left -= sz;
    return ret;
}
