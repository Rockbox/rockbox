/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Additional LCD routines not present in the core itself
*
* Copyright (C) 2005 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"

#ifdef HAVE_LCD_BITMAP

/*** globals ***/

static struct plugin_api *local_rb = NULL; /* global api struct pointer */

/*** functions ***/

/* library init */
void xlcd_init(struct plugin_api* newrb)
{
    local_rb = newrb;
}

/* draw a filled triangle */
void xlcd_filltriangle(int x1, int y1, int x2, int y2, int x3, int y3)
{
    int x, y;
    long fp_y1, fp_y2, fp_dy1, fp_dy2;

    /* sort vertices by increasing x value */
    if (x1 > x3)
    {
        if (x2 < x3)       /* x2 < x3 < x1 */
        {
            x = x1; x1 = x2; x2 = x3; x3 = x;
            y = y1; y1 = y2; y2 = y3; y3 = y;
        }
        else if (x2 > x1)  /* x3 < x1 < x2 */
        {
            x = x1; x1 = x3; x3 = x2; x2 = x;
            y = y1; y1 = y3; y3 = y2; y2 = y;
        }
        else               /* x3 <= x2 <= x1 */
        {
            x = x1; x1 = x3; x3 = x;
            y = y1; y1 = y3; y3 = y;
        }
    }
    else
    {
        if (x2 < x1)       /* x2 < x1 <= x3 */
        {
            x = x1; x1 = x2; x2 = x;
            y = y1; y1 = y2; y2 = y;
        }
        else if (x2 > x3)  /* x1 <= x3 < x2 */
        {
            x = x2; x2 = x3; x3 = x;
            y = y2; y2 = y3; y3 = y;
        }
        /* else already sorted */
    }

    if (x1 < x3)  /* draw */
    {
        fp_dy1 = ((y3 - y1) << 16) / (x3 - x1);
        fp_y1  = (y1 << 16) + (1<<15) + (fp_dy1 >> 1);

        if (x1 < x2)  /* first part */
        {   
            fp_dy2 = ((y2 - y1) << 16) / (x2 - x1);
            fp_y2  = (y1 << 16) + (1<<15) + (fp_dy2 >> 1);
            for (x = x1; x < x2; x++)
            {
                local_rb->lcd_vline(x, fp_y1 >> 16, fp_y2 >> 16);
                fp_y1 += fp_dy1;
                fp_y2 += fp_dy2;
            }
        }
        if (x2 < x3)  /* second part */
        {
            fp_dy2 = ((y3 - y2) << 16) / (x3 - x2);
            fp_y2 = (y2 << 16) + (1<<15) + (fp_dy2 >> 1);
            for (x = x2; x < x3; x++)
            {
                local_rb->lcd_vline(x, fp_y1 >> 16, fp_y2 >> 16);
                fp_y1 += fp_dy1;
                fp_y2 += fp_dy2;
            }
        }
    }
}

#if LCD_DEPTH >= 8

void xlcd_scroll_left(int count)
{
    fb_data *data, *data_end;
    int length, oldmode;

    if ((unsigned)count >= LCD_WIDTH)
        return;

    data = local_rb->lcd_framebuffer;
    data_end = data + LCD_WIDTH*LCD_HEIGHT;
    length = LCD_WIDTH - count;

    do
    {
        local_rb->memmove(data, data + count, length * sizeof(fb_data));
        data += LCD_WIDTH;
    }
    while (data < data_end);

    oldmode = local_rb->lcd_get_drawmode();
    local_rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    local_rb->lcd_fillrect(length, 0, count, LCD_HEIGHT);
    local_rb->lcd_set_drawmode(oldmode);
}

void xlcd_scroll_right(int count)
{
    fb_data *data, *data_end;
    int length, oldmode;

    if ((unsigned)count >= LCD_WIDTH)
        return;

    data = local_rb->lcd_framebuffer;
    data_end = data + LCD_WIDTH*LCD_HEIGHT;
    length = LCD_WIDTH - count;

    do
    {
        local_rb->memmove(data + count, data, length * sizeof(fb_data));
        data += LCD_WIDTH;
    }
    while (data < data_end);

    oldmode = local_rb->lcd_get_drawmode();
    local_rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    local_rb->lcd_fillrect(0, 0, count, LCD_HEIGHT);
    local_rb->lcd_set_drawmode(oldmode);
}

void xlcd_scroll_up(int count)
{
    long length, oldmode;

    if ((unsigned)count >= LCD_HEIGHT)
        return;

    length = LCD_HEIGHT - count;

    local_rb->memmove(local_rb->lcd_framebuffer,
                      local_rb->lcd_framebuffer + count * LCD_WIDTH,
                      length * LCD_WIDTH * sizeof(fb_data));

    oldmode = local_rb->lcd_get_drawmode();
    local_rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    local_rb->lcd_fillrect(0, length, LCD_WIDTH, count);
    local_rb->lcd_set_drawmode(oldmode);
}

void xlcd_scroll_down(int count)
{
    long length, oldmode;

    if ((unsigned)count >= LCD_HEIGHT)
        return;

    length = LCD_HEIGHT - count;

    local_rb->memmove(local_rb->lcd_framebuffer + count * LCD_WIDTH,
                      local_rb->lcd_framebuffer,
                      length * LCD_WIDTH * sizeof(fb_data));

    oldmode = local_rb->lcd_get_drawmode();
    local_rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    local_rb->lcd_fillrect(0, 0, LCD_WIDTH, count);
    local_rb->lcd_set_drawmode(oldmode);
}

#endif /* LCD_DEPTH >= 8 */

#endif /* HAVE_LCD_BITMAP */

