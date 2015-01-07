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

#include "xracer.h"
#include "generator.h"
#include "graphics.h"

#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"
#include "lib/xlcd.h"

#include "fixedpoint.h" /* only used for FOV computation */

/* can move to audiobuf if too big... */
struct road_segment road[MAX_ROAD_LENGTH];

/* this can be changed to allow for variable-sized maps */
unsigned int road_length=MAX_ROAD_LENGTH;

int camera_height = CAMERA_HEIGHT;

void generate_test_road(void)
{
    add_road(road, 0, road_length, 0, 0);
    for(unsigned int i=0;i<road_length;++i)
        add_sprite(road+i, &sprites[0]);
}

/* camera_calc_depth(): calculates the camera depth given its FOV by evaluating */
/* LCD_WIDTH / tan(fov) */
int camera_calc_depth(int fov)
{
    long fov_sin, fov_cos;
    /* nasty (but fast) fixed-point math! */
    fov_sin = fp14_sin(fov);
    fov_cos = fp14_cos(fov);

    long tan;
    /* fp14_sin/cos has 14 fractional bits */
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
    struct camera_t camera;
    camera.depth = camera_calc_depth(CAMERA_FOV);

    LOGF("camera depth: %d", camera.depth);

    camera.pos.x = 0;
    /* y is automatically calculated */
    camera.pos.z = 0;

    generate_random_road(road, road_length, HILLS, CURVES);

    //generate_test_road();

    while(1)
    {
        static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };
        int button = pluginlib_getaction(0, plugin_contexts, ARRAYLEN(plugin_contexts));
        switch(button)
        {
        case PLA_UP:
        case PLA_UP_REPEAT:
            camera_height+=MANUAL_SPEED;
            break;
        case PLA_DOWN:
        case PLA_DOWN_REPEAT:
            camera_height-=MANUAL_SPEED;
            break;
        case PLA_LEFT:
        case PLA_LEFT_REPEAT:
            camera.pos.x-=MANUAL_SPEED;
            break;
        case PLA_RIGHT:
        case PLA_RIGHT_REPEAT:
            camera.pos.x+=MANUAL_SPEED;
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
        camera.pos.z += 512;
        camera.pos.z %= (road_length - DRAW_DIST) * SEGMENT_LENGTH;

        render(&camera, road, road_length, camera_height);

        rb->lcd_set_foreground(LCD_WHITE);
        rb->lcd_putsf(0, 0, "camera: (%d, %d, %d)", camera.pos.x, camera.pos.y, camera.pos.z);

        rb->lcd_update();

        rb->yield();
    }
}
