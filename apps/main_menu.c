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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <timefuncs.h>
#include "config.h"
#include "options.h"

#include "menu.h"
#include "tree.h"
#include "credits.h"
#include "lcd.h"
#include "font.h"
#include "button.h"
#include "kernel.h"
#include "main_menu.h"
#include "version.h"
#include "debug_menu.h"
#include "sprintf.h"
#include <string.h>
#include "playlist.h"
#include "settings.h"
#include "settings_menu.h"
#include "power.h"
#include "powermgmt.h"
#include "sound_menu.h"
#include "status.h"
#include "fat.h"
#include "bookmark.h"
#include "wps.h"
#include "buffer.h"
#include "screens.h"
#include "playlist_menu.h"
#include "talk.h"
#ifdef HAVE_FMRADIO
#include "radio.h"
#endif
#include "misc.h"
#include "lang.h"

#ifdef HAVE_MAS3587F
#include "recording.h"
#endif

#ifdef HAVE_LCD_BITMAP
#include "bmp.h"
#include "icons.h"
#endif /* End HAVE_LCD_BITMAP */

int show_logo( void )
{
#ifdef HAVE_LCD_BITMAP
    char version[32];
    int font_h, font_w;

    lcd_clear_display();

    lcd_bitmap(rockbox112x37, 0, 10, 112, 37, false);

#if 0
    /*
     * This code is not used anymore, but I kept it here since it shows
     * one way of using the BMP reader function to display an externally
     * providing logo.
     */
    unsigned char buffer[112 * 8];
    int width, height;

    int failure;
    failure = read_bmp_file("/rockbox112.bmp", &width, &height, buffer);

    debugf("read_bmp_file() returned %d, width %d height %d\n",
           failure, width, height);
           
    lcd_bitmap(&buffer, 0, 10, width, height, false);
#endif

    snprintf(version, sizeof(version), "Ver. %s", appsversion);
    lcd_setfont(FONT_SYSFIXED);
    lcd_getstringsize("A", &font_w, &font_h);
    lcd_putsxy((LCD_WIDTH/2) - ((strlen(version)*font_w)/2),
               LCD_HEIGHT-font_h, version);
    lcd_update();

#else
    char *rockbox = "  ROCKbox!";
    lcd_clear_display();
    lcd_double_height(true);
    lcd_puts(0, 0, rockbox);
    lcd_puts(0, 1, appsversion);
#endif

    return 0;
}

bool show_credits(void)
{
    int j = 0;
    int btn;

    show_logo();
#ifdef HAVE_LCD_CHARCELLS
    lcd_double_height(false);
#endif
    
    for (j = 0; j < 10; j++) {
        sleep((HZ*2)/10);

        btn = button_get(false);
        if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
            return false;
    }
    roll_credits();
    return false;
}

#ifdef SIMULATOR
extern bool simulate_usb(void);
#endif
bool show_info(void)
{
    char s[32];
    /* avoid overflow for 8MB mod :) was: ((mp3end - mp3buf) * 1000) / 0x100000; */
    int buflen = ((mp3end - mp3buf) * 100) / 0x19999;
    int integer, decimal;
    bool done = false;
    int key;
    int state = 1;
    unsigned int size, free;

    fat_size( &size, &free );
    size /= 1024;
    free /= 1024;

    if (global_settings.talk_menu)
    {   /* say whatever is reasonable, no real connection to the screen */
        bool enqueue = false; /* enqueue all but the first */
        if (battery_level() >= 0)
        {
            talk_id(LANG_BATTERY_TIME, enqueue);
            enqueue = true;
            talk_value(battery_level(), UNIT_PERCENT, true);
        }

        talk_id(LANG_DISK_FREE_STAT, enqueue);
        talk_number(free / 1024, true);
        decimal = free % 1024 / 100;
        talk_id(VOICE_POINT, true);
        talk_value(decimal, UNIT_GB, true);

#ifdef HAVE_RTC
        {
            struct tm* tm = get_time();
            talk_id(VOICE_CURRENT_TIME, true);
            talk_value(tm->tm_hour, UNIT_HOUR, true);
            talk_value(tm->tm_min, UNIT_MIN, true);
            talk_value(tm->tm_sec, UNIT_SEC, true);
            talk_id(LANG_MONTH_JANUARY + tm->tm_mon, true);
            talk_number(tm->tm_mday, true);
            talk_number(1900 + tm->tm_year, true);
        }
#endif
    }

    while(!done)
    {
        int y=0;
        lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
        lcd_puts(0, y++, str(LANG_ROCKBOX_INFO));
        y++;
        state = 3;
#endif

        if (state & 1) {
            integer = buflen / 1000;
            decimal = buflen % 1000;
#ifdef HAVE_LCD_CHARCELLS
            snprintf(s, sizeof(s), str(LANG_BUFFER_STAT_PLAYER),
                     integer, decimal);
#else
            snprintf(s, sizeof(s), str(LANG_BUFFER_STAT_RECORDER),
                     integer, decimal);
#endif
            lcd_puts(0, y++, s);
            
#ifdef HAVE_CHARGE_CTRL
            if (charge_state == 1)
                snprintf(s, sizeof(s), str(LANG_BATTERY_CHARGE));
            else if (charge_state == 2)
                snprintf(s, sizeof(s), str(LANG_BATTERY_TOPOFF_CHARGE));
            else if (charge_state == 3)
                snprintf(s, sizeof(s), str(LANG_BATTERY_TRICKLE_CHARGE));
            else
#endif
            if (battery_level() >= 0)
                snprintf(s, sizeof(s), str(LANG_BATTERY_TIME), battery_level(),
                         battery_time() / 60, battery_time() % 60);
            else
                strncpy(s, "(n/a)", sizeof(s));
            lcd_puts(0, y++, s);
        }

        if (state & 2) {
            integer = size / 1024;
            decimal = size % 1024 / 100;
            snprintf(s, sizeof s, str(LANG_DISK_STAT), integer, decimal);
            lcd_puts(0, y++, s);
            
            integer = free / 1024;
            decimal = free % 1024 / 100;
            snprintf(s, sizeof s, str(LANG_DISK_FREE_STAT), integer, decimal);
            lcd_puts(0, y++, s);
        }
        lcd_update();

        /* Wait for a key to be pushed */
        key = button_get_w_tmo(HZ*5);
        switch(key) {
#if defined(HAVE_PLAYER_KEYPAD) || defined(HAVE_NEO_KEYPAD) || defined(HAVE_ONDIO_KEYPAD)
            case BUTTON_STOP | BUTTON_REL:
#else
            case BUTTON_LEFT | BUTTON_REL:
            case BUTTON_OFF | BUTTON_REL:
#endif
                done = true;
                break;

#if defined(HAVE_RECORDER_KEYPAD) || defined(HAVE_ONDIO_KEYPAD)
            case BUTTON_LEFT:
            case BUTTON_RIGHT:
                if (state == 1)
                    state = 2;
                else
                    state = 1;
                break;
#endif
           default:
               if(default_event_handler(key) == SYS_USB_CONNECTED)
                   return true;
               break;
        }
    }

    return false;
}

