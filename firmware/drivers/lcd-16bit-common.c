/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Dave Chapman
 *
 * Rockbox driver for 16-bit colour LCDs
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


/* to be #included by lcd-16bit*.c */

#if !defined(ROW_INC) || !defined(COL_INC)
#error ROW_INC or COL_INC not defined
#endif

enum fill_opt {
    OPT_NONE = 0,
    OPT_SET,
    OPT_COPY
};

/*** globals ***/
fb_data lcd_static_framebuffer[LCD_FBHEIGHT][LCD_FBWIDTH]
    IRAM_LCDFRAMEBUFFER CACHEALIGN_AT_LEAST_ATTR(16);
fb_data *lcd_framebuffer = &lcd_static_framebuffer[0][0];

static fb_data* lcd_backdrop = NULL;
static long lcd_backdrop_offset IDATA_ATTR = 0;

static struct viewport default_vp =
{
    .x        = 0,
    .y        = 0,
    .width    = LCD_WIDTH,
    .height   = LCD_HEIGHT,
    .font     = FONT_SYSFIXED,
    .drawmode = DRMODE_SOLID,
    .fg_pattern = LCD_DEFAULT_FG,
    .bg_pattern = LCD_DEFAULT_BG,
    .lss_pattern = LCD_DEFAULT_BG,
    .lse_pattern = LCD_DEFAULT_BG,
    .lst_pattern = LCD_DEFAULT_BG,
};

static struct viewport* current_vp IDATA_ATTR = &default_vp;

/* LCD init */
void lcd_init(void)
{
    lcd_clear_display();

    /* Call device specific init */
    lcd_init_device();
    scroll_init();
}
/*** Viewports ***/

void lcd_set_viewport(struct viewport* vp)
{
    if (vp == NULL)
        current_vp = &default_vp;
    else
        current_vp = vp;

#if defined(SIMULATOR)
    /* Force the viewport to be within bounds.  If this happens it should
     *  be considered an error - the viewport will not draw as it might be
     *  expected.
     */
    if((unsigned) current_vp->x > (unsigned) LCD_WIDTH
        || (unsigned) current_vp->y > (unsigned) LCD_HEIGHT
        || current_vp->x + current_vp->width > LCD_WIDTH
        || current_vp->y + current_vp->height > LCD_HEIGHT)
    {
#if !defined(HAVE_VIEWPORT_CLIP)
        DEBUGF("ERROR: "
#else
        DEBUGF("NOTE: "
#endif
            "set_viewport out of bounds: x: %d y: %d width: %d height:%d\n",
            current_vp->x, current_vp->y,
            current_vp->width, current_vp->height);
    }

#endif
}

void lcd_update_viewport(void)
{
    lcd_update_rect(current_vp->x, current_vp->y,
                    current_vp->width, current_vp->height);
}

void lcd_update_viewport_rect(int x, int y, int width, int height)
{
    lcd_update_rect(current_vp->x + x, current_vp->y + y, width, height);
}

/* Clear the current viewport */
void lcd_clear_viewport(void)
{
    fb_data *dst, *dst_end;
    int x, y, width, height;
    int len, step;

    x = current_vp->x;
    y = current_vp->y;
    width = current_vp->width;
    height = current_vp->height;

#if defined(HAVE_VIEWPORT_CLIP)
    /********************* Viewport on screen clipping ********************/
    /* nothing to draw? */
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)
        || (x + width <= 0) || (y + height <= 0))
        return;

    /* clip image in viewport in screen */
    if (x < 0)
    {
        width += x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        y = 0;
    }
    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y;
#endif

    len  = STRIDE_MAIN(width, height);
    step = STRIDE_MAIN(ROW_INC, COL_INC);

    dst = FBADDR(x, y);
    dst_end = FBADDR(x + width - 1 , y + height - 1);

    if (current_vp->drawmode & DRMODE_INVERSEVID)
    {
        do
        {
            memset16(dst, current_vp->fg_pattern, len);
            dst += step;
        }
        while (dst <= dst_end);
    }
    else
    {
        if (!lcd_backdrop)
        {
            do
            {
                memset16(dst, current_vp->bg_pattern, len);
                dst += step;
            }
            while (dst <= dst_end);
        }
        else
        {
            do
            {
                memcpy(dst, (void *)((long)dst + lcd_backdrop_offset),
                       len * sizeof(fb_data));
                dst += step;
            }
            while (dst <= dst_end);
        }
    }

    if (current_vp == &default_vp)
    {
        lcd_scroll_info.lines = 0;
    }
    else
    {
        lcd_scroll_stop(current_vp);
    }
}

/*** parameter handling ***/

void lcd_set_drawmode(int mode)
{
    current_vp->drawmode = mode & (DRMODE_SOLID|DRMODE_INVERSEVID);
}

