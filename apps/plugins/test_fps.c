/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Peter D'Hoye
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP

#if (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD)
#define FPS_QUIT BUTTON_MENU
#else
#ifdef BUTTON_OFF
#define FPS_QUIT BUTTON_OFF
#else
#define FPS_QUIT BUTTON_POWER
#endif
#endif

PLUGIN_HEADER

static struct plugin_api* rb;

/* plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    char str[64];                   /* text buffer */
    int time_start = 0;             /* start tickcount */
    int frame_count = 0;            /* frame counter */
    int part14_x = LCD_WIDTH/4;     /* x-offset for 1/4 update test */
    int part14_w = LCD_WIDTH/2;     /* x-size for 1/4 update test */
    int part14_y = LCD_HEIGHT/4;    /* y-offset for 1/4 update test */
    int part14_h = LCD_HEIGHT/2;    /* y-size for 1/4 update test */

    /* standard stuff */
    (void)parameter;
    rb = api;

    rb->lcd_clear_display();
    rb->lcd_puts(0, 0, "FPS Measurements:");
    rb->lcd_update();

    /* TEST 1: FULL LCD UPDATE */
    frame_count = 0;
    time_start = *rb->current_tick;
    while(*rb->current_tick - time_start < 2*HZ)
    {
        rb->lcd_update();
        frame_count++;
    }

    rb->snprintf(str, sizeof(str), "1:1 = %d.%d (cpu = %d)", frame_count/2,
        (frame_count%2)*5, *rb->cpu_frequency );
    rb->lcd_puts(0, 1, str);
    rb->lcd_update();

    /* TEST 2: QUARTER LCD UPDATE */
    frame_count = 0;
    time_start = *rb->current_tick;
    while(*rb->current_tick - time_start < 2*HZ)
    {
        rb->lcd_update_rect(part14_x, part14_y, part14_w, part14_h);
        frame_count++;
    }

    rb->snprintf(str, sizeof(str), "1:4 = %d.%d (cpu = %d)", frame_count/2,
        (frame_count%2)*5, *rb->cpu_frequency );
    rb->lcd_puts(0, 2, str);
    rb->lcd_update();

    /* wait until user closes plugin */
    while (rb->button_get(true) != FPS_QUIT);

    return PLUGIN_OK;
}
#endif
