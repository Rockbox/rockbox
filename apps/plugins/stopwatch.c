/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 Mike Holden
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"
#include "lib/pluginlib_actions.h"


#ifdef HAVE_LCD_BITMAP
#define TIMER_Y 1
#else
#define TIMER_Y 0
#endif

#define LAP_Y TIMER_Y+1
#define MAX_LAPS 64

#define STOPWATCH_FILE PLUGIN_APPS_DATA_DIR "/stopwatch.dat"

/* this set the context to use with PLA */
static const struct button_mapping *plugin_contexts[] = {
    pla_main_ctx,
#if defined(HAVE_REMOTE_LCD)
    pla_remote_ctx,
#endif
};

/* we use PLA */
#ifdef HAVE_SCROLLWHEEL
#   define STOPWATCH_START_STOP           PLA_SELECT_REL
#   define STOPWATCH_RESET_TIMER          PLA_SELECT_REPEAT
#   define STOPWATCH_LAP_TIMER            PLA_RIGHT
#   define STOPWATCH_SCROLL_UP            PLA_SCROLL_BACK
#   define STOPWATCH_SCROLL_DOWN          PLA_SCROLL_FWD
#   define STOPWATCH_SCROLL_UP_REPEAT     PLA_SCROLL_BACK_REPEAT
#   define STOPWATCH_SCROLL_DOWN_REPEAT   PLA_SCROLL_FWD_REPEAT
#else
#   define STOPWATCH_START_STOP           PLA_SELECT_REL
#   define STOPWATCH_RESET_TIMER          PLA_SELECT_REPEAT
#   define STOPWATCH_LAP_TIMER            PLA_RIGHT
#   define STOPWATCH_SCROLL_UP            PLA_UP
#   define STOPWATCH_SCROLL_DOWN          PLA_DOWN
#   define STOPWATCH_SCROLL_UP_REPEAT     PLA_UP_REPEAT
#   define STOPWATCH_SCROLL_DOWN_REPEAT   PLA_DOWN_REPEAT
#endif/* HAVE_SCROLLWHEEL */

#define STOPWATCH_QUIT PLA_EXIT

static int stopwatch = 0;
static long start_at = 0;
static int prev_total = 0;
static bool counting = false;
static int curr_lap = 0;
static int lap_scroll = 0;
static int lap_start;
static int lap_times[MAX_LAPS];

static void ticks_to_string(int ticks,int lap,int buflen, char * buf)
{
    int hours, minutes, seconds, cs;

    hours = ticks / (HZ * 3600);
    ticks -= (HZ * hours * 3600);
    minutes = ticks / (HZ * 60);
    ticks -= (HZ * minutes * 60);
    seconds = ticks / HZ;
    ticks -= (HZ * seconds);
    cs = ticks;
    if (!lap)
    {
        rb->snprintf(buf, buflen,
                     "%2d:%02d:%02d.%02d",
                     hours, minutes, seconds, cs);
    }
    else
    {

        if (lap > 1)
        {
            int last_ticks, last_hours, last_minutes, last_seconds, last_cs;
            last_ticks = lap_times[(lap-1)%MAX_LAPS] - lap_times[(lap-2)%MAX_LAPS];
            last_hours = last_ticks / (HZ * 3600);
            last_ticks -= (HZ * last_hours * 3600);
            last_minutes = last_ticks / (HZ * 60);
            last_ticks -= (HZ * last_minutes * 60);
            last_seconds = last_ticks / HZ;
            last_ticks -= (HZ * last_seconds);
            last_cs = last_ticks;

            rb->snprintf(buf, buflen,
                         "%2d %2d:%02d:%02d.%02d [%2d:%02d:%02d.%02d]",
                         lap, hours, minutes, seconds, cs, last_hours,
                         last_minutes, last_seconds, last_cs);
        }
        else
        {
            rb->snprintf(buf, buflen,
                         "%2d %2d:%02d:%02d.%02d",
                         lap, hours, minutes, seconds, cs);
        }
    }
}

/* 
 * Load saved stopwatch state, if exists.
 */
static void load_stopwatch(void)
{
    int fd;
    
    fd = rb->open(STOPWATCH_FILE, O_RDONLY);
    
    if (fd < 0)
    {
        return;
    }
    
    /* variable stopwatch isn't saved/loaded, because it is only used
     * temporarily in main loop
     */
    
    rb->read(fd, &start_at, sizeof(start_at));
    rb->read(fd, &prev_total, sizeof(prev_total));
    rb->read(fd, &counting, sizeof(counting));
    rb->read(fd, &curr_lap, sizeof(curr_lap));
    rb->read(fd, &lap_scroll, sizeof(lap_scroll));
    rb->read(fd, &lap_start, sizeof(lap_start));
    rb->read(fd, lap_times, sizeof(lap_times));
    
    if (counting && start_at > *rb->current_tick)
    {
        /* Stopwatch started in the future? Unlikely; probably started on a
         * previous session and powered off in-between.  We'll keep
         * everything intact (user can clear manually) but stop the
         * stopwatch to avoid negative timing.
         */
        start_at = 0;
        counting = false;
    }
    
    rb->close(fd);
}