int lcd_get_drawmode(void)
{
    return current_vp->drawmode;
}

void lcd_set_foreground(unsigned color)
{
    current_vp->fg_pattern = color;
}

unsigned lcd_get_foreground(void)
{
    return current_vp->fg_pattern;
}

void lcd_set_background(unsigned color)
{
    current_vp->bg_pattern = color;
}

unsigned lcd_get_background(void)
{
    return current_vp->bg_pattern;
}

void lcd_set_selector_start(unsigned color)
{
    current_vp->lss_pattern = color;
}

void lcd_set_selector_end(unsigned color)
{
    current_vp->lse_pattern = color;
}

void lcd_set_selector_text(unsigned color)
{
    current_vp->lst_pattern = color;
}

void lcd_set_drawinfo(int mode, unsigned fg_color, unsigned bg_color)
{
    lcd_set_drawmode(mode);
    current_vp->fg_pattern = fg_color;
    current_vp->bg_pattern = bg_color;
}

int lcd_getwidth(void)
{
    return current_vp->width;
}

int lcd_getheight(void)
{
    return current_vp->height;
}

void lcd_setfont(int newfont)
{
    current_vp->font = newfont;
}

int lcd_getfont(void)
{
    return current_vp->font;
}

int lcd_getstringsize(const unsigned char *str, int *w, int *h)
{
    return font_getstringsize(str, w, h, current_vp->font);
}

/*** low-level drawing functions ***/

static void ICODE_ATTR setpixel(fb_data *address)
{
    *address = current_vp->fg_pattern;
}

static void ICODE_ATTR clearpixel(fb_data *address)
{
    *address = current_vp->bg_pattern;
}

static void ICODE_ATTR clearimgpixel(fb_data *address)
{
    *address = *(fb_data *)((long)address + lcd_backdrop_offset);
}

static void ICODE_ATTR flippixel(fb_data *address)
{
    *address = ~(*address);
}

static void ICODE_ATTR nopixel(fb_data *address)
{
    (void)address;
}

lcd_fastpixelfunc_type* const lcd_fastpixelfuncs_bgcolor[8] = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearpixel, nopixel, clearpixel
};

lcd_fastpixelfunc_type* const lcd_fastpixelfuncs_backdrop[8] = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearimgpixel, nopixel, clearimgpixel
};

lcd_fastpixelfunc_type* const * lcd_fastpixelfuncs = lcd_fastpixelfuncs_bgcolor;

void lcd_set_backdrop(fb_data* backdrop)
{
    lcd_backdrop = backdrop;
    if (backdrop)
    {
        lcd_backdrop_offset = (long)backdrop - (long)lcd_framebuffer;
        lcd_fastpixelfuncs = lcd_fastpixelfuncs_backdrop;
    }
    else
    {
        lcd_backdrop_offset = 0;
        lcd_fastpixelfuncs = lcd_fastpixelfuncs_bgcolor;
    }
}

fb_data* lcd_get_backdrop(void)
{
    return lcd_backdrop;
}

/* Clear the whole display */
void lcd_clear_display(void)
{
    struct viewport* old_vp = current_vp;

    current_vp = &default_vp;

    lcd_clear_viewport();

    current_vp = old_vp;
}

/* Set a single pixel */
void lcd_drawpixel(int x, int y)
{
    if (   ((unsigned)x < (unsigned)current_vp->width)
        && ((unsigned)y < (unsigned)current_vp->height)
#if defined(HAVE_VIEWPORT_CLIP)
        && ((unsigned)x < (unsigned)LCD_WIDTH)
        && ((unsigned)y < (unsigned)LCD_HEIGHT)
#endif
        )
        lcd_fastpixelfuncs[current_vp->drawmode](FBADDR(current_vp->x+x, current_vp->y+y));
}

