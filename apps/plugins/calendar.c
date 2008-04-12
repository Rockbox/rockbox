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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"

#if defined(HAVE_LCD_BITMAP) && (CONFIG_RTC != 0)

#include <timefuncs.h>

PLUGIN_HEADER

static struct plugin_api* rb;

static bool leap_year;
static int days_in_month[2][13] = {
    {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};

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
#ifdef SIMULATOR
    today->wday = 3;
    today->mday = 13;
#endif
    shown->mday = today->mday;
    shown->mon = today->mon;
    shown->year = today->year;
    shown->wday = today->wday;
#endif
    shown->firstday = calc_weekday(shown);
    leap_year = is_leap_year(shown->year);
}

static int space = LCD_WIDTH / 7;
static void draw_headers(void)
{
    int i,w,h;
    char *Dayname[7] = {"M","T","W","T","F","S","S"};
    int ws = 2;
    rb->lcd_getstringsize("A",&w,&h);
    for (i = 0; i < 8;)
    {
        rb->lcd_putsxy(ws, 0 , Dayname[i++]);
        ws += space;
    }
    rb->lcd_hline(0, LCD_WIDTH-1 ,h);
}

static bool day_has_memo[31];
static bool wday_has_memo[6];
static void draw_calendar(struct shown *shown)
{
    int w,h;
    int ws,row,pos,days_per_month,j;
    char buffer[9];
    char *Monthname[] = {
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
    rb->lcd_getstringsize("A",&w,&h);
    rb->lcd_clear_display();
    draw_headers();
    if (shown->firstday > 6)
        shown->firstday -= 7;
    row = 1;
    pos = shown->firstday;
    days_per_month = days_in_month[leap_year][shown->mon];
    ws = 2 + (pos * space);
    for (j = 0; j < days_per_month;)
    {
        if ( (day_has_memo[++j]) || (wday_has_memo[pos]) )
            rb->snprintf(buffer,4,"%02d.", j);
        else
            rb->snprintf(buffer,4,"%02d", j);
        rb->lcd_putsxy(ws, (row * h) + 5 ,buffer);
        if (shown->mday == j)
        {
            rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
            rb->lcd_fillrect(ws, row*h+5, space, h);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            shown->wday = pos;
        }
        ws += space;
        pos++;
        if (pos >= 7)
        {
            row++;
            pos = 0;
            ws = 2;
        }
    }
    rb->lcd_vline(60,LCD_HEIGHT-h-3,LCD_HEIGHT-1);
    rb->lcd_hline(60,LCD_WIDTH-1,LCD_HEIGHT-h-3);
    rb->snprintf(buffer,9,"%s %04d",Monthname[shown->mon-1],shown->year);
    rb->lcd_putsxy(62,(LCD_HEIGHT-h-1),buffer);
    shown->lastday = pos;
    rb->lcd_update();
}

#define MAX_CHAR_MEMO_LEN 63
#define MAX_MEMOS_IN_A_MONTH 127
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
    for (k = 0; k < memos_in_memory; k++)
    {
        memos[k].day = 0;
        memos[k].month = 0;
        memos[k].file_pointer_start = 0;
        memos[k].file_pointer_end = 0;
        memos[k].year = 0;
        memos[k].type = 0;
        memos[k].wday = 0;
        for (i = 0; i <= MAX_CHAR_MEMO_LEN; i++)
            rb->strcpy(&memos[k].message[i],"");
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
                                  (temp_memo1[0] != '\t') )
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
                    rb->strcpy(&memos[memos_in_memory].message[0], "");
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
        char temp[MAX_CHAR_MEMO_LEN + 1];
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
        for (i = memos[changed].file_pointer_end;
             i < rb->filesize(fp); i++)
        {
            rb->read(fp, temp, 1);
            rb->write(fq,temp,1);
        }
        rb->close(fp);
        fp = rb->creat(ROCKBOX_DIR "/.memo");
        rb->lseek(fp, 0, SEEK_SET);
        rb->lseek(fq, 0, SEEK_SET);
        for (i = 0; i < rb->filesize(fq); i++)
        {
            rb->read(fq, temp, 1);
            rb->write(fp,temp,1);
        }
        rb->close(fp);
        rb->close(fq);
        rb->remove(ROCKBOX_DIR "/~temp");
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
                memos_in_memory++;
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
        rb->lcd_puts(0,0,"Event added");
    else
        rb->lcd_puts(0,0,"Event not added");
    rb->lcd_update();
    rb->sleep(HZ/2);
}

