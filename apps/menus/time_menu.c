/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: main_menu.c 17985 2008-07-08 02:30:58Z jdgordon $
 *
 * Copyright (C) 2007 Jonathan Gordon
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

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include "config.h"
#include "string.h"
#include "sprintf.h"
#include "lang.h"
#include "action.h"
#include "settings.h"
#include "powermgmt.h"
#include "menu.h"
#include "misc.h"
#include "exported_menus.h"
#include "yesno.h"
#include "keyboard.h"
#include "talk.h"
#include "splash.h"
#include "version.h"
#include "time.h"
#include "viewport.h"
#include "list.h"
#include "alarm_menu.h"
#include "screens.h"
#include "radio.h"

static int timedate_set(void)
{
    struct tm tm;
    int result;

    /* Make a local copy of the time struct */
    memcpy(&tm, get_time(), sizeof(struct tm));

    /* do some range checks */
    /* This prevents problems with time/date setting after a power loss */
    if (!valid_time(&tm))
    {
/* Macros to convert a 2-digit string to a decimal constant. 
        (YEAR), MONTH and DAY are set by the date command, which outputs
        DAY as 00..31 and MONTH as 01..12. The leading zero would lead to
        misinterpretation as an octal constant. */
#define S100(x) 1 ## x
#define C2DIG2DEC(x) (S100(x)-100)

        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        tm.tm_mday = C2DIG2DEC(DAY);
        tm.tm_mon =  C2DIG2DEC(MONTH)-1;
        tm.tm_wday = 1;
        tm.tm_year = YEAR-1900;
    }

    result = (int)set_time_screen(str(LANG_SET_TIME), &tm);

    if(tm.tm_year != -1) {
        set_time(&tm);
    }
    return result;
}

MENUITEM_FUNCTION(time_set, 0, ID2P(LANG_SET_TIME), 
                  timedate_set, NULL, NULL, Icon_NOICON);
MENUITEM_SETTING(timeformat, &global_settings.timeformat, NULL);

/* in main_menu.c */
extern const struct menu_item_ex sleep_timer_call;

#ifdef HAVE_RTC_ALARM
MENUITEM_FUNCTION(alarm_screen_call, 0, ID2P(LANG_ALARM_MOD_ALARM_MENU),
                  (menu_function)alarm_screen, NULL, NULL, Icon_NOICON);
#if CONFIG_TUNER || defined(HAVE_RECORDING)

#if CONFIG_TUNER && !defined(HAVE_RECORDING)
/* This need only be shown if we dont have recording, because if we do
   then always show the setting item, because there will always be at least
   2 items */
static int alarm_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_REQUEST_MENUITEM:
            if (radio_hardware_present() == 0)
                return ACTION_EXIT_MENUITEM;
            break;
    }
    return action;
}
#else
#define alarm_callback NULL
#endif /* CONFIG_TUNER && !HAVE_RECORDING */
/* have to do this manually because the setting screen
   doesnt handle variable item count */
static int alarm_setting(void)
{
    struct opt_items items[ALARM_START_COUNT];
    int i = 0;
    items[i].string = str(LANG_RESUME_PLAYBACK);
    items[i].voice_id = LANG_RESUME_PLAYBACK;
    i++;
#if CONFIG_TUNER
    if (radio_hardware_present())
    {
        items[i].string = str(LANG_FM_RADIO);
        items[i].voice_id = LANG_FM_RADIO;
        i++;
    }
#endif
#ifdef HAVE_RECORDING
    items[i].string = str(LANG_RECORDING);
    items[i].voice_id = LANG_RECORDING;
    i++;
#endif
    return set_option(str(LANG_ALARM_WAKEUP_SCREEN),
                      &global_settings.alarm_wake_up_screen, 
                      INT, items, i, NULL);
}

MENUITEM_FUNCTION(alarm_wake_up_screen, 0, ID2P(LANG_ALARM_WAKEUP_SCREEN),
                  alarm_setting, NULL, alarm_callback, Icon_Menu_setting);
#endif /* CONFIG_TUNER || defined(HAVE_RECORDING) */

#endif /* HAVE_RTC_ALARM */
static void talk_timedate(void)
{
    struct tm *tm = get_time();
    if (!global_settings.talk_menu)
        return;
    talk_id(VOICE_CURRENT_TIME, false);
    if (valid_time(tm))
    {
        talk_time(tm, true);
        talk_date(get_time(), true);
    }
    else
    {
        talk_id(LANG_UNKNOWN, true);
    }
}

