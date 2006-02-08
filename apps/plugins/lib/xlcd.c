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

#ifdef HAVE_LCD_COLOR
static const fb_data graylut[256] = {
#if LCD_PIXELFORMAT == RGB565
    0x0000, 0x0000, 0x0000, 0x0020, 0x0020, 0x0821, 0x0821, 0x0841,
    0x0841, 0x0841, 0x0841, 0x0861, 0x0861, 0x1062, 0x1062, 0x1082,
    0x1082, 0x1082, 0x1082, 0x10a2, 0x10a2, 0x18a3, 0x18a3, 0x18c3,
    0x18c3, 0x18c3, 0x18c3, 0x18e3, 0x18e3, 0x20e4, 0x20e4, 0x2104,
    0x2104, 0x2104, 0x2104, 0x2124, 0x2124, 0x2124, 0x2925, 0x2945,
    0x2945, 0x2945, 0x2945, 0x2965, 0x2965, 0x2965, 0x3166, 0x3186,
    0x3186, 0x3186, 0x3186, 0x31a6, 0x31a6, 0x31a6, 0x39a7, 0x39c7,
    0x39c7, 0x39c7, 0x39c7, 0x39e7, 0x39e7, 0x39e7, 0x41e8, 0x4208,
    0x4208, 0x4208, 0x4208, 0x4228, 0x4228, 0x4228, 0x4a29, 0x4a49,
    0x4a49, 0x4a49, 0x4a49, 0x4a69, 0x4a69, 0x4a69, 0x4a69, 0x528a,
    0x528a, 0x528a, 0x528a, 0x52aa, 0x52aa, 0x52aa, 0x52aa, 0x5aab,
    0x5acb, 0x5acb, 0x5acb, 0x5acb, 0x5aeb, 0x5aeb, 0x5aeb, 0x62ec,
    0x630c, 0x630c, 0x630c, 0x630c, 0x632c, 0x632c, 0x632c, 0x6b2d,
    0x6b4d, 0x6b4d, 0x6b4d, 0x6b4d, 0x6b6d, 0x6b6d, 0x6b6d, 0x6b6d,
    0x738e, 0x738e, 0x738e, 0x738e, 0x73ae, 0x73ae, 0x73ae, 0x73ae,
    0x7bcf, 0x7bcf, 0x7bcf, 0x7bcf, 0x7bef, 0x7bef, 0x7bef, 0x7bef,
    0x8410, 0x8410, 0x8410, 0x8410, 0x8430, 0x8430, 0x8430, 0x8430,
    0x8c51, 0x8c51, 0x8c51, 0x8c51, 0x8c71, 0x8c71, 0x8c71, 0x8c71,
    0x9492, 0x9492, 0x9492, 0x9492, 0x94b2, 0x94b2, 0x94b2, 0x94b2,
    0x94d2, 0x9cd3, 0x9cd3, 0x9cd3, 0x9cf3, 0x9cf3, 0x9cf3, 0x9cf3,
    0x9d13, 0xa514, 0xa514, 0xa514, 0xa534, 0xa534, 0xa534, 0xa534,
    0xa554, 0xad55, 0xad55, 0xad55, 0xad55, 0xad75, 0xad75, 0xad75,
    0xad75, 0xb596, 0xb596, 0xb596, 0xb596, 0xb5b6, 0xb5b6, 0xb5b6,
    0xb5b6, 0xb5d6, 0xbdd7, 0xbdd7, 0xbdd7, 0xbdf7, 0xbdf7, 0xbdf7,
    0xbdf7, 0xbe17, 0xc618, 0xc618, 0xc618, 0xc638, 0xc638, 0xc638,
    0xc638, 0xc658, 0xce59, 0xce59, 0xce59, 0xce79, 0xce79, 0xce79,
    0xce79, 0xce99, 0xd69a, 0xd69a, 0xd69a, 0xd6ba, 0xd6ba, 0xd6ba,
    0xd6ba, 0xd6da, 0xd6da, 0xdedb, 0xdedb, 0xdefb, 0xdefb, 0xdefb,
    0xdefb, 0xdf1b, 0xdf1b, 0xe71c, 0xe71c, 0xe73c, 0xe73c, 0xe73c,
    0xe73c, 0xe75c, 0xe75c, 0xef5d, 0xef5d, 0xef7d, 0xef7d, 0xef7d,
    0xef7d, 0xef9d, 0xef9d, 0xf79e, 0xf79e, 0xf7be, 0xf7be, 0xf7be,
    0xf7be, 0xf7de, 0xf7de, 0xffdf, 0xffdf, 0xffff, 0xffff, 0xffff
#elif LCD_PIXELFORMAT == RGB565SWAPPED
    0x0000, 0x0000, 0x0000, 0x2000, 0x2000, 0x2108, 0x2108, 0x4108,
    0x4108, 0x4108, 0x4108, 0x6108, 0x6108, 0x6210, 0x6210, 0x8210,
    0x8210, 0x8210, 0x8210, 0xa210, 0xa210, 0xa318, 0xa318, 0xc318,
    0xc318, 0xc318, 0xc318, 0xe318, 0xe318, 0xe420, 0xe420, 0x0421,
    0x0421, 0x0421, 0x0421, 0x2421, 0x2421, 0x2421, 0x2529, 0x4529,
    0x4529, 0x4529, 0x4529, 0x6529, 0x6529, 0x6529, 0x6631, 0x8631,
    0x8631, 0x8631, 0x8631, 0xa631, 0xa631, 0xa631, 0xa739, 0xc739,
    0xc739, 0xc739, 0xc739, 0xe739, 0xe739, 0xe739, 0xe841, 0x0842,
    0x0842, 0x0842, 0x0842, 0x2842, 0x2842, 0x2842, 0x294a, 0x494a,
    0x494a, 0x494a, 0x494a, 0x694a, 0x694a, 0x694a, 0x694a, 0x8a52,
    0x8a52, 0x8a52, 0x8a52, 0xaa52, 0xaa52, 0xaa52, 0xaa52, 0xab5a,
    0xcb5a, 0xcb5a, 0xcb5a, 0xcb5a, 0xeb5a, 0xeb5a, 0xeb5a, 0xec62,
    0x0c63, 0x0c63, 0x0c63, 0x0c63, 0x2c63, 0x2c63, 0x2c63, 0x2d6b,
    0x4d6b, 0x4d6b, 0x4d6b, 0x4d6b, 0x6d6b, 0x6d6b, 0x6d6b, 0x6d6b,
    0x8e73, 0x8e73, 0x8e73, 0x8e73, 0xae73, 0xae73, 0xae73, 0xae73,
    0xcf7b, 0xcf7b, 0xcf7b, 0xcf7b, 0xef7b, 0xef7b, 0xef7b, 0xef7b,
    0x1084, 0x1084, 0x1084, 0x1084, 0x3084, 0x3084, 0x3084, 0x3084,
    0x518c, 0x518c, 0x518c, 0x518c, 0x718c, 0x718c, 0x718c, 0x718c,
    0x9294, 0x9294, 0x9294, 0x9294, 0xb294, 0xb294, 0xb294, 0xb294,
    0xd294, 0xd39c, 0xd39c, 0xd39c, 0xf39c, 0xf39c, 0xf39c, 0xf39c,
    0x139d, 0x14a5, 0x14a5, 0x14a5, 0x34a5, 0x34a5, 0x34a5, 0x34a5,
    0x54a5, 0x55ad, 0x55ad, 0x55ad, 0x55ad, 0x75ad, 0x75ad, 0x75ad,
    0x75ad, 0x96b5, 0x96b5, 0x96b5, 0x96b5, 0xb6b5, 0xb6b5, 0xb6b5,
    0xb6b5, 0xd6b5, 0xd7bd, 0xd7bd, 0xd7bd, 0xf7bd, 0xf7bd, 0xf7bd,
    0xf7bd, 0x17be, 0x18c6, 0x18c6, 0x18c6, 0x38c6, 0x38c6, 0x38c6,
    0x38c6, 0x58c6, 0x59ce, 0x59ce, 0x59ce, 0x79ce, 0x79ce, 0x79ce,
    0x79ce, 0x99ce, 0x9ad6, 0x9ad6, 0x9ad6, 0xbad6, 0xbad6, 0xbad6,
    0xbad6, 0xdad6, 0xdad6, 0xdbde, 0xdbde, 0xfbde, 0xfbde, 0xfbde,
    0xfbde, 0x1bdf, 0x1bdf, 0x1ce7, 0x1ce7, 0x3ce7, 0x3ce7, 0x3ce7,
    0x3ce7, 0x5ce7, 0x5ce7, 0x5def, 0x5def, 0x7def, 0x7def, 0x7def,
    0x7def, 0x9def, 0x9def, 0x9ef7, 0x9ef7, 0xbef7, 0xbef7, 0xbef7,
    0xbef7, 0xdef7, 0xdef7, 0xdfff, 0xdfff, 0xffff, 0xffff, 0xffff
#endif /* LCD_PIXELFORMAT */
};
#endif /* HAVE_LCD_COLOR */

