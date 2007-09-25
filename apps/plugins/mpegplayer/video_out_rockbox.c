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

void vo_draw_frame (uint8_t * const * buf)
{
#ifdef HAVE_LCD_COLOR
    rb->lcd_yuv_blit(buf,
                    0,0,image_width,
                    output_x,output_y,output_width,output_height);
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