static bool plugin_browse(void)
{
    return rockbox_browse(PLUGIN_DIR, SHOW_PLUGINS);
}

#ifdef HAVE_MAS3587F
static bool recording_settings(void)
{
    return recording_menu(false);
}
#endif

#ifdef HAVE_MAS3587F

bool rec_menu(void)
{
    int m;
    bool result;

    /* recording menu */
    static const struct menu_item items[] = {
        { ID2P(LANG_RECORDING_MENU),     recording_screen  },
        { ID2P(LANG_RECORDING_SETTINGS), recording_settings},
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}
#endif

bool info_menu(void)
{
    int m;
    bool result;

    /* info menu */
    static const struct menu_item items[] = {
        { ID2P(LANG_MENU_SHOW_ID3_INFO), browse_id3        },
        { ID2P(LANG_INFO_MENU),          show_info         },
        { ID2P(LANG_VERSION),            show_credits      },
#ifndef SIMULATOR
        { ID2P(LANG_DEBUG),              debug_menu        },
#else
        { ID2P(LANG_USB),                simulate_usb      },
#endif
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

bool main_menu(void)
{
    int m;
    bool result;
    int i = 0;

    /* main menu */
    struct menu_item items[9];

    items[i].desc = ID2P(LANG_BOOKMARK_MENU);
    items[i++].function = bookmark_menu;

    items[i].desc = ID2P(LANG_SOUND_SETTINGS);
    items[i++].function = sound_menu;

    items[i].desc = ID2P(LANG_GENERAL_SETTINGS);
    items[i++].function = settings_menu;

#ifdef HAVE_FMRADIO
    if(radio_hardware_present()) {
        items[i].desc = ID2P(LANG_FM_RADIO);
        items[i++].function = radio_screen;
    }
#endif

#ifdef HAVE_MAS3587F
    items[i].desc = ID2P(LANG_RECORDING);
    items[i++].function = rec_menu;
#endif

    items[i].desc = ID2P(LANG_PLAYLIST_MENU);
    items[i++].function = playlist_menu;

    items[i].desc = ID2P(LANG_PLUGINS);
    items[i++].function = plugin_browse;

    items[i].desc = ID2P(LANG_INFO);
    items[i++].function = info_menu;

    items[i].desc = ID2P(LANG_SHUTDOWN);
    items[i++].function = clean_shutdown;

    m=menu_init( items, i, NULL, NULL, NULL, NULL );
#ifdef HAVE_LCD_CHARCELLS
    status_set_param(true);
#endif
    result = menu_run(m);
#ifdef HAVE_LCD_CHARCELLS
    status_set_param(false);
#endif
    menu_exit(m);

    settings_save();

    return result;
}

/* -----------------------------------------------------------------
 * vim: et sw=4 ts=8 sts=4 tw=78
 */
