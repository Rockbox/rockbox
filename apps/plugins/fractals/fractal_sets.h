/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Tomer Shalev
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
#ifndef _FRACTAL_SETS_H
#define _FRACTAL_SETS_H

#include "lib/grey.h"
#include "lib/xlcd.h"

#define DELTA 8 /* Panning moves 1/DELTA of screen */

#define LCD_SHIFT_X (LCD_WIDTH / DELTA)
#define LCD_SHIFT_Y (LCD_HEIGHT / DELTA)

#define X_DELTA(x) (((x) * LCD_WIDTH) / DELTA)
#define Y_DELTA(y) (((y) * LCD_HEIGHT) / DELTA)

struct fractal_rect
{
    int px_min;
    int py_min;
    int px_max;
    int py_max;
    int valid;
};

struct fractal_ops
{
    void (*init)(void);
    int (*calc)(struct fractal_rect *rect, int (*button_yield_cb)(void *ctx),
            void *button_yield_ctx);
    void (*move)(int dx, int dy);
    int (*zoom)(int factor);
    int (*precision)(int d);
};

#endif
