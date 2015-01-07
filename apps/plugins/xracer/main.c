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

#include "road.h"

#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"
#include "lib/xlcd.h"

#include "fixedpoint.h" /* only used for FOV computation */

/* colors */
#define SKY_COLOR1 LCD_RGBPACK(0, 0x96, 0xff)
#define SKY_COLOR2 LCD_RGBPACK(0xfe, 0xfe, 0xfe)
#define GRASS_COLOR1 LCD_RGBPACK(0, 0xc5, 0)
#define GRASS_COLOR2 LCD_RGBPACK(0, 0x9a, 0)
#define ROAD_COLOR1 LCD_RGBPACK(120, 120, 120)
#define ROAD_COLOR2 LCD_RGBPACK(80, 80, 80)
#define BORDER_COLOR1 LCD_RGBPACK(240, 0, 0)
#define BORDER_COLOR2 LCD_RGBPACK(240, 240, 240)
#define LANE_COLOR LCD_RGBPACK(0xcc, 0xcc, 0xcc)

/* road parameters */
#define ROAD_WIDTH (LCD_WIDTH/2) /* actually half the road width for easier math */
#define MAX_ROAD_LENGTH 4096
#define SEGMENT_LENGTH (LCD_WIDTH * 12)

/* road generator parameters */
#define HILLS true
#define CURVES true

/* this specifies the length of each road and border color in segments */
#define RUMBLE_LENGTH 3
#define LANES 3

/* camera parameters */
#define DRAW_DIST 386
/* in degrees */
#define CAMERA_FOV 72
#define CAMERA_HEIGHT 100

/* game parameters */
/* currently this is how far the camera moves per keypress */
#define MANUAL_SPEED 256

/* NASTY! */
#define ROUND(x) ((int)((float)x+.5))

/* some basic types */

struct point_3d {
    int x;
    int y;
    int z;
};

struct point_2d {
    int x;
    int y;
    int w;
};

/* can move to audiobuf if too big... */
struct road_segment road[MAX_ROAD_LENGTH];

/* this can be changed to allow for variable-sized maps */
unsigned int road_length=MAX_ROAD_LENGTH;

int camera_depth; /* calculated */

/* returns the road segment corresponding to a z position */
inline struct road_segment *find_segment(int z)
{
    return &road[(int)((float)z/SEGMENT_LENGTH)%road_length];
}

/* fill poly takes 4 screenspace coordinates and fills them */
/* order of arguments matters: */
/* (x1, y1)      (x2, y2) */
/* (x3, y3)      (x4, y4) */
inline void fill_poly(int x1, int y1,
                      int x2, int y2,
                      int x3, int y3,
                      int x4, int y4,
                      unsigned color)
{
    rb->lcd_set_foreground(color);
    xlcd_filltriangle(x1, y1, x2, y2, x3, y3);
    xlcd_filltriangle(x1, y1, x3, y3, x4, y4);
}

/* project(): performs 3d projection of pt_3d onto pt_2d given camera parameters */
/* this takes arguments by value so as to allow easy changing of camera positions */

inline int project(int camera_x, int camera_y, int camera_z, int camera_depth,
                    struct point_3d *pt_3d, struct point_2d *pt_2d)
{
    /* calculate the coordinates relative to the camera */
    int rel_x, rel_y, rel_z;
    rel_x = pt_3d->x - camera_x;
    rel_y = pt_3d->y - camera_y;
    rel_z = pt_3d->z - camera_z;
    float scale = (float)camera_depth / rel_z;
    pt_2d->x = (LCD_WIDTH/2) + (scale * rel_x * (LCD_WIDTH/2));
    pt_2d->y = (LCD_WIDTH/2) - (scale * rel_y * (LCD_HEIGHT/2));
    pt_2d->w = scale * ROAD_WIDTH * (LCD_WIDTH/2);
    return rel_z;
}

inline int border_width(int projected_road_width, int lanes)
{
    return projected_road_width/MAX(6, 2*lanes);
}

