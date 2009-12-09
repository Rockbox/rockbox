/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
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
#include <string.h>
#include <stdio.h>
#include "backlight.h"
#include "action.h"
#include "lcd.h"
#include "lang.h"
#include "icons.h"
#include "font.h"
#include "audio.h"
#include "mp3_playback.h"
#include "usb.h"
#include "settings.h"
#include "status.h"
#include "playlist.h"
#include "sprintf.h"
#include "kernel.h"
#include "power.h"
#include "system.h"
#include "powermgmt.h"
#include "talk.h"
#include "misc.h"
#include "metadata.h"
#include "screens.h"
#include "debug.h"
#include "led.h"
#include "sound.h"
#include "splash.h"
#include "statusbar.h"
#include "screen_access.h"
#include "pcmbuf.h"
#include "list.h"
#include "yesno.h"
#include "backdrop.h"
#include "viewport.h"
#include "language.h"

#if CONFIG_CODEC == SWCODEC
#include "dsp.h"
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
int mmc_remove_request(void)
{
    struct queue_event ev;
    int i;
    FOR_NB_SCREENS(i)
        screens[i].clear_display();
    splash(0, ID2P(LANG_REMOVE_MMC));

    while (1)
    {
        queue_wait_w_tmo(&button_queue, &ev, HZ/2);
        switch (ev.id)
        {
            case SYS_HOTSWAP_EXTRACTED:
                return SYS_HOTSWAP_EXTRACTED;

            case SYS_USB_DISCONNECTED:
                return SYS_USB_DISCONNECTED;
        }
    }
}
#endif

/* the charging screen is only used for archos targets */
#if CONFIG_CHARGING && !defined(HAVE_POWEROFF_WHILE_CHARGING) && defined(CPU_SH)

#ifdef HAVE_LCD_BITMAP
static void charging_display_info(bool animate)
{
    unsigned char charging_logo[36];
    const int pox_x = (LCD_WIDTH - sizeof(charging_logo)) / 2;
    const int pox_y = 32;
    static unsigned phase = 3;
    unsigned i;

#ifdef NEED_ATA_POWER_BATT_MEASURE
    if (ide_powered()) /* FM and V2 can only measure when ATA power is on */
#endif
    {
        int battv = battery_voltage();
        lcd_putsf(0, 7, "  Batt: %d.%02dV %d%%  ", battv / 1000,
                 (battv % 1000) / 10, battery_level());
    }

#ifdef ARCHOS_RECORDER
    lcd_puts(0, 2, "Charge mode:");

    const char *s;
    if (charge_state == CHARGING)
        s = str(LANG_BATTERY_CHARGE);
    else if (charge_state == TOPOFF)
        s = str(LANG_BATTERY_TOPOFF_CHARGE);
    else if (charge_state == TRICKLE)
        s = str(LANG_BATTERY_TRICKLE_CHARGE);
    else
        s = "not charging";

    lcd_puts(0, 3, s);
    if (!charger_enabled())
        animate = false;
#endif /* ARCHOS_RECORDER */

    /* middle part */
    memset(charging_logo+3, 0x00, 32);
    charging_logo[0] = 0x3C;
    charging_logo[1] = 0x24;
    charging_logo[2] = charging_logo[35] = 0xFF;

    if (!animate)
    {   /* draw the outline */
        /* middle part */
        lcd_mono_bitmap(charging_logo, pox_x, pox_y + 8,
                        sizeof(charging_logo), 8);
        lcd_set_drawmode(DRMODE_FG);
        /* upper line */
        charging_logo[0] = charging_logo[1] = 0x00;
        memset(charging_logo+2, 0x80, 34);
        lcd_mono_bitmap(charging_logo, pox_x, pox_y, sizeof(charging_logo), 8);
        /* lower line */
        memset(charging_logo+2, 0x01, 34);
        lcd_mono_bitmap(charging_logo, pox_x, pox_y + 16,
                        sizeof(charging_logo), 8);
        lcd_set_drawmode(DRMODE_SOLID);
    }
    else
    {   /* animate the middle part */
        for (i = 3; i<MIN(sizeof(charging_logo)-1, phase); i++)
        {
            if ((i-phase) % 8 == 0)
            {   /* draw a "bubble" here */
                unsigned bitpos;
                bitpos = (phase + i/8) % 15; /* "bounce" effect */
                if (bitpos > 7)
                    bitpos = 14 - bitpos;
                charging_logo[i] = BIT_N(bitpos);
            }
        }
        lcd_mono_bitmap(charging_logo, pox_x, pox_y + 8,
                        sizeof(charging_logo), 8);
        phase++;
    }
    lcd_update();
}
#else /* not HAVE_LCD_BITMAP */

