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
#elif defined(BUTTON_OFF)
#define FPS_QUIT BUTTON_OFF
#else
#define FPS_QUIT BUTTON_POWER
#endif

PLUGIN_HEADER

static struct plugin_api* rb;

/* plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    char str[64];                   /* text buffer */
    int time_start;                 /* start tickcount */
    int frame_count;                /* frame counter */
    int cpu_freq;
    int line = 0;
    int part14_x = LCD_WIDTH/4;     /* x-offset for 1/4 update test */
    int part14_w = LCD_WIDTH/2;     /* x-size for 1/4 update test */
    int part14_y = LCD_HEIGHT/4;    /* y-offset for 1/4 update test */
    int part14_h = LCD_HEIGHT/2;    /* y-size for 1/4 update test */
#ifdef HAVE_REMOTE_LCD
    int r_line = 0;
    int r_part14_x = LCD_REMOTE_WIDTH/4;  /* x-offset for 1/4 update test */
    int r_part14_w = LCD_REMOTE_WIDTH/2;  /* x-size for 1/4 update test */
    int r_part14_y = LCD_REMOTE_HEIGHT/4; /* y-offset for 1/4 update test */
    int r_part14_h = LCD_REMOTE_HEIGHT/2; /* y-size for 1/4 update test */
#endif

    /* standard stuff */
    (void)parameter;
    rb = api;

    rb->lcd_clear_display();
    rb->lcd_puts(0, line++, "FPS Measurement");
#ifdef HAVE_REMOTE_LCD
    rb->lcd_puts(0, line++, "Main LCD");
    rb->lcd_remote_puts(0, r_line++, "FPS Measurement");
    rb->lcd_remote_puts(0, r_line++, "Main LCD");
    rb->lcd_remote_update();
#endif
    rb->lcd_update();

    cpu_freq = *rb->cpu_frequency; /* remember CPU frequency */

    /* TEST 1: FULL LCD UPDATE */
    frame_count = 0;
    time_start = *rb->current_tick;
    while(*rb->current_tick - time_start < 2*HZ)
    {
        rb->lcd_update();
        frame_count++;
    }

    rb->snprintf(str, sizeof(str), "1/1: %d.%d fps", frame_count / 2,
                 (frame_count % 2) * 5);
    rb->lcd_puts(0, line++, str);
    rb->lcd_update();

    /* TEST 2: QUARTER LCD UPDATE */
    frame_count = 0;
    time_start = *rb->current_tick;
    while(*rb->current_tick - time_start < 2*HZ)
    {
        rb->lcd_update_rect(part14_x, part14_y, part14_w, part14_h);
        frame_count++;
    }

    rb->snprintf(str, sizeof(str), "1/4: %d.%d fps", frame_count/2,
                 (frame_count%2)*5);
    rb->lcd_puts(0, line++, str);
    
    if (*rb->cpu_frequency != cpu_freq)
        rb->snprintf(str, sizeof(str), "CPU: frequency changed!");
    else
        rb->snprintf(str, sizeof(str), "CPU: %d Hz", cpu_freq);

    rb->lcd_puts(0, line++, str);
    rb->lcd_update();

#ifdef HAVE_REMOTE_LCD
    rb->lcd_puts(0, line++, "Remote LCD");
    rb->lcd_update();
    rb->lcd_remote_puts(0, r_line++, "Remote LCD");
    rb->lcd_remote_update();

    cpu_freq = *rb->cpu_frequency; /* remember CPU frequency */

    /* TEST 1: FULL LCD UPDATE */
    frame_count = 0;
    time_start = *rb->current_tick;
    while(*rb->current_tick - time_start < 2*HZ)
    {
        rb->lcd_remote_update();
        frame_count++;
    }

    rb->snprintf(str, sizeof(str), "1/1: %d.%d fps", frame_count / 2,
                 (frame_count % 2) * 5);
    rb->lcd_puts(0, line++, str);
    rb->lcd_update();

    /* TEST 2: QUARTER LCD UPDATE */
    frame_count = 0;
    time_start = *rb->current_tick;
    while(*rb->current_tick - time_start < 2*HZ)
    {
        rb->lcd_remote_update_rect(r_part14_x, r_part14_y, r_part14_w, 
                                   r_part14_h);
        frame_count++;
    }

    rb->snprintf(str, sizeof(str), "1/4: %d.%d fps", frame_count/2,
                 (frame_count%2)*5);
    rb->lcd_puts(0, line++, str);
    
    if (*rb->cpu_frequency != cpu_freq)
        rb->snprintf(str, sizeof(str), "CPU: frequency changed!");
    else
        rb->snprintf(str, sizeof(str), "CPU: %d Hz", cpu_freq);

    rb->lcd_puts(0, line++, str);
    rb->lcd_update();
#endif

    /* wait until user closes plugin */
    while (rb->button_get(true) != FPS_QUIT);

    return PLUGIN_OK;
}
#endif