inline int lane_marker_width(int projected_road_width, int lanes)
{
    return projected_road_width/MAX(32, 8*lanes);
}

/* fill_gradient(): draws a gradient across the whole screen from top to bottom,
   changing from top_color to bottom_color */

void fill_gradient(int top, unsigned top_color, int bottom, unsigned bottom_color)
{
    int top_r, top_g, top_b;
    top_r = RGB_UNPACK_RED(top_color);
    top_g = RGB_UNPACK_GREEN(top_color);
    top_b = RGB_UNPACK_BLUE(top_color);

    int bottom_r, bottom_g, bottom_b;
    bottom_r = RGB_UNPACK_RED(bottom_color);
    bottom_g = RGB_UNPACK_GREEN(bottom_color);
    bottom_b = RGB_UNPACK_BLUE(bottom_color);

    float r_step = (float)(bottom_r-top_r) / (float)(bottom-top);
    float g_step = (float)(bottom_g-top_g) / (float)(bottom-top);
    float b_step = (float)(bottom_b-top_b) / (float)(bottom-top);

    float r = top_r;
    float g = top_g;
    float b = top_b;

    for(int i = top; i<bottom; ++i)
    {
        rb->lcd_set_foreground(LCD_RGBPACK( ROUND(r), ROUND(g), ROUND(b) ));
        rb->lcd_hline(0, LCD_WIDTH, i);
        r += r_step;
        g += g_step;
        b += b_step;
    }
}

/* draws a segment on screen given its 2d screen coordinates and other data */

inline void render_segment(struct point_2d *p1, struct point_2d *p2,
                           unsigned road_color, unsigned border_color,
                           int lanes, unsigned lane_color, unsigned grass_color)
{
    int border_1 = border_width(p1->w, lanes);
    int border_2 = border_width(p2->w, lanes);

    /* draw grass first */
    rb->lcd_set_foreground(grass_color);
    rb->lcd_fillrect(0, p2->y, LCD_WIDTH, p1->y - p2->y);

    /* draw borders */
    fill_poly(p1->x-p1->w-border_1, p1->y, p1->x-p1->w, p1->y,
              p2->x-p2->w, p2->y, p2->x-p2->w-border_2, p2->y, border_color);

    fill_poly(p1->x+p1->w+border_1, p1->y, p1->x+p1->w, p1->y,
              p2->x+p2->w, p2->y, p2->x+p2->w+border_2, p2->y, border_color);

    /* draw road */
    fill_poly(p2->x-p2->w, p2->y, p2->x+p2->w, p2->y, p1->x+p1->w, p1->y, p1->x-p1->w, p1->y, road_color);

    /* draw lanes */
    if(lanes > 1)
    {
        int lane_1 = lane_marker_width(p1->w, lanes);
        int lane_2 = lane_marker_width(p2->w, lanes);

        int lane_w1, lane_w2, lane_x1, lane_x2;
        lane_w1 = p1->w*2/lanes;
        lane_w2 = p2->w*2/lanes;
        lane_x1 = p1->x - p1->w + lane_w1;
        lane_x2 = p2->x - p2->w + lane_w2;
        for(int i=1; i<lanes; lane_x1 += lane_w1, lane_x2 += lane_w2, ++i)
        {
            fill_poly(lane_x1 - lane_1/2, p1->y, lane_x1 + lane_1/2, p1->y,
                      lane_x2 + lane_2/2, p2->y, lane_x2 - lane_2/2, p2->y, lane_color);
        }
    }
}

/* project_segment(): does the 3d calculations on a road segment */

inline bool project_segment(struct road_segment *base, struct road_segment *seg, int x, int dx,
                            struct point_3d *camera, struct point_2d *p1, struct point_2d *p2, int *maxy)
{
    seg->looped = (seg->idx < base->idx);

    /* future optimization: do one projection per segment because segment n's p2 is the same as segments n+1's p1*/

    /* input points (3d space) */

    struct point_3d p1_3d={-x,    seg->p1_y, seg->p1_z};
    struct point_3d p2_3d={-x-dx, seg->p2_y, seg->p2_z};

