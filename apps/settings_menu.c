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

static Menu show_hidden_files(void)
{
    set_bool( "[Show hidden files]", &global_settings.show_hidden_files );
    return MENU_OK;
}

static Menu contrast(void)
{
    set_int( "[Contrast]", "", &global_settings.contrast, 
             lcd_set_contrast, 1, 0, MAX_CONTRAST_SETTING );
    return MENU_OK;
}

static Menu shuffle(void)
{
    set_bool( "[Shuffle]", &global_settings.playlist_shuffle );
    return MENU_OK;
}

static Menu mp3_filter(void)
{
    set_bool( "[MP3/M3U filter]", &global_settings.mp3filter );
    return MENU_OK;
}

static Menu sort_case(void)
{
    set_bool( "[Sort case sensitive]", &global_settings.sort_case );
    return MENU_OK;
}

static Menu resume(void)
{
    char* names[] = { "off", "ask", "on " };
    set_option( "[Resume]", &global_settings.resume, names, 3 );
    return MENU_OK;
}

static Menu backlight_timer(void)
{
    char* names[] = { "off", "on ",
                      "1s ", "2s ", "3s ", "4s ", "5s ",
                      "6s ", "7s ", "8s ", "9s ", "10s",
                      "15s", "20s", "25s", "30s", "45s",
                      "60s", "90s"};
    set_option("[Backlight]", &global_settings.backlight, names, 19 );
    backlight_time(global_settings.backlight);
    return MENU_OK;
}

static Menu scroll_speed(void)
{
    set_int("Scroll speed indicator...", "", &global_settings.scroll_speed, 
            &lcd_scroll_speed, 1, 1, 20 );
    return MENU_OK;
}

static Menu wps_set(void)
{
#ifdef HAVE_LCD_BITMAP
#ifdef CUSTOM_WPS
    char* names[] = { "ID3 Tags", "File     ", "Parse    ", "Custom WPS" };
    set_option("[WPS display]", &global_settings.wps_display, names, 4 );
#else
    char* names[] = { "ID3 Tags", "File     ", "Parse    " };
    set_option("[WPS display]", &global_settings.wps_display, names, 3 );
#endif
#else
#ifdef CUSTOM_WPS
    char* names[] = { "1 Line ID3", "2 Line ID3", "File      ",
        "Parse     ", "Custom WPS " };
    set_option("[WPS display]", &global_settings.wps_display, names, 5 );
#else
    char* names[] = { "1 Line ID3", "2 Line ID3", "File      ",
        "Parse     " };
    set_option("[WPS display]", &global_settings.wps_display, names, 4 );
#endif

#endif
    return MENU_OK;
}

#ifdef HAVE_CHARGE_CTRL
static Menu deep_discharge(void)
{
    set_bool( "[Deep discharge]", &global_settings.discharge );
    charge_restart_level = global_settings.discharge ? CHARGE_RESTART_LO : CHARGE_RESTART_HI;
    return MENU_OK;
}
#endif

#ifdef HAVE_RTC
static Menu timedate_set(void)
{
    int timedate[7]; /* hour,minute,second,year,month,day,dayofweek */


    timedate[0] = rtc_read(0x03); /* hour   */
    timedate[1] = rtc_read(0x02); /* minute */
    timedate[2] = rtc_read(0x01); /* second */
    timedate[3] = rtc_read(0x07); /* year   */
    timedate[4] = rtc_read(0x06); /* month  */
    timedate[5] = rtc_read(0x05); /* day    */
    /* day of week not read, calculated */
    timedate[0] = ((timedate[0] & 0x70) >> 4) * 10 + (timedate[0] & 0x0f); /* hour   */
    timedate[1] = ((timedate[1] & 0xf0) >> 4) * 10 + (timedate[1] & 0x0f); /* minute */
    timedate[2] = ((timedate[2] & 0x30) >> 4) * 10 + (timedate[2] & 0x0f); /* second */
    timedate[3] = ((timedate[3] & 0x30) >> 4) * 10 + (timedate[3] & 0x0f); /* year   */
    timedate[4] = ((timedate[4] & 0x30) >> 4) * 10 + (timedate[4] & 0x0f); /* month  */
    timedate[5] = ((timedate[5] & 0x30) >> 4) * 10 + (timedate[5] & 0x0f); /* day    */

    set_time("[Set time/date]",timedate);

    if(timedate[0] != -1) {
        timedate[0] = ((timedate[0]/10) << 4 | timedate[0]%10) & 0x3f; /* hour   */
        timedate[1] = ((timedate[1]/10) << 4 | timedate[1]%10) & 0x7f; /* minute */
        timedate[2] = ((timedate[2]/10) << 4 | timedate[2]%10) & 0x7f; /* second */
        timedate[3] = ((timedate[3]/10) << 4 | timedate[3]%10) & 0xff; /* year   */
        timedate[4] = ((timedate[4]/10) << 4 | timedate[4]%10) & 0x1f; /* month  */
        timedate[5] = ((timedate[5]/10) << 4 | timedate[5]%10) & 0x3f; /* day    */
        rtc_write(0x03, timedate[0] | (rtc_read(0x03) & 0xc0));  /* hour               */
        rtc_write(0x02, timedate[1] | (rtc_read(0x02) & 0x80));  /* minute             */
        rtc_write(0x01, timedate[2] | (rtc_read(0x01) & 0x80));  /* second             */
        rtc_write(0x07, timedate[3]);                            /* year               */
        rtc_write(0x06, timedate[4] | (rtc_read(0x06) & 0xe0));  /* month              */
        rtc_write(0x05, timedate[5] | (rtc_read(0x05) & 0xc0));  /* day                */
        rtc_write(0x04, timedate[6] | (rtc_read(0x04) & 0xf8));  /* dayofweek          */
        rtc_write(0x00, 0x00);                                   /* 0.1 + 0.01 seconds */
    }
    return MENU_OK;
}
#endif

static Menu spindown(void)
{
    set_int("[Disk spindown]", "s", &global_settings.disk_spindown,
            ata_spindown, 1, 1, 254 );
    return MENU_OK;
}

static Menu ff_rewind_accel(void) 
{ 
    char* names[] = { "off   ", "2x/1s ", "2x/2s ", "2x/3s ", 
                      "2x/4s ", "2x/5s ", "2x/6s ", "2x/7s ", 
                      "2x/8s ", "2x/9s ", "2x/10s", "2x/11s",
                      "2x/12s", "2x/13s", "2x/14s", "2x/15s", };
    set_option("[FF/rewind accel]", &global_settings.ff_rewind_accel, 
                names, 16 ); 
    return MENU_OK; 
} 

Menu settings_menu(void)
{
    int m;
    Menu result;

    struct menu_items items[] = {
        { "Shuffle",         shuffle         }, 
        { "MP3/M3U filter",  mp3_filter      },
        { "Sort mode",       sort_case       },
        { "Backlight Timer", backlight_timer },
        { "Contrast",        contrast    },  
        { "Scroll speed",    scroll_speed    },  
        { "While Playing",   wps_set         },
#ifdef HAVE_CHARGE_CTRL
        { "Deep discharge",  deep_discharge  },
#endif
#ifdef HAVE_RTC
        { "Time/Date",       timedate_set    },
#endif
        { "Show hidden files", show_hidden_files },
        { "FF/Rewind Accel", ff_rewind_accel },
        { "Resume",          resume          },
        { "Disk spindown",   spindown        },
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
