/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __FT6x06_H__
#define __FT6x06_H__

#include "config.h"
#include <stdbool.h>

enum ft6x06_event {
    FT6x06_EVT_NONE = -1,
    FT6x06_EVT_PRESS = 0,
    FT6x06_EVT_RELEASE = 1,
    FT6x06_EVT_CONTACT = 2,
};

struct ft6x06_point {
    int event;
    int touch_id;
    int pos_x;
    int pos_y;
    int weight;
    int area;
};

struct ft6x06_state {
    int gesture;
    int nr_points;
    struct ft6x06_point points[FT6x06_NUM_POINTS];
};

extern struct ft6x06_state ft6x06_state;

typedef void(*ft6x06_event_cb)(struct ft6x06_state* state);

void ft6x06_init(void);
void ft6x06_set_event_cb(ft6x06_event_cb fn);
void ft6x06_enable(bool en);
void ft6x06_irq_handler(void);

#endif /* __FT6x06_H__ */
