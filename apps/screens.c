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
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
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
#include "adc.h"
#include "action.h"
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

#ifdef HAVE_LCD_BITMAP
#include <bitmaps/usblogo.h>
#endif

#ifdef HAVE_REMOTE_LCD
#include <bitmaps/remote_usblogo.h>
#endif

#ifdef HAVE_MMC
#include "ata_mmc.h"
#endif
#if CONFIG_CODEC == SWCODEC
#include "dsp.h"
#endif

#ifdef HAVE_LCD_BITMAP
#define SCROLLBAR_WIDTH  6
#endif

/* only used in set_time screen */
#if defined(HAVE_LCD_BITMAP) && (CONFIG_RTC != 0)
static int clamp_value_wrap(int value, int max, int min)
{
    if (value > max)
        return min;
    if (value < min)
        return max;
    return value;
}
#endif

void usb_screen(void)
{
#ifdef USB_NONE
    /* nothing here! */
#else
    int i;
    bool statusbar = global_settings.statusbar; /* force the statusbar */
    global_settings.statusbar = true;
#if LCD_DEPTH > 1
    show_main_backdrop();
#endif
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
    show_remote_main_backdrop();
#endif
    FOR_NB_SCREENS(i)
    {
        screens[i].backlight_on();
        screens[i].clear_display();
#if NB_SCREENS > 1
        if (i == SCREEN_REMOTE)
        {
            screens[i].bitmap(remote_usblogo,
                      (LCD_REMOTE_WIDTH-BMPWIDTH_remote_usblogo),
                      (LCD_REMOTE_HEIGHT-BMPHEIGHT_remote_usblogo)/2,
                      BMPWIDTH_remote_usblogo, BMPHEIGHT_remote_usblogo);
        }
        else 
        {
#endif
#ifdef HAVE_LCD_BITMAP
            screens[i].transparent_bitmap(usblogo, 
                        (LCD_WIDTH-BMPWIDTH_usblogo),
                        (LCD_HEIGHT-BMPHEIGHT_usblogo)/2,
                         BMPWIDTH_usblogo, BMPHEIGHT_usblogo);
#else
            screens[i].double_height(false);
            screens[i].puts_scroll(0, 0, "[USB Mode]");
            status_set_param(false);
            status_set_audio(false);
            status_set_usb(true);
#endif /* HAVE_LCD_BITMAP */
#if NB_SCREENS > 1
        }
#endif
        screens[i].update();
    }

    gui_syncstatusbar_draw(&statusbars, true);
#ifdef SIMULATOR
    while (button_get(true) & BUTTON_REL);
#else
    usb_acknowledge(SYS_USB_CONNECTED_ACK);
    while(usb_wait_for_disconnect_w_tmo(&button_queue, HZ)) {
        if(usb_inserted()) {
#ifdef HAVE_MMC /* USB-MMC bridge can report activity */
            led(mmc_usb_active(HZ));
#endif /* HAVE_MMC */
            gui_syncstatusbar_draw(&statusbars, false);
        }
    }
#endif /* SIMULATOR */
#ifdef HAVE_LCD_CHARCELLS
    status_set_usb(false);
#endif /* HAVE_LCD_CHARCELLS */
    FOR_NB_SCREENS(i)
        screens[i].backlight_on();
    global_settings.statusbar = statusbar;
#endif /* USB_NONE */
}

#ifdef HAVE_MMC
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
    char buf[32];
    (void)buf;

#ifdef NEED_ATA_POWER_BATT_MEASURE
    if (ide_powered()) /* FM and V2 can only measure when ATA power is on */
#endif
    {
        int battv = battery_voltage();
        snprintf(buf, 32, "  Batt: %d.%02dV %d%%  ", battv / 1000,
                 (battv % 1000) / 10, battery_level());
        lcd_puts(0, 7, buf);
    }

#if CONFIG_CHARGING == CHARGING_CONTROL

    snprintf(buf, 32, "Charge mode:");
    lcd_puts(0, 2, buf);

    if (charge_state == CHARGING)
        snprintf(buf, 32, str(LANG_BATTERY_CHARGE));
    else if (charge_state == TOPOFF)
        snprintf(buf, 32, str(LANG_BATTERY_TOPOFF_CHARGE));
    else if (charge_state == TRICKLE)
        snprintf(buf, 32, str(LANG_BATTERY_TRICKLE_CHARGE));
    else
        snprintf(buf, 32, "not charging");

    lcd_puts(0, 3, buf);
    if (!charger_enabled)
        animate = false;
