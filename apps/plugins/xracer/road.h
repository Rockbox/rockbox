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

#ifndef _ROAD_H_
#define _ROAD_H_

struct point_2d;
struct sprite_t;

struct road_segment {
    int idx;
    int p1_z;
    int p2_z;
    int p1_y;
    int p2_y;
    struct point_2d p1;
    struct point_2d p2;
    float p1_scale; /* slow... */
    long curve; /* fixed-point with FRACBITS fractional bits */
    unsigned color;
    unsigned border_color;
    int lanes;
    unsigned lane_color;
    unsigned grass_color;
    int clip;
    /* linked list of sprites */
    struct sprite_t *sprites;
};

#endif
