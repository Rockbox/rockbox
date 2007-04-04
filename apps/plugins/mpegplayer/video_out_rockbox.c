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

#define CSUB_X 2
#define CSUB_Y 2

static int image_width;
static int image_height;
static int image_chroma_x;
static int image_chroma_y;
static int output_x;
static int output_y;
static int output_width;
static int output_height;

#if defined(SIMULATOR) && defined(HAVE_LCD_COLOR)

#define RYFAC (31*257)
#define GYFAC (63*257)
#define BYFAC (31*257)
#define RVFAC 11170     /* 31 * 257 *  1.402    */
#define GVFAC (-11563)  /* 63 * 257 * -0.714136 */
#define GUFAC (-5572)   /* 63 * 257 * -0.344136 */
#define BUFAC 14118     /* 31 * 257 *  1.772    */

#define ROUNDOFFS (127*257)

/* Draw a partial YUV colour bitmap - taken from the Rockbox JPEG viewer */
static void yuv_bitmap_part(unsigned char * const src[3],
                            int src_x, int src_y, int stride,
                            int x, int y, int width, int height)
{
    fb_data *dst, *dst_end;

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

    dst = rb->lcd_framebuffer + LCD_WIDTH * y + x;
    dst_end = dst + LCD_WIDTH * height;

    do
    {
        fb_data *dst_row = dst;
        fb_data *row_end = dst_row + width;
        const unsigned char *ysrc = src[0] + stride * src_y + src_x;
        int y, u, v;
        int red, green, blue;
        unsigned rbits, gbits, bbits;

        if (CSUB_Y) /* colour */
        {
            /* upsampling, YUV->RGB conversion and reduction to RGB565 in one go */
            const unsigned char *usrc = src[1] + (stride/CSUB_X) * (src_y/CSUB_Y)
                                               + (src_x/CSUB_X);
            const unsigned char *vsrc = src[2] + (stride/CSUB_X) * (src_y/CSUB_Y)
                                               + (src_x/CSUB_X);
            int xphase = src_x % CSUB_X;
            int rc, gc, bc;

            u = *usrc++ - 128;
            v = *vsrc++ - 128;
            rc = RVFAC * v + ROUNDOFFS;
            gc = GVFAC * v + GUFAC * u + ROUNDOFFS;
            bc = BUFAC * u + ROUNDOFFS;

            do
            {
                y = *ysrc++;
                red   = RYFAC * y + rc;
                green = GYFAC * y + gc;
                blue  = BYFAC * y + bc;

                if ((unsigned)red > (RYFAC*255+ROUNDOFFS))
                {
                    if (red < 0)
                        red = 0;
                    else
                        red = (RYFAC*255+ROUNDOFFS);
                }
                if ((unsigned)green > (GYFAC*255+ROUNDOFFS))
                {
                    if (green < 0)
                        green = 0;
                    else
                        green = (GYFAC*255+ROUNDOFFS);
                }
                if ((unsigned)blue > (BYFAC*255+ROUNDOFFS))
                {
                    if (blue < 0)
                        blue = 0;
                    else
                        blue = (BYFAC*255+ROUNDOFFS);
                }
                rbits = ((unsigned)red) >> 16 ;
                gbits = ((unsigned)green) >> 16 ;
                bbits = ((unsigned)blue) >> 16 ;
#if LCD_PIXELFORMAT == RGB565
                *dst_row++ = (rbits << 11) | (gbits << 5) | bbits;
#elif LCD_PIXELFORMAT == RGB565SWAPPED
                *dst_row++ = swap16((rbits << 11) | (gbits << 5) | bbits);
#endif

                if (++xphase >= CSUB_X)
                {
                    u = *usrc++ - 128;
                    v = *vsrc++ - 128;
                    rc = RVFAC * v + ROUNDOFFS;
                    gc = GVFAC * v + GUFAC * u + ROUNDOFFS;
                    bc = BUFAC * u + ROUNDOFFS;
                    xphase = 0;
                }
            }
            while (dst_row < row_end);
        }
        else /* monochrome */
        {
            do
            {
                y = *ysrc++;
                red   = RYFAC * y + ROUNDOFFS;  /* blue == red */
                green = GYFAC * y + ROUNDOFFS;
                rbits = ((unsigned)red) >> 16;
                gbits = ((unsigned)green) >> 16;
#if LCD_PIXELFORMAT == RGB565
                *dst_row++ = (rbits << 11) | (gbits << 5) | rbits;
#elif LCD_PIXELFORMAT == RGB565SWAPPED
                *dst_row++ = swap16((rbits << 11) | (gbits << 5) | rbits);
#endif
            }
            while (dst_row < row_end);
        }

        src_y++;
        dst += LCD_WIDTH;
    }
    while (dst < dst_end);
}
#endif

void vo_draw_frame (uint8_t * const * buf)
{
#ifdef HAVE_LCD_COLOR
#ifdef SIMULATOR
    yuv_bitmap_part(buf,0,0,image_width,
                    output_x,output_y,output_width,output_height);
    rb->lcd_update_rect(output_x,output_y,output_width,output_height);
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

void vo_setup(unsigned int display_width, unsigned int display_height, unsigned int width, unsigned int height, 
             unsigned int chroma_width, unsigned int chroma_height)
{
    image_width=width;
    image_height=height;
    image_chroma_x=image_width/chroma_width;
    image_chroma_y=image_height/chroma_height;

    if (display_width >= SCREEN_WIDTH) {
        output_width = SCREEN_WIDTH;
        output_x = 0;
    } else {
        output_width = display_width;
        output_x = (SCREEN_WIDTH-display_width)/2;
    }

    if (display_height >= SCREEN_HEIGHT) {
        output_height = SCREEN_HEIGHT;
        output_y = 0;
    } else {
        output_height = display_height;
        output_y = (SCREEN_HEIGHT-display_height)/2;
    }
}