/* Draw a line */
void lcd_drawline(int x1, int y1, int x2, int y2)
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;
    lcd_fastpixelfunc_type *pfunc = lcd_fastpixelfuncs[current_vp->drawmode];

    deltay = abs(y2 - y1);
    if (deltay == 0)
    {
        /* DEBUGF("lcd_drawline() called for horizontal line - optimisation.\n"); */
        lcd_hline(x1, x2, y1);
        return;
    }
    deltax = abs(x2 - x1);
    if (deltax == 0)
    {
        /* DEBUGF("lcd_drawline() called for vertical line - optimisation.\n"); */
        lcd_vline(x1, y1, y2);
        return;
    }
    xinc2 = 1;
    yinc2 = 1;

    if (deltax >= deltay)
    {
        numpixels = deltax;
        d = 2 * deltay - deltax;
        dinc1 = deltay * 2;
        dinc2 = (deltay - deltax) * 2;
        xinc1 = 1;
        yinc1 = 0;
    }
    else
    {
        numpixels = deltay;
        d = 2 * deltax - deltay;
        dinc1 = deltax * 2;
        dinc2 = (deltax - deltay) * 2;
        xinc1 = 0;
        yinc1 = 1;
    }
    numpixels++; /* include endpoints */

    if (x1 > x2)
    {
        xinc1 = -xinc1;
        xinc2 = -xinc2;
    }

    if (y1 > y2)
    {
        yinc1 = -yinc1;
        yinc2 = -yinc2;
    }

    x = x1;
    y = y1;

    for (i = 0; i < numpixels; i++)
    {
        if (   ((unsigned)x < (unsigned)current_vp->width)
            && ((unsigned)y < (unsigned)current_vp->height)
#if defined(HAVE_VIEWPORT_CLIP)
            && ((unsigned)x < (unsigned)LCD_WIDTH)
            && ((unsigned)y < (unsigned)LCD_HEIGHT)
#endif
            )
            pfunc(FBADDR(x + current_vp->x, y + current_vp->y));

        if (d < 0)
        {
            d += dinc1;
            x += xinc1;
            y += yinc1;
        }
        else
        {
            d += dinc2;
            x += xinc2;
            y += yinc2;
        }
    }
}

/* Draw a rectangular box */
void lcd_drawrect(int x, int y, int width, int height)
{
    if ((width <= 0) || (height <= 0))
        return;

    int x2 = x + width - 1;
    int y2 = y + height - 1;

    lcd_vline(x, y, y2);
    lcd_vline(x2, y, y2);
    lcd_hline(x, x2, y);
    lcd_hline(x, x2, y2);
}

/* Fill a rectangular area */
void lcd_fillrect(int x, int y, int width, int height)
{
    unsigned bits = 0;
    enum fill_opt fillopt = OPT_NONE;
    fb_data *dst, *dst_end;
    int len, step;

    /******************** In viewport clipping **********************/
    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= current_vp->width) ||
        (y >= current_vp->height) || (x + width <= 0) || (y + height <= 0))
        return;

    if (x < 0)
    {
        width += x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        y = 0;
    }
    if (x + width > current_vp->width)
        width = current_vp->width - x;
    if (y + height > current_vp->height)
        height = current_vp->height - y;

    /* adjust for viewport */
    x += current_vp->x;
    y += current_vp->y;

#if defined(HAVE_VIEWPORT_CLIP)
    /********************* Viewport on screen clipping ********************/
    /* nothing to draw? */
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)
        || (x + width <= 0) || (y + height <= 0))
        return;

    /* clip image in viewport in screen */
    if (x < 0)
    {
        width += x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        y = 0;
    }
    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y;
#endif

    /* drawmode and optimisation */
    if (current_vp->drawmode & DRMODE_INVERSEVID)
    {
        if (current_vp->drawmode & DRMODE_BG)
        {
            if (!lcd_backdrop)
            {
                fillopt = OPT_SET;
                bits = current_vp->bg_pattern;
            }
            else
                fillopt = OPT_COPY;
        }
    }
    else
    {
        if (current_vp->drawmode & DRMODE_FG)
        {
            fillopt = OPT_SET;
            bits = current_vp->fg_pattern;
        }
    }
    if (fillopt == OPT_NONE && current_vp->drawmode != DRMODE_COMPLEMENT)
        return;

    dst = FBADDR(x, y);
    dst_end = FBADDR(x + width - 1, y + height - 1);

    len  = STRIDE_MAIN(width, height);
    step = STRIDE_MAIN(ROW_INC, COL_INC);

    do
    {
        switch (fillopt)
        {
          case OPT_SET:
            memset16(dst, bits, len);
            break;

          case OPT_COPY:
            memcpy(dst, (void *)((long)dst + lcd_backdrop_offset),
                   len * sizeof(fb_data));
            break;

          case OPT_NONE:  /* DRMODE_COMPLEMENT */
          {
            fb_data *start = dst;
            fb_data *end = start + len;
            do
                *start = ~(*start);
            while (++start < end);
            break;
          }
        }
        dst += step;
    }
    while (dst <= dst_end);
}

/* About Rockbox' internal monochrome bitmap format:
 *
 * A bitmap contains one bit for every pixel that defines if that pixel is
 * black (1) or white (0). Bits within a byte are arranged vertically, LSB
 * at top.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. The first row of bytes defines pixel rows
 * 0..7, the second row defines pixel row 8..15 etc.
 *
 * This is the mono bitmap format used on all other targets so far; the
 * pixel packing doesn't really matter on a 8bit+ target. */

/* Draw a partial monochrome bitmap */

