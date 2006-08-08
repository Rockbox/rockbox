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

extern struct plugin_api* rb;

#include "mpeg2.h"
#include "video_out.h"

static int starttick;

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

#if (LCD_DEPTH == 16) && \
    ((LCD_PIXELFORMAT == RGB565) || (LCD_PIXELFORMAT == RGB565SWAPPED))

#define RYFAC (31*257)
#define GYFAC (63*257)
#define BYFAC (31*257)
#define RVFAC 11170     /* 31 * 257 *  1.402    */
#define GVFAC (-11563)  /* 63 * 257 * -0.714136 */
#define GUFAC (-5572)   /* 63 * 257 * -0.344136 */
#define BUFAC 14118     /* 31 * 257 *  1.772    */

#define ROUNDOFFS (127*257)

/* Draw a partial YUV colour bitmap - taken from the Rockbox JPEG viewer */
void yuv_bitmap_part(unsigned char * const src[3],
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

static void rockbox_draw_frame (vo_instance_t * instance,
                                uint8_t * const * buf, void * id)
{
    char str[80];
    static int frame=0;
    int ticks,fps;

    (void)id;
    (void)instance;

#if (CONFIG_LCD == LCD_IPODCOLOR || CONFIG_LCD == LCD_IPODNANO \
     || CONFIG_LCD == LCD_H300) && !defined(SIMULATOR)
    rb->lcd_yuv_blit(buf,
                    0,0,image_width,
                    output_x,output_y,output_width,output_height);
#elif (LCD_DEPTH == 16) && \
    ((LCD_PIXELFORMAT == RGB565) || (LCD_PIXELFORMAT == RGB565SWAPPED))
    yuv_bitmap_part(buf,0,0,image_width,
                    output_x,output_y,output_width,output_height);
    rb->lcd_update_rect(output_x,output_y,output_width,output_height);
#endif

    if (starttick==0) starttick=*rb->current_tick-1; /* Avoid divby0 */

    /* Calculate fps */
    frame++;
    if ((frame % 125) == 0) {
        ticks=(*rb->current_tick)-starttick;

        fps=(frame*1000)/ticks;
        rb->snprintf(str,sizeof(str),"%d.%d",(fps/10),fps%10);
        rb->lcd_putsxy(0,0,str);

        rb->lcd_update_rect(0,0,80,8);
    }
}

vo_instance_t static_instance;

static vo_instance_t * internal_open (int setup (vo_instance_t *, unsigned int,
                                                 unsigned int, unsigned int,
                                                 unsigned int,
                                                 vo_setup_result_t *),
                                      void draw (vo_instance_t *,
                                                 uint8_t * const *, void *))
{
    vo_instance_t * instance;

    instance = (vo_instance_t *) &static_instance;
    if (instance == NULL)
        return NULL;

    instance->setup = setup;
    instance->setup_fbuf = NULL;
    instance->set_fbuf = NULL;
    instance->start_fbuf = NULL;
    instance->draw = draw;
    instance->discard = NULL;
    //instance->close = (void (*) (vo_instance_t *)) free;

    return instance;
}

static int rockbox_setup (vo_instance_t * instance, unsigned int width,
                          unsigned int height, unsigned int chroma_width,
                          unsigned int chroma_height,
                          vo_setup_result_t * result)
{
    (void)instance;

    result->convert = NULL;

    image_width=width;
    image_height=height;
    image_chroma_x=image_width/chroma_width;
    image_chroma_y=image_height/chroma_height;

    if (image_width >= LCD_WIDTH) {
        output_width = LCD_WIDTH;
        output_x = 0;
    } else {
        output_width = image_width;
        output_x = (LCD_WIDTH-image_width)/2;
    }

    if (image_height >= LCD_HEIGHT) {
        output_height = LCD_HEIGHT;
        output_y = 0;
    } else {
        output_height = image_height;
        output_y = (LCD_HEIGHT-image_height)/2;
    }

    return 0;
}

vo_instance_t * vo_rockbox_open (void)
{
    return internal_open (rockbox_setup, rockbox_draw_frame);
}
