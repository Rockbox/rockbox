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
#include <stdlib.h>
#include "backlight.h"
#include "action.h"
#include "lcd.h"
#include "lang.h"
#include "icons.h"
#include "font.h"
#include "audio.h"
#include "usb.h"
#include "settings.h"
#include "status.h"
#include "playlist.h"
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
#include "list.h"
#include "yesno.h"
#include "backdrop.h"
#include "viewport.h"
#include "language.h"
#include "replaygain.h"

#include "ctype.h"

#if CONFIG_CHARGING
void charging_splash(void)
{
    splash(2*HZ, str(LANG_BATTERY_CHARGE));
    button_clear_queue();
}
#endif


#if (CONFIG_RTC != 0)

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
    struct viewport viewports[NB_SCREENS];
    bool done = false, usb = false;
    int cursorpos = 0;
    unsigned char offsets_ptr[] =
        { OFF_HOURS, OFF_MINUTES, OFF_SECONDS, OFF_YEAR, 0, OFF_DAY };

    if (lang_is_rtl())
    {
        offsets_ptr[IDX_HOURS] = OFF_SECONDS;
        offsets_ptr[IDX_SECONDS] = OFF_HOURS;
        offsets_ptr[IDX_YEAR] = OFF_DAY;
        offsets_ptr[IDX_DAY] = OFF_YEAR;
    }

    /* speak selection when screen is entered */
    say_time(cursorpos, tm);

#ifdef HAVE_TOUCHSCREEN
    enum touchscreen_mode old_mode = touchscreen_get_mode();
    touchscreen_set_mode(TOUCHSCREEN_BUTTON);
#endif
    while (!done) {
        int button;
        unsigned int i, realyear, min, max;
        unsigned char *ptr[6];
#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
        /* The format strings in the snprintf can in theory need 60 characters
           This will not happen in practice (because seconds are 0..60 and not
           full-range integers etc.), but -D_FORTIFY_SOURCE will still warn
           about it, so we use 60 characters for HOSTED to make the compiler
           happy. Native builds still use 20, which is enough in practice.  */
        unsigned char buffer[60];
#else
        unsigned char buffer[20];
#endif
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
            /* 6 possible cursor possitions, 2 values stored for each: x, y */
            unsigned int cursor[6][2];
            struct viewport *vp = &viewports[s];
            struct viewport *last_vp;
            struct screen *screen = &screens[s];
            static unsigned char rtl_idx[] =
                { IDX_SECONDS, IDX_MINUTES, IDX_HOURS, IDX_DAY, IDX_MONTH, IDX_YEAR };

            viewport_set_defaults(vp, s);
            last_vp = screen->set_viewport(vp);
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
                cursor[i][INDEX_Y] = prev_line_height;
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
            screen->set_viewport(last_vp);
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
                    done = usb = true;
                break;
        }
    }
    FOR_NB_SCREENS(s)
        screens[s].scroll_stop_viewport(&viewports[s]);
#ifdef HAVE_TOUCHSCREEN
    touchscreen_set_mode(old_mode);
#endif
    return usb;
}
#endif /* (CONFIG_RTC != 0) */

static const int id3_headers[]=
{
    LANG_ID3_TITLE,
    LANG_ID3_ARTIST,
    LANG_ID3_COMPOSER,
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
    LANG_ID3_TRACK_GAIN,
    LANG_ID3_ALBUM_GAIN,
    LANG_FILESIZE,
    LANG_ID3_PATH,
};

struct id3view_info {
    struct mp3entry* id3;
    int count;
    int info_id[ARRAYLEN(id3_headers)];
};

/* Spell out a buffer, but when successive digits are encountered, say
   the whole number. Useful for some ID3 tags that usually contain a
   number but are in fact free-form. */