void ICODE_ATTR lcd_mono_bitmap_part(const unsigned char *src, int src_x,
                                     int src_y, int stride, int x, int y,
                                     int width, int height)
{
    const unsigned char *src_end;
    fb_data *dst, *dst_col;
    unsigned dmask = 0x100; /* bit 8 == sentinel */
    int drmode = current_vp->drawmode;
    int row;

    /******************** Image in viewport clipping **********************/
    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= current_vp->width) ||
        (y >= current_vp->height) || (x + width <= 0) || (y + height <= 0))
        return;

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
    if (x + width > current_vp->width)
        width = current_vp->width - x;
    if (y + height > current_vp->height)
        height = current_vp->height - y;

    /* adjust for viewport */
    x += current_vp->x;
    y += current_vp->y;

#if defined(HAVE_VIEWPORT_CLIP)
    /********************* Viewport on screen clipping ********************/
    /* nothing to draw? */
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)
        || (x + width <= 0) || (y + height <= 0))
        return;

    /* clip image in viewport in screen */
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
#endif

    src += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y  &= 7;
    src_end = src + width;
    dst_col = FBADDR(x, y);


    if (drmode & DRMODE_INVERSEVID)
    {
        dmask = 0x1ff;          /* bit 8 == sentinel */
        drmode &= DRMODE_SOLID; /* mask out inversevid */
    }

    /* go through each column and update each pixel  */
    do
    {
        const unsigned char *src_col = src++;
        unsigned data = (*src_col ^ dmask) >> src_y;
        int fg, bg;
        long bo;

        dst = dst_col;
        dst_col += COL_INC;
        row = height;

#define UPDATE_SRC  do {                  \
            data >>= 1;                   \
            if (data == 0x001) {          \
                src_col += stride;        \
                data = *src_col ^ dmask;  \
            }                             \
        } while (0)

        switch (drmode)
        {
          case DRMODE_COMPLEMENT:
            do
            {
                if (data & 0x01)
                    *dst = ~(*dst);

                dst += ROW_INC;
                UPDATE_SRC;
            }
            while (--row);
            break;

          case DRMODE_BG:
            if (lcd_backdrop)
            {
                bo = lcd_backdrop_offset;
                do
                {
                    if (!(data & 0x01))
                        *dst = *(fb_data *)((long)dst + bo);

                    dst += ROW_INC;
                    UPDATE_SRC;
                }
                while (--row);
            }
            else
            {
                bg = current_vp->bg_pattern;
                do
                {
                    if (!(data & 0x01))
                        *dst = bg;

                    dst += ROW_INC;
                    UPDATE_SRC;
                }
                while (--row);
            }
            break;

          case DRMODE_FG:
            fg = current_vp->fg_pattern;
            do
            {
                if (data & 0x01)
                    *dst = fg;

                dst += ROW_INC;
                UPDATE_SRC;
            }
            while (--row);
            break;

          case DRMODE_SOLID:
            fg = current_vp->fg_pattern;
            if (lcd_backdrop)
            {
                bo = lcd_backdrop_offset;
                do
                {
                    *dst = (data & 0x01) ? fg
                               : *(fb_data *)((long)dst + bo);
                    dst += ROW_INC;
                    UPDATE_SRC;
                }
                while (--row);
            }
            else
            {
                bg = current_vp->bg_pattern;
                do
                {
                    *dst = (data & 0x01) ? fg : bg;
                    dst += ROW_INC;
                    UPDATE_SRC;
                }
                while (--row);
            }
            break;
        }
    }
    while (src < src_end);
}
/* Draw a full monochrome bitmap */
void lcd_mono_bitmap(const unsigned char *src, int x, int y, int width, int height)
{
    lcd_mono_bitmap_part(src, 0, 0, width, x, y, width, height);
}


/* About Rockbox' internal alpha channel format (for ALPHA_COLOR_FONT_DEPTH == 2)
 *
 * For each pixel, 4bit  of alpha information is stored in a byte-stream,
 * so two pixels are packed into one byte.
 * The lower nibble is the first pixel, the upper one the second. The stride is
 * horizontal. E.g row0: pixel0: byte0[0:3], pixel1: byte0[4:7], pixel2: byte1[0:3],...
 * The format is independant of the internal display orientation and color
 * representation, as to support the same font files on all displays.
 * The values go linear from 0 (fully transparent) to 15 (fully opaque).
 *
 * This might suggest that rows need to have an even number of pixels.
 * However this is generally not the case. lcd_alpha_bitmap_part_mix() can deal
 * with uneven colums (i.e. two rows can share one byte). And font files do
 * exploit this.
 * However, this is difficult to do for image files, especially bottom-up bitmaps,
 * so lcd_bmp() do expect even rows.
 */

