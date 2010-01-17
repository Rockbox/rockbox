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
#ifndef _FRACTAL_RECT_H
#define _FRACTAL_RECT_H

#include "fractal_sets.h"

void rects_queue_init(void);
void rects_calc_all(int (*calc)(struct fractal_rect *, int (*)(void *), void *),
        int (*button_yield_cb)(void *), void *ctx);
int rects_move_up(void);
int rects_move_down(void);
int rects_move_left(void);
int rects_move_right(void);

#endif

