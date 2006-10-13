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
                        unsigned flags)
{
    int inner_x, inner_y, inner_wd, inner_ht, inner_len;
    int min, max, range;
    int start, size;
#ifdef HAVE_LCD_COLOR
    int infill;
#endif

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

    range = max - min;

    inner_x  = x + 1;
    inner_y  = y + 1;
    inner_wd = width  - 2;
    inner_ht = height - 2;

    if (flags & HORIZONTAL)
        inner_len = inner_wd;
    else
        inner_len = inner_ht;

    /* avoid overflows */
    while (items > (INT_MAX / inner_len)) {
        items >>= 1;
        min >>= 1;
        max >>= 1;
    }

    /* calc start and end of the knob */
    if (items > 0 && items > range) {
        size = inner_len * range / items;
        if (size == 0) { /* width of knob is null */
            size = 1;
            start = (inner_len - 1) * min / items;
        } else {
            start = (inner_len - size) * min / (items - range);
        }
    } else {  /* if null draw full bar */
        size = inner_len;
        start = 0;
    }

    /* draw box */
#ifdef HAVE_LCD_COLOR
    /* must avoid corners if case of (flags & FOREGROUND) */
    screen->hline(inner_x, x + inner_wd, y);
    screen->hline(inner_x, x + inner_wd, y + height - 1);
    screen->vline(x, inner_y, y + inner_ht);
    screen->vline(x + width - 1, inner_y, y + inner_ht);
#else
    screen->drawrect(x, y, width, height);
#endif

    screen->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);

#ifdef HAVE_LCD_COLOR
    infill = flags & (screen->depth > 1 ? INNER_FILL_MASK : INNER_FILL);

    if (!(flags & FOREGROUND))
    {
#endif
        /* clear corner pixels */
        screen->drawpixel(x, y);
        screen->drawpixel(x + width - 1, y);
        screen->drawpixel(x, y + height - 1);
        screen->drawpixel(x + width - 1, y + height - 1);

#ifdef HAVE_LCD_COLOR
        if (infill != INNER_BGFILL)
            infill = INNER_FILL;
    }

    if (infill == INNER_FILL)
#endif
    {
        /* clear pixels in progress bar */
        screen->fillrect(inner_x, inner_y, inner_wd, inner_ht);
    }

    screen->set_drawmode(DRMODE_SOLID);

#ifdef HAVE_LCD_COLOR
    if (infill == INNER_BGFILL)
    {
        /* fill inner area with current background color */
        unsigned fg = screen->get_foreground();
        screen->set_foreground(screen->get_background());
        screen->fillrect(inner_x, inner_y, inner_wd, inner_ht);
        screen->set_foreground(fg);
    }
#endif

    if (flags & HORIZONTAL)
    {
        inner_x += start;
        inner_wd = size;
    }
    else
    {
        inner_y += start;
        inner_ht = size;
    }

    screen->fillrect(inner_x, inner_y, inner_wd, inner_ht);
}

void gui_bitmap_scrollbar_draw(struct screen * screen, struct bitmap bm, int x, int y,
                        int width, int height, int items,
                        int min_shown, int max_shown,
                        unsigned flags)
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

    if (flags & VERTICAL)
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

    if (flags & HORIZONTAL) {
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
    } else {
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
    }
}
#endif /* HAVE_LCD_BITMAP */