#define ALPHA_COLOR_FONT_DEPTH 2
#define ALPHA_COLOR_LOOKUP_SHIFT (1 << ALPHA_COLOR_FONT_DEPTH)
#define ALPHA_COLOR_LOOKUP_SIZE ((1 << ALPHA_COLOR_LOOKUP_SHIFT) - 1)
#define ALPHA_COLOR_PIXEL_PER_BYTE (8 >> ALPHA_COLOR_FONT_DEPTH)
#define ALPHA_COLOR_PIXEL_PER_WORD (32 >> ALPHA_COLOR_FONT_DEPTH)
#ifdef CPU_ARM
#define BLEND_INIT do {} while (0)
#define BLEND_FINISH do {} while(0)
#define BLEND_START(acc, color, alpha) \
    asm volatile("mul %0, %1, %2" : "=&r" (acc) : "r" (color), "r" (alpha))
#define BLEND_CONT(acc, color, alpha) \
    asm volatile("mla %0, %1, %2, %0" : "+&r" (acc) : "r" (color), "r" (alpha))
#define BLEND_OUT(acc) do {} while (0)
#elif defined(CPU_COLDFIRE)
#define ALPHA_BITMAP_READ_WORDS
#define BLEND_INIT \
    unsigned long _macsr = coldfire_get_macsr(); \
    coldfire_set_macsr(EMAC_UNSIGNED)
#define BLEND_FINISH \
    coldfire_set_macsr(_macsr)
#define BLEND_START(acc, color, alpha) \
    asm volatile("mac.l %0, %1, %%acc0" :: "%d" (color), "d" (alpha))
#define BLEND_CONT BLEND_START
#define BLEND_OUT(acc) asm volatile("movclr.l %%acc0, %0" : "=d" (acc))
#else
#define BLEND_INIT do {} while (0)
#define BLEND_FINISH do {} while(0)
#define BLEND_START(acc, color, alpha) ((acc) = (color) * (alpha))
#define BLEND_CONT(acc, color, alpha) ((acc) += (color) * (alpha))
#define BLEND_OUT(acc) do {} while (0)
#endif

/* Blend the given two colors */
static inline unsigned blend_two_colors(unsigned c1, unsigned c2, unsigned a)
{
    a += a >> (ALPHA_COLOR_LOOKUP_SHIFT - 1);
#if (LCD_PIXELFORMAT == RGB565SWAPPED)
    c1 = swap16(c1);
    c2 = swap16(c2);
#endif
    unsigned c1l = (c1 | (c1 << 16)) & 0x07e0f81f;
    unsigned c2l = (c2 | (c2 << 16)) & 0x07e0f81f;
    unsigned p;
    BLEND_START(p, c1l, a);
    BLEND_CONT(p, c2l, ALPHA_COLOR_LOOKUP_SIZE + 1 - a);
    BLEND_OUT(p);
    p = (p >> ALPHA_COLOR_LOOKUP_SHIFT) & 0x07e0f81f;
    p |= (p >> 16);
#if (LCD_PIXELFORMAT == RGB565SWAPPED)
    return swap16(p);
#else
    return p;
#endif
}

/* Blend the given color with the value from the alpha_color_lookup table */
static inline unsigned blend_color(unsigned c, unsigned a)
{
    return blend_two_colors(c, current_vp->fg_pattern, a);
}

/* Blend an image with an alpha channel
 * if image is NULL, drawing will happen according to the drawmode
 * src is the alpha channel (4bit per pixel) */
static void ICODE_ATTR lcd_alpha_bitmap_part_mix(const fb_data* image,
                                      const unsigned char *src, int src_x,
                                      int src_y, int x, int y,
                                      int width, int height,
                                      int stride_image, int stride_src)
{
    fb_data *dst, *dst_row;
    const fb_data *image_row;
    unsigned dmask = 0x00000000;
    int drmode = current_vp->drawmode;
    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= current_vp->width) ||
         (y >= current_vp->height) || (x + width <= 0) || (y + height <= 0))
        return;
    /* initialize blending */
    BLEND_INIT;

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
    if (x + width > current_vp->width)
        width = current_vp->width - x;
    if (y + height > current_vp->height)
        height = current_vp->height - y;

    /* adjust for viewport */
    x += current_vp->x;
    y += current_vp->y;

