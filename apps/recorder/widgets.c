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

#include "widgets.h"

#ifdef HAVE_LCD_BITMAP

/*
 * Print a scroll bar
 */
void scrollbar(int x, int y, int width, int height, int items, int min_shown, 
               int max_shown, int orientation)
{
    int min;
    int max;
    int inner_len;
    int start;
    int size;

    /* draw box */
    lcd_drawrect(x, y, width, height);

    lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);

    /* clear edge pixels */
    lcd_drawpixel(x, y);
    lcd_drawpixel((x + width - 1), y);
    lcd_drawpixel(x, (y + height - 1));
    lcd_drawpixel((x + width - 1), (y + height - 1));

    /* clear pixels in progress bar */
    lcd_fillrect(x + 1, y + 1, width - 2, height - 2);

    /* min should be min */
    if(min_shown < max_shown) { 
        min = min_shown;
        max = max_shown;
    }
    else {
        min = max_shown;
        max = min_shown;
    }

    /* limit min and max */
    if(min < 0)
        min = 0;
    if(min > items)
        min = items;

    if(max < 0)
        max = 0;
    if(max > items)
        max = items;

    if (orientation == VERTICAL)
        inner_len = height - 2;
    else
        inner_len = width - 2;

    /* avoid overflows */
    while (items > (INT_MAX / inner_len)) {
        items >>= 1;
        min >>= 1;
        max >>= 1;
    }

    /* calc start and end of the knob */
    if (items > 0 && items > (max - min)) {
        size = inner_len * (max - min) / items;
        if (size == 0) { /* width of knob is null */
            size = 1;
            start = (inner_len - 1) * min / items;
        } else {
            start = (inner_len - size) * min / (items - (max - min));
        }
    } else {  /* if null draw full bar */
        size = inner_len;
        start = 0;
    }

    lcd_set_drawmode(DRMODE_SOLID);

    if(orientation == VERTICAL)
        lcd_fillrect(x + 1, y + start + 1, width - 2, size);
    else
        lcd_fillrect(x + start + 1, y + 1, size, height - 2);
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
