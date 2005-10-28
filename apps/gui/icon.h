/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin FERRARE
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _GUI_ICON_H_
#define _GUI_ICON_H_
#include "lcd.h"
#include "screen_access.h"
/* Defines a type for the icons since it's not the same thing on
 * char-based displays and bitmap displays */
#ifdef HAVE_LCD_BITMAP
    #define ICON const unsigned char *
#else
    #define ICON unsigned short
#endif

#define CURSOR_CHAR 0x92
#define CURSOR_WIDTH 6
#define CURSOR_HEIGHT 8
/*
 * Draws a cursor at a given position
 * - screen : the screen where we put the cursor
 * - x, y : the position, in character, not in pixel !!
 */
extern void screen_put_cursorxy(struct screen * screen, int x, int y);

/*
 * Put an icon on a screen at a given position
 * (the position is given in characters)
 * - screen : the screen where we put our icon
 * - x, y : the position, in character, not in pixel !!
 * - icon : the icon to put
 */
extern void screen_put_iconxy(struct screen * screen, int x, int y, ICON icon);

#endif /*_GUI_ICON_H_*/