#if defined(HAVE_VIEWPORT_CLIP)
    /********************* Viewport on screen clipping ********************/
    /* nothing to draw? */
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)
        || (x + width <= 0) || (y + height <= 0))
    {
        BLEND_FINISH;
        return;
    }

    /* clip image in viewport in screen */
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
#endif

    if (drmode & DRMODE_INVERSEVID)
    {
        dmask = 0xffffffff;
        drmode &= DRMODE_SOLID; /* mask out inversevid */
    }
    /* sourcing from an image ignore drawmode.
     * Set to DRMODE_BG as we use its code path in the switch below */
    if (image != NULL)
    {
        drmode = DRMODE_BG;
    }
    if (drmode == DRMODE_BG)
    {
        dmask = ~dmask;
    }

    dst_row = FBADDR(x, y);

    int col, row = height;
    unsigned data, pixels;
    unsigned skip_end = (stride_src - width);
    unsigned skip_start = src_y * stride_src + src_x;
    unsigned skip_start_image = STRIDE_MAIN(src_y * stride_image + src_x,
                                            src_x * stride_image + src_y);

#ifdef ALPHA_BITMAP_READ_WORDS
    uint32_t *src_w = (uint32_t *)((uintptr_t)src & ~3);
    skip_start += ALPHA_COLOR_PIXEL_PER_BYTE * ((uintptr_t)src & 3);
    src_w += skip_start / ALPHA_COLOR_PIXEL_PER_WORD;
    data = letoh32(*src_w++) ^ dmask;
    pixels = skip_start % ALPHA_COLOR_PIXEL_PER_WORD;
#else
    src += skip_start / ALPHA_COLOR_PIXEL_PER_BYTE;
    data = *src ^ dmask;
    pixels = skip_start % ALPHA_COLOR_PIXEL_PER_BYTE;
#endif
    data >>= pixels * ALPHA_COLOR_LOOKUP_SHIFT;
#ifdef ALPHA_BITMAP_READ_WORDS
    pixels = 8 - pixels;
#endif
    if (image)
        image += skip_start_image;
    image_row = image;

    /* go through the rows and update each pixel */
    do
    {
        col = width;
        dst = dst_row;
        dst_row += ROW_INC;
        if (image_row) {
            image = image_row;
            image_row += STRIDE_MAIN(stride_image,1);
        }
        else
            image = dst;
#ifdef ALPHA_BITMAP_READ_WORDS
#define UPDATE_SRC_ALPHA    do { \
            if (--pixels) \
                data >>= ALPHA_COLOR_LOOKUP_SHIFT; \
            else \
            { \
                data = letoh32(*src_w++) ^ dmask; \
                pixels = ALPHA_COLOR_PIXEL_PER_WORD; \
            } \
        } while (0)
#elif ALPHA_COLOR_PIXEL_PER_BYTE == 2
#define UPDATE_SRC_ALPHA    do { \
            if (pixels ^= 1) \
                data >>= ALPHA_COLOR_LOOKUP_SHIFT; \
            else \
                data = *(++src) ^ dmask; \
        } while (0)
#else
#define UPDATE_SRC_ALPHA    do { \
            if (pixels = (++pixels % ALPHA_COLOR_PIXEL_PER_BYTE)) \
                data >>= ALPHA_COLOR_LOOKUP_SHIFT; \
            else \
                data = *(++src) ^ dmask; \
        } while (0)
#endif
        /* we don't want to have this in our inner
         * loop and the codesize increase is minimal */
        switch (drmode)
        {
            case DRMODE_COMPLEMENT:
                do
                {
                    *dst = blend_two_colors(*dst, ~(*dst),
                                data & ALPHA_COLOR_LOOKUP_SIZE );
                    dst += COL_INC;
                    UPDATE_SRC_ALPHA;
                }
                while (--col);
                break;
            case DRMODE_BG:
                if(lcd_backdrop)
                {
                    uintptr_t bo = lcd_backdrop_offset;
                    do
                    {
                        *dst = blend_two_colors(*(fb_data *)((uintptr_t)dst + bo),
                                    *image, data & ALPHA_COLOR_LOOKUP_SIZE );

                        dst += COL_INC;
                        image += STRIDE_MAIN(1, stride_image);
                        UPDATE_SRC_ALPHA;
                    }
                    while (--col);
                }
                else
                {
                    do
                    {
                        *dst = blend_two_colors(current_vp->bg_pattern,
                                    *image, data & ALPHA_COLOR_LOOKUP_SIZE );
                        dst += COL_INC;
                        image += STRIDE_MAIN(1, stride_image);
                        UPDATE_SRC_ALPHA;
                    }
                    while (--col);
                }
                break;
            case DRMODE_FG:
                do
                {
                    *dst = blend_color(*dst, data & ALPHA_COLOR_LOOKUP_SIZE );
                    dst += COL_INC;
                    UPDATE_SRC_ALPHA;
                }
                while (--col);
                break;
            case DRMODE_SOLID:
                if(lcd_backdrop)
                {
                    uintptr_t bo = lcd_backdrop_offset;
                    do
                    {
                        *dst = blend_color(*(fb_data *)((uintptr_t)dst + bo),
                                        data & ALPHA_COLOR_LOOKUP_SIZE );
                        dst += COL_INC;
                        UPDATE_SRC_ALPHA;
                    }
                    while (--col);
                }
                else
                {
                    do
                    {
                        *dst = blend_color(current_vp->bg_pattern,
                                        data & ALPHA_COLOR_LOOKUP_SIZE );
                        dst += COL_INC;
                        UPDATE_SRC_ALPHA;
                    }
                    while (--col);
                }
                break;
        }
