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

#if LCD_DEPTH > 1

#include "lcd.h"
#include "bmp.h"

bool load_main_backdrop(const char* filename);
bool load_wps_backdrop(const char* filename);

void unload_main_backdrop(void);
void unload_wps_backdrop(void);

void show_main_backdrop(void);
void show_wps_backdrop(void);

#endif /* LCD_DEPTH > 1 */

#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
bool load_remote_wps_backdrop(const char* filename);
void unload_remote_wps_backdrop(void);
void show_remote_wps_backdrop(void);
void show_remote_main_backdrop(void); /* only clears the wps backdrop */
#endif

#endif /* _BACKDROP_H */
