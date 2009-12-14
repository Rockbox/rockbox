/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * (based upon 1.1 by calpefrosch) updated by www.HuwSy.ukhackers.net
 *
 * Copyright (C) 2002
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

#include <timefuncs.h>
#include "lib/playback_control.h"
#include "lib/configfile.h"

PLUGIN_HEADER

#if CONFIG_KEYPAD == RECORDER_PAD
#define CALENDAR_QUIT       BUTTON_OFF
#define CALENDAR_SELECT     BUTTON_PLAY
#define CALENDAR_NEXT_WEEK  BUTTON_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_UP
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH (BUTTON_ON|BUTTON_DOWN)
#define CALENDAR_PREV_MONTH (BUTTON_ON|BUTTON_UP)

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define CALENDAR_QUIT       BUTTON_OFF
#define CALENDAR_SELECT     BUTTON_SELECT
#define CALENDAR_NEXT_WEEK  BUTTON_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_UP
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH (BUTTON_ON|BUTTON_DOWN)
#define CALENDAR_PREV_MONTH (BUTTON_ON|BUTTON_UP)

#elif CONFIG_KEYPAD == ONDIO_PAD
#define CALENDAR_QUIT       BUTTON_OFF
#define CALENDAR_SELECT     (BUTTON_MENU|BUTTON_REL)
#define CALENDAR_NEXT_WEEK  BUTTON_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_UP
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH (BUTTON_MENU|BUTTON_DOWN)
#define CALENDAR_PREV_MONTH (BUTTON_MENU|BUTTON_UP)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define CALENDAR_QUIT       BUTTON_OFF
#define CALENDAR_SELECT     BUTTON_SELECT
#define CALENDAR_NEXT_WEEK  BUTTON_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_UP
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH BUTTON_MODE
#define CALENDAR_PREV_MONTH BUTTON_REC

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define CALENDAR_QUIT       (BUTTON_SELECT|BUTTON_MENU)
#define CALENDAR_SELECT     (BUTTON_SELECT|BUTTON_REL)
#define CALENDAR_NEXT_WEEK  BUTTON_SCROLL_FWD
#define CALENDAR_PREV_WEEK  BUTTON_SCROLL_BACK
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH BUTTON_PLAY
#define CALENDAR_PREV_MONTH (BUTTON_MENU|BUTTON_REL)

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define CALENDAR_QUIT       BUTTON_POWER
#define CALENDAR_SELECT     BUTTON_SELECT
#define CALENDAR_NEXT_WEEK  BUTTON_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_UP
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH BUTTON_PLAY
#define CALENDAR_PREV_MONTH BUTTON_REC

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define CALENDAR_QUIT       BUTTON_POWER
#define CALENDAR_SELECT     BUTTON_SELECT
#define CALENDAR_NEXT_WEEK  BUTTON_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_UP
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH BUTTON_VOL_DOWN
#define CALENDAR_PREV_MONTH BUTTON_VOL_UP

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define CALENDAR_QUIT       BUTTON_PLAY
#define CALENDAR_SELECT     BUTTON_SELECT
#define CALENDAR_NEXT_WEEK  BUTTON_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_UP
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH BUTTON_MODE
#define CALENDAR_PREV_MONTH BUTTON_EQ

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define CALENDAR_QUIT       BUTTON_POWER
#define CALENDAR_SELECT     BUTTON_SELECT
#define CALENDAR_NEXT_WEEK  BUTTON_SCROLL_FWD
#define CALENDAR_PREV_WEEK  BUTTON_SCROLL_BACK
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH BUTTON_DOWN
#define CALENDAR_PREV_MONTH BUTTON_UP

#elif CONFIG_KEYPAD == SANSA_FUZE_PAD
#define CALENDAR_QUIT       (BUTTON_HOME|BUTTON_REPEAT)
#define CALENDAR_SELECT     BUTTON_SELECT
#define CALENDAR_NEXT_WEEK  BUTTON_SCROLL_FWD
#define CALENDAR_PREV_WEEK  BUTTON_SCROLL_BACK
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH BUTTON_DOWN
#define CALENDAR_PREV_MONTH BUTTON_UP

#elif (CONFIG_KEYPAD == SANSA_C200_PAD) || \
      (CONFIG_KEYPAD == SANSA_CLIP_PAD) || \
      (CONFIG_KEYPAD == SANSA_M200_PAD)
