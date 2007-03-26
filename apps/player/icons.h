/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Justin Heiner
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _ICONS_H_
#define _ICONS_H_

#include <lcd.h>

/* 
 * Icons of size 5x7 pixels for the Player LCD
 */

#ifdef HAVE_LCD_CHARCELLS

enum {
    Icon_Queued = 'Q',
    Icon_Moving = 'M',
    Icon_Unknown = 0xe100,
    Icon_Bookmark,
    Icon_Plugin,
    Icon_Folder,
    Icon_Firmware,
    Icon_Language,
    Icon_Audio,
    Icon_Wps,
    Icon_Playlist,
    Icon_Text,
    Icon_Config,
};

/* put icons from the 6x8 enum here if the player
   doesnt have an icon for it */
enum unused_but_needed {
    Icon_Cursor,
    Icon_Font,
    Icon_Preset,
    Icon_Keyboard,
    Icon_Reverse_Cursor,
    Icon_Questionmark,
    Icon_Menu_setting,
    Icon_Menu_functioncall,
    Icon_Submenu,
    Icon_Submenu_Entered,
    Icon_Recording,
    Icon_Voice,
    Icon_General_settings_menu,
    Icon_System_menu,
    Icon_Playback_menu,
    Icon_Display_menu,
    Icon_Remote_Display_menu,
    Icon_Radio_screen,
    Icon_file_view_menu,
    Icon_EQ,
    Icon_Rockbox,
    Icon6x8Last,
};
#endif

#endif /* _ICONS_H_ */
