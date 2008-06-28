/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Michiel van der Kolk 
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
#ifndef SEARCHENGINE_H
#define SEARCHENGINE_H
#include <plugin.h>
#include <database.h>
#include <autoconf.h>

extern int w, h, y;
#ifdef HAVE_LCD_BITMAP
#define PUTS(str) do { \
      rb->lcd_putsxy(1, y, str); \
      rb->lcd_getstringsize(str, &w, &h); \
      y += h + 1; \
} while (0); \
rb->lcd_update()
#else
#define PUTS(str) do { \
      rb->lcd_puts(0, y, str); \
      y = (y + 1) % 2; \
} while (0); \
rb->lcd_update()
#endif

extern const struct plugin_api* rb;

void *my_malloc(size_t size);
void setmallocpos(void *pointer);

#endif
