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
#include "backdrop.h"
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

enum Colors {
    COLOR_FG = 0,
    COLOR_BG,
    COLOR_LSS,
    COLOR_LSE,
    COLOR_LST,
    COLOR_COUNT
};
static struct colour_info
{
    int *setting;
    int lang_id;
} colors[COLOR_COUNT] = {
    [COLOR_FG] = {&global_settings.fg_color, LANG_FOREGROUND_COLOR},
    [COLOR_BG] = {&global_settings.bg_color, LANG_BACKGROUND_COLOR},
    [COLOR_LSS] = {&global_settings.lss_color, LANG_SELECTOR_START_COLOR},
    [COLOR_LSE] = {&global_settings.lse_color, LANG_SELECTOR_END_COLOR},
    [COLOR_LST] = {&global_settings.lst_color, LANG_SELECTOR_TEXT_COLOR},
};

/**
 * Menu for fore/back/selection colors
 */
static int set_color_func(void* color)
{
    int res, c = (intptr_t)color, banned_color=-1;
    
    /* Don't let foreground be set the same as background and vice-versa */
    if (c == COLOR_BG)
        banned_color = *colors[COLOR_FG].setting;
    else if (c == COLOR_FG)
        banned_color = *colors[COLOR_BG].setting;

    res = (int)set_color(&screens[SCREEN_MAIN],str(colors[c].lang_id),
                         colors[c].setting, banned_color);
    settings_save();
    settings_apply(false);
    return res;
}

static int reset_color(void)
{
    global_settings.fg_color = LCD_DEFAULT_FG;
    global_settings.bg_color = LCD_DEFAULT_BG;
    global_settings.lss_color = LCD_DEFAULT_LS;
    global_settings.lse_color = LCD_DEFAULT_BG;
    global_settings.lst_color = LCD_DEFAULT_FG;
    
    settings_save();
    settings_apply(false);
    return 0;
}
MENUITEM_FUNCTION(set_bg_col, MENU_FUNC_USEPARAM, ID2P(LANG_BACKGROUND_COLOR),
                  set_color_func, (void*)COLOR_BG, NULL, Icon_NOICON);
MENUITEM_FUNCTION(set_fg_col, MENU_FUNC_USEPARAM, ID2P(LANG_FOREGROUND_COLOR),
                  set_color_func, (void*)COLOR_FG, NULL, Icon_NOICON);
MENUITEM_FUNCTION(set_lss_col, MENU_FUNC_USEPARAM, ID2P(LANG_SELECTOR_START_COLOR),
                  set_color_func, (void*)COLOR_LSS, NULL, Icon_NOICON);
MENUITEM_FUNCTION(set_lse_col, MENU_FUNC_USEPARAM, ID2P(LANG_SELECTOR_END_COLOR),
                  set_color_func, (void*)COLOR_LSE, NULL, Icon_NOICON);
MENUITEM_FUNCTION(set_lst_col, MENU_FUNC_USEPARAM, ID2P(LANG_SELECTOR_TEXT_COLOR),
                  set_color_func, (void*)COLOR_LST, NULL, Icon_NOICON);
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