static unsigned long logo_chars[4];
static const unsigned char logo_pattern[] = {
    0x07, 0x04, 0x1c, 0x14, 0x1c, 0x04, 0x07, 0, /* char 1 */
    0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0, /* char 2 */
    0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0, /* char 3 */
    0x1f, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1f, 0, /* char 4 */
};

static void logo_lock_patterns(bool on)
{
    int i;

    if (on)
    {
        for (i = 0; i < 4; i++)
            logo_chars[i] = lcd_get_locked_pattern();
    }
    else
    {
        for (i = 0; i < 4; i++)
            lcd_unlock_pattern(logo_chars[i]);
    }
}

static void charging_display_info(bool animate)
{
    int battv;
    unsigned i, ypos;
    static unsigned phase = 3;
    char buf[32];

    battv = battery_voltage();
    lcd_putsf(4, 1, " %d.%02dV", battv / 1000, (battv % 1000) / 10);

    memcpy(buf, logo_pattern, 32); /* copy logo patterns */

    if (!animate) /* build the screen */
    {
        lcd_double_height(false);
        lcd_puts(0, 0, "[Charging]");
        for (i = 0; i < 4; i++)
            lcd_putc(i, 1, logo_chars[i]);
    }
    else          /* animate the logo */
    {
        for (i = 3; i < MIN(19, phase); i++)
        {
            if ((i - phase) % 5 == 0)
            {    /* draw a "bubble" here */
                ypos = (phase + i/5) % 9; /* "bounce" effect */
                if (ypos > 4)
                    ypos = 8 - ypos;
                buf[5 - ypos + 8 * (i/5)] |= 0x10u >> (i%5);
            }
        }
        phase++;
    }

    for (i = 0; i < 4; i++)
        lcd_define_pattern(logo_chars[i], buf + 8 * i);

    lcd_update();
}
#endif /* (not) HAVE_LCD_BITMAP */

/* blocks while charging, returns on event:
   1 if charger cable was removed
   2 if Off/Stop key was pressed
   3 if On key was pressed
   4 if USB was connected */

int charging_screen(void)
{
    unsigned int button;
    int rc = 0;

    ide_power_enable(false); /* power down the disk, else would be spinning */

    lcd_clear_display();
    backlight_set_timeout(global_settings.backlight_timeout);
#ifdef HAVE_REMOTE_LCD
    remote_backlight_set_timeout(global_settings.remote_backlight_timeout);
#endif
    backlight_set_timeout_plugged(global_settings.backlight_timeout_plugged);

#ifdef HAVE_LCD_CHARCELLS
    logo_lock_patterns(true);
#endif
    charging_display_info(false);

    do
    {
        gui_syncstatusbar_draw(&statusbars, false);
        charging_display_info(true);
        button = get_action(CONTEXT_STD,HZ/3);
        if (button == ACTION_STD_OK)
            rc = 2;
        else if (usb_detect() == USB_INSERTED)
            rc = 3;
        else if (!charger_inserted())
            rc = 1;
    } while (!rc);

#ifdef HAVE_LCD_CHARCELLS
    logo_lock_patterns(false);
#endif
    return rc;
}
#endif /* CONFIG_CHARGING && !HAVE_POWEROFF_WHILE_CHARGING && defined(CPU_SH) */

#if CONFIG_CHARGING
void charging_splash(void)
{
    splash(2*HZ, str(LANG_BATTERY_CHARGE));
    button_clear_queue();
}
#endif


#if defined(HAVE_LCD_BITMAP) && (CONFIG_RTC != 0)