/* Draw a partial greyscale bitmap, canonical 8 bit format */
void xlcd_gray_bitmap_part(const unsigned char *src, int src_x, int src_y,
                           int stride, int x, int y, int width, int height)
{
    const unsigned char *src_end;
    fb_data *dst;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= LCD_WIDTH) || (y >= LCD_HEIGHT) 
        || (x + width <= 0) || (y + height <= 0))
        return;
        
    /* clipping */
    if (x < 0)
    {
        width += x;
        src_x -= x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        src_y -= y;
        y = 0;
    }
    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y;

    src    += stride * src_y + src_x; /* move starting point */
    src_end = src + stride * height;
    dst     = local_rb->lcd_framebuffer + LCD_WIDTH * y + x;

    do
    {
        const unsigned char *src_row = src;
        const unsigned char *row_end = src_row + width;
        fb_data *dst_row = dst;

#ifdef HAVE_LCD_COLOR
        do
            *dst_row++ = graylut[*src_row++];
        while (src_row < row_end);
#endif

        src +=  stride;
        dst += LCD_WIDTH;
    }
    while (src < src_end);
}

/* Draw a full greyscale bitmap, canonical 8 bit format */
void xlcd_gray_bitmap(const unsigned char *src, int x, int y, int width,
                      int height)
{
    xlcd_gray_bitmap_part(src, 0, 0, width, x, y, width, height);
}

