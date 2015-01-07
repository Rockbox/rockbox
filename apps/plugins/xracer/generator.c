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

#include "plugin.h"

#include "xracer.h"
#include "compat.h"
#include "generator.h"
#include "graphics.h"
#include "road.h"
#include "sprite.h"
#include "util.h"


static int last_y=0;

void gen_reset(void)
{
    last_y = 0;
}

/* add_road(): adds a section of road segments between s and s+len,
   each with the properties specified */
void add_road(struct road_segment *road, unsigned int road_length, unsigned int s, unsigned int len, long curve, int hill)
{
    unsigned int i;
    for(i = s; i < s+len && i < road_length; ++i)
    {
        struct road_segment tmp;
        tmp.idx  = i;
        tmp.p1_z = i * SEGMENT_LENGTH;
        tmp.p2_z = (i + 1) * SEGMENT_LENGTH;

        /* change the lines below for hills */
        tmp.p1_y = last_y;
        tmp.p2_y = last_y+hill;
        last_y += hill;

        /* change this line for curves */
        tmp.curve = curve;

        tmp.color = (i/RUMBLE_LENGTH)%2?ROAD_COLOR1:ROAD_COLOR2;
        tmp.border_color = (i/RUMBLE_LENGTH)%2?BORDER_COLOR1:BORDER_COLOR2;
        tmp.lanes = (i/RUMBLE_LENGTH)%2?LANES:1;
        tmp.lane_color = LANE_COLOR;
        tmp.grass_color = (i/RUMBLE_LENGTH)%2?GRASS_COLOR1:GRASS_COLOR2;
        tmp.clip = 0;
        tmp.sprites = NULL;
        road[i] = tmp;
    }
    if(i > road_length)
    {
        warning("road segment out of bounds (i=%d, len=%d)", i, road_length);
    }
}

/* enter_uphill(): adds a road section with slopes 0,1...slope-1 */
/* assumes slope is positive, for negative slopes, use *_downhill */
static inline void enter_uphill(struct road_segment *road, unsigned int road_length, unsigned int s, int slope, long curve)
{
    for(int i=0; i<slope; ++i)
    {
        add_road(road, road_length, s + i, 1, curve, i);
    }
}

/* exit_hill(): opposite of enter_hill, adds a road section with slopes slope-1...0 */
/* assumes slope is positive, for negative slopes, use *_downhill */
static inline void exit_uphill(struct road_segment *road, unsigned int road_length, unsigned int s, int slope, long curve)
{
    int n = slope;
    for(int i = 0; i<n; ++i, --slope)
    {
        add_road(road, road_length, s + i, 1, curve, slope);
    }
}

/* add_uphill(): currently unused */
/* assumes slope is positive, for negative slopes, use *_downhill */
void add_uphill(struct road_segment *road, unsigned int road_length, unsigned int s, int slope, int hold, long curve)
{
    enter_uphill(road, road_length, s, slope, curve);
    add_road(road, road_length, s+slope, hold, curve, slope);
    exit_uphill(road, road_length, s+slope+hold, slope, curve);
}

/* enter_downhill: adds a road section with slopes 0,-1,...slope+1 */
/* assumes slope is negative, for positive slopes use *_uphill */
static inline void enter_downhill(struct road_segment *road, unsigned int road_length, unsigned int s, int slope, long curve)
{
    int n = 0;
    for(int i=0; i<-slope;++i, --n)
    {
        add_road(road, road_length, s + i, 1, curve, n);
    }
}

/* adds a road section with slopes slope, slope+1,...,-1 */
static inline void exit_downhill(struct road_segment *road, unsigned int road_length, unsigned int s, int slope, long curve)
{
    int n = slope;
    for(int i=0; i<-slope; ++i, ++n)
    {
        add_road(road, road_length, s + i, 1, curve, n);
    }
}

void add_downhill(struct road_segment *road, unsigned int road_length, unsigned int s, int slope, int hold, long curve)
{
    enter_downhill(road, road_length, s, slope, curve);
    add_road(road, road_length, s-slope, hold, curve, slope);
    exit_downhill(road, road_length, s-slope+hold, slope, curve);
}

/* generate_random_road(): generates a random track road_length segments long */

void generate_random_road(struct road_segment *road, unsigned int road_length, bool hills, bool curves)
{
    gen_reset();
    rb->srand(*rb->current_tick);
    int len;
    for(unsigned int i = 0; i < road_length; i += len)
    {
        /* get an EVEN number */
        len = (rb->rand()% 20 + 10) * 2;
        if(i + len >= road_length)
            len = road_length - i;

        int r = rb->rand() % 12;

        /* probability distribution */
        /* 0, 1:       uphill */
        /* 2, 3:       downhill */
        /* 4, 5, 6, 7: straight */
        /* 8, 9:       left */
        /* 10,11:      right */
        switch(r)
        {
        case 0:
        case 1:
            enter_uphill(road, road_length, i, 5, 0);
            add_road(road, road_length, i+5, len-10, 0, ( hills ? 5 : 0));
            exit_uphill(road, road_length, i+len-5, 5, 0);
            break;
        case 2:
        case 3:
            enter_downhill(road, road_length, i, -5, 0);
            add_road(road, road_length, i+5, len-10, 0, (hills ? -5 : 0));
            exit_downhill(road, road_length, i+len-5, -5, 0);
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            add_road(road, road_length, i, len, 0, 0);
            break;
        case 8:
        case 9:
            add_road(road, road_length, i, len, (curves ? -1 << FRACBITS: 0), 0);
            break;
        case 10:
        case 11:
            add_road(road, road_length, i, len, (curves ? 1 << FRACBITS: 0), 0);
            break;
        }
    }
}

void add_sprite(struct road_segment *seg, struct sprite_data_t *data)
{
    LOGF("adding sprite");
    struct sprite_t *s = util_alloc(sizeof(struct sprite_t));
    s->data = data;
    s->next = NULL;
    if(!seg->sprites)
    {
        seg->sprites = s;
        return;
    }
    struct sprite_t *i = seg->sprites;
    while(i->next)
        i = i->next;
    i->next = s;
}