#endif /* CONFIG_CHARGING == CHARGING_CONTROL */


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
                charging_logo[i] = 0x01 << bitpos;
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
    snprintf(buf, sizeof(buf), " %d.%02dV", battv / 1000, (battv % 1000) / 10);
    lcd_puts(4, 1, buf);

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
                buf[5 - ypos + 8 * (i/5)] |= 0x10 >> (i%5);
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
    gui_syncstatusbar_draw(&statusbars, true);

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
bool set_time_screen(const char* title, struct tm *tm)
{
    bool done = false;
    int button;
    unsigned int i, j, s;
    int cursorpos = 0;
    unsigned int julianday;
    unsigned int realyear;
    unsigned int width;
    unsigned int min, max;
    unsigned int statusbar_height = 0;
    unsigned int separator_width, weekday_width;
    unsigned int prev_line_height;
    int daysinmonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    unsigned char buffer[20];
    struct viewport vp[NB_SCREENS];
    int nb_lines;

    /* 6 possible cursor possitions, 2 values stored for each: x, y */
    unsigned int cursor[6][2];

    int *valptr = NULL;
    unsigned char *ptr[6];

    if(global_settings.statusbar)
        statusbar_height = STATUSBAR_HEIGHT;

    /* speak selection when screen is entered */
    say_time(cursorpos, tm);

    while (!done) {
        /* for easy acess in the drawing loop */
        ptr[0] = buffer;                     /* hours */
        ptr[1] = buffer + 3;                 /* minutes */
        ptr[2] = buffer + 6;                 /* seconds */
        ptr[3] = buffer + 9;                 /* year */
        ptr[4] = str(LANG_MONTH_JANUARY + tm->tm_mon); /* monthname */
        ptr[5] = buffer + 14;                /* day of month */

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
        julianday = tm->tm_mday;
        for(i = 0; (int)i < tm->tm_mon; i++) {
           julianday += daysinmonth[i];
        }

        tm->tm_wday = (realyear + julianday + (realyear - 1) / 4 -
                       (realyear - 1) / 100 + (realyear - 1) / 400 + 7 - 1) % 7;

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
            viewport_set_defaults(&vp[s], s);
            nb_lines = viewport_get_nb_lines(&vp[s]);
            
            /* minimum lines needed is 2 + title line */
            if (nb_lines < 4)
            {
                vp[s].font = FONT_SYSFIXED;
                nb_lines = viewport_get_nb_lines(&vp[s]);
            }
            
            /* recalculate the positions and offsets */
            if (nb_lines >= 3)
                screens[s].getstringsize(title, NULL, &prev_line_height);
            else
                prev_line_height = 0;

            screens[s].getstringsize(SEPARATOR, &separator_width, NULL);

            /* weekday */
            screens[s].getstringsize(str(LANG_WEEKDAY_SUNDAY + tm->tm_wday),
                                     &weekday_width, NULL);
            screens[s].getstringsize(" ", &separator_width, NULL);

            for(i=0, j=0; i < 6; i++)
            {
                if(i==3) /* second row */
                {
                    j = weekday_width + separator_width;;
                    prev_line_height *= 2;
                }
                screens[s].getstringsize(ptr[i], &width, NULL);
                cursor[i][INDEX_Y] = prev_line_height + statusbar_height;
                cursor[i][INDEX_X] = j;
                j += width + separator_width;
            }

            /* draw the screen */
            screens[s].set_viewport(&vp[s]);
            screens[s].clear_viewport();
            /* display the screen title */
            screens[s].puts_scroll(0, 0, title);

            /* these are not selectable, so we draw them outside the loop */
            /* name of the week day */
            screens[s].putsxy(0, cursor[3][INDEX_Y],
                              str(LANG_WEEKDAY_SUNDAY + tm->tm_wday));

            /* draw the selected item with drawmode set to
                DRMODE_SOLID|DRMODE_INVERSEVID, all other selectable
                items with drawmode DRMODE_SOLID */
            for(i=0; i<6; i++)
            {
                if (cursorpos == (int)i)
                    vp[s].drawmode = (DRMODE_SOLID|DRMODE_INVERSEVID);

                screens[s].putsxy(cursor[i][INDEX_X], 
                                  cursor[i][INDEX_Y], ptr[i]);

                vp[s].drawmode = DRMODE_SOLID;

                screens[s].putsxy(cursor[i/4 +1][INDEX_X] - separator_width,
                                  cursor[0][INDEX_Y], SEPARATOR);
            }

            /* print help text */
            if (nb_lines > 4)
                screens[s].puts(0, 4, str(LANG_TIME_SET_BUTTON));
            if (nb_lines > 5)
                screens[s].puts(0, 5, str(LANG_TIME_REVERT));
            screens[s].update_viewport();
            screens[s].set_viewport(NULL);
        }
        gui_syncstatusbar_draw(&statusbars, true);

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

        button = get_action(CONTEXT_SETTINGS_TIME, TIMEOUT_BLOCK);
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
    int info_id[sizeof(id3_headers)/sizeof(id3_headers[0])];
};
static char * id3_get_info(int selected_item, void* data,
                           char *buffer, size_t buffer_len)
{
    struct id3view_info *info = (struct id3view_info*)data;
    struct mp3entry* id3 =info->id3;
    int info_no=selected_item/2;
    if(!(selected_item%2))
    {/* header */
        return( str(id3_headers[info->info_id[info_no]]));
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
    for (i=0; i<sizeof(id3_headers)/sizeof(id3_headers[0]); i++)
    {
        char temp[8];
        info.info_id[i] = i;
        if (id3_get_info((i*2)+1, &info, temp, 8) != NULL)
            info.info_id[info.count++] = i;
    }

    gui_synclist_init(&id3_lists, &id3_get_info, &info, true, 2, NULL);
    gui_synclist_set_nb_items(&id3_lists, info.count*2);
    gui_synclist_draw(&id3_lists);
    gui_syncstatusbar_draw(&statusbars, true);
    while (true) {
        gui_syncstatusbar_draw(&statusbars, false);
        key = get_action(CONTEXT_LIST,HZ/2);
        if(key!=ACTION_NONE && key!=ACTION_UNKNOWN
        && !gui_synclist_do_button(&id3_lists, &key,LIST_WRAP_UNLESS_HELD))
        {
            return(default_event_handler(key) == SYS_USB_CONNECTED);
        }
    }
}

static char* runtime_get_data(int selected_item, void* data,
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
        if (charger_inserted()
#ifdef HAVE_USB_POWER
            || usb_powered()
#endif
        )
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
        gui_syncstatusbar_draw(&statusbars, true);
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
