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

/*
 * This code draws heavily on
 * Jake Gordon's "How to build a racing game" tutorial at
 *
 * <http://codeincomplete.com/projects/racer/>
 *
 * and
 *
 * Louis Gorenfield's "Lou's Pseudo 3d Page" at
 *
 * <http://www.extentofthejam.com/pseudo/>
 *
 * Thanks!
 */

#include "plugin.h"

#include "lib/xlcd.h"

#define SKY_COLOR LCD_RGBPACK(0, 0, 255)
#define GRASS_COLOR LCD_RGBPACK(0, 255, 0)
#define ROAD_COLOR1 LCD_RGBPACK(120, 120, 120)
#define ROAD_COLOR2 LCD_RGBPACK(80, 80, 80)

#define BORDER_COLOR1 LCD_RGBPACK(240, 240, 240)
#define BORDER_COLOR2 LCD_RGBPACK(240, 0, 0)

#define LANE_COLOR LCD_RGBPACK(0xcc, 0xcc, 0xcc)

#define ROAD_WIDTH 128

#define ROAD_LENGTH 1024

#define SEGMENT_LENGTH 2048

#define DRAW_DIST 512

#define CAMERA_DEPTH 100 /* arbitrary */

#define RUMBLE_LENGTH 3

#define LANES 3

struct point_3d {
    int x;
    int y;
    int z;
    int cam_z;
};

struct point_2d {
    int x;
    int y;
    int w;
};

struct road_segment {
    int idx;
    int p1_z;
    int p2_z;
    unsigned color;
    unsigned border_color;
    unsigned lane_color; /* #000000 for no lane */
};

struct road_segment road[ROAD_LENGTH];

inline struct road_segment *find_segment(int z)
{
    return &road[(int)((float)z/SEGMENT_LENGTH)%ROAD_LENGTH];
}

inline void fill_poly(int x1, int y1,
                      int x2, int y2,
                      int x3, int y3,
                      int x4, int y4,
                      unsigned color)
{
    rb->lcd_set_foreground(color);
    LOGF("fill_poly(%d, %d, %d, %d, %d, %d, %d, %d)", x1, y1, x2, y2, x3, y3, x4, y4);
    xlcd_filltriangle(x1, y1, x2, y2, x3, y3);
    xlcd_filltriangle(x2, y2, x3, y3, x4, y4);
}

inline void project(struct point_3d *camera, int camera_depth,
                    struct point_3d *pt_3d, struct point_2d *pt_2d)
{
    int cam_x, cam_y;
    cam_x = pt_3d->x - camera->x;
    cam_y = pt_3d->y - camera->y;
    pt_3d->cam_z = pt_3d->z - camera->z;
    float scale = (float)camera_depth / pt_3d->cam_z;
    pt_2d->x = (LCD_WIDTH/2) + (scale * cam_x * (LCD_WIDTH/2));
    pt_2d->y = (LCD_WIDTH/2) - (scale * cam_y * (LCD_HEIGHT/2));
    pt_2d->w = scale * ROAD_WIDTH * LCD_WIDTH/2;
}

inline int rumble_width(int projected_road_width, int lanes)
{
    return projected_road_width/MAX(6, 2*lanes);
}

inline int marker_width(int projected_road_width, int lanes)
{
    return projected_road_width/MAX(32, 8*lanes);
}

inline void render_segment(struct point_2d *p1, struct point_2d *p2, unsigned road_color, unsigned border_color, int lanes, unsigned lane_color)
{
    LOGF("render_segment");
    int rumble_1 = rumble_width(p1->w, lanes);
    int rumble_2 = rumble_width(p2->w, lanes);

    /* draw grass first */
    rb->lcd_set_foreground(GRASS_COLOR);
    rb->lcd_fillrect(0, p2->y, LCD_WIDTH-1, p1->y - p2->y);

    /* draw borders */
    fill_poly(p1->x-p1->w-rumble_1, p1->y, p1->x-p1->w, p1->y, p2->x-p2->w, p2->y, p2->x-p2->w-rumble_2, p2->y, border_color);
    fill_poly(p1->x+p1->w+rumble_1, p1->y, p1->x+p1->w, p1->y, p2->x+p2->w, p2->y, p2->x+p2->w+rumble_2, p2->y, border_color);

    /* draw road */
    fill_poly(p1->x-p1->w, p1->y, p1->x+p1->w, p1->y, p2->x+p2->w, p2->y, p2->x-p2->w, p2->y, road_color);

    /* draw lanes */
    if(lane_color)
    {
        int lane_1 = marker_width(p1->w, lanes);
        int lane_2 = marker_width(p2->w, lanes);

        int lane_w1, lane_w2, lane_x1, lane_x2;
        lane_w1 = p1->w*2/lanes;
        lane_w2 = p2->w*2/lanes;
        lane_x1 = p1->x - p1->w + lane_w1;
        lane_x2 = p2->x - p2->w + lane_w2;
        for(int i=1;i<=lanes;lane_x1 += lane_w1, lane_x2 += lane_w2, ++i)
        {
            fill_poly(lane_x1 - lane_1/2, p1->y, lane_x1 + lane_1/2, p1->y, lane_x2 + lane_2/2, p2->y, lane_x2 - lane_2/2, p2->y, lane_color);
        }
    }
}

void render(struct point_3d *camera)
{
    rb->lcd_set_background(SKY_COLOR);
    rb->lcd_clear_display();
    struct road_segment *base=find_segment(camera->z);
    for(int i=0;i<DRAW_DIST;++i)
    {
        struct road_segment *seg = &road[(base->idx+i)%ROAD_LENGTH];

        struct point_2d p1, p2; /* projected points */
        struct point_3d p1_3d={0, 0, seg->p1_z, 0},
                        p2_3d={0, 0, seg->p2_z, 0};

        project(camera, CAMERA_DEPTH, &p1_3d, &p1);
        project(camera, CAMERA_DEPTH, &p2_3d, &p2);

        if(p1_3d.cam_z <= CAMERA_DEPTH || /* behind camera */
           p2.y >= LCD_HEIGHT) /* clipped */
        {
            LOGF("clipped");
            continue;
        }

        render_segment(&p1, &p2, seg->color, seg->border_color, LANES, seg->lane_color);
    }
}

enum plugin_status plugin_start(const void *param)
{
    (void) param;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    struct point_3d camera;
    camera.x=0;
    camera.y=100;
    camera.z=0;
    for(unsigned int i=0;i<ARRAYLEN(road);++i)
    {
        struct road_segment tmp;
        tmp.idx=i;
        tmp.p1_z=i*SEGMENT_LENGTH;
        tmp.p2_z=(i+1)*SEGMENT_LENGTH;
        tmp.color=(int)((float)i/RUMBLE_LENGTH)%2?ROAD_COLOR1:ROAD_COLOR2;
        tmp.border_color=(int)((float)i/RUMBLE_LENGTH)%2?BORDER_COLOR1:BORDER_COLOR2;
        tmp.lane_color=(int)((float)i/RUMBLE_LENGTH)%2?LANE_COLOR:0;
        road[i]=tmp;
    }
    while(1)
    {
        render(&camera);
        rb->lcd_update();
        camera.z+=100;
    }
}
