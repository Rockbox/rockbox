/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dave Chapman
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

#include <stdio.h>
#include "config.h"
#include "lcd.h"
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#include "backdrop.h"

bool backdrop_load(const char* filename, char *backdrop_buffer)
{
    struct bitmap bm;
    int ret;

    /* load the image */
    bm.data = backdrop_buffer;
    ret = read_bmp_file(filename, &bm, LCD_BACKDROP_BYTES,
                        FORMAT_NATIVE | FORMAT_DITHER, NULL);

    return ((ret > 0)
            && (bm.width == LCD_WIDTH) && (bm.height == LCD_HEIGHT));
}
  
  
void backdrop_show(char *backdrop_buffer)
{
    lcd_set_backdrop((fb_data*)backdrop_buffer);
}
  

#if defined(HAVE_REMOTE_LCD)

#if LCD_REMOTE_DEPTH > 1
/* api functions */
bool remote_backdrop_load(const char *filename, char* backdrop_buffer)
{
    struct bitmap bm;
    int ret;

    /* load the image */
    bm.data = backdrop_buffer;
    ret = read_bmp_file(filename, &bm, REMOTE_LCD_BACKDROP_BYTES,
                        FORMAT_NATIVE | FORMAT_DITHER | FORMAT_REMOTE, NULL);
    return ((ret > 0)
            && (bm.width == REMOTE_LCD_WIDTH) && (bm.height == REMOTE_LCD_HEIGHT));
}
#else /* needs stubs */

bool remote_backdrop_load(const char *filename, char* backdrop_buffer)
{
    (void)filename; (void) backdrop_buffer;
    return false;
}
void remote_backdrop_show(char* backdrop_buffer)
{
    (void)backdrop_buffer;
}
#endif
#endif
