/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 William Wilgus
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
#include "icon_helper.h"

const unsigned char* cbmp_get_icon(unsigned int cbmp_fmt, unsigned int index, int *width, int *height)
{
    const unsigned char* bmp = NULL;
    while (cbmp_fmt < CBMP_BitmapFormatLast)
    {
        const struct cbmp_bitmap_info_entry *cbmp = &rb->core_bitmaps[cbmp_fmt];
        if (index > cbmp->count)
            break;
        int w = cbmp->width;
        int h = cbmp->height;
        /* ((height/CHAR_BIT) Should always be 1 thus far */

        off_t offset = (((unsigned)h/CHAR_BIT) * (index * w));
        bmp = cbmp->pbmp + offset;

        if (width)
            *width = w;
        if (height)
            *height = h;
        break;
    }

    return bmp;
}
