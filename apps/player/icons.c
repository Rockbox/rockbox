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
#include "lcd.h"
#include "icon.h"

#ifdef HAVE_LCD_CHARCELLS
/* For the moment, charcell cant load custom maps... */

enum old_values{
    old_Icon_Queued = 'Q',
    old_Icon_Moving = 'M',
    old_Icon_Unknown = 0xe100,
    old_Icon_Bookmark,
    old_Icon_Plugin,
    old_Icon_Folder,
    old_Icon_Firmware,
    old_Icon_Language,
    old_Icon_Audio,
    old_Icon_Wps,
    old_Icon_Playlist,
    old_Icon_Text,
    old_Icon_Config,
};

static const unsigned short icons[Icon_Last_Themeable] = {
    [0 ... Icon_Last_Themeable-1] = ' ',
    
    [Icon_Audio] = old_Icon_Audio,
    [Icon_Folder] = old_Icon_Folder,
    [Icon_Playlist] = old_Icon_Playlist,
    [Icon_Cursor] = CURSOR_CHAR,
    [Icon_Wps] = old_Icon_Wps,
    [Icon_Firmware] = old_Icon_Firmware,
    [Icon_Language] = old_Icon_Language,
    [Icon_Config] = old_Icon_Config,
    [Icon_Plugin] = old_Icon_Plugin,
    [Icon_Bookmark] = old_Icon_Bookmark,
    [Icon_Queued] = old_Icon_Queued,
    [Icon_Moving] = old_Icon_Moving,
    
    /*
    [Icon_Keyboard] = ,
    [Icon_Font] = ,
    [Icon_Preset] = ,
    [Icon_Reverse_Cursor] = ,
    [Icon_Questionmark] = ,
    [Icon_Menu_setting] = ,
    [Icon_Menu_functioncall] = ,
    [Icon_Submenu] = ,
    [Icon_Submenu_Entered] = ,
    [Icon_Recording] = ,
    [Icon_Voice] = ,
    [Icon_General_settings_menu] = ,
    [Icon_System_menu] = ,
    [Icon_Playback_menu] = ,
    [Icon_Display_menu] = ,
    [Icon_Remote_Display_menu] = ,
    [Icon_Radio_screen] = ,
    [Icon_file_view_menu] = ,
    [Icon_EQ] = ,
    [Icon_Rockbox] = ,
    */
};

/* as above, but x,y are letter position, NOT PIXEL */
extern void screen_put_iconxy(struct screen * screen,
                            int x, int y, enum themable_icons icon)
{
    if (icon == Icon_NOICON)
        screen->putc(x, y, ' ');
    else if (icon >= Icon_Last_Themeable)
        screen->putc(x, y, old_Icon_Unknown);
    else
        screen->putc(x, y, icons[icon]);
}

void screen_put_cursorxy(struct screen * display, int x, int y, bool on)
{
    screen_put_iconxy(display, x, y, on?Icon_Cursor:-1);

}

void icons_init(void)
{
}






#endif
