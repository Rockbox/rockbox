/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 Franklin Wei
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#ifndef _SPRITE_H_
#define _SPRITE_H_

struct sprite_data_t {
    int x, y;
    int w, h;
};

struct sprite_data_t sprites[1];

/* the sprite_t object/struct/whatever should be as lightweight as possible */
/* here it consists of just a pointer to the data */

struct sprite_t {
    struct sprite_t *next;
    struct sprite_data_t *data;
    /* offset ranges from -1 (FP) to 1 (FP) */
    long offset;
};

#endif
