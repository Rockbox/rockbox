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
* Header for the in-memory JPEG decoder.
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
#include "resize.h"
#include "bmp.h"
#include "jpeg_common.h"

#ifndef _JPEG_MEM_H
#define _JPEG_MEM_H

int get_jpeg_dim_mem(unsigned char *data, unsigned long len,
                     struct dim *size);

int decode_jpeg_mem(unsigned char *data, unsigned long len,
                    struct bitmap *bm,
                    int maxsize,
                    int format,
                    const struct custom_format *cformat);

#endif /* _JPEG_MEM_H */
