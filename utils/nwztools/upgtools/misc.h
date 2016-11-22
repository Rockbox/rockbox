/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include <stddef.h>

#define _STR(a) #a
#define STR(a) _STR(a)

#define ROUND_UP(val, round) ((((val) + (round) - 1) / (round)) * (round))

typedef const char color_t[];

extern color_t OFF, GREY, RED, GREEN, YELLOW, BLUE;
void color(color_t c);
void enable_color(bool enable);

typedef void (*generic_printf_t)(void *u, bool err, color_t c, const char *f, ...);

void generic_std_printf(void *u, bool err, color_t c, const char *f, ...);

#endif /* __MISC_H__ */
