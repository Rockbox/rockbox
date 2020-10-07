/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Additional LCD routines not present in the rockbox core
* Core functions
*
* Copyright (C) 2005 Jens Arnold
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

#include "xlcd.h"

fb_data* get_framebuffer(struct viewport *vp, size_t *stride)
{
    struct viewport *vp_main = *(rb->screens[SCREEN_MAIN]->current_viewport);
    if (vp)
        *vp = *vp_main;
    if (stride)
        *stride = vp_main->buffer->stride;
    return vp_main->buffer->fb_ptr;
}
