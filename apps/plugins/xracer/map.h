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

#ifndef _MAP_H_
#define _MAP_H_

#include <stdint.h>

#define MAP_VERSION 0x0000

struct road_segment;

struct road_section {
    unsigned char type;
    uint32_t len;
    int32_t  slope;

    /* FP, 8 fracbits */
    long     curve;
};

/* takes a map in internal format and converts it to road_segment's */

unsigned int load_map(struct road_segment*, unsigned max_road_length, struct road_section*, unsigned map_len);

#endif
