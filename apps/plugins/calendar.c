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

#if defined(HAVE_LCD_BITMAP) && (CONFIG_RTC != 0)

#include <timefuncs.h>

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

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define CALENDAR_QUIT       BUTTON_POWER
#define CALENDAR_SELECT     BUTTON_SELECT
#define CALENDAR_NEXT_WEEK  BUTTON_SCROLL_FWD
#define CALENDAR_PREV_WEEK  BUTTON_SCROLL_BACK
#define CALENDAR_NEXT_DAY   BUTTON_RIGHT
#define CALENDAR_PREV_DAY   BUTTON_LEFT
#define CALENDAR_NEXT_MONTH BUTTON_DOWN
#define CALENDAR_PREV_MONTH BUTTON_UP

#elif CONFIG_KEYPAD == SANSA_C200_PAD || CONFIG_KEYPAD == SANSA_CLIP_PAD
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

#else
#error "No keypad setting."
#endif

#define X_OFFSET ((LCD_WIDTH%7)/2)
#if LCD_HEIGHT <= 80
#define Y_OFFSET 1
#else
#define Y_OFFSET 4
#endif
#define CELL_WIDTH  (LCD_WIDTH / 7)
#define CELL_HEIGHT (LCD_HEIGHT / 7)

static const struct plugin_api* rb;

MEM_FUNCTION_WRAPPERS(rb)

