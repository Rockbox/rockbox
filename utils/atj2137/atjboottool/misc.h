/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2010 Amaury Pouly
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
#ifndef __MISC_H__
#define __MISC_H__

#include <stdbool.h>
#include <stdio.h>

#define cprintf(col, ...) do {color(col); printf(__VA_ARGS__); }while(0)

#define cprintf_field(str1, ...) do{ cprintf(GREEN, str1); cprintf(YELLOW, __VA_ARGS__); }while(0)

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

typedef char color_t[];

extern color_t OFF, GREY, RED, GREEN, YELLOW, BLUE;
void *xmalloc(size_t s);
void color(color_t c);
void enable_color(bool enable);

#endif /* __MISC_H__ */