static void say_number_and_spell(char *buf, bool year_style)
{
    char *ptr = buf;
    while(*ptr) {
        if(isdigit(*ptr)) {
            /* parse the number */
            int n = atoi(ptr);
            /* skip over digits to rest of string */
            while(isdigit(*++ptr));
            /* say the number */
            if(year_style)
                talk_value(n, UNIT_DATEYEAR, true);
            else talk_number(n, true);
        }else{
            /* Spell a sequence of non-digits */
            char tmp, *start = ptr;
            while(*++ptr && !isdigit(*ptr));
            /* temporarily truncate the string here */
            tmp = *ptr;
            *ptr = '\0';
            talk_spell(start, true);
            *ptr = tmp; /* restore string */
        }
    }
}

/* Say a replaygain ID3 value from its text form */
static void say_gain(char *buf)
{
    /* Expected form is "-5.74 dB". We'll try to parse out the number
       until the dot, say it (forcing the + sign), then say dot and
       spell the following numbers, and then say the decibel unit. */
    char *ptr = buf;
    if(*ptr == '-' || *ptr == '+')
        /* skip sign */
        ++ptr;
    /* See if we can parse out a number. */
    if(isdigit(*ptr)) {
        char tmp;
        /* skip successive digits */
        while(isdigit(*++ptr));
        /* temporarily truncate the string here */
        tmp = *ptr;
        *ptr = '\0';
        /* parse out the number we just skipped */
        talk_value(atoi(buf), UNIT_SIGNED, true); /* say the number with sign */
        *ptr = tmp; /* restore the string */
        if(*ptr == '.') {
            /* found the dot, get fractional part */
            buf = ptr;
            while (isdigit(*++ptr));
            while (*--ptr == '0');
            if (ptr > buf) {
                tmp = *++ptr;
                *ptr = '\0';
                talk_id(LANG_POINT, true);
                while (*++buf == '0')
                    talk_id(VOICE_ZERO, true);
                talk_number(atoi(buf), true);
                *ptr = tmp;
            }
            ptr = buf;
            while (isdigit(*++ptr));
        }
        buf = ptr;
        if(strlen(buf) >2 && !strcmp(buf+strlen(buf)-2, "dB")) {
            /* String does end with "dB" */
            /* point to that "dB" */
            ptr = buf+strlen(buf)-2;
            /* backup any spaces */
            while (ptr >buf && ptr[-1] == ' ')
                --ptr;
            if (ptr > buf)
                talk_spell(buf, true);
            else talk_id(VOICE_DB, true); /* say the dB unit */
        }else /* doesn't end with dB, just spell everything after the
                 number of dot. */
            talk_spell(buf, true);
    }else /* we didn't find a number, just spell everything */
        talk_spell(buf, true);
}

