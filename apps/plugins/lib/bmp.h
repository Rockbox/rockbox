/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Antoine Cellerier <dionoea -at- videolan -dot- org>
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
#ifndef _LIB_BMP_H_
#define _LIB_BMP_H_

#include "lcd.h"
#include "plugin.h"

#ifdef HAVE_LCD_COLOR
/**
 * Save bitmap to file
 */
int save_bmp_file( char* filename, struct bitmap *bm, const struct plugin_api* rb  );
#endif

/**
   Very simple image scale from src to dst (nearest neighbour).
   Source and destination dimensions are read from the struct bitmap.
*/
void simple_resize_bitmap(struct bitmap *src, struct bitmap *dst);

/**
   Advanced image scale from src to dst (bilinear) based on imlib2.
   Source and destination dimensions are read from the struct bitmap.
 */
void smooth_resize_bitmap(struct bitmap *src,  struct bitmap *dst);

#endif