    int camera_offset = (seg->looped ? road_length * SEGMENT_LENGTH - camera->z : 0);

    /* 3d project the 2nd point FIRST because its results can be used to save some work */

    int p2_cz = project(camera->x, camera->y, camera->z - camera_offset, camera_depth, &p2_3d, p2);

    /* decide whether or not to draw this segment */

    if(p2_cz <= camera_depth || /* behind camera */
       p2->y >= *maxy)          /* clipped */
    {
        return false;
    }

    /* nothing is drawn below this */
    *maxy = p2->y;

    /* now 3d project the first point */

    project(camera->x, camera->y, camera->z - camera_offset, camera_depth, &p1_3d, p1);

    /* still try to exit early */
    if(p2->y >= p1->y) /* backface cull */
    {
        return false;
    }
    return true;
}

/* interpolate(): linearly interpolates a and b */

inline int interpolate(int a, int b, float percent)
{
    return a + (b-a) * percent;
}

/* render(): The primary render driver function
 *
 * calculates segment coordinates and calls render_segment to draw the segment on-screen
 * also handles the faking of curves
 */

void render(struct point_3d *camera)
{
    rb->lcd_clear_display();

    fill_gradient(0, SKY_COLOR1, LCD_HEIGHT, SKY_COLOR2);

    struct road_segment *base = find_segment(camera->z);

    float base_percent = (camera->z % SEGMENT_LENGTH) / SEGMENT_LENGTH;
    int dx = - (base->curve * base_percent);
    int x  = 0;

    /* clipping height, nothing is drawn above this */
    int maxy = LCD_HEIGHT;

    /* move the camera to a fixed point above the road */
    /* interpolate so as to prevent jumpy camera movement on hills */
    camera->y = interpolate(base->p1_y, base->p2_y, base_percent) + CAMERA_HEIGHT;

    /* output points (screenspace) */
    struct point_2d p1, p2;

    for(int i = 0; i < DRAW_DIST; ++i)
    {
        struct road_segment *seg = &road[(base->idx + i) % road_length];

        if(project_segment(base, seg, x, dx, camera, &p1, &p2, &maxy))
            render_segment(&p1, &p2, seg->color, seg->border_color, seg->lanes, seg->lane_color, seg->grass_color);

        /* curve calculation */
        x  += dx;
        dx += seg->curve;
    }
}

/* add_road(): adds a section of road segments between s and s+len,
   each with the properties specified */

void add_road(int s, int len, int curve, int hill)
{
    static int last_y=0;
    for(int i = s; i < s+len; ++i)
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

        tmp.color = (int)((float)i/RUMBLE_LENGTH)%2?ROAD_COLOR1:ROAD_COLOR2;
        tmp.border_color = (int)((float)i/RUMBLE_LENGTH)%2?BORDER_COLOR1:BORDER_COLOR2;
        tmp.lanes = (int)((float)i/RUMBLE_LENGTH)%2?LANES:1;
        tmp.lane_color = LANE_COLOR;
        tmp.grass_color = (int)((float)i/RUMBLE_LENGTH)%2?GRASS_COLOR1:GRASS_COLOR2;
        tmp.looped = false;

        road[i] = tmp;
    }
}

/* enter_hill(): adds a road section with slopes 0,1...slope-1 */
void enter_hill(int s, int slope, int curve)
{
    for(int i=0; i<slope; ++i)
    {
        add_road(s + i, 1, curve, i);
    }
}

/* exit_hill(): opposite of enter_hill, adds a road section with slopes slope-1...0 */
void exit_hill(int s, int slope, int curve)
{
    int n = slope;
    int i=0;
    for(i = 0; i<n; ++i, --slope)
    {
        add_road(s + i, 1, curve, slope);
    }
}

void add_uphill(int s, int slope, int hold, int curve)
{
    enter_hill(s, slope, curve);
    add_road(s+slope, hold, curve, slope);
    exit_hill(s+slope+hold, slope, curve);
}

