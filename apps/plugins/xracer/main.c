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
#include "compat.h"
#include "generator.h"
#include "graphics.h"
#include "map.h"
#include "maps.h"
#include "road.h"
#include "sprite.h"
#include "util.h"

#include "lib/helper.h"
#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"

#include "fixedpoint.h" /* only used for FOV computation */

/* can move to audiobuf if too big... */
struct road_segment road[MAX_ROAD_LENGTH];

/* this can be changed to allow for variable-sized maps */
unsigned int road_length=MAX_ROAD_LENGTH;

int camera_height = CAMERA_HEIGHT;

void generate_test_road(void)
{
    gen_reset();
    add_road(road, road_length, 0, road_length, 0, 0);
    for(unsigned int i = 0; i < road_length; ++i)
        add_sprite(road + i, &sprites[0]);
}

void do_early_init(void)
{
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    /* GIMME DAT RAW POWAH!!! */
    rb->cpu_boost(true);
#endif

    /* disable backlight timeout */
    backlight_ignore_timeout();

    init_alloc();
}

void do_late_init(void)
{
#if (LCD_DEPTH < 4)
    /* greylib init */
    size_t buf_sz = util_alloc_remaining();
    if(!grey_init(util_alloc(buf_sz), buf_sz,
                  GREY_BUFFERED|GREY_RAWMAPPED|GREY_ON_COP,
                  LCD_WIDTH, LCD_HEIGHT, NULL))
    {
        error("grey init failed");
    }
    grey_show(true);
#endif
}

enum plugin_status do_flythrough(void)
{
    struct camera_t camera;
    camera.depth = camera_calc_depth(CAMERA_FOV);

    LOGF("camera depth: %d", camera.depth);

    camera.pos.x = 0;
    /* y is automatically calculated */
    camera.pos.z = 0;

    //generate_test_road();

    //road_length = load_map(road, MAX_ROAD_LENGTH, loop_map, ARRAYLEN(loop_map));

    road_length = load_external_map(road, MAX_ROAD_LENGTH, "/output.xrm");

    if(road_length == (unsigned int)-1)
    {
        warning("Failed to load map, using random map");

        road_length = MAX_ROAD_LENGTH;

        generate_random_road(road, road_length, HILLS, CURVES);
    }

    long last_timestamp = *rb->current_tick;

    do_late_init();

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
        /* loop the track right before going off the "end" */
        camera.pos.z %= (road_length - DRAW_DIST) * SEGMENT_LENGTH;

        render(&camera, road, road_length, camera_height);

        SET_FOREGROUND(LCD_WHITE);
        SET_BACKGROUND(LCD_BLACK);
        int dt = *rb->current_tick - last_timestamp;
        char buf[16];
        //rb->snprintf(buf, sizeof(buf), "FPS: %ld", HZ/(!dt?1:dt));
        rb->snprintf(buf, sizeof(buf), "DT: %d", dt);
        PUTSXY(0, 0, buf);

        LCD_UPDATE();

        //rb->sleep((HZ/100)-dt);
        rb->yield();

        last_timestamp = *rb->current_tick;
    }
}

/* plugin_start(): plugin entry point */
/* contains main loop */
enum plugin_status plugin_start(const void *param)
{
    (void) param;

    do_early_init();

    enum plugin_status rc = do_flythrough();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    return rc;
}
