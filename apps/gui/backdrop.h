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

enum backdrop_type {
    BACKDROP_MAIN,
    BACKDROP_SKIN_WPS,
};

#if LCD_DEPTH > 1 && !defined(__PCTOOL__)

#include "lcd.h"
#include "bmp.h"

bool backdrop_load(enum backdrop_type bdrop, const char*);
void backdrop_unload(enum backdrop_type bdrop);
void backdrop_show(enum backdrop_type bdrop);
void backdrop_hide(void);

#endif

#if defined(HAVE_REMOTE_LCD)
/* no main backdrop, stubs! */
#if LCD_REMOTE_DEPTH > 1
bool remote_backdrop_load(enum backdrop_type bdrop,const char* filename);
void remote_backdrop_unload(enum backdrop_type bdrop);
void remote_backdrop_show(enum backdrop_type bdrop);
void remote_backdrop_hide(void);
#endif
#endif

#endif /* _BACKDROP_H */