#ifdef HAVE_LCD_COLOR
/* Draw a partial colour bitmap, canonical 24 bit RGB format */
void xlcd_color_bitmap_part(const unsigned char *src, int src_x, int src_y,
                            int stride, int x, int y, int width, int height)
{
    const unsigned char *src_end;
    fb_data *dst;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= LCD_WIDTH) || (y >= LCD_HEIGHT) 
        || (x + width <= 0) || (y + height <= 0))
        return;
        
    /* clipping */
    if (x < 0)
    {
        width += x;
        src_x -= x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        src_y -= y;
        y = 0;
    }
    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y;

    src    += 3 * (stride * src_y + src_x); /* move starting point */
    src_end = src + 3 * stride * height;
    dst     = local_rb->lcd_framebuffer + LCD_WIDTH * y + x;

    do
    {
        const unsigned char *src_row = src;
        const unsigned char *row_end = src_row + 3 * width;
        fb_data *dst_row = dst;

        do
        {   /* only RGB565 and RGB565SWAPPED so far */
            unsigned red   = 31 * (*src_row++) + 127;
            unsigned green = 63 * (*src_row++) + 127;
            unsigned blue  = 31 * (*src_row++) + 127;

            red   = (red + (red >> 8)) >> 8;     /* approx red /= 255: */
            green = (green + (green >> 8)) >> 8; /* approx green /= 255: */
            blue  = (blue + (blue >> 8)) >> 8;   /* approx blue /= 255: */

#if LCD_PIXELFORMAT == RGB565
            *dst_row++ = (red << 11) | (green << 5) | blue;
#elif LCD_PIXELFORMAT == RGB565SWAPPED
            *dst_row++ = swap16((red << 11) | (green << 5) | blue);
#endif
        }
        while (src_row < row_end);

        src +=  3 * stride;
        dst += LCD_WIDTH;
    }
    while (src < src_end);
}

/* Draw a full colour bitmap, canonical 24 bit RGB format */
void xlcd_color_bitmap(const unsigned char *src, int x, int y, int width,
                       int height)
{
    xlcd_color_bitmap_part(src, 0, 0, width, x, y, width, height);
}
#endif /* HAVE_LCD_COLOR */

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