/* generate_random_road(): generates a random track road_length segments long */

void generate_random_road(bool hills, bool curves)
{
    rb->srand(*rb->current_tick);
    int len;
    for(unsigned int i = 0; i < road_length - DRAW_DIST; i += len)
    {
        /* get an EVEN number between 2 and 40 */
        len = (rb->rand()% 20 + 10) * 2;
        if(i + len >= road_length)
            len = road_length - i;

        int r = rb->rand() % 12;

        if(r<=1)
        {
            enter_hill(i, 5, 0);
            add_road(i+5, len-10, 0, ( hills ? 5 : 0));
            exit_hill(i+len-5, 5, 0);
        }
        else if(r<=3)
        {
            enter_hill(i, 5, 0);
            add_road(i+5, len-10, 0, (hills ? 5 : 0));
            exit_hill(i+len-5, 5, 0);
        }
        else if(r<=7)
            add_road(i, len, 0, 0);
        else if(r<=9)
            add_road(i, len, (curves ? -1 : 0), 0);
        else if(r<12)
            add_road(i, len, (curves ? 1 : 0), 0);
    }

    /* copy the first part of the track to the last to make it loop better */
    memcpy(road + road_length - DRAW_DIST, road, sizeof(struct road_segment) * DRAW_DIST);
}
#if 0
void generate_test_road(void)
{
    add_road(0, 100, 0, 0);
    enter_hill(100, 5, 0);
    add_road(105, 20, 0, 5);
    exit_hill(125, 5, 0);
    add_road(130, 100, 0, 0);
    road_length=230;
}
#endif
/* camera_calc_depth(): calculates the camera depth given its FOV by evaluating */
/* LCD_WIDTH / tan(fov) */
int camera_calc_depth(int fov)
{
    long fov_sin, fov_cos;
    /* nasty fixed-point math! */
    /* 0xffffffff = 2*pi */
    fov_sin = fp14_sin(fov);
    fov_cos = fp14_cos(fov);

    long tan;
    /* fp_sincos has 14 fractional bits */
    tan = fp_div(fov_sin, fov_cos, 14);

    long width = LCD_WIDTH << 14;
    return fp_div(width, tan, 14) >> 14;
}

/* plugin_start(): plugin entry point */
/* contains main loop */
enum plugin_status plugin_start(const void *param)
{
    (void) param;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    /* GIMME DAT RAW POWAH!!! */
    rb->cpu_boost(true);
#endif

    camera_depth = camera_calc_depth(CAMERA_FOV);

    LOGF("camera depth: %d", camera_depth);

    struct point_3d camera;
    camera.x = 0;
    camera.y = CAMERA_HEIGHT;
    camera.z = 0;

    generate_random_road(HILLS, CURVES);

    //generate_test_road();

    while(1)
    {
        static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };
        int button = pluginlib_getaction(0, plugin_contexts, ARRAYLEN(plugin_contexts));
        switch(button)
        {
        case PLA_UP:
        case PLA_UP_REPEAT:
            camera.z+=MANUAL_SPEED;
            break;
        case PLA_DOWN:
        case PLA_DOWN_REPEAT:
            camera.z-=MANUAL_SPEED;
            break;
        case PLA_LEFT:
        case PLA_LEFT_REPEAT:
            camera.x-=MANUAL_SPEED;
            break;
        case PLA_RIGHT:
        case PLA_RIGHT_REPEAT:
            camera.x+=MANUAL_SPEED;
            break;
        case PLA_CANCEL:
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(false);
#endif
            return PLUGIN_OK;
        default:
            exit_on_usb(button);
            break;
        }
        camera.z += 512;
        camera.z %= (road_length - DRAW_DIST) * SEGMENT_LENGTH;

        render(&camera);

        rb->lcd_set_foreground(LCD_WHITE);
        rb->lcd_putsf(0, 0, "camera: (%d, %d, %d)", camera.x, camera.y, camera.z);

        rb->lcd_update();

        rb->yield();
    }
}
