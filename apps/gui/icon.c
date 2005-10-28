/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) Robert E. Hak(2002), Kévin FERRARE (2005)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "icon.h"
#include "screen_access.h"
#include "icons.h"

/* Count in letter positions, NOT pixels */
void screen_put_iconxy(struct screen * display, int x, int y, ICON icon)
{
#ifdef HAVE_LCD_BITMAP
    if(icon==0)/* Don't display invalid icons */
        return;
    int xpos, ypos;
    xpos = x*CURSOR_WIDTH;
    ypos = y*display->char_height + display->getymargin();
    if ( display->char_height > CURSOR_HEIGHT )/* center the cursor */
        ypos += (display->char_height - CURSOR_HEIGHT) / 2;
    display->mono_bitmap(icon, xpos, ypos, CURSOR_WIDTH, CURSOR_HEIGHT);
#else
    display->putc(x, y, icon);
#endif
}

void screen_put_cursorxy(struct screen * display, int x, int y)
{
#ifdef HAVE_LCD_BITMAP
    screen_put_iconxy(display, x, y, bitmap_icons_6x8[Icon_Cursor]);
#else
    screen_put_iconxy(display, x, y, CURSOR_CHAR);
#endif
}
