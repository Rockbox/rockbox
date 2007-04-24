/*
 * video_out_null.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "mpeg2dec_config.h"

#include "plugin.h"
#include "gray.h"

extern struct plugin_api* rb;

#include "mpeg2.h"
#include "video_out.h"

static int image_width;
static int image_height;
static int image_chroma_x;
static int image_chroma_y;
static int output_x;
static int output_y;
static int output_width;
static int output_height;

#if defined(SIMULATOR) && defined(HAVE_LCD_COLOR)

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

/* Draw a partial YUV colour bitmap - similiar behavior to lcd_yuv_blit
   in the core */
static void yuv_bitmap_part(unsigned char * const src[3],
                            int src_x, int src_y, int stride,
                            int x, int y, int width, int height)
{
    const unsigned char *ysrc, *usrc, *vsrc;
    fb_data *dst, *row_end;
    off_t z;

    /* width and height must be >= 2 and an even number */
    width &= ~1;
    height >>= 1;

#if LCD_WIDTH >= LCD_HEIGHT
    dst     = rb->lcd_framebuffer + LCD_WIDTH * y + x;
    row_end = dst + width;
#else
    dst     = rb->lcd_framebuffer + x * LCD_WIDTH + (LCD_WIDTH - y) - 1;
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
    while (--height > 0);
}
#endif /* defined(SIMULATOR) && defined(HAVE_LCD_COLOR) */

void vo_draw_frame (uint8_t * const * buf)
{
#ifdef HAVE_LCD_COLOR
#ifdef SIMULATOR
    yuv_bitmap_part(buf,0,0,image_width,
                    output_x,output_y,output_width,output_height);
#if LCD_WIDTH >= LCD_HEIGHT
    rb->lcd_update_rect(output_x,output_y,output_width,output_height);
#else
    rb->lcd_update_rect(output_y,output_x,output_height,output_width);
#endif
#else
    rb->lcd_yuv_blit(buf,
                    0,0,image_width,
                    output_x,output_y,output_width,output_height);
#endif
#else
    gray_ub_gray_bitmap_part(buf[0],0,0,image_width,
                             output_x,output_y,output_width,output_height);
#endif
}

#if LCD_WIDTH >= LCD_HEIGHT
#define SCREEN_WIDTH LCD_WIDTH
#define SCREEN_HEIGHT LCD_HEIGHT
#else /* Assume the screen is rotates on portraid LCDs */
#define SCREEN_WIDTH LCD_HEIGHT
#define SCREEN_HEIGHT LCD_WIDTH
#endif

void vo_setup(const mpeg2_sequence_t * sequence)
{
    image_width=sequence->width;
    image_height=sequence->height;
    image_chroma_x=image_width/sequence->chroma_width;
    image_chroma_y=image_height/sequence->chroma_height;

    if (sequence->display_width >= SCREEN_WIDTH) {
        output_width = SCREEN_WIDTH;
        output_x = 0;
    } else {
        output_width = sequence->display_width;
        output_x = (SCREEN_WIDTH-sequence->display_width)/2;
    }

    if (sequence->display_height >= SCREEN_HEIGHT) {
        output_height = SCREEN_HEIGHT;
        output_y = 0;
    } else {
        output_height = sequence->display_height;
        output_y = (SCREEN_HEIGHT-sequence->display_height)/2;
    }
}
