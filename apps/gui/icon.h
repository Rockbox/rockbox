/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _GUI_ICON_H_
#define _GUI_ICON_H_
#include "screen_access.h"
/* Defines a type for the icons since it's not the same thing on
 * char-based displays and bitmap displays */
#ifdef HAVE_LCD_BITMAP
typedef const unsigned char * ICON;
#else
typedef long ICON;
#endif

#define NOICON Icon_NOICON
#define FORCE_INBUILT_ICON 0x80000000
/* Don't #ifdef icon values, or we wont be able to use the same 
   bmp for every target. */
enum themable_icons {
    Icon_NOICON = -1, /* Dont put this in a .bmp */
    Icon_Audio,
    Icon_Folder,
    Icon_Playlist,
    Icon_Cursor,
    Icon_Wps,
    Icon_Firmware,
    Icon_Font,
    Icon_Language,
    Icon_Config,
    Icon_Plugin,
    Icon_Bookmark,
    Icon_Preset,
    Icon_Queued,
    Icon_Moving,
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
    Icon_Last_Themeable,
};

/*
 * Draws a cursor at a given position, if th
 * - screen : the screen where we put the cursor
 * - x, y : the position, in character, not in pixel !!
 * - on : true if the cursor must be shown, false if it must be erased
 */
extern void screen_put_cursorxy(struct screen * screen, int x, int y, bool on);

/*
 * Put an icon on a screen at a given position
 * (the position is given in characters)
 * If the given icon is Icon_blank, the icon
 * at the given position will be erased
 * - screen : the screen where we put our icon
 * - x, y : the position, pixel value !!
 * - icon : the icon to put
 */
extern void screen_put_iconxy(struct screen * screen,
                              int x, int y, enum themable_icons icon);
#ifdef HAVE_LCD_CHARCELLS
# define screen_put_icon(s, x, y, i) screen_put_iconxy(s, x, y, i)
# define screen_put_icon_with_offset(s, x, y, w, h, i) screen_put_icon(s, x, y, i)
#else
/* For both of these, the icon will be placed in the center of the rectangle */
/* as above, but x,y are letter position, NOT PIXEL */
extern void screen_put_icon(struct screen * screen,
                              int x, int y, enum themable_icons icon);
/* as above (x,y are letter pos), but with a pxiel offset for both */
extern void screen_put_icon_with_offset(struct screen * display, 
                       int x, int y, int off_x, int off_y,
                       enum themable_icons icon);
#endif

void icons_init(void);


#ifdef HAVE_LCD_CHARCELLS
# define CURSOR_CHAR 0xe10c
# define get_icon_width(a) 6
#else
int get_icon_width(enum screen_type screen_type);
#endif

#endif /*_GUI_ICON_H_*/
