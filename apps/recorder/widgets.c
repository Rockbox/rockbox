/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Markus Braun
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <lcd.h>
#include <limits.h>
#include "scrollbar.h"

#ifdef HAVE_LCD_BITMAP

/*
 * Print a scroll bar
 */
void scrollbar(int x, int y, int width, int height, int items, int min_shown, 
               int max_shown, int orientation)
{
    gui_scrollbar_draw(&screens[SCREEN_MAIN],x,y,width,height,
                       items,min_shown, max_shown, orientation);
}

/*
 * Print a checkbox
 */
void checkbox(int x, int y, int width, int height, bool checked)
{
    /* draw box */
    lcd_drawrect(x, y, width, height);

    /* clear inner area */
    lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    lcd_fillrect(x + 1, y + 1, width - 2, height - 2);
    lcd_set_drawmode(DRMODE_SOLID);

    if (checked){
        lcd_drawline(x + 2, y + 2, x + width - 2 - 1 , y + height - 2 - 1);
        lcd_drawline(x + 2, y + height - 2 - 1, x + width - 2 - 1, y + 2);
    }
}

#endif