static bool leap_year;
/* days_in_month[][0] is for December */
static const int days_in_month[2][13] = {
    {31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};

static const char *dayname_long[7] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
static const char *dayname_short[7] = {"M","T","W","T","F","S","S"};

struct today {
    int     mday;        /* day of the month */
    int     mon;         /* month */
    int     year;        /* year since 1900 */
    int     wday;        /* day of the week */
};

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

static void calendar_init(struct today *today, struct shown *shown)
{
    int w,h;
#if CONFIG_RTC
    struct tm *tm;
#else
    (void)today;
#endif
    rb->lcd_getstringsize("A",&w,&h);
    if ( ((w * 14) > LCD_WIDTH) || ((h * 7) > LCD_HEIGHT) )
    {
        rb->lcd_setfont(FONT_SYSFIXED);
        use_system_font = true;
    }
    rb->lcd_clear_display();
#if CONFIG_RTC
    tm = rb->get_time();
    today->mon = tm->tm_mon +1;
    today->year = 2000+tm->tm_year%100;
    today->wday = tm->tm_wday-1;
    today->mday = tm->tm_mday;
    shown->mday = today->mday;
    shown->mon = today->mon;
    shown->year = today->year;
    shown->wday = today->wday;
#endif
    shown->firstday = calc_weekday(shown);
    leap_year = is_leap_year(shown->year);
}

static void draw_headers(void)
{
    int i,w,h;
    int x = X_OFFSET;
    const char **dayname = (const char**)&dayname_long;

    for (i = 0; i < 7; i++)
    {
        rb->lcd_getstringsize(dayname[i],&w,&h);
        if (w > CELL_WIDTH)
        {
            dayname = (const char**)&dayname_short;
            break;
        }
    }

    rb->lcd_getstringsize("A",&w,&h);
    for (i = 0; i < 7; i++)
    {
        rb->lcd_putsxy(x, 0 , dayname[i]);
        x += CELL_WIDTH;
    }
    rb->lcd_hline(0, LCD_WIDTH-1 ,h);
}

static bool day_has_memo[32];
static bool wday_has_memo[7];
static void draw_calendar(struct shown *shown)
{
    int w,h;
    int x,y,pos,days_per_month,j;
    char buffer[9];
    const char *monthname[] = {
                          "Jan",
                          "Feb",
                          "Mar",
                          "Apr",
                          "May",
                          "Jun",
                          "Jul",
                          "Aug",
                          "Sep",
                          "Oct",
                          "Nov",
                          "Dec"
                        };
    if(use_system_font)
        rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_getstringsize("A",&w,&h);
    rb->lcd_clear_display();
    draw_headers();
    if (shown->firstday > 6)
        shown->firstday -= 7;
    pos = shown->firstday;
    days_per_month = days_in_month[leap_year][shown->mon];
    x = X_OFFSET + (pos * CELL_WIDTH);
    y = Y_OFFSET + h;
    for (j = 1; j <= days_per_month; j++)
    {
        if ( (day_has_memo[j]) || (wday_has_memo[pos]) )
            rb->snprintf(buffer,4,"%02d.", j);
        else
            rb->snprintf(buffer,4,"%02d", j);
        if (shown->mday == j)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_fillrect(x, y - 1, CELL_WIDTH - 1, CELL_HEIGHT);
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            shown->wday = pos;
        }
        else
        {
            rb->lcd_set_drawmode(DRMODE_SOLID);
        }
        rb->lcd_putsxy(x, y, buffer);
        x += CELL_WIDTH;
        pos++;
        if (pos >= 7)
        {
            pos = 0;
            x = X_OFFSET;
            y += CELL_HEIGHT;
        }
    }
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_vline(LCD_WIDTH-w*8-10,LCD_HEIGHT-h-3,LCD_HEIGHT-1);
    rb->lcd_hline(LCD_WIDTH-w*8-10,LCD_WIDTH-1,LCD_HEIGHT-h-3);
    rb->snprintf(buffer,9,"%s %04d",monthname[shown->mon-1],shown->year);
    rb->lcd_putsxy(LCD_WIDTH-w*8-8,LCD_HEIGHT-h-1,buffer);
    shown->lastday = pos;
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
    int i, k, fp;
    bool exit = false;
    char temp_memo1[2];
    char temp_memo2[3];
    char temp_memo4[5];
    temp_memo1[1] = 0;
    temp_memo2[2] = 0;
    temp_memo4[4] = 0;
    for (k = 0; k < memos_in_memory; k++)
    {
        memos[k].day = 0;
        memos[k].month = 0;
        memos[k].file_pointer_start = 0;
        memos[k].file_pointer_end = 0;
        memos[k].year = 0;
        memos[k].type = 0;
        memos[k].wday = 0;
        for (i = 0; i < MAX_CHAR_MEMO_LEN; i++)
            memos[k].message[i] = 0;
    }
    for (k = 1; k < 32; k++)
        day_has_memo[k] = false;
    for (k = 0; k < 7; k++)
        wday_has_memo[k] = false;
    memos_in_memory = 0;
    fp = rb->open(ROCKBOX_DIR "/.memo",O_RDONLY);
    if (fp > -1)
    {
        int count = rb->filesize(fp);
        rb->lseek(fp, 0, SEEK_SET);
        while (!exit)
        {
            memos[memos_in_memory].file_pointer_start = rb->lseek(fp, 0,
                                                                  SEEK_CUR);
            if (rb->read(fp, temp_memo2, 2) == 2)
                memos[memos_in_memory].day = rb->atoi(&temp_memo2[0]);
            else
                memos[memos_in_memory].day = 0;
            if (rb->read(fp, temp_memo2, 2) == 2)
                memos[memos_in_memory].month = rb->atoi(&temp_memo2[0]);
            else
                memos[memos_in_memory].month = 0;
            if (rb->read(fp, temp_memo4, 4) == 4)
                memos[memos_in_memory].year = rb->atoi(&temp_memo4[0]);
            else
                memos[memos_in_memory].year = 0;
            /* as the year returned is sometimes yearmonth, ie if yr should =
               2003, and month = 06, then it returns 200306 */
            if (memos[memos_in_memory].year > (shown->year * 10))
                memos[memos_in_memory].year = (memos[memos_in_memory].year -
                                               memos[memos_in_memory].month) /
                    100;
            if (rb->read(fp, temp_memo1, 1) == 1)
                memos[memos_in_memory].wday = rb->atoi(&temp_memo1[0]);
            else
                memos[memos_in_memory].wday = 0;
            if (rb->read(fp, temp_memo1, 1) == 1)
                memos[memos_in_memory].type = rb->atoi(&temp_memo1[0]);
            else
                memos[memos_in_memory].type = 0;
            for (k = 0; k <= count; k++)
            {
                if (rb->read(fp, temp_memo1, 1) == 1)
                {
                    if (
                        (memos[memos_in_memory].type < 2)
                        ||
                        (
                            (memos[memos_in_memory].type == 2)
                            &&
                            (memos[memos_in_memory].month == shown->mon)
                        )
                        ||
                        (
                            (memos[memos_in_memory].type > 2)
                            &&
                            (memos[memos_in_memory].month == shown->mon)
                            &&
                            (memos[memos_in_memory].year == shown->year)
                        )
                       )
                    {
                        if (temp_memo1[0] == '\n')
                        {
                            if (memos[memos_in_memory].type > 0)
                                day_has_memo[memos[memos_in_memory].day] =
                                    true;
                            else
                                wday_has_memo[memos[memos_in_memory].wday] =
                                    true;
                            memos[memos_in_memory++].file_pointer_end =
                                rb->lseek(fp, 0, SEEK_CUR);
                        }
                        else if ( (temp_memo1[0] != '\r') &&
                                  (temp_memo1[0] != '\t') &&
                                  k < MAX_CHAR_MEMO_LEN-1 )
                            memos[memos_in_memory].message[k] = temp_memo1[0];
                    }
                    if (temp_memo1[0] == '\n')
                        break;
                }
                else
                {
                    memos[memos_in_memory].day = 0;
                    memos[memos_in_memory].month = 0;
                    memos[memos_in_memory].file_pointer_start = 0;
                    memos[memos_in_memory].file_pointer_end = 0;
                    memos[memos_in_memory].year = 0;
                    memos[memos_in_memory].type = 0;
                    memos[memos_in_memory].wday = 0;
                    memos[memos_in_memory].message[0] = 0;
                    exit = true;
                    break;
                }
            }
        }
    }
    rb->close(fp);
}