#define CALENDAR_QUIT       BUTTON_POWER
#define CALENDAR_SELECT     BUTTON_SELECT
#define CALENDAR_NEXT_WEEK  BUTTON_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_UP
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH BUTTON_VOL_UP
#define CALENDAR_PREV_MONTH BUTTON_VOL_DOWN

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define CALENDAR_QUIT       BUTTON_POWER
#define CALENDAR_SELECT     BUTTON_PLAY
#define CALENDAR_NEXT_WEEK  BUTTON_SCROLL_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_SCROLL_UP
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH BUTTON_FF
#define CALENDAR_PREV_MONTH BUTTON_REW

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define CALENDAR_QUIT       BUTTON_BACK
#define CALENDAR_SELECT     BUTTON_SELECT
#define CALENDAR_NEXT_WEEK  BUTTON_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_UP
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH BUTTON_NEXT
#define CALENDAR_PREV_MONTH BUTTON_PREV

#elif CONFIG_KEYPAD == MROBE100_PAD
#define CALENDAR_QUIT       BUTTON_POWER
#define CALENDAR_SELECT     BUTTON_SELECT
#define CALENDAR_NEXT_WEEK  BUTTON_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_UP
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH (BUTTON_MENU|BUTTON_DOWN)
#define CALENDAR_PREV_MONTH (BUTTON_MENU|BUTTON_UP)

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define CALENDAR_QUIT       BUTTON_RC_REC
#define CALENDAR_SELECT     BUTTON_RC_PLAY
#define CALENDAR_NEXT_WEEK  BUTTON_RC_VOL_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_RC_VOL_UP
#define CALENDAR_NEXT_DAY   BUTTON_RC_FF
#define CALENDAR_PREV_DAY   BUTTON_RC_REW
#define CALENDAR_NEXT_MONTH BUTTON_RC_MODE
#define CALENDAR_PREV_MONTH BUTTON_RC_MENU

#elif (CONFIG_KEYPAD == COWOND2_PAD)
#define CALENDAR_QUIT       BUTTON_POWER
#define CALENDAR_SELECT     BUTTON_CENTER
#define CALENDAR_NEXT_WEEK  BUTTON_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_UP
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH BUTTON_BOTTOMRIGHT
#define CALENDAR_PREV_MONTH BUTTON_BOTTOMLEFT

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define CALENDAR_QUIT       BUTTON_BACK
#define CALENDAR_SELECT     BUTTON_SELECT
#define CALENDAR_NEXT_WEEK  BUTTON_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_UP
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH BUTTON_CUSTOM
#define CALENDAR_PREV_MONTH BUTTON_PLAY

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define CALENDAR_QUIT       BUTTON_POWER
#define CALENDAR_SELECT     BUTTON_SELECT
#define CALENDAR_NEXT_WEEK  BUTTON_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_UP
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH BUTTON_VOL_DOWN
#define CALENDAR_PREV_MONTH BUTTON_VOL_UP

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define CALENDAR_QUIT       BUTTON_POWER
#define CALENDAR_SELECT     BUTTON_PLAY
#define CALENDAR_NEXT_WEEK  BUTTON_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_UP
#define CALENDAR_NEXT_DAY   BUTTON_NEXT
#define CALENDAR_PREV_DAY   BUTTON_PREV
#define CALENDAR_NEXT_MONTH BUTTON_VOL_DOWN
#define CALENDAR_PREV_MONTH BUTTON_VOL_UP

#elif (CONFIG_KEYPAD == ONDAVX747_PAD)
#define CALENDAR_QUIT       BUTTON_POWER
#define CALENDAR_SELECT     BUTTON_MENU
#define CALENDAR_NEXT_WEEK  BUTTON_VOL_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_VOL_UP
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH BUTTON_BOTTOMRIGHT
#define CALENDAR_PREV_MONTH BUTTON_BOTTOMLEFT

#elif (CONFIG_KEYPAD == ONDAVX777_PAD)
#define CALENDAR_QUIT       BUTTON_POWER

#elif CONFIG_KEYPAD == MROBE500_PAD
#define CALENDAR_QUIT       BUTTON_POWER

#elif CONFIG_KEYPAD == SAMSUNG_YH_PAD
#define CALENDAR_QUIT       BUTTON_REC
#define CALENDAR_SELECT     BUTTON_PLAY
#define CALENDAR_NEXT_WEEK  BUTTON_DOWN
#define CALENDAR_PREV_WEEK  BUTTON_UP
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH BUTTON_FFWD
#define CALENDAR_PREV_MONTH BUTTON_REW