/* 
 * Save stopwatch state.
 */
static void save_stopwatch(void)
{
    int fd;
    
    fd = rb->open(STOPWATCH_FILE, O_CREAT|O_WRONLY|O_TRUNC, 0666);
    
    if (fd < 0)
    {
        return;
    }
    
    /* variable stopwatch isn't saved/loaded, because it is only used
     * temporarily in main loop
     */
    
    rb->write(fd, &start_at, sizeof(start_at));
    rb->write(fd, &prev_total, sizeof(prev_total));
    rb->write(fd, &counting, sizeof(counting));
    rb->write(fd, &curr_lap, sizeof(curr_lap));
    rb->write(fd, &lap_scroll, sizeof(lap_scroll));
    rb->write(fd, &lap_start, sizeof(lap_start));
    rb->write(fd, lap_times, sizeof(lap_times));
    
    rb->close(fd);
}

enum plugin_status plugin_start(const void* parameter)
{
    char buf[32];
    int button;
    int lap;
    int done = false;
    bool update_lap = true;
    int lines;

    (void)parameter;

#ifdef HAVE_LCD_BITMAP
    int h;
    rb->lcd_setfont(FONT_UI);
    rb->lcd_getstringsize("M", NULL, &h);
    lines = (LCD_HEIGHT / h) - (LAP_Y);
#else
    lines = 1;
#endif

    load_stopwatch();
    
    rb->lcd_clear_display();
    
    while (!done)
    {
        if (counting)
        {
            stopwatch = prev_total + *rb->current_tick - start_at;
        }
        else
        {
            stopwatch = prev_total;
        }

        ticks_to_string(stopwatch,0,32,buf);
        rb->lcd_puts(0, TIMER_Y, buf);

        if(update_lap)
        {
            lap_start = curr_lap - lap_scroll;
            for (lap = lap_start; lap > lap_start - lines; lap--)
            {
                if (lap > 0)
                {
                    ticks_to_string(lap_times[(lap-1)%MAX_LAPS],lap,32,buf);
                    rb->lcd_puts_scroll(0, LAP_Y + lap_start - lap, buf);
                }
                else
                {
                    rb->lcd_puts(0, LAP_Y + lap_start - lap,
                                 "                  ");
                }
            }
            update_lap = false;
        }

        rb->lcd_update();

        if (! counting)
        {
            button = pluginlib_getaction(TIMEOUT_BLOCK, plugin_contexts,
                                         ARRAYLEN(plugin_contexts)); 
        }
        else
        {
            button = pluginlib_getaction(10, plugin_contexts,
                                        ARRAYLEN(plugin_contexts));

            /* Make sure that the jukebox isn't powered off
               automatically */
            rb->reset_poweroff_timer();
        }
        switch (button)
        {

            /* exit */
#ifdef STOPWATCH_RC_QUIT
            case STOPWATCH_RC_QUIT:
#endif
            case STOPWATCH_QUIT:
                  save_stopwatch();
                  done = true;
                  break;

            /* Stop/Start toggle */
            case STOPWATCH_START_STOP:
                 counting = ! counting;
                 if (counting)
                 {
                     start_at = *rb->current_tick;
                     stopwatch = prev_total + *rb->current_tick - start_at;
                 }
                 else
                 {
                     prev_total += *rb->current_tick - start_at;
                     stopwatch = prev_total;
                 }
                 break;

            /* Reset timer */
            case STOPWATCH_RESET_TIMER:
                 if (!counting)
                 {
                     prev_total = 0;
                     curr_lap = 0;
                     update_lap = true;
                 }
                 break;

            /* Lap timer */
            case STOPWATCH_LAP_TIMER:
                 /*check if we're timing, and start if not*/
                 if (counting)
                 {
                     lap_times[curr_lap%MAX_LAPS] = stopwatch;
                     curr_lap++;
                     update_lap = true;
                 }
                 else
                 {
                     counting = ! counting;
                     start_at = *rb->current_tick;
                     stopwatch = prev_total + *rb->current_tick - start_at;
                 }
                 break;

            /* Scroll Lap timer up */
            case STOPWATCH_SCROLL_UP:
            case STOPWATCH_SCROLL_UP_REPEAT:
                 if (lap_scroll > 0)
                 {
                     lap_scroll --;
                     update_lap = true;
                 }
                 break;

            /* Scroll Lap timer down */
            case STOPWATCH_SCROLL_DOWN:
            case STOPWATCH_SCROLL_DOWN_REPEAT:
                 if ((lap_scroll < curr_lap - lines) &&
                     (lap_scroll < (MAX_LAPS - lines)) )
                 {
                     lap_scroll ++;
                     update_lap = true;
                 }
                 break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
    }
    return PLUGIN_OK;
}
