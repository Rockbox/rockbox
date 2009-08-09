/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2009 by Andrew Mahone
*
* This is a wrapper for the core jpeg_load.c
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

#include <plugin.h>
#include "feature_wrappers.h"
#include "read_image.h"

int read_image_file(const char* filename, struct bitmap *bm, int maxsize,
                    int format, const struct custom_format *cformat)
{
    int namelen = rb->strlen(filename);
    if (rb->strcmp(filename + namelen - 4, ".bmp"))
        return read_jpeg_file(filename, bm, maxsize, format, cformat);
    else
        return scaled_read_bmp_file(filename, bm, maxsize, format, cformat);
}

