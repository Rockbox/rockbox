/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "menu.h"
#include "tree.h"
#include "list.h"
#ifdef HAVE_LCD_BITMAP
#include "peakmeter.h"
#endif
#include "talk.h"
#include "color_picker.h"
#include "lcd.h"
#include "lcd-remote.h"

#if (LCD_DEPTH > 1) || (defined(HAVE_LCD_REMOTE) && (LCD_REMOTE_DEPTH > 1))
#include "backdrop.h"
#endif
#include "exported_menus.h"


#if LCD_DEPTH > 1
/**
* Menu to clear the backdrop image
 */
static int clear_main_backdrop(void)
{
    global_settings.backdrop_file[0]=0;
    unload_main_backdrop();
    show_main_backdrop();
    settings_save();
    return 0;
}
MENUITEM_FUNCTION(clear_main_bd, 0, ID2P(LANG_CLEAR_BACKDROP),
                    clear_main_backdrop, NULL, NULL, Icon_NOICON);
#endif
#ifdef HAVE_LCD_COLOR
/**
 * Menu for fore/back colors
 */
static int set_fg_color(void)
{
    int res;
    res = (int)set_color(&screens[SCREEN_MAIN],str(LANG_FOREGROUND_COLOR),
                     &global_settings.fg_color,global_settings.bg_color);

    screens[SCREEN_MAIN].set_foreground(global_settings.fg_color);
    settings_save();
    settings_apply();
    return res;
}

static int set_bg_color(void)
{
    int res;
    res = (int)set_color(&screens[SCREEN_MAIN],str(LANG_BACKGROUND_COLOR),
                     &global_settings.bg_color,global_settings.fg_color);

    screens[SCREEN_MAIN].set_background(global_settings.bg_color);
    settings_save();
    settings_apply();
    return res;
}

/* Line selector colour */
static int set_lss_color(void)
{
    int res;
    res = (int)set_color(&screens[SCREEN_MAIN],str(LANG_SELECTOR_START_COLOR),
                         &global_settings.lss_color,-1);

    screens[SCREEN_MAIN].set_selector_start(global_settings.lss_color);
    settings_save();
    settings_apply();
    return res;
}

static int set_lse_color(void)
{
    int res;
    res = (int)set_color(&screens[SCREEN_MAIN],str(LANG_SELECTOR_END_COLOR),
                         &global_settings.lse_color,-1);

    screens[SCREEN_MAIN].set_selector_end(global_settings.lse_color);
    settings_save();
    settings_apply();
    return res;
}

/* Line selector text colour */
static int set_lst_color(void)
{
    int res;
    res = (int)set_color(&screens[SCREEN_MAIN],str(LANG_SELECTOR_TEXT_COLOR),
                         &global_settings.lst_color,global_settings.lss_color);

    screens[SCREEN_MAIN].set_selector_text(global_settings.lst_color);
    settings_save();
    settings_apply();
    return res;
}

static int reset_color(void)
{
    global_settings.fg_color = LCD_DEFAULT_FG;
    global_settings.bg_color = LCD_DEFAULT_BG;
    global_settings.lss_color = LCD_DEFAULT_LS;
    global_settings.lse_color = LCD_DEFAULT_BG;
    global_settings.lst_color = LCD_DEFAULT_FG;

    screens[SCREEN_MAIN].set_foreground(global_settings.fg_color);
    screens[SCREEN_MAIN].set_background(global_settings.bg_color);
    screens[SCREEN_MAIN].set_selector_start(global_settings.lss_color);
    screens[SCREEN_MAIN].set_selector_end(global_settings.lse_color);
    screens[SCREEN_MAIN].set_selector_text(global_settings.lst_color);
    settings_save();
    settings_apply();
    return 0;
}
MENUITEM_FUNCTION(set_bg_col, 0, ID2P(LANG_BACKGROUND_COLOR),
                    set_bg_color, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(set_fg_col, 0, ID2P(LANG_FOREGROUND_COLOR),
                    set_fg_color, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(set_lss_col, 0, ID2P(LANG_SELECTOR_START_COLOR),
                    set_lss_color, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(set_lse_col, 0, ID2P(LANG_SELECTOR_END_COLOR),
                    set_lse_color, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(set_lst_col, 0, ID2P(LANG_SELECTOR_TEXT_COLOR),
                    set_lst_color, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(reset_colors, 0, ID2P(LANG_RESET_COLORS),
                    reset_color, NULL, NULL, Icon_NOICON);

MAKE_MENU(lss_settings, ID2P(LANG_SELECTOR_COLOR_MENU),
            NULL, Icon_NOICON,
            &set_lss_col, &set_lse_col, &set_lst_col
         );

/* now the actual menu */
MAKE_MENU(colors_settings, ID2P(LANG_COLORS_MENU),
            NULL, Icon_Display_menu,
            &lss_settings,
            &set_bg_col, &set_fg_col, &reset_colors
         );
         
#endif /* HAVE_LCD_COLOR */
/*    LCD MENU                    */
/***********************************/

#ifdef HAVE_LCD_BITMAP
static struct browse_folder_info fonts = {FONT_DIR, SHOW_FONT};
#endif
static struct browse_folder_info wps = {WPS_DIR, SHOW_WPS};
#ifdef HAVE_REMOTE_LCD
static struct browse_folder_info rwps = {WPS_DIR, SHOW_RWPS};
#endif
static struct browse_folder_info themes = {THEME_DIR, SHOW_CFG};

int browse_folder(void *param)
{
    const struct browse_folder_info *info =
        (const struct browse_folder_info*)param;
    return rockbox_browse(info->dir, info->show_options);
}

#ifdef HAVE_LCD_BITMAP
MENUITEM_FUNCTION(browse_fonts, MENU_FUNC_USEPARAM, 
        ID2P(LANG_CUSTOM_FONT), 
        browse_folder, (void*)&fonts, NULL, Icon_Font);
#endif
MENUITEM_FUNCTION(browse_wps, MENU_FUNC_USEPARAM, 
        ID2P(LANG_WHILE_PLAYING), 
        browse_folder, (void*)&wps, NULL, Icon_Wps);
#ifdef HAVE_REMOTE_LCD
MENUITEM_FUNCTION(browse_rwps, MENU_FUNC_USEPARAM, 
        ID2P(LANG_REMOTE_WHILE_PLAYING), 
        browse_folder, (void*)&rwps, NULL, Icon_Wps);
#endif

MENUITEM_SETTING(show_icons, &global_settings.show_icons, NULL);
MENUITEM_FUNCTION(browse_themes, MENU_FUNC_USEPARAM, 
        ID2P(LANG_CUSTOM_THEME), 
        browse_folder, (void*)&themes, NULL, Icon_Config);
#ifdef HAVE_LCD_BITMAP
MENUITEM_SETTING(cursor_style, &global_settings.cursor_style, NULL);
#endif

MAKE_MENU(theme_menu, ID2P(LANG_THEME_MENU),
            NULL, Icon_Wps,
            &browse_themes,
#ifdef HAVE_LCD_BITMAP
            &browse_fonts,
#endif
            &browse_wps,
#ifdef HAVE_REMOTE_LCD
            &browse_rwps,
#endif
            &show_icons,
#if LCD_DEPTH > 1
            &clear_main_bd,
#endif
#ifdef HAVE_LCD_BITMAP
            &cursor_style,
#endif
#ifdef HAVE_LCD_COLOR
            &colors_settings,
#endif
            );
