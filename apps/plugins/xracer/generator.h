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

#ifndef _GENERATOR_H_
#define _GENERATOR_H_

#include <stdbool.h>

struct sprite_data_t;
struct road_segment;

void gen_reset(void);

void add_road(struct road_segment *road, unsigned int road_length, unsigned int idx, unsigned int len, long curve, int hill);

/* slope *must* be positive! for negative slopes, use add_downhill */
void add_uphill(struct road_segment *road, unsigned int road_length, unsigned int idx, int slope, int hold, long curve);

/* slope *must* be negative! for positive slopes, use add_uphill */
void add_downhill(struct road_segment *road, unsigned int road_length, unsigned int idx, int slope, int hold, long curve);

void generate_random_road(struct road_segment *road, unsigned int road_length, bool hills, bool curves);

/* appends a sprite to the end of the linked list of sprites */
/* data must be static! */
void add_sprite(struct road_segment *seg, struct sprite_data_t *data);

#endif
