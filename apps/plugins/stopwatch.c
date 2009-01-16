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

PLUGIN_HEADER

#ifdef HAVE_LCD_BITMAP
#define TIMER_Y 1
#else
#define TIMER_Y 0
#endif

#define LAP_Y TIMER_Y+1
#define MAX_LAPS 64

/* FIXME: Use PLUGIN_APPS_DIR */
#define STOPWATCH_FILE ROCKBOX_DIR "/apps/stopwatch.dat"

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define STOPWATCH_QUIT BUTTON_OFF
#define STOPWATCH_START_STOP BUTTON_PLAY
#define STOPWATCH_RESET_TIMER BUTTON_LEFT
#define STOPWATCH_LAP_TIMER BUTTON_ON
#define STOPWATCH_SCROLL_UP BUTTON_UP
#define STOPWATCH_SCROLL_DOWN BUTTON_DOWN
#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define STOPWATCH_QUIT BUTTON_OFF
#define STOPWATCH_START_STOP BUTTON_SELECT
#define STOPWATCH_RESET_TIMER BUTTON_LEFT
#define STOPWATCH_LAP_TIMER BUTTON_ON
#define STOPWATCH_SCROLL_UP BUTTON_UP
#define STOPWATCH_SCROLL_DOWN BUTTON_DOWN
#elif CONFIG_KEYPAD == ONDIO_PAD
#define STOPWATCH_QUIT BUTTON_OFF
#define STOPWATCH_START_STOP BUTTON_RIGHT
#define STOPWATCH_RESET_TIMER BUTTON_LEFT
#define STOPWATCH_LAP_TIMER BUTTON_MENU
#define STOPWATCH_SCROLL_UP BUTTON_UP
#define STOPWATCH_SCROLL_DOWN BUTTON_DOWN
#elif CONFIG_KEYPAD == PLAYER_PAD
#define STOPWATCH_QUIT BUTTON_MENU
#define STOPWATCH_START_STOP BUTTON_PLAY
#define STOPWATCH_RESET_TIMER BUTTON_STOP
#define STOPWATCH_LAP_TIMER BUTTON_ON
#define STOPWATCH_SCROLL_UP BUTTON_RIGHT
#define STOPWATCH_SCROLL_DOWN BUTTON_LEFT
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define STOPWATCH_QUIT BUTTON_OFF
#define STOPWATCH_START_STOP BUTTON_SELECT
#define STOPWATCH_RESET_TIMER BUTTON_DOWN
#define STOPWATCH_LAP_TIMER BUTTON_ON
#define STOPWATCH_SCROLL_UP BUTTON_RIGHT
#define STOPWATCH_SCROLL_DOWN BUTTON_LEFT

#define STOPWATCH_RC_QUIT BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define STOPWATCH_QUIT BUTTON_MENU
#define STOPWATCH_START_STOP BUTTON_SELECT
#define STOPWATCH_RESET_TIMER BUTTON_LEFT
#define STOPWATCH_LAP_TIMER BUTTON_RIGHT
#define STOPWATCH_SCROLL_UP BUTTON_SCROLL_FWD
#define STOPWATCH_SCROLL_DOWN BUTTON_SCROLL_BACK
#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define STOPWATCH_QUIT BUTTON_PLAY
#define STOPWATCH_START_STOP BUTTON_MODE
#define STOPWATCH_RESET_TIMER BUTTON_EQ
#define STOPWATCH_LAP_TIMER BUTTON_SELECT
#define STOPWATCH_SCROLL_UP BUTTON_UP
#define STOPWATCH_SCROLL_DOWN BUTTON_DOWN
#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define STOPWATCH_QUIT BUTTON_POWER
#define STOPWATCH_START_STOP BUTTON_PLAY
#define STOPWATCH_RESET_TIMER BUTTON_REC
#define STOPWATCH_LAP_TIMER BUTTON_SELECT
#define STOPWATCH_SCROLL_UP BUTTON_UP
#define STOPWATCH_SCROLL_DOWN BUTTON_DOWN
#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define STOPWATCH_QUIT BUTTON_POWER
#define STOPWATCH_START_STOP BUTTON_SELECT
#define STOPWATCH_RESET_TIMER BUTTON_A
#define STOPWATCH_LAP_TIMER BUTTON_MENU
#define STOPWATCH_SCROLL_UP BUTTON_UP
#define STOPWATCH_SCROLL_DOWN BUTTON_DOWN

