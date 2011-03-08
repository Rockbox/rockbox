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
    dst_col = LCDADDR(x, y);
    

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

/* draw alpha bitmap for anti-alias font */
#define ALPHA_COLOR_FONT_DEPTH 2
#define ALPHA_COLOR_LOOKUP_SHIFT (1 << ALPHA_COLOR_FONT_DEPTH)
#define ALPHA_COLOR_LOOKUP_SIZE ((1 << ALPHA_COLOR_LOOKUP_SHIFT) - 1)
#define ALPHA_COLOR_PIXEL_PER_BYTE (8 >> ALPHA_COLOR_FONT_DEPTH)
#define ALPHA_COLOR_PIXEL_PER_WORD (32 >> ALPHA_COLOR_FONT_DEPTH)
#ifdef CPU_ARM
#define BLEND_INIT do {} while (0)
#define BLEND_START(acc, color, alpha) \
    asm volatile("mul %0, %1, %2" : "=&r" (acc) : "r" (color), "r" (alpha))
#define BLEND_CONT(acc, color, alpha) \
    asm volatile("mla %0, %1, %2, %0" : "+&r" (acc) : "r" (color), "r" (alpha))
#define BLEND_OUT(acc) do {} while (0)
#elif defined(CPU_COLDFIRE)
#define ALPHA_BITMAP_READ_WORDS
#define BLEND_INIT coldfire_set_macsr(EMAC_UNSIGNED)
#define BLEND_START(acc, color, alpha) \
    asm volatile("mac.l %0, %1, %%acc0" :: "%d" (color), "d" (alpha))
#define BLEND_CONT BLEND_START
#define BLEND_OUT(acc) asm volatile("movclr.l %%acc0, %0" : "=d" (acc))
#else
#define BLEND_INIT do {} while (0)
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

void ICODE_ATTR lcd_alpha_bitmap_part(const unsigned char *src, int src_x,
                                      int src_y, int stride, int x, int y,
                                      int width, int height)
{
    fb_data *dst, *dst_row;
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

    if (drmode & DRMODE_INVERSEVID)
    {
        dmask = 0xffffffff;
        drmode &= DRMODE_SOLID; /* mask out inversevid */
    }
    if (drmode == DRMODE_BG)
    {
        dmask = ~dmask;
    }

    dst_row = LCDADDR(x, y);

    int col, row = height;
    unsigned data, pixels;
    unsigned skip_end = (stride - width);
    unsigned skip_start = src_y * stride + src_x;

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

    /* go through the rows and update each pixel */
    do
    {
        col = width;
        dst = dst_row;
        dst_row += ROW_INC;
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
                        *dst = blend_two_colors(*dst, *(fb_data *)((uintptr_t)dst + bo),
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
                        *dst = blend_two_colors(*dst, current_vp->bg_pattern,
                                    data & ALPHA_COLOR_LOOKUP_SIZE );
                        dst += COL_INC;
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
}
