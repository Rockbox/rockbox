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
#include "action.h"
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
#if CONFIG_TUNER
#include "radio.h"
#endif
#include "misc.h"
#include "lang.h"
#include "logfdisp.h"
#include "plugin.h"
#include "filetypes.h"
#include "splash.h"

#ifdef HAVE_RECORDING
#include "recording.h"
#endif

#ifdef HAVE_RECORDING

static bool rec_menu_recording_screen(void)
{
    return recording_screen(false);
}

static bool recording_settings(void)
{
    bool ret;
#ifdef HAVE_FMRADIO_IN
    int rec_source = global_settings.rec_source;
#endif

    ret = recording_menu(false);

#ifdef HAVE_FMRADIO_IN
    if (rec_source != global_settings.rec_source)
    {
        if (rec_source == AUDIO_SRC_FMRADIO)
            radio_stop();
        /* If AUDIO_SRC_FMRADIO was selected from something else,
           the recording screen will start the radio */       
    }
#endif

    return ret;
}

bool rec_menu(void)
{
    int m;
    bool result;

    /* recording menu */
    static const struct menu_item items[] = {
        { ID2P(LANG_RECORDING_MENU),     rec_menu_recording_screen  },
        { ID2P(LANG_RECORDING_SETTINGS), recording_settings},
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}
#endif


#if 0
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
    static bool inside_menu = false;


    /* main menu */
    struct menu_item items[11];

    if(inside_menu) return false;
    inside_menu = true;

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

#if CONFIG_TUNER
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

    inside_menu = false;

    return result;
}
#endif
/* -----------------------------------------------------------------
 * vim: et sw=4 ts=8 sts=4 tw=78
 */