/* little helper function for voice output */
static void say_time(int cursorpos, const struct tm *tm)
{
    int value = 0;
    int unit = 0;

    if (!global_settings.talk_menu)
        return;

    switch(cursorpos)
    {
    case 0:
        value = tm->tm_hour;
        unit = UNIT_HOUR;
        break;
    case 1:
        value = tm->tm_min;
        unit = UNIT_MIN;
        break;
    case 2:
        value = tm->tm_sec;
        unit = UNIT_SEC;
        break;
    case 3:
        value = tm->tm_year + 1900;
        break;
    case 5:
        value = tm->tm_mday;
        break;
    }

    if (cursorpos == 4) /* month */
        talk_id(LANG_MONTH_JANUARY + tm->tm_mon, false);
    else
        talk_value(value, unit, false);
}


#define INDEX_X 0
#define INDEX_Y 1

#define SEPARATOR ":"

#define IDX_HOURS   0
#define IDX_MINUTES 1
#define IDX_SECONDS 2
#define IDX_YEAR    3
#define IDX_MONTH   4
#define IDX_DAY     5

#define OFF_HOURS   0
#define OFF_MINUTES 3
#define OFF_SECONDS 6
#define OFF_YEAR    9
#define OFF_DAY     14

bool set_time_screen(const char* title, struct tm *tm)
{
    bool done = false;
    int cursorpos = 0;
    unsigned int statusbar_height = 0;
    unsigned char offsets_ptr[] =
        { OFF_HOURS, OFF_MINUTES, OFF_SECONDS, OFF_YEAR, 0, OFF_DAY };

    if (lang_is_rtl())
    {
        offsets_ptr[IDX_HOURS] = OFF_SECONDS;
        offsets_ptr[IDX_SECONDS] = OFF_HOURS;
        offsets_ptr[IDX_YEAR] = OFF_DAY;
        offsets_ptr[IDX_DAY] = OFF_YEAR;
    }

    if(global_settings.statusbar)
        statusbar_height = STATUSBAR_HEIGHT;

    /* speak selection when screen is entered */
    say_time(cursorpos, tm);

    while (!done) {
        int button;
        unsigned int i, s, realyear, min, max;
        unsigned char *ptr[6];
        unsigned char buffer[20];
        int *valptr = NULL;
        static unsigned char daysinmonth[] =
            {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        /* for easy acess in the drawing loop */
        for (i = 0; i < 6; i++)
            ptr[i] = buffer + offsets_ptr[i];
        ptr[IDX_MONTH] = str(LANG_MONTH_JANUARY + tm->tm_mon); /* month name */

        /* calculate the number of days in febuary */
        realyear = tm->tm_year + 1900;
        if((realyear % 4 == 0 && !(realyear % 100 == 0)) || realyear % 400 == 0)
            daysinmonth[1] = 29;
        else
            daysinmonth[1] = 28;

        /* fix day if month or year changed */
        if (tm->tm_mday > daysinmonth[tm->tm_mon])
            tm->tm_mday = daysinmonth[tm->tm_mon];

        /* calculate day of week */
        set_day_of_week(tm);

        /* put all the numbers we want from the tm struct into
           an easily printable buffer */
        snprintf(buffer, sizeof(buffer),
                 "%02d " "%02d " "%02d " "%04d " "%02d",
                 tm->tm_hour, tm->tm_min, tm->tm_sec,
                 tm->tm_year+1900, tm->tm_mday);

        /* convert spaces in the buffer to '\0' to make it possible to work
           directly on the buffer */
        for(i=0; i < sizeof(buffer); i++)
        {
            if(buffer[i] == ' ')
                buffer[i] = '\0';
        }

        FOR_NB_SCREENS(s)
        {
            int pos, nb_lines;
            unsigned int separator_width, weekday_width;
            unsigned int j, width, prev_line_height;
            struct viewport viewports[NB_SCREENS];
            /* 6 possible cursor possitions, 2 values stored for each: x, y */
            unsigned int cursor[6][2];
            struct viewport *vp = &viewports[s];
            struct screen *screen = &screens[s];
            static unsigned char rtl_idx[] =
                { IDX_SECONDS, IDX_MINUTES, IDX_HOURS, IDX_DAY, IDX_MONTH, IDX_YEAR };

            viewport_set_defaults(vp, s);
            screen->set_viewport(vp);
            nb_lines = viewport_get_nb_lines(vp);

            /* minimum lines needed is 2 + title line */
            if (nb_lines < 4)
            {
                vp->font = FONT_SYSFIXED;
                nb_lines = viewport_get_nb_lines(vp);
            }

            /* recalculate the positions and offsets */
            if (nb_lines >= 3)
                screen->getstringsize(title, NULL, &prev_line_height);
            else
                prev_line_height = 0;

            screen->getstringsize(SEPARATOR, &separator_width, NULL);

            /* weekday */
            screen->getstringsize(str(LANG_WEEKDAY_SUNDAY + tm->tm_wday),
                                     &weekday_width, NULL);
            screen->getstringsize(" ", &separator_width, NULL);

            for(i=0, j=0; i < 6; i++)
            {
                if(i==3) /* second row */
                {
                    j = weekday_width + separator_width;
                    prev_line_height *= 2;
                }
                screen->getstringsize(ptr[i], &width, NULL);
                cursor[i][INDEX_Y] = prev_line_height + statusbar_height;
                cursor[i][INDEX_X] = j;
                j += width + separator_width;
            }

            /* draw the screen */
            screen->set_viewport(vp);
            screen->clear_viewport();
            /* display the screen title */
            screen->puts_scroll(0, 0, title);

            /* these are not selectable, so we draw them outside the loop */
            /* name of the week day */
            screen->putsxy(0, cursor[3][INDEX_Y],
                              str(LANG_WEEKDAY_SUNDAY + tm->tm_wday));

            pos = lang_is_rtl() ? rtl_idx[cursorpos] : cursorpos;
            /* draw the selected item with drawmode set to
                DRMODE_SOLID|DRMODE_INVERSEVID, all other selectable
                items with drawmode DRMODE_SOLID */
            for(i=0; i<6; i++)
            {
                if (pos == (int)i)
                    vp->drawmode = (DRMODE_SOLID|DRMODE_INVERSEVID);

                screen->putsxy(cursor[i][INDEX_X],
                                  cursor[i][INDEX_Y], ptr[i]);

                vp->drawmode = DRMODE_SOLID;

                screen->putsxy(cursor[i/4 +1][INDEX_X] - separator_width,
                                  cursor[0][INDEX_Y], SEPARATOR);
            }

            /* print help text */
            if (nb_lines > 4)
                screen->puts(0, 4, str(LANG_TIME_SET_BUTTON));
            if (nb_lines > 5)
                screen->puts(0, 5, str(LANG_TIME_REVERT));
            screen->update_viewport();
            screen->set_viewport(NULL);
        }

        /* set the most common numbers */
        min = 0;
        max = 59;
        /* calculate the minimum and maximum for the number under cursor */
        switch(cursorpos) {
            case 0: /* hour */
                max = 23;
                valptr = &tm->tm_hour;
                break;
            case 1: /* minute */
                valptr = &tm->tm_min;
                break;
            case 2: /* second */
                valptr = &tm->tm_sec;
                break;
            case 3: /* year */
                min = 1;
                max = 200;
                valptr = &tm->tm_year;
                break;
            case 4: /* month */
                max = 11;
                valptr = &tm->tm_mon;
                break;
            case 5: /* day */
                min = 1;
                max = daysinmonth[tm->tm_mon];
                valptr = &tm->tm_mday;
                break;
        }

#ifdef HAVE_TOUCHSCREEN
    enum touchscreen_mode old_mode = touchscreen_get_mode();

    touchscreen_set_mode(TOUCHSCREEN_BUTTON);
#endif
        button = get_action(CONTEXT_SETTINGS_TIME, TIMEOUT_BLOCK);
#ifdef HAVE_TOUCHSCREEN
    touchscreen_set_mode(old_mode);
#endif
        switch ( button ) {
            case ACTION_STD_PREV:
                cursorpos = clamp_value_wrap(--cursorpos, 5, 0);
                say_time(cursorpos, tm);
                break;
            case ACTION_STD_NEXT:
                cursorpos = clamp_value_wrap(++cursorpos, 5, 0);
                say_time(cursorpos, tm);
                break;
            case ACTION_SETTINGS_INC:
            case ACTION_SETTINGS_INCREPEAT:
                *valptr = clamp_value_wrap(++(*valptr), max, min);
                say_time(cursorpos, tm);
                break;
            case ACTION_SETTINGS_DEC:
            case ACTION_SETTINGS_DECREPEAT:
                *valptr = clamp_value_wrap(--(*valptr), max, min);
                say_time(cursorpos, tm);
                break;

            case ACTION_STD_OK:
                done = true;
                break;

            case ACTION_STD_CANCEL:
                done = true;
                tm->tm_year = -1;
                break;

            default:
                if (default_event_handler(button) == SYS_USB_CONNECTED)
                    return true;
                break;
        }
    }
    return false;
}
#endif /* defined(HAVE_LCD_BITMAP) && (CONFIG_RTC != 0) */

#if (CONFIG_KEYPAD == RECORDER_PAD) && !defined(HAVE_SW_POWEROFF)
bool shutdown_screen(void)
{
    int button;
    bool done = false;
    long time_entered = current_tick;

    lcd_stop_scroll();

    splash(0, str(LANG_CONFIRM_SHUTDOWN));

    while(!done && TIME_BEFORE(current_tick,time_entered+HZ*2))
    {
        button = get_action(CONTEXT_STD,HZ);
        switch(button)
        {
            case ACTION_STD_CANCEL:
                sys_poweroff();
                break;

            /* do nothing here, because ACTION_NONE might be caused
             * by timeout or button release. In case of timeout the loop
             * is terminated by TIME_BEFORE */
            case ACTION_NONE:
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return true;
                done = true;
                break;
        }
    }
    return false;
}
#endif

static const int id3_headers[]=
{
    LANG_ID3_TITLE,
    LANG_ID3_ARTIST,
    LANG_ID3_ALBUM,
    LANG_ID3_ALBUMARTIST,
    LANG_ID3_GROUPING,
    LANG_ID3_DISCNUM,
    LANG_ID3_TRACKNUM,
    LANG_ID3_COMMENT,
    LANG_ID3_GENRE,
    LANG_ID3_YEAR,
    LANG_ID3_LENGTH,
    LANG_ID3_PLAYLIST,
    LANG_ID3_BITRATE,
    LANG_ID3_FREQUENCY,
#if CONFIG_CODEC == SWCODEC
    LANG_ID3_TRACK_GAIN,
    LANG_ID3_ALBUM_GAIN,
#endif
    LANG_ID3_PATH,
};

struct id3view_info {
    struct mp3entry* id3;
    int count;
    int info_id[ARRAYLEN(id3_headers)];
};

static const char* id3_get_info(int selected_item, void* data,
                                char *buffer, size_t buffer_len)
{
    struct id3view_info *info = (struct id3view_info*)data;
    struct mp3entry* id3 =info->id3;
    int info_no=selected_item/2;
    if(!(selected_item%2))
    {/* header */
        snprintf(buffer, buffer_len,
                 "[%s]", str(id3_headers[info->info_id[info_no]]));
        return buffer;
    }
    else
    {/* data */

        char * val=NULL;
        switch(info->info_id[info_no])
        {
            case 0:/*LANG_ID3_TITLE*/
                val=id3->title;
                break;
            case 1:/*LANG_ID3_ARTIST*/
                val=id3->artist;
                break;
            case 2:/*LANG_ID3_ALBUM*/
                val=id3->album;
                break;
            case 3:/*LANG_ID3_ALBUMARTIST*/
                val=id3->albumartist;
                break;
            case 4:/*LANG_ID3_GROUPING*/
                val=id3->grouping;
                break;
            case 5:/*LANG_ID3_DISCNUM*/
                if (id3->disc_string)
                    val = id3->disc_string;
                else if (id3->discnum)
                {
                    snprintf(buffer, buffer_len, "%d", id3->discnum);
                    val = buffer;
                }
                break;
            case 6:/*LANG_ID3_TRACKNUM*/
                if (id3->track_string)
                    val = id3->track_string;
                else if (id3->tracknum)
                {
                    snprintf(buffer, buffer_len, "%d", id3->tracknum);
                    val = buffer;
                }
                break;
            case 7:/*LANG_ID3_COMMENT*/
                val=id3->comment;
                break;
            case 8:/*LANG_ID3_GENRE*/
                val = id3->genre_string;
                break;
            case 9:/*LANG_ID3_YEAR*/
                if (id3->year_string)
                    val = id3->year_string;
                else if (id3->year)
                {
                    snprintf(buffer, buffer_len, "%d", id3->year);
                    val = buffer;
                }
                break;
            case 10:/*LANG_ID3_LENGTH*/
                format_time(buffer, buffer_len, id3->length);
                val=buffer;
                break;
            case 11:/*LANG_ID3_PLAYLIST*/
                snprintf(buffer, buffer_len, "%d/%d",
                         playlist_get_display_index(), playlist_amount());
                val=buffer;
                break;
            case 12:/*LANG_ID3_BITRATE*/
                snprintf(buffer, buffer_len, "%d kbps%s", id3->bitrate,
            id3->vbr ? str(LANG_ID3_VBR) : (const unsigned char*) "");
                val=buffer;
                break;
            case 13:/*LANG_ID3_FREQUENCY*/
                snprintf(buffer, buffer_len, "%ld Hz", id3->frequency);
                val=buffer;
                break;
#if CONFIG_CODEC == SWCODEC
            case 14:/*LANG_ID3_TRACK_GAIN*/
                val=id3->track_gain_string;
                break;
            case 15:/*LANG_ID3_ALBUM_GAIN*/
                val=id3->album_gain_string;
                break;
            case 16:/*LANG_ID3_PATH*/
#else
            case 14:/*LANG_ID3_PATH*/
#endif
                val=id3->path;
                break;
        }
        return val && *val ? val : NULL;
    }
}

bool browse_id3(void)
{
    struct gui_synclist id3_lists;
    struct mp3entry* id3 = audio_current_track();
    int key;
    unsigned int i;
    struct id3view_info info;
    info.count = 0;
    info.id3 = id3;
    for (i = 0; i < ARRAYLEN(id3_headers); i++)
    {
        char temp[8];
        info.info_id[i] = i;
        if (id3_get_info((i*2)+1, &info, temp, 8) != NULL)
            info.info_id[info.count++] = i;
    }

    gui_synclist_init(&id3_lists, &id3_get_info, &info, true, 2, NULL);
    gui_synclist_set_nb_items(&id3_lists, info.count*2);
    gui_synclist_draw(&id3_lists);
    while (true) {
        key = get_action(CONTEXT_LIST,HZ/2);
        if(key!=ACTION_NONE && key!=ACTION_UNKNOWN
        && !gui_synclist_do_button(&id3_lists, &key,LIST_WRAP_UNLESS_HELD))
        {
            return(default_event_handler(key) == SYS_USB_CONNECTED);
        }
    }
}

static const char* runtime_get_data(int selected_item, void* data,
                                    char* buffer, size_t buffer_len)
{
    (void)data;
    int t;
    switch (selected_item)
    {
        case 0: return str(LANG_RUNNING_TIME);
        case 1: t = global_status.runtime;      break;
        case 2: return str(LANG_TOP_TIME);
        case 3: t = global_status.topruntime;   break;
        default:
            return "";
    }

    snprintf(buffer, buffer_len, "%dh %dm %ds",
        t / 3600, (t % 3600) / 60, t % 60);
    return buffer;
}

static int runtime_speak_data(int selected_item, void* data)
{
    (void) data;
    talk_ids(false,
             (selected_item < 2) ? LANG_RUNNING_TIME : LANG_TOP_TIME,
             TALK_ID((selected_item < 2) ? global_status.runtime
                     : global_status.topruntime, UNIT_TIME));
    return 0;
}


bool view_runtime(void)
{
    static const char *lines[]={ID2P(LANG_CLEAR_TIME)};
    static const struct text_message message={lines, 1};

    struct gui_synclist lists;
    int action;
    gui_synclist_init(&lists, runtime_get_data, NULL, false, 2, NULL);
#if !defined(HAVE_LCD_CHARCELLS)
    gui_synclist_set_title(&lists, str(LANG_RUNNING_TIME), NOICON);
#else
    gui_synclist_set_title(&lists, NULL, NOICON);
#endif
    if(global_settings.talk_menu)
        gui_synclist_set_voice_callback(&lists, runtime_speak_data);
    gui_synclist_set_icon_callback(&lists, NULL);
    gui_synclist_set_nb_items(&lists, 4);
    gui_synclist_speak_item(&lists);
    while(1)
    {
#if CONFIG_CHARGING
        if (charger_inserted())
        {
            global_status.runtime = 0;
        }
        else
#endif
        {
            global_status.runtime += ((current_tick - lasttime) / HZ);
        }
        lasttime = current_tick;
        gui_synclist_draw(&lists);
        list_do_action(CONTEXT_STD, HZ,
                    &lists, &action, LIST_WRAP_UNLESS_HELD);
        if(action == ACTION_STD_CANCEL)
            break;
        if(action == ACTION_STD_OK) {
            if(gui_syncyesno_run(&message, NULL, NULL)==YESNO_YES)
            {
                if (!(gui_synclist_get_sel_pos(&lists)/2))
                    global_status.runtime = 0;
                else
                    global_status.topruntime = 0;
                gui_synclist_speak_item(&lists);
            }
        }
        if(default_event_handler(action) == SYS_USB_CONNECTED)
            return true;
    }
    return false;
}

#ifdef HAVE_TOUCHSCREEN
static int get_sample(struct touchscreen_calibration *cal, int x, int y, int i,
                      struct screen* screen)
{
    int action;
    short ts_x, ts_y;

    /* Draw a cross */
    screen->drawline(x - 10, y, x - 2, y);
    screen->drawline(x + 2, y, x + 10, y);
    screen->drawline(x, y - 10, x, y - 2);
    screen->drawline(x, y + 2, x, y + 10);
    screen->update();

    /* Wait for a touchscreen press */
    while(true)
    {
        action = get_action(CONTEXT_STD, TIMEOUT_BLOCK);
        if(action == ACTION_TOUCHSCREEN)
        {
            if(action_get_touchscreen_press(&ts_x, &ts_y) == BUTTON_REL)
                break;
        }
        else if(action == ACTION_STD_CANCEL)
            return -1;
    }

    cal->x[i][0] = ts_x;
    cal->y[i][0] = ts_y;
    cal->x[i][1] = x;
    cal->y[i][1] = y;

    return 0;
}


int calibrate(void)
{
    short points[3][2] = {
        {LCD_WIDTH/10, LCD_HEIGHT/10},
        {7*LCD_WIDTH/8, LCD_HEIGHT/2},
        {LCD_WIDTH/2, 7*LCD_HEIGHT/8}
    };
    struct screen* screen = &screens[SCREEN_MAIN];
    enum touchscreen_mode old_mode = touchscreen_get_mode();
    struct touchscreen_calibration cal;
    int i, ret = 0;
    
    /* hide the statusbar */
    viewportmanager_theme_enable(SCREEN_MAIN, false, NULL);

    touchscreen_disable_mapping(); /* set raw mode */
    touchscreen_set_mode(TOUCHSCREEN_POINT);
    
    for(i=0; i<3; i++)
    {
        screen->clear_display();

        if(get_sample(&cal, points[i][0], points[i][1], i, screen))
        {
            ret = -1;
            break;
        }
    }

    if(ret == 0)
        touchscreen_calibrate(&cal);
    else
        touchscreen_reset_mapping();

    memcpy(&global_settings.ts_calibration_data, &calibration_parameters,
        sizeof(struct touchscreen_parameter));

    touchscreen_set_mode(old_mode);
    viewportmanager_theme_undo(SCREEN_MAIN);

    settings_save();
    return ret;
}

int reset_mapping(void)
{
    touchscreen_reset_mapping();

    memcpy(&global_settings.ts_calibration_data, &calibration_parameters,
        sizeof(struct touchscreen_parameter));

    settings_save();
    return 0;
}
#endif
