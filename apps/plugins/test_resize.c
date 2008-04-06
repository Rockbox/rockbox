/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Jonas Hurrelmann
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* Resizing test plugin. Loads /test.bmp (max 100x100) and displays a resized
 * version. Use the scrollwheel or the left/right keys to change the size of
 * the resizded version.
 */

#include "plugin.h"
#include "pluginlib_actions.h"
#include "bmp.h"

PLUGIN_HEADER

static struct plugin_api* rb;

const struct button_mapping *plugin_contexts[]
= {generic_actions, generic_directions};

#define NB_ACTION_CONTEXTS sizeof(plugin_contexts)/sizeof(plugin_contexts[0])

/* Key assignement */
#define SIZE_INCREASE           PLA_UP
#define SIZE_INCREASE_REPEAT    PLA_UP_REPEAT
#define SIZE_DECREASE           PLA_DOWN
#define SIZE_DECREASE_REPEAT    PLA_DOWN_REPEAT

#define WIDTH_INCREASE           PLA_RIGHT
#define WIDTH_INCREASE_REPEAT    PLA_RIGHT_REPEAT
#define WIDTH_DECREASE           PLA_LEFT
#define WIDTH_DECREASE_REPEAT    PLA_LEFT_REPEAT

#define BUTTON_QUIT             PLA_QUIT
#define CHANGE_MODE             PLA_MENU

#define MAX_OUTPUT_WIDTH    LCD_WIDTH
#define MAX_OUTPUT_HEIGHT   LCD_HEIGHT

static fb_data *b;

static struct bitmap input_bmp;
static struct bitmap output_bmp;

static fb_data input_bmp_data[200*200];
static fb_data output_bmp_data[MAX_OUTPUT_WIDTH*MAX_OUTPUT_HEIGHT];


/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    (void)parameter;

    rb = api;
    b = rb->lcd_framebuffer;

    rb->lcd_set_background(LCD_RGBPACK(0,0,0));
    rb->lcd_clear_display(); // TODO: Optimizes this by e.g. invalidating rects

    input_bmp.data = (char*)input_bmp_data;
    output_bmp.data = (char*)output_bmp_data;

    int ret = rb->read_bmp_file("/test.bmp", &input_bmp, sizeof(input_bmp_data),
                                FORMAT_NATIVE);

    if (ret < 0) {
        rb->splash(HZ, "Could not load /test.bmp");
        return PLUGIN_ERROR;
    }

    int button;
    output_bmp.width = 50;
    output_bmp.height = 50;

    DEBUGF("input_bmp_data starts at %p\n", input_bmp_data);
    DEBUGF("output_bmp_data starts at %p\n", output_bmp_data);

    int scale_algorithm = 0;

    while(1) {
        rb->lcd_clear_display();
        rb->lcd_bitmap(input_bmp_data, 0, 0, input_bmp.width, input_bmp.height);

        switch ( scale_algorithm ) {
            case 0:
                smooth_resize_bitmap(&input_bmp, &output_bmp);
                rb->lcd_putsxy(0,0,"smooth_resize_bitmap");
                break;
            case 1:
                simple_resize_bitmap(&input_bmp, &output_bmp);
                rb->lcd_putsxy(0,0,"simple_resize_bitmap");
                break;
        }

        rb->lcd_bitmap(output_bmp_data, 0, 100, output_bmp.width,
                       output_bmp.height);

        rb->lcd_update();
        button = pluginlib_getaction(rb, HZ,
                                     plugin_contexts, NB_ACTION_CONTEXTS);
        switch (button) {
            case BUTTON_QUIT:
                return PLUGIN_OK;
            case SIZE_INCREASE:
            case SIZE_INCREASE_REPEAT:
                if (output_bmp.width < MAX_OUTPUT_WIDTH - 2)
                    output_bmp.width += 2;
                if (output_bmp.height < MAX_OUTPUT_HEIGHT - 2)
                    output_bmp.height += 2;
                break;

            case SIZE_DECREASE:
            case SIZE_DECREASE_REPEAT:
                if (output_bmp.width > 2) output_bmp.width -= 2;
                if (output_bmp.height > 2) output_bmp.height -= 2;
                break;

            case WIDTH_INCREASE:
            case WIDTH_INCREASE_REPEAT:
                if (output_bmp.width < MAX_OUTPUT_WIDTH - 2)
                    output_bmp.width += 2;
                break;

            case WIDTH_DECREASE:
            case WIDTH_DECREASE_REPEAT:
                if (output_bmp.width > 2) output_bmp.width -= 2;
                break;

            case CHANGE_MODE:
                scale_algorithm = (scale_algorithm+1)%2;
                break;
        }
    }
    return PLUGIN_OK;
}