static bool save_memo(int changed, bool new_mod, struct shown *shown)
{
    int fp,fq;
    fp = rb->open(ROCKBOX_DIR "/.memo",O_RDONLY | O_CREAT);
    fq = rb->creat(ROCKBOX_DIR "/~temp");
    if ( (fq != -1) && (fp != -1) )
    {
        int i;
        char temp[MAX_CHAR_MEMO_LEN];
        rb->lseek(fp, 0, SEEK_SET);
        for (i = 0; i < memos[changed].file_pointer_start; i++)
        {
            rb->read(fp, temp, 1);
            rb->write(fq,temp,1);
        }
        if (new_mod)
        {
            rb->fdprintf(fq, "%02d%02d%04d%01d%01d%s\n",
                         memos[changed].day,
                         memos[changed].month,
                         memos[changed].year,
                         memos[changed].wday,
                         memos[changed].type,
                         memos[changed].message);
        }
        rb->lseek(fp, memos[changed].file_pointer_end, SEEK_SET);
        while(rb->read(fp, temp, 1) == 1)
        {
            rb->write(fq,temp,1);
        }
        rb->close(fp);
        rb->close(fq);
        rb->remove(ROCKBOX_DIR "/.memo");
        rb->rename(ROCKBOX_DIR "/~temp", ROCKBOX_DIR "/.memo");
        load_memo(shown);
        return true;
    }
    else if (fp != -1)
        rb->close(fp);
    else if (fq != -1)
        rb->close(fq);
    return false;
}

static void add_memo(struct shown *shown, int type)
{
    bool saved = false;
    if (rb->kbd_input(memos[memos_in_memory].message,
                      sizeof memos[memos_in_memory].message) != -1)
    {
        if (rb->strlen(memos[memos_in_memory].message))
        {
            memos[memos_in_memory].file_pointer_start = 0;
            memos[memos_in_memory].file_pointer_end = 0;
            memos[memos_in_memory].day = shown->mday;
            memos[memos_in_memory].month = shown->mon;
            memos[memos_in_memory].wday = shown->wday;
            memos[memos_in_memory].year = shown->year;
            memos[memos_in_memory].type = type;
            if (save_memo(memos_in_memory,true,shown))
            {
                saved = true;
            }
            else
            {
                memos[memos_in_memory].file_pointer_start = 0;
                memos[memos_in_memory].file_pointer_end = 0;
                memos[memos_in_memory].day = 0;
                memos[memos_in_memory].month = 0;
                memos[memos_in_memory].year = 0;
                memos[memos_in_memory].type = 0;
                memos[memos_in_memory].wday = 0;
            }
        }
    }
    rb->lcd_clear_display();
    if(use_system_font)
        rb->lcd_setfont(FONT_SYSFIXED);
    if (saved)
        rb->splash(HZ/2,"Event added");
    else
        rb->splash(HZ/2,"Event not added");
}