#ifdef ALPHA_BITMAP_READ_WORDS
        if (skip_end < pixels)
        {
            pixels -= skip_end;
            data >>= skip_end * ALPHA_COLOR_LOOKUP_SHIFT;
        } else {
            pixels = skip_end - pixels;
            src_w += pixels / ALPHA_COLOR_PIXEL_PER_WORD;
            pixels %= ALPHA_COLOR_PIXEL_PER_WORD;
            data = letoh32(*src_w++) ^ dmask;
            data >>= pixels * ALPHA_COLOR_LOOKUP_SHIFT;
            pixels = 8 - pixels;
        }
#else
        if (skip_end)
        {
            pixels += skip_end;
            if (pixels >= ALPHA_COLOR_PIXEL_PER_BYTE)
            {
                src += pixels / ALPHA_COLOR_PIXEL_PER_BYTE;
                pixels %= ALPHA_COLOR_PIXEL_PER_BYTE;
                data = *src ^ dmask;
                data >>= pixels * ALPHA_COLOR_LOOKUP_SHIFT;
            } else
                data >>= skip_end * ALPHA_COLOR_LOOKUP_SHIFT;
        }
#endif
    } while (--row);

    BLEND_FINISH;
}

/* Draw a full native bitmap */
void lcd_bitmap(const fb_data *src, int x, int y, int width, int height)
{
    lcd_bitmap_part(src, 0, 0, STRIDE(SCREEN_MAIN, width, height), x, y, width, height);
}

/* Draw a full native bitmap with a transparent color */
void lcd_bitmap_transparent(const fb_data *src, int x, int y,
                            int width, int height)
{
    lcd_bitmap_transparent_part(src, 0, 0,
                                STRIDE(SCREEN_MAIN, width, height), x, y, width, height);
}

/* draw alpha bitmap for anti-alias font */
void ICODE_ATTR lcd_alpha_bitmap_part(const unsigned char *src, int src_x,
                                      int src_y, int stride, int x, int y,
                                      int width, int height)
{
    lcd_alpha_bitmap_part_mix(NULL, src, src_x, src_y, x, y, width, height, 0, stride);
}

/* Draw a partial bitmap (mono or native) including alpha channel */
void ICODE_ATTR lcd_bmp_part(const struct bitmap* bm, int src_x, int src_y,
                                int x, int y, int width, int height)
{
    int bitmap_stride = STRIDE_MAIN(bm->width, bm->height);
    if (bm->format == FORMAT_MONO)
        lcd_mono_bitmap_part(bm->data, src_x, src_y, bitmap_stride, x, y, width, height);
    else if (bm->alpha_offset > 0)
        lcd_alpha_bitmap_part_mix((fb_data*)bm->data, bm->data+bm->alpha_offset,
            src_x, src_y, x, y, width, height,
            bitmap_stride, ALIGN_UP(bm->width, 2));
    else
        lcd_bitmap_transparent_part((fb_data*)bm->data,
            src_x, src_y, bitmap_stride, x, y, width, height);
}

/* Draw a native bitmap with alpha channel */
void ICODE_ATTR lcd_bmp(const struct bitmap *bmp, int x, int y)
{
    lcd_bmp_part(bmp, 0, 0, x, y, bmp->width, bmp->height);
}

/**
 * |R|   |1.000000 -0.000001  1.402000| |Y'|
 * |G| = |1.000000 -0.334136 -0.714136| |Pb|
 * |B|   |1.000000  1.772000  0.000000| |Pr|
 * Scaled, normalized, rounded and tweaked to yield RGB 565:
 * |R|   |74   0 101| |Y' -  16| >> 9
 * |G| = |74 -24 -51| |Cb - 128| >> 8
 * |B|   |74 128   0| |Cr - 128| >> 9
 */
#define YFAC    (74)
#define RVFAC   (101)
#define GUFAC   (-24)
#define GVFAC   (-51)
#define BUFAC   (128)

static inline int clamp(int val, int min, int max)
{
    if (val < min)
        val = min;
    else if (val > max)
        val = max;
    return val;
}

#ifndef _WIN32
/*
 * weak attribute doesn't work for win32 as of gcc 4.6.2 and binutils 2.21.52
 * When building win32 simulators, we won't be using an optimized version of
 * lcd_blit_yuv(), so just don't use the weak attribute.
 */
