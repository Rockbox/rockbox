/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Robert Hak
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdbool.h>

#include "lcd.h"
#include "menu.h"
#include "mpeg.h"
#include "button.h"
#include "kernel.h"
#include "sprintf.h"
#include "settings.h"
#include "settings_menu.h"
#include "backlight.h"
#include "playlist.h"  /* for playlist_shuffle */
#include "fat.h"                /* For dotfile settings */
#include "powermgmt.h"
#include "rtc.h"
#include "ata.h"
#include "lang.h"

static Menu show_hidden_files(void)
{
    set_bool_options( str(LANG_HIDDEN), &global_settings.show_hidden_files,
                      str(LANG_HIDDEN_SHOW), str(LANG_HIDDEN_HIDE) );
    return MENU_OK;
}

static Menu contrast(void)
{
    set_int( str(LANG_CONTRAST), "", &global_settings.contrast, 
             lcd_set_contrast, 1, 0, MAX_CONTRAST_SETTING );
    return MENU_OK;
}

#ifndef HAVE_RECORDER_KEYPAD
static Menu shuffle(void)
{
    set_bool( str(LANG_SHUFFLE), &global_settings.playlist_shuffle );
    return MENU_OK;
}
#endif

static Menu play_selected(void)
{
    set_bool( str(LANG_PLAY_SELECTED), &global_settings.play_selected );
    return MENU_OK;
}

static Menu mp3_filter(void)
{
    set_bool( str(LANG_MP3FILTER), &global_settings.mp3filter );
    return MENU_OK;
}

static Menu sort_case(void)
{
    set_bool( str(LANG_SORT_CASE), &global_settings.sort_case );
    return MENU_OK;
}

static Menu resume(void)
{
    char* names[] = { str(LANG_OFF), 
                      str(LANG_RESUME_SETTING_ASK),
                      str(LANG_ON) };
    set_option( str(LANG_RESUME), &global_settings.resume, names, 3, NULL );
    return MENU_OK;
}

static Menu backlight_timer(void)
{
    char* names[] = { str(LANG_OFF), str(LANG_ON),
                      "1s ", "2s ", "3s ", "4s ", "5s ",
                      "6s ", "7s ", "8s ", "9s ", "10s",
                      "15s", "20s", "25s", "30s", "45s",
                      "60s", "90s"};
    set_option(str(LANG_BACKLIGHT), &global_settings.backlight, names, 19, 
               backlight_time );
    return MENU_OK;
}

static Menu scroll_speed(void)
{
    set_int(str(LANG_SCROLL), "", &global_settings.scroll_speed, 
            &lcd_scroll_speed, 1, 1, 30 );
    return MENU_OK;
}

#ifdef HAVE_CHARGE_CTRL
static Menu deep_discharge(void)
{
    set_bool( str(LANG_DISCHARGE), &global_settings.discharge );
    charge_restart_level = global_settings.discharge ? 
        CHARGE_RESTART_LO : CHARGE_RESTART_HI;
    return MENU_OK;
}
#endif