static int edit_menu_cb(int action, const struct menu_item_ex *this_item)
{
    (void) this_item;
    if (action == ACTION_REQUEST_MENUITEM && memos_in_shown_memory <= 0)
        return ACTION_EXIT_MENUITEM;
    return action;
}

static bool edit_memo(int change, struct shown *shown)
{
    bool exit = false;
    int selected = 0;

    MENUITEM_RETURNVALUE(edit_menu_remove, "Remove", 0,
                         edit_menu_cb, Icon_NOICON);
    MENUITEM_RETURNVALUE(edit_menu_edit, "Edit", 1,
                         edit_menu_cb, Icon_NOICON);
    MENUITEM_RETURNVALUE(edit_menu_weekly, "New Weekly", 2,
                         NULL, Icon_NOICON);
    MENUITEM_RETURNVALUE(edit_menu_monthly, "New Monthly", 3,
                         NULL, Icon_NOICON);
    MENUITEM_RETURNVALUE(edit_menu_yearly, "New Yearly", 4,
                         NULL, Icon_NOICON);
    MENUITEM_RETURNVALUE(edit_menu_oneoff, "New One off", 5,
                         NULL, Icon_NOICON);

    MAKE_MENU(edit_menu, "Edit menu",
              NULL, Icon_NOICON,
              &edit_menu_remove, &edit_menu_edit,
              &edit_menu_weekly, &edit_menu_monthly,
              &edit_menu_yearly, &edit_menu_oneoff);

    while (!exit)
    {
        switch (rb->do_menu(&edit_menu, &selected, NULL, false))
        {
            case 0: /* remove */
                save_memo(pointer_array[change],false,shown);
                return false;

            case 1: /* edit */
                if(rb->kbd_input(memos[pointer_array[change]].message,
                           sizeof memos[pointer_array[change]].message) != -1)
                    save_memo(pointer_array[change],true,shown);
                return false;

            case 2: /* weekly */
                add_memo(shown,0);
                return false;

            case 3: /* monthly */
                add_memo(shown,1);
                return false;

            case 4: /* yearly */
                add_memo(shown,2);
                return false;

            case 5: /* one off */
                add_memo(shown,3);
                return false;

            case GO_TO_PREVIOUS:
                return false;

            case MENU_ATTACHED_USB:
                been_in_usb_mode = true;
                break;
        }
    }
    return false;
}

static char * get_event_text(int selected, void *data,
                                  char *buffer, size_t buffer_len)
{
    struct shown *shown = (struct shown *) data;
    if (selected < 0 || memos_in_shown_memory <= selected)
    {
        return NULL;
    }
    if (memos[pointer_array[selected]].type == 2)
        rb->snprintf(buffer, buffer_len, "%s (%d yrs)",
                     memos[pointer_array[selected]].message,
                     shown->year - memos[pointer_array[selected]].year);
    else
        rb->snprintf(buffer, buffer_len, "%s",
                     memos[pointer_array[selected]].message);
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
        rb->gui_syncstatusbar_draw(rb->statusbars, true);
        button = rb->get_action(CONTEXT_LIST,TIMEOUT_BLOCK);
        rb->gui_synclist_do_button(&gui_memos,&button,LIST_WRAP_UNLESS_HELD);

        switch (button)
        {
            case ACTION_STD_OK:
                selected = rb->gui_synclist_get_sel_pos(&gui_memos);
                return edit_memo(selected, shown);
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
    memos_in_shown_memory = 0;
    for (i = 0; i < memos_in_memory; i++)
        if (
            (
             (memos[i].type >= 1)
             &&
             (memos[i].day == shown->mday)
            )
            ||
            (
             (memos[i].type < 1)
             &&
             (memos[i].wday == shown->wday)
            )
           )
            pointer_array[memos_in_shown_memory++] = i;
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
        shown->mon=1;
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

enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    struct today today;
    struct shown shown;
    bool exit = false;
    int button;
    
    (void)(parameter);

    rb = api;

    calendar_init(&today, &shown);
    load_memo(&shown);
    any_events(&shown, false);
    draw_calendar(&shown);
    while (!exit)
    {
        button = rb->button_get(true);
        switch (button)
        {
            case CALENDAR_QUIT:
                return PLUGIN_OK;

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
    return been_in_usb_mode?PLUGIN_USB_CONNECTED:PLUGIN_OK;
}

#endif