static const char * id3_get_or_speak_info(int selected_item, void* data,
                                          char *buffer, size_t buffer_len,
                                          bool say_it)
{
    struct id3view_info *info = (struct id3view_info*)data;
    struct mp3entry* id3 =info->id3;
    int info_no=selected_item/2;
    if(!(selected_item%2))
    {/* header */
        if(say_it)
            talk_id(id3_headers[info->info_id[info_no]], false);
        snprintf(buffer, buffer_len,
                 "[%s]", str(id3_headers[info->info_id[info_no]]));
        return buffer;
    }
    else
    {/* data */

        char * val=NULL;
        switch(id3_headers[info->info_id[info_no]])
        {
            case LANG_ID3_TITLE:
                val=id3->title;
                if(say_it && val)
                    talk_spell(val, true);
                break;
            case LANG_ID3_ARTIST:
                val=id3->artist;
                if(say_it && val)
                    talk_spell(val, true);
                break;
            case LANG_ID3_ALBUM:
                val=id3->album;
                if(say_it && val)
                    talk_spell(val, true);
                break;
            case LANG_ID3_ALBUMARTIST:
                val=id3->albumartist;
                if(say_it && val)
                    talk_spell(val, true);
                break;
            case LANG_ID3_GROUPING:
                val=id3->grouping;
                if(say_it && val)
                    talk_spell(val, true);
                break;
            case LANG_ID3_DISCNUM:
                if (id3->disc_string)
                {
                    val = id3->disc_string;
                    if(say_it)
                        say_number_and_spell(val, true);
                }
                else if (id3->discnum)
                {
                    snprintf(buffer, buffer_len, "%d", id3->discnum);
                    val = buffer;
                    if(say_it)
                        talk_number(id3->discnum, true);
                }
                break;
            case LANG_ID3_TRACKNUM:
                if (id3->track_string)
                {
                    val = id3->track_string;
                    if(say_it)
                        say_number_and_spell(val, true);
                }
                else if (id3->tracknum)
                {
                    snprintf(buffer, buffer_len, "%d", id3->tracknum);
                    val = buffer;
                    if(say_it)
                        talk_number(id3->tracknum, true);
                }
                break;
            case LANG_ID3_COMMENT:
                if (!id3->comment)
                    return NULL;
                snprintf(buffer, buffer_len, "%s", id3->comment);
                val=buffer;
                if(say_it && val)
                    talk_spell(val, true);
                break;
            case LANG_ID3_GENRE:
                val = id3->genre_string;
                if(say_it && val)
                    talk_spell(val, true);
                break;
            case LANG_ID3_YEAR:
                if (id3->year_string)
                {
                    val = id3->year_string;
                    if(say_it && val)
                        say_number_and_spell(val, true);
                }
                else if (id3->year)
                {
                    snprintf(buffer, buffer_len, "%d", id3->year);
                    val = buffer;
                    if(say_it)
                        talk_value(id3->year, UNIT_DATEYEAR, true);
                }
                break;
            case LANG_ID3_LENGTH:
                format_time(buffer, buffer_len, id3->length);
                val=buffer;
                if(say_it)
                    talk_value(id3->length /1000, UNIT_TIME, true);
                break;
            case LANG_ID3_PLAYLIST:
                snprintf(buffer, buffer_len, "%d/%d",
                         playlist_get_display_index(), playlist_amount());
                val=buffer;
                if(say_it)
                {
                    talk_number(playlist_get_display_index(), true);
                    talk_id(VOICE_OF, true);
                    talk_number(playlist_amount(), true);
                }
                break;
            case LANG_ID3_BITRATE:
                snprintf(buffer, buffer_len, "%d kbps%s", id3->bitrate,
            id3->vbr ? str(LANG_ID3_VBR) : (const unsigned char*) "");
                val=buffer;
                if(say_it)
                {
                    talk_value(id3->bitrate, UNIT_KBIT, true);
                    if(id3->vbr)
                        talk_id(LANG_ID3_VBR, true);
                }
                break;
            case LANG_ID3_FREQUENCY:
                snprintf(buffer, buffer_len, "%ld Hz", id3->frequency);
                val=buffer;
                if(say_it)
                    talk_value(id3->frequency, UNIT_HERTZ, true);
                break;
            case LANG_ID3_TRACK_GAIN:
                replaygain_itoa(buffer, buffer_len, id3->track_level);
                val=(id3->track_level) ? buffer : NULL; /* only show level!=0 */
                if(say_it && val)
                    say_gain(val);
                break;
            case LANG_ID3_ALBUM_GAIN:
                replaygain_itoa(buffer, buffer_len, id3->album_level);
                val=(id3->album_level) ? buffer : NULL; /* only show level!=0 */
                if(say_it && val)
                    say_gain(val);
                break;
            case LANG_ID3_PATH:
                val=id3->path;
                if(say_it && val)
                    talk_fullpath(val, true);
                break;
            case LANG_ID3_COMPOSER:
                val=id3->composer;
                if(say_it && val)
                    talk_spell(val, true);
                break;
            case LANG_FILESIZE: /* not LANG_ID3_FILESIZE because the string is shared */
                output_dyn_value(buffer, buffer_len, id3->filesize, byte_units, 4, true);
                val=buffer;
                if(say_it && val)
                    output_dyn_value(NULL, 0, id3->filesize, byte_units, 4, true);
                break;
        }
        if((!val || !*val) && say_it)
            talk_id(LANG_ID3_NO_INFO, true);
        return val && *val ? val : NULL;
    }
}

