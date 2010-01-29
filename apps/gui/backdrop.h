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

#ifndef _BACKDROP_H
#define _BACKDROP_H

#if LCD_DEPTH > 1 && !defined(__PCTOOL__)

#include "lcd.h"
#include "bmp.h"

#define LCD_BACKDROP_BYTES (LCD_FBHEIGHT*LCD_FBWIDTH*sizeof(fb_data))
bool backdrop_load(const char *filename, char* backdrop_buffer);
void backdrop_show(char* backdrop_buffer);

#else
#define LCD_BACKDROP_BYTES 0
#endif

#if defined(HAVE_REMOTE_LCD) 
bool remote_backdrop_load(const char *filename, char* backdrop_buffer);
void remote_backdrop_show(char* backdrop_buffer);

#if LCD_DEPTH > 1 && LCD_REMOTE_DEPTH > 1 && !defined(__PCTOOL__)
#define REMOTE_LCD_BACKDROP_BYTES \
            (LCD_REMOTE_FBHEIGHT*LCD_REMOTE_FBWIDTH*sizeof(fb_remote_data))
#else
#define REMOTE_LCD_BACKDROP_BYTES 0
#endif

#endif

#endif /* _BACKDROP_H */
