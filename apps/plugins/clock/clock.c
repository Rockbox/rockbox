/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Kévin Ferrare, graphic design 2003 Zakk Roberts
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
#include "time.h"
#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"
#include "lib/xlcd.h"

#include "clock.h"
#include "clock_counter.h"
#include "clock_draw.h"
#include "clock_menu.h"
#include "clock_settings.h"



/* Keymaps */
const struct button_mapping* plugin_contexts[]={
    pla_main_ctx,
#ifdef HAVE_REMOTE_LCD
    pla_remote_ctx,
#endif
};
#define PLA_ARRAY_COUNT sizeof(plugin_contexts)/sizeof(plugin_contexts[0])

#define ACTION_COUNTER_TOGGLE           PLA_SELECT
#define ACTION_COUNTER_RESET            PLA_SELECT_REPEAT
#define ACTION_MENU                     PLA_CANCEL
#define ACTION_MODE_NEXT                PLA_RIGHT
#define ACTION_MODE_NEXT_REPEAT         PLA_RIGHT_REPEAT
#define ACTION_MODE_PREV                PLA_LEFT
#define ACTION_MODE_PREV_REPEAT         PLA_LEFT_REPEAT
#define ACTION_SKIN_NEXT                PLA_UP
#define ACTION_SKIN_NEXT_REPEAT         PLA_UP_REPEAT
#define ACTION_SKIN_PREV                PLA_DOWN
#define ACTION_SKIN_PREV_REPEAT         PLA_DOWN_REPEAT

/**************************
 * Cleanup on plugin return
 *************************/
static void cleanup(void)
{
    clock_draw_restore_colors();
    if(clock_settings.general.save_settings == 1)
        save_settings();

    /* restore set backlight timeout */
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
}

/* puts the current time into the time struct */
static void clock_update_time( struct time* time){
    struct tm* current_time = rb->get_time();
    time->hour = current_time->tm_hour;
    time->minute = current_time->tm_min;
    time->second = current_time->tm_sec;

    /*********************
        * Date info
        *********************/
    time->year = current_time->tm_year + 1900;
    time->day = current_time->tm_mday;
    time->month = current_time->tm_mon + 1;

}

void format_date(char* buffer, struct time* time, enum date_format format){
    switch(format){
        case JAPANESE:
            rb->snprintf(buffer, 20, "%04d/%02d/%02d",
                         time->year, time->month, time->day);
            break;
        case EUROPEAN:
            rb->snprintf(buffer, 20, "%02d/%02d/%04d",
                         time->day, time->month, time->year);
            break;
        case ENGLISH:
            rb->snprintf(buffer, 20, "%02d/%02d/%04d",
                         time->month, time->day, time->year);
            break;
        case NONE:
        default:
            break;
    }
}

/**********************************************************************
 * Plugin starts here
 **********************************************************************/
enum plugin_status plugin_start(const void* parameter){
    int button;
    int last_second = -1;
    bool redraw=true;
    struct time time;
    struct counter counter;
    bool exit_clock = false;
    (void)parameter;
    atexit(cleanup);

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif

    load_settings();

    /* init xlcd functions */
    counter_init(&counter);
    clock_draw_set_colors();

    while(!exit_clock){
        clock_update_time(&time);

        if(!clock_settings.general.idle_poweroff)
            rb->reset_poweroff_timer();

        /*************************
         * Scan for button presses
         ************************/
        button =  pluginlib_getaction(HZ/10, plugin_contexts, PLA_ARRAY_COUNT);
        redraw=true;/* we'll set it to false afterwards if there was no action */
        switch (button){
            case ACTION_COUNTER_TOGGLE: /* start/stop counter */
                if(clock_settings.general.show_counter)
                    counter_toggle(&counter);
                break;

            case ACTION_COUNTER_RESET: /* reset counter */
                if(clock_settings.general.show_counter)
                    counter_reset(&counter);
                break;

            case ACTION_MODE_NEXT_REPEAT:
            case ACTION_MODE_NEXT:
                clock_settings.mode++;
                if(clock_settings.mode >= NB_CLOCK_MODES)
                    clock_settings.mode = 0;
                break;

            case ACTION_MODE_PREV_REPEAT:
            case ACTION_MODE_PREV:
                clock_settings.mode--;
                if(clock_settings.mode < 0)
                    clock_settings.mode = NB_CLOCK_MODES-1;
                break;
            case ACTION_SKIN_PREV_REPEAT:
            case ACTION_SKIN_PREV:
                clock_settings_skin_next(&clock_settings);
                break;
            case ACTION_SKIN_NEXT_REPEAT:
            case ACTION_SKIN_NEXT:
                clock_settings_skin_previous(&clock_settings);
                break;
            case ACTION_MENU:
                clock_draw_restore_colors();
                exit_clock=main_menu();
                break;
            default:
                exit_on_usb(button);
                if(time.second != last_second){
                    last_second=time.second;
                    redraw=true;
                }else
                    redraw=false;
                break;
        }

        if(redraw){
            clock_draw_set_colors();
            FOR_NB_SCREENS(i)
                clock_draw(rb->screens[i], &time, &counter);
            redraw=false;
        }
    }

    return PLUGIN_OK;
}