#else
#error "No keypad setting."
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef CALENDAR_QUIT
#define CALENDAR_QUIT       BUTTON_MIDLEFT
#endif
#ifndef CALENDAR_SELECT
#define CALENDAR_SELECT     BUTTON_CENTER
#endif
#ifndef CALENDAR_NEXT_DAY
#define CALENDAR_NEXT_DAY   BUTTON_TOPLEFT
#endif
#ifndef CALENDAR_PREV_DAY
#define CALENDAR_PREV_DAY   BUTTON_BOTTOMLEFT
#endif
#ifndef CALENDAR_NEXT_WEEK
#define CALENDAR_NEXT_WEEK  BUTTON_TOPMIDDLE
#endif
#ifndef CALENDAR_PREV_WEEK
#define CALENDAR_PREV_WEEK  BUTTON_BOTTOMMIDDLE
#endif
#ifndef CALENDAR_NEXT_MONTH
#define CALENDAR_NEXT_MONTH BUTTON_TOPRIGHT
#endif
#ifndef CALENDAR_PREV_MONTH
#define CALENDAR_PREV_MONTH BUTTON_BOTTOMRIGHT
#endif
#endif

#define MEMO_FILE PLUGIN_APPS_DIR "/.memo"
#define TEMP_FILE PLUGIN_APPS_DIR "/~temp"

#define X_OFFSET ((LCD_WIDTH%7)/2)
#if LCD_HEIGHT <= 80
#define Y_OFFSET 1
#else
#define Y_OFFSET 4
#endif
#define CELL_WIDTH  (LCD_WIDTH / 7)
#define CELL_HEIGHT (LCD_HEIGHT / 7)

#define CFG_FILE "calendar.cfg"
struct info {
    int first_wday;
#if (CONFIG_RTC == 0)
    int last_mon;
    int last_year;
#endif
};
static struct info info = { .first_wday = 0 }, old_info;
static struct configdata config[] = {
    { TYPE_INT, 0, 6, { .int_p = &(info.first_wday) }, "first wday", NULL },
#if (CONFIG_RTC == 0)
    { TYPE_INT, 1, 12, { .int_p = &(info.last_mon) }, "last mon", NULL },
    { TYPE_INT, 1, 3000, { .int_p = &(info.last_year) }, "last year", NULL },
#endif
};

