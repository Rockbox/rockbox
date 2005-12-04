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
#include "lcd.h"
#include "font.h"
#include "button.h"
#include "kernel.h"
#include "main_menu.h"
#include "debug_menu.h"
#include "sprintf.h"
#include <string.h>
#include "settings.h"
#include "settings_menu.h"
#include "power.h"
#include "powermgmt.h"
#include "sound_menu.h"
#include "status.h"
#include "fat.h"
#include "bookmark.h"
#include "buffer.h"
#include "screens.h"
#include "playlist_menu.h"
#include "talk.h"
#ifdef CONFIG_TUNER
#include "radio.h"
#endif
#include "misc.h"
#include "lang.h"
#include "logfdisp.h"
#include "plugin.h"
#include "filetypes.h"

#ifdef HAVE_RECORDING
#include "recording.h"
#endif

#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif

bool show_credits(void)
{
    plugin_load("/.rockbox/rocks/credits.rock",NULL);
    return false;
}

#ifdef SIMULATOR
extern bool simulate_usb(void);
#endif

#ifdef HAVE_LCD_CHARCELLS
#define SIZE_FMT "%s%s"
#else
#define SIZE_FMT "%s %s"
#endif

bool show_info(void)
{
    char s[32], s1[32];
    long buflen = ((audiobufend - audiobuf) * 2) / 2097;  /* avoid overflow */
    int integer, decimal;
    bool done = false;
    int key;
    int state = 1;
    unsigned long size, free;
#ifdef HAVE_MULTIVOLUME
    char s2[32];
    unsigned long size2 = 0;
    unsigned long free2 = 0;
#endif

    const unsigned char *kbyte_units[] = {
        ID2P(LANG_KILOBYTE),
        ID2P(LANG_MEGABYTE),
        ID2P(LANG_GIGABYTE)
    };

    fat_size( IF_MV2(0,) &size, &free );
#ifdef HAVE_MULTIVOLUME
    if (fat_ismounted(1))
        fat_size( 1, &size2, &free2 );
#endif

    if (global_settings.talk_menu)
    {   /* say whatever is reasonable, no real connection to the screen */
        bool enqueue = false; /* enqueue all but the first */
        if (battery_level() >= 0)
        {
            talk_id(LANG_BATTERY_TIME, enqueue);
            enqueue = true;
            talk_value(battery_level(), UNIT_PERCENT, true);
        }

        talk_id(LANG_DISK_FREE_INFO, enqueue);
#ifdef HAVE_MULTIVOLUME
        talk_id(LANG_DISK_NAME_INTERNAL, true);
        output_dyn_value(NULL, 0, free, kbyte_units, true);
        if (size2)
        {
            talk_id(LANG_DISK_NAME_MMC, true);
            output_dyn_value(NULL, 0, free2, kbyte_units, true);
        }
#else
        output_dyn_value(NULL, 0, free, kbyte_units, true); /* NULL == talk */
#endif

#ifdef CONFIG_RTC
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
#ifdef HAVE_MULTIVOLUME
            output_dyn_value(s1, sizeof s1, free, kbyte_units, true);
            output_dyn_value(s2, sizeof s2, size, kbyte_units, true);
            snprintf(s, sizeof s, "%s %s/%s", str(LANG_DISK_NAME_INTERNAL),
                     s1, s2);
#else
            output_dyn_value(s1, sizeof s1, size, kbyte_units, true);
            snprintf(s, sizeof s, SIZE_FMT, str(LANG_DISK_SIZE_INFO), s1);
#endif
            lcd_puts(0, y++, s);

#ifdef HAVE_MULTIVOLUME
            if (size2) {
                output_dyn_value(s1, sizeof s1, free2, kbyte_units, true);
                output_dyn_value(s2, sizeof s2, size2, kbyte_units, true);
                snprintf(s, sizeof s, "%s %s/%s", str(LANG_DISK_NAME_MMC),
                         s1, s2);
                lcd_puts(0, y++, s);
            }
#else
            output_dyn_value(s1, sizeof s1, free, kbyte_units, true);
            snprintf(s, sizeof s, SIZE_FMT, str(LANG_DISK_FREE_INFO), s1);
            lcd_puts(0, y++, s);
#endif
        }
        lcd_update();

        /* Wait for a key to be pushed */
        key = button_get_w_tmo(HZ*5);
        switch(key) {

            case SETTINGS_OK:
#ifdef SETTINGS_OK2
            case SETTINGS_OK2:
#endif
            case SETTINGS_CANCEL:
                done = true;
                break;

            case SETTINGS_INC:
            case SETTINGS_DEC:
                if (state == 1)
                    state = 2;
                else
                    state = 1;
                break;

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

static bool custom_theme_browse(void)
{
    return rockbox_browse(THEME_DIR, SHOW_CFG);
}

#ifdef HAVE_RECORDING

static bool recording_settings(void)
{
    return recording_menu(false);
}

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
        { ID2P(LANG_INFO_MENU),          show_info         },
        { ID2P(LANG_VERSION),            show_credits      },
#ifndef SIMULATOR
        { ID2P(LANG_DEBUG),              debug_menu        },
#else
        { ID2P(LANG_USB),                simulate_usb      },
#ifdef ROCKBOX_HAS_LOGF
        {"logf", logfdisplay },
        {"logfdump", logfdump },
#endif
#endif
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

#ifdef HAVE_LCD_CHARCELLS
static bool do_shutdown(void)
{
    sys_poweroff();
    return false;
}
#endif
    
bool main_menu(void)
{
    int m;
    bool result;
    int i = 0;

    /* main menu */
    struct menu_item items[11];

    items[i].desc = ID2P(LANG_BOOKMARK_MENU_RECENT_BOOKMARKS);
    items[i++].function = bookmark_mrb_load;

    items[i].desc = ID2P(LANG_SOUND_SETTINGS);
    items[i++].function = sound_menu;

    items[i].desc = ID2P(LANG_GENERAL_SETTINGS);
    items[i++].function = settings_menu;

    items[i].desc = ID2P(LANG_MANAGE_MENU);
    items[i++].function = manage_settings_menu;

    items[i].desc = ID2P(LANG_CUSTOM_THEME);
    items[i++].function = custom_theme_browse;

#ifdef CONFIG_TUNER
    if(radio_hardware_present()) {
        items[i].desc = ID2P(LANG_FM_RADIO);
        items[i++].function = radio_screen;
    }
#endif

#ifdef HAVE_RECORDING
    items[i].desc = ID2P(LANG_RECORDING);
    items[i++].function = rec_menu;
#endif

    items[i].desc = ID2P(LANG_PLAYLIST_MENU);
    items[i++].function = playlist_menu;

    items[i].desc = ID2P(LANG_PLUGINS);
    items[i++].function = plugin_browse;

    items[i].desc = ID2P(LANG_INFO);
    items[i++].function = info_menu;

#ifdef HAVE_LCD_CHARCELLS
    items[i].desc = ID2P(LANG_SHUTDOWN);
    items[i++].function = do_shutdown;
#endif
    
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