#ifdef HAVE_LCD_BITMAP
static Menu timedate_set(void)
{
    int timedate[7]; /* hour,minute,second,year,month,day,dayofweek */

#ifdef HAVE_RTC
    timedate[0] = rtc_read(0x03); /* hour   */
    timedate[1] = rtc_read(0x02); /* minute */
    timedate[2] = rtc_read(0x01); /* second */
    timedate[3] = rtc_read(0x07); /* year   */
    timedate[4] = rtc_read(0x06); /* month  */
    timedate[5] = rtc_read(0x05); /* day    */
    /* day of week not read, calculated */
    /* hour   */
    timedate[0] = ((timedate[0] & 0x30) >> 4) * 10 + (timedate[0] & 0x0f); 
    /* minute */
    timedate[1] = ((timedate[1] & 0x70) >> 4) * 10 + (timedate[1] & 0x0f); 
    /* second */
    timedate[2] = ((timedate[2] & 0x70) >> 4) * 10 + (timedate[2] & 0x0f); 
    /* year   */
    timedate[3] = ((timedate[3] & 0xf0) >> 4) * 10 + (timedate[3] & 0x0f); 
    /* month  */
    timedate[4] = ((timedate[4] & 0x10) >> 4) * 10 + (timedate[4] & 0x0f); 
    /* day    */
    timedate[5] = ((timedate[5] & 0x30) >> 4) * 10 + (timedate[5] & 0x0f);
#else /* SIMULATOR */
    /* hour   */
    timedate[0] = 0;
    /* minute */
    timedate[1] = 0;
    /* second */
    timedate[2] = 0;
    /* year   */
    timedate[3] = 0;
    /* month  */
    timedate[4] = 1;
    /* day    */
    timedate[5] = 1;
#endif

    set_time(str(LANG_TIME),timedate);

#ifdef HAVE_RTC
    if(timedate[0] != -1) {
        /* hour   */
        timedate[0] = ((timedate[0]/10) << 4 | timedate[0]%10) & 0x3f; 
        /* minute */
        timedate[1] = ((timedate[1]/10) << 4 | timedate[1]%10) & 0x7f; 
        /* second */
        timedate[2] = ((timedate[2]/10) << 4 | timedate[2]%10) & 0x7f; 
        /* year   */
        timedate[3] = ((timedate[3]/10) << 4 | timedate[3]%10) & 0xff; 
        /* month  */
        timedate[4] = ((timedate[4]/10) << 4 | timedate[4]%10) & 0x1f; 
        /* day    */
        timedate[5] = ((timedate[5]/10) << 4 | timedate[5]%10) & 0x3f; 

        rtc_write(0x03, timedate[0] | (rtc_read(0x03) & 0xc0)); /* hour */
        rtc_write(0x02, timedate[1] | (rtc_read(0x02) & 0x80)); /* minute */
        rtc_write(0x01, timedate[2] | (rtc_read(0x01) & 0x80)); /* second */
        rtc_write(0x07, timedate[3]);                           /* year */
        rtc_write(0x06, timedate[4] | (rtc_read(0x06) & 0xe0)); /* month */
        rtc_write(0x05, timedate[5] | (rtc_read(0x05) & 0xc0)); /* day */
        rtc_write(0x04, timedate[6] | (rtc_read(0x04) & 0xf8)); /* dayofweek */
        rtc_write(0x00, 0x00); /* 0.1 + 0.01 seconds */
    }
#endif
    return MENU_OK;
}
#endif

static Menu spindown(void)
{
    set_int(str(LANG_SPINDOWN), "s", &global_settings.disk_spindown,
            ata_spindown, 1, 3, 254 );
    return MENU_OK;
}

static Menu ff_rewind_min_step(void) 
{ 
    char* names[] = { "1s", "2s", "3s", "4s",
                      "5s", "6s", "8s", "10s",
                      "15s", "20s", "25s", "30s",
                      "45s", "60s" };
    set_option(str(LANG_FFRW_STEP), &global_settings.ff_rewind_min_step,
                names, 14, NULL ); 
    return MENU_OK; 
} 

static Menu ff_rewind_accel(void) 
{ 
    char* names[] = { str(LANG_OFF), "2x/1s", "2x/2s", "2x/3s", 
                      "2x/4s", "2x/5s", "2x/6s", "2x/7s", 
                      "2x/8s", "2x/9s", "2x/10s", "2x/11s",
                      "2x/12s", "2x/13s", "2x/14s", "2x/15s", };
    set_option(str(LANG_FFRW_ACCEL), &global_settings.ff_rewind_accel, 
                names, 16, NULL ); 
    return MENU_OK; 
} 

static Menu browse_current(void)
{
    set_bool( str(LANG_FOLLOW), &global_settings.browse_current );
    return MENU_OK;
}