/* gui_synclist callback */
static const char* id3_get_info(int selected_item, void* data,
                                char *buffer, size_t buffer_len)
{
    return id3_get_or_speak_info(selected_item, data, buffer,
                                 buffer_len, false);
}

static int id3_speak_item(int selected_item, void* data)
{
    char buffer[MAX_PATH];
    selected_item &= ~1; /* Make sure it's even, to indicate the header */
    /* say field name */
    id3_get_or_speak_info(selected_item, data, buffer, MAX_PATH, true);
    /* and field value */
    id3_get_or_speak_info(selected_item+1, data, buffer, MAX_PATH, true);
    return 0;
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
    bool ret = false;
    push_current_activity(ACTIVITY_ID3SCREEN);
    for (i = 0; i < ARRAYLEN(id3_headers); i++)
    {
        char temp[8];
        info.info_id[i] = i;
        if (id3_get_info((i*2)+1, &info, temp, 8) != NULL)
            info.info_id[info.count++] = i;
    }

    gui_synclist_init(&id3_lists, &id3_get_info, &info, true, 2, NULL);
    if(global_settings.talk_menu)
        gui_synclist_set_voice_callback(&id3_lists, id3_speak_item);
    gui_synclist_set_nb_items(&id3_lists, info.count*2);
    gui_synclist_draw(&id3_lists);
    gui_synclist_speak_item(&id3_lists);
    while (true) {
        if(!list_do_action(CONTEXT_LIST,HZ/2,
                           &id3_lists, &key,LIST_WRAP_UNLESS_HELD)
           && key!=ACTION_NONE && key!=ACTION_UNKNOWN)
        {
            if (key == ACTION_STD_OK || key == ACTION_STD_CANCEL)
            {
                ret = false;
                break;
            }
            else if (key == ACTION_STD_MENU ||
                        default_event_handler(key) == SYS_USB_CONNECTED)
            {
                ret =  true;
                break;
            }
        }
    }

    pop_current_activity();
    return ret;
}

static const char* runtime_get_data(int selected_item, void* data,
                                    char* buffer, size_t buffer_len)
{
    (void)data;
    long t;
    switch (selected_item)
    {
        case 0: return str(LANG_RUNNING_TIME);
        case 1: t = global_status.runtime;      break;
        case 2: return str(LANG_TOP_TIME);
        case 3: t = global_status.topruntime;   break;
        default:
            return "";
    }

    format_time_auto(buffer, buffer_len, t, UNIT_SEC, false);

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


int view_runtime(void)
{
    static const char *lines[]={ID2P(LANG_CLEAR_TIME)};
    static const struct text_message message={lines, 1};
    bool say_runtime = true;

    struct gui_synclist lists;
    int action;
    gui_synclist_init(&lists, runtime_get_data, NULL, false, 2, NULL);
    gui_synclist_set_title(&lists, str(LANG_RUNNING_TIME), NOICON);
    if(global_settings.talk_menu)
        gui_synclist_set_voice_callback(&lists, runtime_speak_data);
    gui_synclist_set_icon_callback(&lists, NULL);
    gui_synclist_set_nb_items(&lists, 4);

    while(1)
    {
        global_status.runtime += ((current_tick - lasttime) / HZ);

        lasttime = current_tick;
        if (say_runtime)
        {
            gui_synclist_speak_item(&lists);
            say_runtime = false;
        }
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
                say_runtime = true;
            }
        }
        if(default_event_handler(action) == SYS_USB_CONNECTED)
            return 1;
    }
    return 0;
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
    viewportmanager_theme_undo(SCREEN_MAIN, false);

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