__attribute__((weak))
#endif
void lcd_yuv_set_options(unsigned options)
{
    (void)options;
}

/* Draw a partial YUV colour bitmap */
#ifndef _WIN32
__attribute__((weak))
#endif
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    const unsigned char *ysrc, *usrc, *vsrc;
    int linecounter;
    fb_data *dst, *row_end;
    long z;

    /* width and height must be >= 2 and an even number */
    width &= ~1;
    linecounter = height >> 1;

#if LCD_WIDTH >= LCD_HEIGHT
    dst     = FBADDR(x, y);
    row_end = dst + width;
#else
    dst     = FBADDR(LCD_WIDTH - y - 1, x);
    row_end = dst + LCD_WIDTH * width;
#endif

    z    = stride * src_y;
    ysrc = src[0] + z + src_x;
    usrc = src[1] + (z >> 2) + (src_x >> 1);
    vsrc = src[2] + (usrc - src[1]);

    /* stride => amount to jump from end of last row to start of next */
    stride -= width;

    /* upsampling, YUV->RGB conversion and reduction to RGB565 in one go */

    do
    {
        do
        {
            int y, cb, cr, rv, guv, bu, r, g, b;

            y  = YFAC*(*ysrc++ - 16);
            cb = *usrc++ - 128;
            cr = *vsrc++ - 128;

            rv  =            RVFAC*cr;
            guv = GUFAC*cb + GVFAC*cr;
            bu  = BUFAC*cb;

            r = y + rv;
            g = y + guv;
            b = y + bu;

            if ((unsigned)(r | g | b) > 64*256-1)
            {
                r = clamp(r, 0, 64*256-1);
                g = clamp(g, 0, 64*256-1);
                b = clamp(b, 0, 64*256-1);
            }

            *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

#if LCD_WIDTH >= LCD_HEIGHT
            dst++;
#else
            dst += LCD_WIDTH;
#endif

            y = YFAC*(*ysrc++ - 16);
            r = y + rv;
            g = y + guv;
            b = y + bu;

            if ((unsigned)(r | g | b) > 64*256-1)
            {
                r = clamp(r, 0, 64*256-1);
                g = clamp(g, 0, 64*256-1);
                b = clamp(b, 0, 64*256-1);
            }

            *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

#if LCD_WIDTH >= LCD_HEIGHT
            dst++;
#else
            dst += LCD_WIDTH;
#endif
        }
        while (dst < row_end);

        ysrc    += stride;
        usrc    -= width >> 1;
        vsrc    -= width >> 1;

#if LCD_WIDTH >= LCD_HEIGHT
        row_end += LCD_WIDTH;
        dst     += LCD_WIDTH - width;
#else
        row_end -= 1;
        dst     -= LCD_WIDTH*width + 1;
#endif

        do
        {
            int y, cb, cr, rv, guv, bu, r, g, b;

            y  = YFAC*(*ysrc++ - 16);
            cb = *usrc++ - 128;
            cr = *vsrc++ - 128;

            rv  =            RVFAC*cr;
            guv = GUFAC*cb + GVFAC*cr;
            bu  = BUFAC*cb;

            r = y + rv;
            g = y + guv;
            b = y + bu;

            if ((unsigned)(r | g | b) > 64*256-1)
            {
                r = clamp(r, 0, 64*256-1);
                g = clamp(g, 0, 64*256-1);
                b = clamp(b, 0, 64*256-1);
            }

            *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

#if LCD_WIDTH >= LCD_HEIGHT
            dst++;
#else
            dst += LCD_WIDTH;
#endif

            y = YFAC*(*ysrc++ - 16);
            r = y + rv;
            g = y + guv;
            b = y + bu;

            if ((unsigned)(r | g | b) > 64*256-1)
            {
                r = clamp(r, 0, 64*256-1);
                g = clamp(g, 0, 64*256-1);
                b = clamp(b, 0, 64*256-1);
            }

            *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

#if LCD_WIDTH >= LCD_HEIGHT
            dst++;
#else
            dst += LCD_WIDTH;
#endif
        }
        while (dst < row_end);

        ysrc    += stride;
        usrc    += stride >> 1;
        vsrc    += stride >> 1;

#if LCD_WIDTH >= LCD_HEIGHT
        row_end += LCD_WIDTH;
        dst     += LCD_WIDTH - width;
#else
        row_end -= 1;
        dst     -= LCD_WIDTH*width + 1;
#endif
    }
    while (--linecounter > 0);

#if LCD_WIDTH >= LCD_HEIGHT
    lcd_update_rect(x, y, width, height);
#else
    lcd_update_rect(LCD_WIDTH - y - height, x, height, width);
#endif
}