Menu playback_settings_menu(void)
{
    int m;
    Menu result;

    struct menu_items items[] = {
#ifndef HAVE_RECORDER_KEYPAD
        { str(LANG_SHUFFLE), shuffle },
#endif
        { str(LANG_PLAY_SELECTED), play_selected },
        { str(LANG_RESUME), resume },
        { str(LANG_FFRW_STEP), ff_rewind_min_step },
        { str(LANG_FFRW_ACCEL), ff_rewind_accel },
    };

    bool old_shuffle = global_settings.playlist_shuffle;
    
    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    result = menu_run(m);
    menu_exit(m);

    if (old_shuffle != global_settings.playlist_shuffle)
    {
        if (global_settings.playlist_shuffle)
        {
            randomise_playlist(current_tick);
        }
        else
        {
            sort_playlist(true);
        }
    }
    return result;
}

static Menu reset_settings(void)
{
    int button = 0;

    lcd_clear_display();
#ifdef HAVE_LCD_CHARCELLS
    lcd_puts(0,0,str(LANG_RESET_ASK_PLAYER));
    lcd_puts(0,1,str(LANG_RESET_CONFIRM_PLAYER));
#else
    lcd_puts(0,0,str(LANG_RESET_ASK_RECORDER));
    lcd_puts(0,1,str(LANG_RESET_CONFIRM_RECORDER));
    lcd_puts(0,2,str(LANG_RESET_CANCEL_RECORDER));
#endif
    lcd_update();
    button = button_get(true);
    if (button == BUTTON_PLAY) {
        settings_reset();
        lcd_clear_display();
        lcd_puts(0,0,str(LANG_RESET_DONE_SETTING));
        lcd_puts(0,1,str(LANG_RESET_DONE_CLEAR));
        lcd_update();
        sleep(HZ);
        return(true);
    } else {
        lcd_clear_display();
        lcd_puts(0,0,str(LANG_RESET_DONE_CANCEL));
        lcd_update();
        sleep(HZ);
        return(false);
    }
}

static Menu fileview_settings_menu(void)
{
    int m;
    Menu result;

    struct menu_items items[] = {
        { str(LANG_CASE_MENU),    sort_case           },
        { str(LANG_MP3FILTER),    mp3_filter          },
        { str(LANG_HIDDEN),       show_hidden_files   },
        { str(LANG_FOLLOW),       browse_current      },
    };

    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    result = menu_run(m);
    menu_exit(m);
    return result;
}

static Menu display_settings_menu(void)
{
    int m;
    Menu result;

    struct menu_items items[] = {
        { str(LANG_SCROLL_MENU),     scroll_speed    },  
        { str(LANG_BACKLIGHT),       backlight_timer },
        { str(LANG_CONTRAST),        contrast        },  
    };
    
    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    result = menu_run(m);
    menu_exit(m);
    return result;
}

static Menu system_settings_menu(void)
{
    int m;
    Menu result;

    struct menu_items items[] = {
        { str(LANG_SPINDOWN),    spindown        },
#ifdef HAVE_CHARGE_CTRL
        { str(LANG_DISCHARGE),   deep_discharge  },
#endif
#ifdef HAVE_LCD_BITMAP
        { str(LANG_TIME),        timedate_set    },
#endif
        { str(LANG_RESET),        reset_settings },
    };
    
    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    result = menu_run(m);
    menu_exit(m);
    return result;
}

Menu settings_menu(void)
{
    int m;
    Menu result;

    struct menu_items items[] = {
        { str(LANG_PLAYBACK),        playback_settings_menu },
        { str(LANG_FILE),            fileview_settings_menu },
        { str(LANG_DISPLAY),         display_settings_menu  },
        { str(LANG_SYSTEM),          system_settings_menu   },
    };
    
    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    result = menu_run(m);
    menu_exit(m);
    return result;
}