static bool leap_year;
/* days_in_month[][0] is for December */
static const int days_in_month[2][13] = {
    {31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};

static const char *dayname_long[7] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
static const char *dayname_short[7] = {"M","T","W","T","F","S","S"};

struct shown {
    int     mday;        /* day of the month */
    int     mon;         /* month */
    int     year;        /* year since 1900 */
    int     wday;        /* day of the week */
    int     firstday;    /* first (w)day of month */
    int     lastday;     /* last (w)day of month */
};

static bool use_system_font = false;

static bool been_in_usb_mode = false;

/* leap year -- account for gregorian reformation in 1752 */
static int is_leap_year(int yr)
{
    return ((yr) <= 1752 ? !((yr) % 4) : \
        (!((yr) % 4) && ((yr) % 100)) || !((yr) % 400))  ? 1:0 ;
}

/* searches the weekday of the first day in month,
 * relative to the given values */
static int calc_weekday( struct shown *shown )
{
    return ( shown->wday + 36 - shown->mday ) % 7 ;
}

#if (CONFIG_RTC == 0)
/* from timefunc.c */
static void my_set_day_of_week(struct shown *shown)
{
    int y = shown->year;
    int d = shown->mday;
    int m = shown->mon-1;
    static const char mo[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

    if(m == 0 || m == 1) y--;
    shown->wday = (d + mo[m] + y + y/4 - y/100 + y/400) % 7 - 1;
}
#endif

static void calendar_init(struct shown *shown)
{
    int w, h;
#if CONFIG_RTC
    struct tm *tm;
#endif
    rb->lcd_getstringsize("A", &w, &h);
    if ( ((w * 14) > LCD_WIDTH) || ((h * 7) > LCD_HEIGHT) )
    {
        use_system_font = true;
    }
#if CONFIG_RTC
    tm = rb->get_time();
    shown->mday = tm->tm_mday;
    shown->mon = tm->tm_mon + 1;
    shown->year = 2000 + (tm->tm_year%100);
    shown->wday = tm->tm_wday - 1;
#else
#define S100(x) 1 ## x
#define C2DIG2DEC(x) (S100(x)-100)
    if(info.last_mon == 0 || info.last_year == 0)
    {
        shown->mon = C2DIG2DEC(MONTH);
        shown->year = YEAR;
    }
    else
    {
        shown->mon = info.last_mon;
        shown->year = info.last_year;
    }
    shown->mday = 1;
    my_set_day_of_week(shown);
#endif
    shown->firstday = calc_weekday(shown);
    leap_year = is_leap_year(shown->year);
}

static void draw_headers(void)
{
    int i, w, h;
    int x = X_OFFSET;
    int wday;
    const char **dayname = dayname_long;

    for (i = 0; i < 7; i++)
    {
        rb->lcd_getstringsize(dayname[i], &w, &h);
        if (w > CELL_WIDTH)
        {
            dayname = dayname_short;
            break;
        }
    }

    wday = info.first_wday;
    rb->lcd_getstringsize("A", &w, &h);
    for (i = 0; i < 7; i++)
    {
        if (wday >= 7) wday = 0;
        rb->lcd_putsxy(x, 0, dayname[wday++]);
        x += CELL_WIDTH;
    }
    rb->lcd_hline(0, LCD_WIDTH-1, h);
}

static bool day_has_memo[32];
static bool wday_has_memo[7];
static void draw_calendar(struct shown *shown)
{
    int w, h;
    int x, y, pos, days_per_month, j;
    int wday;
    char buffer[12];
    const char *monthname[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    if(use_system_font)
    {
        rb->lcd_setfont(FONT_SYSFIXED);
    }
    rb->lcd_getstringsize("A", &w, &h);
    rb->lcd_clear_display();
    draw_headers();
    wday = shown->firstday;
    pos = wday + 7 - info.first_wday;
    if (pos >= 7) pos -= 7;

    days_per_month = days_in_month[leap_year][shown->mon];
    x = X_OFFSET + (pos * CELL_WIDTH);
    y = Y_OFFSET + h;
    for (j = 1; j <= days_per_month; j++)
    {
        if ( (day_has_memo[j]) || (wday_has_memo[wday]) )
            rb->snprintf(buffer, 4, "%02d.", j);
        else
            rb->snprintf(buffer, 4, "%02d", j);
        if (shown->mday == j)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_fillrect(x, y - 1, CELL_WIDTH - 1, CELL_HEIGHT);
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            shown->wday = wday;
        }
        else
        {
            rb->lcd_set_drawmode(DRMODE_SOLID);
        }
        rb->lcd_putsxy(x, y, buffer);
        x += CELL_WIDTH;
        wday++;
        if (wday >= 7)
            wday = 0;
        pos++;
        if (pos >= 7)
        {
            pos = 0;
            x = X_OFFSET;
            y += CELL_HEIGHT;
        }
    }
    shown->lastday = wday;
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_vline(LCD_WIDTH-w*8-10, LCD_HEIGHT-h-3, LCD_HEIGHT-1);
    rb->lcd_hline(LCD_WIDTH-w*8-10, LCD_WIDTH-1, LCD_HEIGHT-h-3);
    rb->snprintf(buffer, sizeof(buffer), "%s %04d",
                 monthname[shown->mon-1], shown->year);
    rb->lcd_putsxy(LCD_WIDTH-w*8-8, LCD_HEIGHT-h-1, buffer);
    rb->lcd_update();
}

#define MAX_CHAR_MEMO_LEN 64
#define MAX_MEMOS_IN_A_MONTH 128
struct memo {
    char    message[MAX_CHAR_MEMO_LEN];
    int     day;
    int     month;
    int     file_pointer_start;
    int     file_pointer_end;
    int     year;
    int     wday;
    int     type;
} memos[MAX_MEMOS_IN_A_MONTH];
static int pointer_array[MAX_MEMOS_IN_A_MONTH];
static int memos_in_memory = 0;
static int memos_in_shown_memory = 0;

static void load_memo(struct shown *shown)
{
    int k, fp;
    bool exit = false;
    char temp_memo1[2];
    char temp_memo2[3];
    char temp_memo4[5];
    temp_memo1[1] = 0;
    temp_memo2[2] = 0;
    temp_memo4[4] = 0;
    for (k = 1; k < 32; k++)
        day_has_memo[k] = false;
    for (k = 0; k < 7; k++)
        wday_has_memo[k] = false;
    memos_in_memory = 0;
    fp = rb->open(MEMO_FILE, O_RDONLY);
    if (fp > -1)
    {
        rb->lseek(fp, 0, SEEK_SET);
        while (!exit)
        {
            bool load_to_memory;
            struct memo *memo = &memos[memos_in_memory];
            rb->memset(memo, 0, sizeof(*memo));
            memo->file_pointer_start = rb->lseek(fp, 0, SEEK_CUR);
            if (rb->read(fp, temp_memo2, 2) == 2)
                memo->day = rb->atoi(temp_memo2);
            if (rb->read(fp, temp_memo2, 2) == 2)
                memo->month = rb->atoi(temp_memo2);
            if (rb->read(fp, temp_memo4, 4) == 4)
                memo->year = rb->atoi(temp_memo4);
            if (rb->read(fp, temp_memo1, 1) == 1)
                memo->wday = rb->atoi(temp_memo1);
            if (rb->read(fp, temp_memo1, 1) == 1)
                memo->type = rb->atoi(temp_memo1);
            load_to_memory = ((memo->type < 2) ||
                              ((memo->type == 2) &&
                                  (memo->month == shown->mon)) ||
                              ((memo->type > 2) &&
                                  (memo->month == shown->mon) &&
                                  (memo->year == shown->year)));
            k = 0;
            while (1)
            {
                if (rb->read(fp, temp_memo1, 1) != 1)
                {
                    memo->day = 0;
                    memo->month = 0;
                    memo->file_pointer_start = 0;
                    memo->file_pointer_end = 0;
                    memo->year = 0;
                    memo->type = 0;
                    memo->wday = 0;
                    memo->message[0] = 0;
                    exit = true;
                    break;
                }
                if (load_to_memory)
                {
                    if (temp_memo1[0] == '\n')
                    {
                        if (memo->type > 0)
                            day_has_memo[memo->day] = true;
                        else
                            wday_has_memo[memo->wday] = true;
                        memo->file_pointer_end = rb->lseek(fp, 0, SEEK_CUR);
                        memos_in_memory++;
                    }
                    else if ( (temp_memo1[0] != '\r') &&
                              (temp_memo1[0] != '\t') &&
                              k < MAX_CHAR_MEMO_LEN-1 )
                        memo->message[k++] = temp_memo1[0];
                }
                if (temp_memo1[0] == '\n')
                    break;
            }
        }
        rb->close(fp);
    }
}

static bool save_memo(int changed, bool new_mod, struct shown *shown)
{
    int fp, fq;
    /* use O_RDWR|O_CREAT so that file is created if it doesn't exist. */
    fp = rb->open(MEMO_FILE, O_RDWR|O_CREAT);
    fq = rb->creat(TEMP_FILE);
    if ( (fq > -1) && (fp > -1) )
    {
        int i;
        char temp[MAX_CHAR_MEMO_LEN];
        struct memo *memo = &memos[changed];
        rb->lseek(fp, 0, SEEK_SET);
        for (i = 0; i < memo->file_pointer_start; i++)
        {
            rb->read(fp, temp, 1);
            rb->write(fq, temp, 1);
        }
        if (new_mod)
        {
            rb->fdprintf(fq, "%02d%02d%04d%01d%01d%s\n",
                         memo->day, memo->month, memo->year, memo->wday,
                         memo->type, memo->message);
        }
        rb->lseek(fp, memo->file_pointer_end, SEEK_SET);
        while(rb->read(fp, temp, 1) == 1)
        {
            rb->write(fq, temp, 1);
        }
        rb->close(fp);
        rb->close(fq);
        rb->remove(MEMO_FILE);
        rb->rename(TEMP_FILE, MEMO_FILE);
        load_memo(shown);
        return true;
    }
    else if (fp > -1)
        rb->close(fp);
    else if (fq > -1)
        rb->close(fq);
    return false;
}

static void add_memo(struct shown *shown, int type)
{
    bool saved = false;
    struct memo *memo = &memos[memos_in_memory];
    if (rb->kbd_input(memo->message, MAX_CHAR_MEMO_LEN) == 0)
    {
        if (memo->message[0])
        {
            memo->file_pointer_start = 0;
            memo->file_pointer_end = 0;
            memo->day = shown->mday;
            memo->month = shown->mon;
            memo->wday = shown->wday;
            memo->year = shown->year;
            memo->type = type;
            if (save_memo(memos_in_memory, true, shown))
            {
                saved = true;
            }
            else
            {
                memo->file_pointer_start = 0;
                memo->file_pointer_end = 0;
                memo->day = 0;
                memo->month = 0;
                memo->year = 0;
                memo->type = 0;
                memo->wday = 0;
            }
        }
    }
    rb->lcd_clear_display();
    if (saved)
        rb->splash(HZ/2, "Event added");
    else
        rb->splash(HZ/2, "Event not added");
}

static int edit_menu_cb(int action, const struct menu_item_ex *this_item)
{
    int i = (intptr_t)this_item;
    if (action == ACTION_REQUEST_MENUITEM
        && memos_in_shown_memory <= 0 && (i==0 || i==1))
        return ACTION_EXIT_MENUITEM;
    return action;
}

static bool edit_memo(int change, struct shown *shown)
{
    bool exit = false;
    int selected = 0;

    static const struct opt_items modes[7] = {
        { "Mon", -1 },
        { "Tue", -1 },
        { "Wed", -1 },
        { "Thu", -1 },
        { "Fri", -1 },
        { "Sat", -1 },
        { "Sun", -1 },
    };

    MENUITEM_STRINGLIST(edit_menu, "Edit menu", edit_menu_cb,
                        "Remove", "Edit",
                        "New Weekly", "New Monthly",
                        "New Yearly", "New One off",
                        "First Day of Week",
                        "Playback Control");

    while (!exit)
    {
        switch (rb->do_menu(&edit_menu, &selected, NULL, false))
        {
            case 0: /* remove */
                save_memo(change, false, shown);
                return false;

            case 1: /* edit */
                if(rb->kbd_input(memos[change].message,
                                 MAX_CHAR_MEMO_LEN)  == 0)
                    save_memo(change, true, shown);
                return false;

            case 2: /* weekly */
                add_memo(shown, 0);
                return false;

            case 3: /* monthly */
                add_memo(shown, 1);
                return false;

            case 4: /* yearly */
                add_memo(shown, 2);
                return false;

            case 5: /* one off */
                add_memo(shown, 3);
                return false;

            case 6: /* weekday */
                rb->set_option("First Day of Week", &info.first_wday,
                                INT, modes, 7, NULL);
                break;

            case 7: /* playback control */
                playback_control(NULL);
                break;

            case GO_TO_PREVIOUS:
                return false;

            case MENU_ATTACHED_USB:
                been_in_usb_mode = true;
                break;
        }
    }
    return false;
}

static const char* get_event_text(int selected, void *data,
                                  char *buffer, size_t buffer_len)
{
    struct shown *shown = (struct shown *) data;
    struct memo *memo;
    if (selected < 0 || memos_in_shown_memory <= selected)
    {
        return NULL;
    }
    memo = &memos[pointer_array[selected]];
    if (memo->type == 2)
        rb->snprintf(buffer, buffer_len, "%s (%d yrs)",
                     memo->message, shown->year - memo->year);
    else
        rb->snprintf(buffer, buffer_len, "%s", memo->message);
    return buffer;
}

static bool view_events(int selected, struct shown *shown)
{
    struct gui_synclist gui_memos;
    bool exit=false;
    int button;

    rb->gui_synclist_init(&gui_memos, &get_event_text, shown, false, 1, NULL);
    rb->gui_synclist_set_title(&gui_memos, "Events (play : menu)", NOICON);
    rb->gui_synclist_set_nb_items(&gui_memos, memos_in_shown_memory);
    rb->gui_synclist_select_item(&gui_memos, selected);
    rb->gui_synclist_draw(&gui_memos);

    while (!exit)
    {
        button = rb->get_action(CONTEXT_LIST, TIMEOUT_BLOCK);
        rb->gui_synclist_do_button(&gui_memos, &button, LIST_WRAP_UNLESS_HELD);

        switch (button)
        {
            case ACTION_STD_OK:
                selected = rb->gui_synclist_get_sel_pos(&gui_memos);
                return edit_memo(pointer_array[selected], shown);
                break;

            case ACTION_STD_CANCEL:
                return false;
                break;

            default:
                if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    been_in_usb_mode = true;
                break;
        }
    }

    return false;
}

static void update_memos_shown(struct shown *shown)
{
    int i;
    struct memo *memo;
    memos_in_shown_memory = 0;
    for (i = 0; i < memos_in_memory; i++)
    {
        memo = &memos[i];
        if (((memo->type >= 1) && (memo->day == shown->mday)) ||
            ((memo->type < 1) && (memo->wday == shown->wday)))
            pointer_array[memos_in_shown_memory++] = i;
    }
}

static bool any_events(struct shown *shown, bool force)
{
    update_memos_shown(shown);

    if (memos_in_shown_memory > 0)
        return view_events(0, shown);
    else if (force)
        return edit_memo(0, shown);
    else
        return false;

    return false;
}

static void next_month(struct shown *shown, int step)
{
    shown->mon++;
    if (shown->mon > 12)
    {
        shown->mon = 1;
        shown->year++;
        leap_year = is_leap_year(shown->year);
    }
    if (step > 0)
        shown->mday = shown->mday - days_in_month[leap_year][shown->mon-1];
    else if (shown->mday > days_in_month[leap_year][shown->mon])
        shown->mday = days_in_month[leap_year][shown->mon];
    shown->firstday = shown->lastday;
    load_memo(shown);
    draw_calendar(shown);
}

static void prev_month(struct shown *shown, int step)
{
    shown->mon--;
    if (shown->mon < 1)
    {
        shown->mon = 12;
        shown->year--;
        leap_year = is_leap_year(shown->year);
    }
    if (step > 0)
        shown->mday = shown->mday + days_in_month[leap_year][shown->mon];
    else if (shown->mday > days_in_month[leap_year][shown->mon])
        shown->mday = days_in_month[leap_year][shown->mon];
    shown->firstday += 7 - (days_in_month[leap_year][shown->mon] % 7);
    if (shown->firstday >= 7)
        shown->firstday -= 7;
    load_memo(shown);
    draw_calendar(shown);
}

static void next_day(struct shown *shown, int step)
{
    shown->mday += step;
    if (shown->mday > days_in_month[leap_year][shown->mon])
        next_month(shown, step);
    else
        draw_calendar(shown);
}

static void prev_day(struct shown *shown, int step)
{
    shown->mday -= step;
    if (shown->mday < 1)
        prev_month(shown, step);
    else
        draw_calendar(shown);
}

enum plugin_status plugin_start(const void* parameter)
{
    struct shown shown;
    bool exit = false;
    int button;

    (void)(parameter);

    configfile_load(CFG_FILE, config, ARRAYLEN(config), 0);
    rb->memcpy(&old_info, &info, sizeof(struct info));

    calendar_init(&shown);
    load_memo(&shown);
    any_events(&shown, false);
    draw_calendar(&shown);

    while (!exit)
    {
        button = rb->button_get(true);
        switch (button)
        {
            case CALENDAR_QUIT:
                exit = true;
                break;

            case CALENDAR_NEXT_MONTH:
            case CALENDAR_NEXT_MONTH | BUTTON_REPEAT:
                next_month(&shown, 0);
                break;

            case CALENDAR_PREV_MONTH:
            case CALENDAR_PREV_MONTH | BUTTON_REPEAT:
                prev_month(&shown, 0);
                break;

            case CALENDAR_NEXT_WEEK:
            case CALENDAR_NEXT_WEEK | BUTTON_REPEAT:
                next_day(&shown, 7);
                break;

            case CALENDAR_PREV_WEEK:
            case CALENDAR_PREV_WEEK | BUTTON_REPEAT:
                prev_day(&shown, 7);
                break;

            case CALENDAR_PREV_DAY:
            case CALENDAR_PREV_DAY | BUTTON_REPEAT:
                prev_day(&shown, 1);
                break;

            case CALENDAR_NEXT_DAY:
            case CALENDAR_NEXT_DAY | BUTTON_REPEAT:
                next_day(&shown, 1);
                break;

            case CALENDAR_SELECT:
                any_events(&shown, true);
                draw_calendar(&shown);
                break;

            default:
                if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    been_in_usb_mode = true;
                draw_calendar(&shown);
                break;
        }
    }


#if (CONFIG_RTC == 0)
    info.last_mon = shown.mon;
    info.last_year = shown.year;
#endif
    if (rb->memcmp(&old_info, &info, sizeof(struct info)))
        configfile_save(CFG_FILE, config, ARRAYLEN(config), 0);
    return been_in_usb_mode?PLUGIN_USB_CONNECTED:PLUGIN_OK;
}
