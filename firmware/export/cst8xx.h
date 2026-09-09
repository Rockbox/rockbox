/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Firmware   |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Jukebox    |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

#ifndef __CST8XX_H__
#define __CST8XX_H__

#include "config.h"
#include <stdbool.h>

/* The controller's own event codes, from bits 7:6 of the X-high byte. There is
 * no NONE: the driver parks at UP, which is what an idle panel reports. */
enum cst8xx_event {
    CST8XX_EVT_DOWN    = 0,
    CST8XX_EVT_UP      = 1,
    CST8XX_EVT_CONTACT = 2,
};

struct cst8xx_point {
    int event;
    int pos_x;
    int pos_y;
};

struct cst8xx_state {
    int nr_points;
    struct cst8xx_point points[CST8XX_NUM_POINTS];
};

extern struct cst8xx_state cst8xx_state;

void cst8xx_init(void);
void cst8xx_enable(bool en);
void cst8xx_irq_handler(void);

#endif /* __CST8XX_H__ */