/* FIXME: e200 could use scrollwheel */
#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
(CONFIG_KEYPAD == SANSA_C200_PAD) || \
(CONFIG_KEYPAD == SANSA_CLIP_PAD) || \
(CONFIG_KEYPAD == SANSA_M200_PAD)
#define STOPWATCH_QUIT BUTTON_POWER
#define STOPWATCH_START_STOP BUTTON_RIGHT
#define STOPWATCH_RESET_TIMER BUTTON_LEFT
#define STOPWATCH_LAP_TIMER BUTTON_SELECT
#define STOPWATCH_SCROLL_UP BUTTON_UP
#define STOPWATCH_SCROLL_DOWN BUTTON_DOWN

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define STOPWATCH_QUIT BUTTON_POWER
#define STOPWATCH_START_STOP BUTTON_RIGHT
#define STOPWATCH_RESET_TIMER BUTTON_LEFT
#define STOPWATCH_LAP_TIMER BUTTON_SELECT
/* FIXME: ipods scroll other way around, investigate */
#define STOPWATCH_SCROLL_UP BUTTON_SCROLL_BACK
#define STOPWATCH_SCROLL_DOWN BUTTON_SCROLL_FWD

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define STOPWATCH_QUIT BUTTON_POWER
#define STOPWATCH_START_STOP BUTTON_PLAY
#define STOPWATCH_RESET_TIMER BUTTON_REW
#define STOPWATCH_LAP_TIMER BUTTON_FF
#define STOPWATCH_SCROLL_UP BUTTON_SCROLL_UP
#define STOPWATCH_SCROLL_DOWN BUTTON_SCROLL_DOWN
#elif CONFIG_KEYPAD == MROBE500_PAD
#define STOPWATCH_QUIT BUTTON_POWER
#define STOPWATCH_START_STOP BUTTON_RC_HEART
#define STOPWATCH_RESET_TIMER BUTTON_RC_MODE
#define STOPWATCH_LAP_TIMER BUTTON_RC_PLAY
#define STOPWATCH_SCROLL_UP BUTTON_RIGHT
#define STOPWATCH_SCROLL_DOWN BUTTON_LEFT
#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define STOPWATCH_QUIT BUTTON_BACK
#define STOPWATCH_START_STOP BUTTON_PLAY
#define STOPWATCH_RESET_TIMER BUTTON_MENU
#define STOPWATCH_LAP_TIMER BUTTON_SELECT
#define STOPWATCH_SCROLL_UP BUTTON_UP
#define STOPWATCH_SCROLL_DOWN BUTTON_DOWN
#elif CONFIG_KEYPAD == MROBE100_PAD
#define STOPWATCH_QUIT BUTTON_POWER
#define STOPWATCH_START_STOP BUTTON_SELECT
#define STOPWATCH_RESET_TIMER BUTTON_DISPLAY
#define STOPWATCH_LAP_TIMER BUTTON_MENU
#define STOPWATCH_SCROLL_UP BUTTON_UP
#define STOPWATCH_SCROLL_DOWN BUTTON_DOWN
#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define STOPWATCH_QUIT BUTTON_RC_REC
#define STOPWATCH_START_STOP BUTTON_RC_PLAY
#define STOPWATCH_RESET_TIMER BUTTON_RC_REW
#define STOPWATCH_LAP_TIMER BUTTON_RC_FF
#define STOPWATCH_SCROLL_UP BUTTON_RC_VOL_UP
#define STOPWATCH_SCROLL_DOWN BUTTON_RC_VOL_DOWN
#define STOPWATCH_RC_QUIT BUTTON_REC
#elif CONFIG_KEYPAD == COWOND2_PAD
#define STOPWATCH_QUIT BUTTON_POWER
#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define STOPWATCH_QUIT BUTTON_MENU
#define STOPWATCH_START_STOP BUTTON_PLAY
#define STOPWATCH_RESET_TIMER BUTTON_STOP
#define STOPWATCH_LAP_TIMER BUTTON_LEFT
#define STOPWATCH_SCROLL_UP BUTTON_VOLUP
#define STOPWATCH_SCROLL_DOWN BUTTON_VOLDOWN
#define STOPWATCH_RC_QUIT BUTTON_POWER
#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define STOPWATCH_QUIT BUTTON_BACK
#define STOPWATCH_START_STOP BUTTON_PLAY
#define STOPWATCH_RESET_TIMER BUTTON_SELECT
#define STOPWATCH_LAP_TIMER BUTTON_CUSTOM
#define STOPWATCH_SCROLL_UP BUTTON_UP
#define STOPWATCH_SCROLL_DOWN BUTTON_DOWN
#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef STOPWATCH_QUIT
#define STOPWATCH_QUIT        BUTTON_TOPLEFT
#endif
#ifndef STOPWATCH_START_STOP
#define STOPWATCH_START_STOP  BUTTON_CENTER
#endif
#ifndef STOPWATCH_RESET_TIMER
#define STOPWATCH_RESET_TIMER BUTTON_MIDRIGHT
#endif
#ifndef STOPWATCH_LAP_TIMER
#define STOPWATCH_LAP_TIMER   BUTTON_MIDLEFT
#endif
#ifndef STOPWATCH_SCROLL_UP
#define STOPWATCH_SCROLL_UP   BUTTON_TOPMIDDLE
#endif
#ifndef STOPWATCH_SCROLL_DOWN
#define STOPWATCH_SCROLL_DOWN BUTTON_BOTTOMMIDDLE
#endif
#endif

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
void load_stopwatch(void)
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
void save_stopwatch(void)
{
    int fd;
    
    fd = rb->open(STOPWATCH_FILE, O_CREAT|O_WRONLY|O_TRUNC);
    
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
            button = rb->button_get(true);
        }
        else
        {
            button = rb->button_get_w_tmo(10);

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
                 lap_times[curr_lap%MAX_LAPS] = stopwatch;
                 curr_lap++;
                 update_lap = true;
                 break;

            /* Scroll Lap timer up */
            case STOPWATCH_SCROLL_UP:
                 if (lap_scroll > 0)
                 {
                     lap_scroll --;
                     update_lap = true;
                 }
                 break;

            /* Scroll Lap timer down */
            case STOPWATCH_SCROLL_DOWN:
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
