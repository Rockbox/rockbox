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
    rb->lcd_yuv_blit(buf, 0,0,image_width,
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

uint8_t* tmpbufa = 0;
uint8_t* tmpbufb = 0;
uint8_t* tmpbufc = 0;
uint8_t* tmpbuf[3];

void vo_draw_frame_thumb (uint8_t * const * buf)
{
  int r,c;

#if LCD_WIDTH >= LCD_HEIGHT
  for (r=0;r<image_width/2;r++)
    for (c=0;c<image_height/2;c++)
      *(tmpbuf[0]+c*image_width/2+r) = 
        *(buf[0]+2*c*image_width+2*r);
  
  for (r=0;r<image_width/4;r++)
    for (c=0;c<image_height/4;c++)
    {
      *(tmpbuf[1]+c*image_width/4+r) = 
        *(buf[1]+c*image_width+2*r);
      *(tmpbuf[2]+c*image_width/4+r) = 
        *(buf[2]+c*image_width+2*r);
    }
#else
  for (r=0;r<image_width/2;r++)
    for (c=0;c<image_height/2;c++)
      *(tmpbuf[0]+(image_width/2-1-r)*image_height/2+c) = 
        *(buf[0]+2*c*image_width+2*r);
  
  for (r=0;r<image_width/4;r++)
    for (c=0;c<image_height/4;c++)
    {
      *(tmpbuf[1]+(image_width/4-1-r)*image_height/4+c) = 
        *(buf[1]+c*image_width+2*r);
      *(tmpbuf[2]+(image_width/4-1-r)*image_height/4+c) = 
        *(buf[2]+c*image_width+2*r);
    }
#endif

rb->lcd_clear_display();
rb->lcd_update();

#ifdef HAVE_LCD_COLOR
#ifdef SIMULATOR
#if LCD_WIDTH >= LCD_HEIGHT
  rb->lcd_yuv_blit(tmpbuf,0,0,image_width/2,
                  (LCD_WIDTH-1-image_width/2)/2,
                  LCD_HEIGHT-50-(image_height/2),
                  output_width/2,output_height/2);

#else
  rb->lcd_yuv_blit(tmpbuf,0,0,image_height/2,
                  LCD_HEIGHT-50-(image_height/2),
                  (LCD_WIDTH-1-image_width/2)/2,
                  output_height/2,output_width/2);
#endif
#else
#if LCD_WIDTH >= LCD_HEIGHT
  rb->lcd_yuv_blit(tmpbuf,0,0,image_width/2,
                   (LCD_WIDTH-1-image_width/2)/2,
                   LCD_HEIGHT-50-(image_height/2),
                   output_width/2,output_height/2);
#else
  rb->lcd_yuv_blit(tmpbuf,0,0,image_height/2,
                   LCD_HEIGHT-50-(image_height/2),
                   (LCD_WIDTH-1-image_width/2)/2,
                   output_height/2,output_width/2);
#endif
#endif
#else
#if LCD_WIDTH >= LCD_HEIGHT
  gray_ub_gray_bitmap_part(tmpbuf[0],0,0,image_width/2,
                           (LCD_WIDTH-1-image_width/2)/2,
                           LCD_HEIGHT-50-(image_height/2),
                           output_width/2,output_height/2);
#else
  gray_ub_gray_bitmap_part(tmpbuf[0],0,0,image_height/2,
                           LCD_HEIGHT-50-(image_height/2),
                           (LCD_WIDTH-1-image_width/2)/2,
                           output_height/2,output_width/2);
#endif
#endif
}

void vo_setup(const mpeg2_sequence_t * sequence)
{
    image_width=sequence->width;
    image_height=sequence->height;

    tmpbufa = (uint8_t*)mpeg2_malloc(sizeof(uint8_t)*image_width*
                                     image_height/4, -2);
    tmpbufb = (uint8_t*)mpeg2_malloc(sizeof(uint8_t)*image_width*
                                     image_height/16, -2);
    tmpbufc = (uint8_t*)mpeg2_malloc(sizeof(uint8_t)*image_width*
                                     image_height/16, -2);
    tmpbuf[0] = tmpbufa;
    tmpbuf[1] = tmpbufb;
    tmpbuf[2] = tmpbufc;

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

void vo_cleanup(void)
{
  if (tmpbufc)
    mpeg2_free(tmpbufc);
  if (tmpbufb)
    mpeg2_free(tmpbufb);
  if (tmpbufa)
    mpeg2_free(tmpbufa);
}