static bool edit_memo(int change, struct shown *shown)
{
    bool exit = false;
    int button;
    
    while (!exit)
    {
        rb->lcd_clear_display();
        if (memos_in_shown_memory > 0)
        {
            rb->lcd_puts(0,0,"Remove : Up");
            rb->lcd_puts(0,1,"Edit : Down");
            rb->lcd_puts(0,2,"New :");
            rb->lcd_puts(2,3,"weekly : Left");
            rb->lcd_puts(2,4,"monthly : Play");
            rb->lcd_puts(2,5,"annually : Right");
            rb->lcd_puts(2,6,"one off : On");
        }
        else
        {
            rb->lcd_puts(0,0,"New :");
            rb->lcd_puts(2,1,"weekly : Left");
            rb->lcd_puts(2,2,"monthly : Play");
            rb->lcd_puts(2,3,"anualy : Right");
            rb->lcd_puts(2,4,"one off : On");
        }
        rb->lcd_update();
        button = rb->button_get(true);
        switch (button)
        {
            case BUTTON_OFF:
                return false;

            case BUTTON_LEFT:
                add_memo(shown,0);
                return false;

            case BUTTON_PLAY:
                add_memo(shown,1);
                return false;

            case BUTTON_RIGHT:
                add_memo(shown,2);
                return false;

            case BUTTON_ON:
                add_memo(shown,3);
                return false;

            case BUTTON_DOWN:
                if (memos_in_shown_memory > 0)
                {
                    if(rb->kbd_input(memos[pointer_array[change]].message,
                                     sizeof memos[pointer_array[change]].message) != -1)
                        save_memo(pointer_array[change],true,shown);
                    if(use_system_font)
                        rb->lcd_setfont(FONT_SYSFIXED);
                    exit = true;
                }
                break;

            case BUTTON_UP:
                if (memos_in_shown_memory > 0)
                {
                    save_memo(pointer_array[change],false,shown);
                    exit = true;
                }
                break;

            default:
                if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    been_in_usb_mode = true;
                break;
        }
    }
    return false;
}

static int start = 0;

static void show_lines(int selected, struct shown *shown)
{
    int lines,j = 1,w,h,i,k = 0, pos = 1,m = 0;
    char temp[MAX_CHAR_MEMO_LEN + 12];
    rb->lcd_getstringsize("A",&w,&h);
    lines = (LCD_HEIGHT / h) - 1;
        
    rb->lcd_clear_display();
    rb->lcd_puts(0,0,"Events (play : menu)");
    
    while (selected >= (lines + start))
        start++;
    while (selected < start)
        start--;
    i = start;
    while ( (i < memos_in_shown_memory) && (k < lines) )
    {
        if (memos[pointer_array[i]].type == 2)
            rb->snprintf(temp, sizeof temp, "%s (%d yrs)",
                         memos[pointer_array[i]].message,
                         shown->year - memos[pointer_array[i]].year);
        else
            rb->snprintf(temp, sizeof temp, "%s",
                         memos[pointer_array[i]].message);
        m = 0;
        if (i == selected)
        {
            pos = k + 1;
            rb->lcd_puts_scroll(m,j++,temp);
        }
        else
            rb->lcd_puts(m,j++,temp);
        k++;
        i++;
    }
    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
    rb->lcd_fillrect(0, (pos) * h, LCD_WIDTH, h);
    rb->lcd_set_drawmode(DRMODE_SOLID);
}

static void update_memos_shown(struct shown *shown)
{
    int i;
    memos_in_shown_memory = 0;
    start = 0;
    for (i = 0; i < memos_in_memory; i++)
        if (
            (memos[i].day == shown->mday)
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
    int lines_displayed = 0;
    bool exit=false;
    int button;
    
    update_memos_shown(shown);
    if (memos_in_shown_memory > 0)
        show_lines(lines_displayed,shown);
    else if (force)
        return edit_memo(lines_displayed, shown);
    else
        return false;
    rb->lcd_update();
    while (!exit)
    {
        button = rb->button_get(true);
        switch (button)
        {
            case BUTTON_DOWN:
                if (memos_in_shown_memory > 0)
                {
                    lines_displayed++;
                    if (lines_displayed >= memos_in_shown_memory)
                        lines_displayed = memos_in_shown_memory - 1;
                    show_lines(lines_displayed,shown);
                    rb->lcd_update();
                }
                break;

            case BUTTON_UP:
                if (memos_in_shown_memory > 0)
                {
                    lines_displayed--;
                    if (lines_displayed < 0)
                        lines_displayed = 0;
                    show_lines(lines_displayed,shown);
                    rb->lcd_update();
                }
                break;

            case BUTTON_PLAY:
                return edit_memo(lines_displayed, shown);

            case BUTTON_OFF:
                return false;

            default:
                if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    been_in_usb_mode = true;
                show_lines(lines_displayed,shown);
                rb->lcd_update();
                break;
        }
    }
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
    else if (step > 0)
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

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
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
            case BUTTON_OFF:
                return false;

            case BUTTON_ON | BUTTON_DOWN:
            case BUTTON_ON | BUTTON_DOWN | BUTTON_REPEAT:
                next_month(&shown, 0);
                break;

            case BUTTON_ON | BUTTON_UP:
            case BUTTON_ON | BUTTON_UP | BUTTON_REPEAT:
                prev_month(&shown, 0);
                break;

            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
                next_day(&shown, 7);
                break;

            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
                prev_day(&shown, 7);
                break;

            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
                prev_day(&shown, 1);
                break;

            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
                next_day(&shown, 1);
                break;

            case BUTTON_PLAY:
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
