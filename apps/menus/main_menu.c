
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:  $
 *
 * Copyright (C) 2007 Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include "config.h"
#include "lang.h"
#include "action.h"
#include "settings.h"
#include "powermgmt.h"
#include "menu.h"
#include "settings_menu.h"
#include "exported_menus.h"
#include "tree.h"
#ifdef CONFIG_TUNER
#include "radio.h"
#endif
#ifdef HAVE_RECORDING
#include "recording.h"
#endif
#include "bookmark.h"

/* lazy coders can use this function if the needed callback 
	is just to say if the item is shown or not */
int dynamicitem_callback(int action,const struct menu_item_ex *this_item);

/***********************************/
/*    MAIN MENU                    */

struct browse_folder_info {
	const char* dir;
	int show_options;
};
static struct browse_folder_info theme = {THEME_DIR, SHOW_CFG};
static struct browse_folder_info rocks = {PLUGIN_DIR, SHOW_PLUGINS};
static int browse_folder(void *param)
{
	const struct browse_folder_info *info =
		(const struct browse_folder_info*)param;
    return rockbox_browse(info->dir, info->show_options);
}
MENUITEM_FUNCTION_WPARAM(browse_themes, ID2P(LANG_CUSTOM_THEME), 
		browse_folder, (void*)&theme, NULL);
MENUITEM_FUNCTION_WPARAM(browse_plugins, ID2P(LANG_PLUGINS),
		browse_folder, (void*)&rocks, NULL);

#ifdef CONFIG_TUNER
MENUITEM_FUNCTION(load_radio_screen, ID2P(LANG_FM_RADIO),
                   (menu_function)radio_screen, dynamicitem_callback);
#endif

#include "settings_menu.h"
MENUITEM_FUNCTION(manage_settings_menu_item, ID2P(LANG_MANAGE_MENU),
                (menu_function)manage_settings_menu, NULL);
bool info_menu(void); /* from apps/main_menu.c TEMP*/
MENUITEM_FUNCTION(info_menu_item, ID2P(LANG_INFO),
                (menu_function)info_menu, NULL);
MENUITEM_FUNCTION(mrb_bookmarks, ID2P(LANG_BOOKMARK_MENU_RECENT_BOOKMARKS),
                   (menu_function)bookmark_mrb_load, NULL);

#ifdef HAVE_LCD_CHARCELLS
static int do_shutdown(void)
{
    sys_poweroff();
    return 0;
}
MENUITEM_FUNCTION(do_shutdown_item, ID2P(LANG_SHUTDOWN), do_shutdown, NULL);
#endif

/* NOTE: This title will be translatable once we decide what to call this menu
         when the root menu comes in... hopefully in the next few days */
MAKE_MENU(main_menu_, "Rockbox Main Menu", NULL,
        &mrb_bookmarks, &sound_settings,
        &settings_menu_item, &manage_settings_menu_item, &browse_themes,
#ifdef CONFIG_TUNER
        &load_radio_screen,
#endif
#ifdef HAVE_RECORDING
        &recording_settings_menu,
#endif
        &playlist_menu_item, &browse_plugins, &info_menu_item
#ifdef HAVE_LCD_CHARCELLS
        ,&do_shutdown_item
#endif
        );
/*    MAIN MENU                    */
/***********************************/

/* lazy coders can use this function if the needed 
   callback is just to say if the item is shown or not */
int dynamicitem_callback(int action,const struct menu_item_ex *this_item)
{
    if (action != ACTION_ENTER_MENUITEM)
        return action;
	
#ifdef CONFIG_TUNER
    if (this_item == &load_radio_screen)
    {
        if (radio_hardware_present() == 0)
            return ACTION_EXIT_MENUITEM;
    }
#else
    (void)this_item;
#endif

    return action;
}
