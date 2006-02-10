/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) Markus Braun (2002)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "scrollbar.h"
#ifdef HAVE_LCD_BITMAP
#include "config.h"
#include "limits.h"
#include "bmp.h"

void gui_scrollbar_draw(struct screen * screen, int x, int y,
                        int width, int height, int items,
                        int min_shown, int max_shown,
                        enum orientation orientation)
{
    int min;
    int max;
    int inner_len;
    int start;
    int size;

    /* draw box */
    screen->drawrect(x, y, width, height);

    screen->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);

    /* clear edge pixels */
    screen->drawpixel(x, y);
    screen->drawpixel((x + width - 1), y);
    screen->drawpixel(x, (y + height - 1));
    screen->drawpixel((x + width - 1), (y + height - 1));

    /* clear pixels in progress bar */
    screen->fillrect(x + 1, y + 1, width - 2, height - 2);

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

    screen->set_drawmode(DRMODE_SOLID);

    if(orientation == VERTICAL)
        screen->fillrect(x + 1, y + start + 1, width - 2, size);
    else
        screen->fillrect(x + start + 1, y + 1, size, height - 2);
}

void gui_bitmap_scrollbar_draw(struct screen * screen, struct bitmap bm, int x, int y,
                        int width, int height, int items,
                        int min_shown, int max_shown,
                        enum orientation orientation)
{
    int min;
    int max;
    int inner_len;
    int start;
    int size;

    screen->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);

    /* clear pixels in progress bar */
    screen->fillrect(x, y, width, height);

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
        inner_len = height;
    else
        inner_len = width;

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

    screen->set_drawmode(DRMODE_SOLID);

    if(orientation == VERTICAL) {
#if LCD_DEPTH > 1
      if (bm.format == FORMAT_MONO)
#endif
        screen->mono_bitmap_part(bm.data, 0, 0, 
                                 bm.width, x, y + start, width, size);
#if LCD_DEPTH > 1
      else 
        screen->transparent_bitmap_part((fb_data *)bm.data, 0, 0, 
                                        bm.width, x, y + start, width, size);
#endif 
    } else {
#if LCD_DEPTH > 1
      if (bm.format == FORMAT_MONO)
#endif
        screen->mono_bitmap_part(bm.data, 0, 0, 
                                 bm.width, x + start, y, size, height);
#if LCD_DEPTH > 1
      else
        screen->transparent_bitmap_part((fb_data *)bm.data, 0, 0, 
                                        bm.width, x + start, y, size, height);
#endif
    }
}
#endif /* HAVE_LCD_BITMAP */