static void draw_timedate(struct viewport *vp, struct screen *display)
{
    struct tm *tm = get_time();
    int w, line;
    char time[16], date[16];
    if (vp->height == 0)
        return;
    display->set_viewport(vp);
    display->clear_viewport();
    if (viewport_get_nb_lines(vp) > 3)
        line = 1;
    else
        line = 0;
    
    if (valid_time(tm))
    {
        snprintf(time, 16, "%02d:%02d:%02d %s", 
                global_settings.timeformat == 0 ? tm->tm_hour :
                        ((tm->tm_hour + 11) % 12) + 1,
                            tm->tm_min, 
                            tm->tm_sec, 
                        global_settings.timeformat == 0 ? "" :
                                tm->tm_hour>11 ? "P" : "A");
        snprintf(date, 16, "%s %d %d", 
                str(LANG_MONTH_JANUARY + tm->tm_mon),
                    tm->tm_mday,
                    tm->tm_year+1900);
    }
    else
    {
        snprintf(time, 16, "%s", "--:--:--");
        snprintf(date, 16, "%s", str(LANG_UNKNOWN));
    }
    display->getstringsize(time, &w, NULL);
    if (w > vp->width)
        display->puts_scroll(0, line, time);
    else
        display->putsxy((vp->width - w)/2, line*font_get(vp->font)->height, time);
    line++;
    
    display->getstringsize(date, &w, NULL);
    if (w > vp->width)
        display->puts_scroll(0, line, date);
    else
        display->putsxy((vp->width - w)/2, line*font_get(vp->font)->height, date);
    display->update_viewport();
}

static struct viewport clock[NB_SCREENS], menu[NB_SCREENS];
static bool menu_was_pressed;
static int time_menu_callback(int action,
                       const struct menu_item_ex *this_item)
{
    (void)this_item;
    int i;
    static int last_redraw = 0;
    bool redraw = false;
    
    if (TIME_BEFORE(last_redraw+HZ/2, current_tick))
        redraw = true;
    switch (action)
    {
        case ACTION_REDRAW:
            redraw = true;
            break;
        case ACTION_STD_CONTEXT:
            talk_timedate();
            action = ACTION_NONE;
            break;
        /* need to tell do_menu() to return, but then get time_screen()
           to return 0! ACTION_STD_MENU will return GO_TO_PREVIOUS from here
           so check do_menu()'s return val and menu_was_pressed */
        case ACTION_STD_MENU:
            menu_was_pressed = true;
            break;
    }
    if (redraw)
    {
        last_redraw = current_tick;
        FOR_NB_SCREENS(i)
            draw_timedate(&clock[i], &screens[i]);
    }
    return action;
}
            

MAKE_MENU(time_menu, ID2P(LANG_TIME_MENU), time_menu_callback, Icon_NOICON,
          &time_set, &sleep_timer_call,
#ifdef HAVE_RTC_ALARM
          &alarm_screen_call,
#if defined(HAVE_RECORDING) || CONFIG_TUNER
          &alarm_wake_up_screen,
#endif
#endif
          &timeformat);
            
int time_screen(void* ignored)
{
    (void)ignored;
    int i, nb_lines, font_h, ret;
    menu_was_pressed = false;

    FOR_NB_SCREENS(i)
    {
        viewport_set_defaults(&clock[i], i);
#ifdef HAVE_BUTTONBAR
        if (global_settings.buttonbar)
        {
            clock[i].height -= BUTTONBAR_HEIGHT;
        }
#endif
        nb_lines = viewport_get_nb_lines(&clock[i]);
        menu[i] = clock[i];
        font_h = font_get(clock[i].font)->height;
        if (nb_lines > 3)
        {
            if (nb_lines >= 5)
            {
                clock[i].height = 3*font_h;
                if (nb_lines > 5)
                    clock[i].height += font_h;
            }
            else
            {
                clock[i].height = 2*font_h;
            }
        }
        else /* disable the clock drawing */
            clock[i].height = 0;
        menu[i].y = clock[i].y + clock[i].height;
        menu[i].height = screens[i].lcdheight - menu[i].y;
#ifdef HAVE_BUTTONBAR
        if (global_settings.buttonbar)
            menu[i].height -= BUTTONBAR_HEIGHT;
#endif
        draw_timedate(&clock[i], &screens[i]);
    }
    ret = do_menu(&time_menu, NULL, menu, false);
    /* see comments above in the button callback */
    if (!menu_was_pressed && ret == GO_TO_PREVIOUS)
        return 0;
    return ret;
}
