/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Additional LCD routines not present in the rockbox core
* Scrolling functions
*
* Copyright (C) 2005 Jens Arnold
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

#include "plugin.h"

#include "xlcd.h"

#if (LCD_DEPTH == 2) && (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)
static const unsigned short patterns[4] = {0xFFFF, 0xFF00, 0x00FF, 0x0000};
#endif

#if   defined(LCD_STRIDEFORMAT) && LCD_STRIDEFORMAT == VERTICAL_STRIDE
void xlcd_scroll_left(int count)
{
    /*size_t dst_stride;*/
    /*struct viewport *vp_main = NULL;*/
    fb_data *lcd_fb = get_framebuffer(NULL, NULL);


    int length, oldmode;

    if ((unsigned)count >= LCD_WIDTH)
    {
        rb->lcd_clear_display();
        return;
    }

    length = (LCD_WIDTH-count)*LCD_FBHEIGHT;

    rb->memmove(lcd_fb, lcd_fb + LCD_HEIGHT*count, length * sizeof(fb_data));

    oldmode = rb->lcd_get_drawmode();
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(LCD_WIDTH-count, 0, count, LCD_HEIGHT);
    rb->lcd_set_drawmode(oldmode);
}

/* Scroll right */
void xlcd_scroll_right(int count)
{
    /*size_t dst_stride;*/
    /*struct viewport *vp_main = NULL;*/
    fb_data *lcd_fb = get_framebuffer(NULL, NULL);


    int length, oldmode;

    if ((unsigned)count >= LCD_WIDTH)
    {
        rb->lcd_clear_display();
        return;
    }

    length = (LCD_WIDTH-count)*LCD_FBHEIGHT;

    rb->memmove(lcd_fb + LCD_HEIGHT*count,
                lcd_fb, length * sizeof(fb_data));

    oldmode = rb->lcd_get_drawmode();
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(0, 0, count, LCD_HEIGHT);
    rb->lcd_set_drawmode(oldmode);
}

/* Scroll up */
void xlcd_scroll_up(int count)
{
    /*size_t dst_stride;*/
    /*struct viewport *vp_main = NULL;*/
    fb_data *lcd_fb = get_framebuffer(NULL, NULL);


    int width, length, oldmode;

    fb_data *data;

    if ((unsigned)count >= LCD_HEIGHT)
    {
        rb->lcd_clear_display();
        return;
    }

    length = LCD_HEIGHT - count;

    width = LCD_WIDTH-1;
    data = lcd_fb;

    do {
        rb->memmove(data,data + count,length * sizeof(fb_data));
        data += LCD_HEIGHT;
    } while(width--);

    oldmode = rb->lcd_get_drawmode();
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(0, length, LCD_WIDTH, count);
    rb->lcd_set_drawmode(oldmode);
}

/* Scroll down */
void xlcd_scroll_down(int count)
{
    /*size_t dst_stride;*/
    /*struct viewport *vp_main = NULL;*/
    fb_data *lcd_fb = get_framebuffer(NULL, NULL);


    int width, length, oldmode;

    fb_data *data;

    if ((unsigned)count >= LCD_HEIGHT)
    {
        rb->lcd_clear_display();
        return;
    }

    length = LCD_HEIGHT - count;

    width = LCD_WIDTH-1;
    data = lcd_fb;

    do {
        rb->memmove(data + count, data, length * sizeof(fb_data));
        data += LCD_HEIGHT;
    } while(width--);

    oldmode = rb->lcd_get_drawmode();
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(0, 0, LCD_WIDTH, count);
    rb->lcd_set_drawmode(oldmode);
}
#else

#if (LCD_PIXELFORMAT == HORIZONTAL_PACKING) && (LCD_DEPTH < 8)

/* Scroll left */
void xlcd_scroll_left(int count)
{
    /*size_t dst_stride;*/
    /*struct viewport *vp_main = NULL;*/
    fb_data *lcd_fb = get_framebuffer(NULL, NULL);


    int bitcount=0, oldmode;
    int blockcount=0, blocklen;

    if ((unsigned) count >= LCD_WIDTH)
    {
        rb->lcd_clear_display();
        return;
    }

#if LCD_DEPTH == 2
    blockcount = count >> 2;
    blocklen = LCD_FBWIDTH - blockcount;
    bitcount = 2 * (count & 3);
#endif

    if (blockcount)
    {
        unsigned char *data = lcd_fb;
        unsigned char *data_end = data + LCD_FBWIDTH*LCD_HEIGHT;

        do
        {
            rb->memmove(data, data + blockcount, blocklen);
            data += LCD_FBWIDTH;
        }
        while (data < data_end);
    }
    if (bitcount)
    {
        int bx, y;
        unsigned char *addr = lcd_fb + blocklen;
#if LCD_DEPTH == 2
        unsigned fill = (0x55 * (~rb->lcd_get_background() & 3)) << bitcount;
#endif

        for (y = 0; y < LCD_HEIGHT; y++)
        {
            unsigned char *row_addr = addr;
            unsigned data = fill;

            for (bx = 0; bx < blocklen; bx++)
            {
                --row_addr;
                data = (data >> 8) | (*row_addr << bitcount);
                *row_addr = data;
            }
            addr += LCD_FBWIDTH;
        }
    }
    oldmode = rb->lcd_get_drawmode();
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(LCD_WIDTH - count, 0, count, LCD_HEIGHT);
    rb->lcd_set_drawmode(oldmode);
}

/* Scroll right */
void xlcd_scroll_right(int count)
{
    /*size_t dst_stride;*/
    /*struct viewport *vp_main = NULL;*/
    fb_data *lcd_fb = get_framebuffer(NULL, NULL);


    int bitcount=0, oldmode;
    int blockcount=0, blocklen;

    if ((unsigned) count >= LCD_WIDTH)
    {
        rb->lcd_clear_display();
        return;
    }

#if LCD_DEPTH == 2
    blockcount = count >> 2;
    blocklen = LCD_FBWIDTH - blockcount;
    bitcount = 2 * (count & 3);
#endif

    if (blockcount)
    {
        unsigned char *data = lcd_fb;
        unsigned char *data_end = data + LCD_FBWIDTH*LCD_HEIGHT;

        do
        {
            rb->memmove(data + blockcount, data, blocklen);
            data += LCD_FBWIDTH;
        }
        while (data < data_end);
    }
    if (bitcount)
    {
        int bx, y;
        unsigned char *addr = lcd_fb + blockcount;
#if LCD_DEPTH == 2
        unsigned fill = 0x55 * (~rb->lcd_get_background() & 3);
#endif

        for (y = 0; y < LCD_HEIGHT; y++)
        {
            unsigned char *row_addr = addr;
            unsigned data = fill;

            for (bx = 0; bx < blocklen; bx++)
            {
                data = (data << 8) | *row_addr;
                *row_addr = data >> bitcount;
                row_addr++;
            }
            addr += LCD_FBWIDTH;
        }
    }
    oldmode = rb->lcd_get_drawmode();
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(0, 0, count, LCD_HEIGHT);
    rb->lcd_set_drawmode(oldmode);
}

#else /* LCD_PIXELFORMAT vertical packed or >= 8bit / pixel */

/* Scroll left */
void xlcd_scroll_left(int count)
{
    /*size_t dst_stride;*/
    /*struct viewport *vp_main = NULL;*/
    fb_data *lcd_fb = get_framebuffer(NULL, NULL);


    fb_data *data, *data_end;
    int length, oldmode;

    if ((unsigned)count >= LCD_WIDTH)
    {
        rb->lcd_clear_display();
        return;
    }

    data = lcd_fb;
    data_end = data + LCD_WIDTH*LCD_FBHEIGHT;
    length = LCD_WIDTH - count;

    do
    {
        rb->memmove(data, data + count, length * sizeof(fb_data));
        data += LCD_WIDTH;
    }
    while (data < data_end);

    oldmode = rb->lcd_get_drawmode();
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(length, 0, count, LCD_HEIGHT);
    rb->lcd_set_drawmode(oldmode);
}

/* Scroll right */
void xlcd_scroll_right(int count)
{
    /*size_t dst_stride;*/
    /*struct viewport *vp_main = NULL;*/
    fb_data *lcd_fb = get_framebuffer(NULL, NULL);


    fb_data *data, *data_end;
    int length, oldmode;

    if ((unsigned)count >= LCD_WIDTH)
    {
        rb->lcd_clear_display();
        return;
    }

    data = lcd_fb;
    data_end = data + LCD_WIDTH*LCD_FBHEIGHT;
    length = LCD_WIDTH - count;

    do
    {
        rb->memmove(data + count, data, length * sizeof(fb_data));
        data += LCD_WIDTH;
    }
    while (data < data_end);

    oldmode = rb->lcd_get_drawmode();
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(0, 0, count, LCD_HEIGHT);
    rb->lcd_set_drawmode(oldmode);
}

#endif /* LCD_PIXELFORMAT, LCD_DEPTH */

#if (LCD_PIXELFORMAT == HORIZONTAL_PACKING) || (LCD_DEPTH >= 8)

/* Scroll up */
void xlcd_scroll_up(int count)
{
    /*size_t dst_stride;*/
    /*struct viewport *vp_main = NULL;*/
    fb_data *lcd_fb = get_framebuffer(NULL, NULL);

    int length, oldmode;

    if ((unsigned)count >= LCD_HEIGHT)
    {
        rb->lcd_clear_display();
        return;
    }

    length = LCD_HEIGHT - count;

    rb->memmove(lcd_fb,
                      lcd_fb + count * LCD_FBWIDTH,
                      length * LCD_FBWIDTH * sizeof(fb_data));

    oldmode = rb->lcd_get_drawmode();
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(0, length, LCD_WIDTH, count);
    rb->lcd_set_drawmode(oldmode);
}

/* Scroll down */
void xlcd_scroll_down(int count)
{
    /*size_t dst_stride;*/
    /*struct viewport *vp_main = NULL;*/
    fb_data *lcd_fb = get_framebuffer(NULL, NULL);

    int length, oldmode;

    if ((unsigned)count >= LCD_HEIGHT)
    {
        rb->lcd_clear_display();
        return;
    }

    length = LCD_HEIGHT - count;

    rb->memmove(lcd_fb + count * LCD_FBWIDTH,
                      lcd_fb,
                      length * LCD_FBWIDTH * sizeof(fb_data));

    oldmode = rb->lcd_get_drawmode();
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(0, 0, LCD_WIDTH, count);
    rb->lcd_set_drawmode(oldmode);
}

#else /* LCD_PIXELFORMAT == VERTICAL_PACKING,
         LCD_PIXELFORMAT == VERTICAL_INTERLEAVED */

/* Scroll up */
void xlcd_scroll_up(int count)
{
    /*size_t dst_stride;*/
    /*struct viewport *vp_main = NULL;*/
    fb_data *lcd_fb = get_framebuffer(NULL, NULL);

    int bitcount=0, oldmode;
    int blockcount=0, blocklen;

    if ((unsigned) count >= LCD_HEIGHT)
    {
        rb->lcd_clear_display();
        return;
    }

#if (LCD_DEPTH == 1) \
 || (LCD_DEPTH == 2) && (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)
    blockcount = count >> 3;
    bitcount = count & 7;
#elif (LCD_DEPTH == 2) && (LCD_PIXELFORMAT == VERTICAL_PACKING)
    blockcount = count >> 2;
    bitcount = 2 * (count & 3);
#endif
    blocklen = LCD_FBHEIGHT - blockcount;

    if (blockcount)
    {
        rb->memmove(lcd_fb,
                          lcd_fb + blockcount * LCD_FBWIDTH,
                          blocklen * LCD_FBWIDTH * sizeof(fb_data));
    }
    if (bitcount)
    {
#if LCD_PIXELFORMAT == VERTICAL_PACKING

#if defined(CPU_COLDFIRE) && (LCD_DEPTH == 2)
        asm (
            "move.l  %[wide],%%d3\n"  /* columns = width */

        ".su_cloop:              \n"  /* repeat for every column */
            "move.l  %[addr],%%a1\n"  /* get start address */
            "move.l  %[rows],%%d2\n"  /* rows = row_count */
            "move.l  %[bkg],%%d1 \n"  /* fill with background */

        ".su_iloop:              \n"  /* repeat for all rows */
            "sub.l   %[wide],%%a1\n"  /* address -= width */

            "lsl.l   #8,%%d1     \n"  /* old data to 2nd byte */
            "move.b  (%%a1),%%d1 \n"  /* combine with new data byte */
            "move.l  %%d1,%%d0   \n"  /* keep data for next round */
            "lsr.l   %[cnt],%%d0 \n"  /* shift right */
            "move.b  %%d0,(%%a1) \n"  /* store data */

            "subq.l  #1,%%d2     \n"  /* rows-- */
            "bne.b   .su_iloop   \n"

            "addq.l  #1,%[addr]  \n"  /* start_address++ */
            "subq.l  #1,%%d3     \n"  /* columns-- */
            "bne.b   .su_cloop   \n"
            : /* outputs */
            : /* inputs */
            [wide]"r"(LCD_FBWIDTH),
            [rows]"r"(blocklen),
            [addr]"a"(lcd_fb + blocklen * LCD_FBWIDTH),
            [cnt] "d"(bitcount),
            [bkg] "d"(0x55 * (~rb->lcd_get_background() & 3))
            : /* clobbers */
            "a1", "d0", "d1", "d2", "d3"
        );
#else /* C version */
        int x, by;
        unsigned char *addr = lcd_fb + blocklen * LCD_FBWIDTH;
#if LCD_DEPTH == 2
        unsigned fill = 0x55 * (~rb->lcd_get_background() & 3);
#else
        const unsigned fill = 0;
#endif

        for (x = 0; x < LCD_WIDTH; x++)
        {
            unsigned char *col_addr = addr++;
            unsigned data = fill;

            for (by = 0; by < blocklen; by++)
            {
                col_addr -= LCD_FBWIDTH;
                data = (data << 8) | *col_addr;
                *col_addr = data >> bitcount;
            }
        }
#endif /* CPU, LCD_DEPTH */

#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED

#if LCD_DEPTH == 2
        int x, by;
        fb_data *addr = lcd_fb + blocklen * LCD_FBWIDTH;
        unsigned fill, mask;

        fill = patterns[rb->lcd_get_background() & 3] << 8;
        mask = (0xFFu >> bitcount) << bitcount;
        mask |= mask << 8;

        for (x = 0; x < LCD_WIDTH; x++)
        {
            fb_data *col_addr = addr++;
            unsigned olddata = fill;
            unsigned data;

            for (by = 0; by < blocklen; by++)
            {
                col_addr -= LCD_FBWIDTH;
                data = *col_addr;
                *col_addr = (olddata ^ ((data ^ olddata) & mask)) >> bitcount;
                olddata = data << 8;
            }
        }
#endif /* LCD_DEPTH == 2 */

#endif /* LCD_PIXELFORMAT */
    }
    oldmode = rb->lcd_get_drawmode();
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(0, LCD_HEIGHT - count, LCD_WIDTH, count);
    rb->lcd_set_drawmode(oldmode);
}

/* Scroll up */
void xlcd_scroll_down(int count)
{
    /*size_t dst_stride;*/
    /*struct viewport *vp_main = NULL;*/
    fb_data *lcd_fb = get_framebuffer(NULL, NULL);

    int bitcount=0, oldmode;
    int blockcount=0, blocklen;

    if ((unsigned) count >= LCD_HEIGHT)
    {
        rb->lcd_clear_display();
        return;
    }

#if (LCD_DEPTH == 1) \
 || (LCD_DEPTH == 2) && (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)
    blockcount = count >> 3;
    bitcount = count & 7;
#elif (LCD_DEPTH == 2) && (LCD_PIXELFORMAT == VERTICAL_PACKING)
    blockcount = count >> 2;
    bitcount = 2 * (count & 3);
#endif
    blocklen = LCD_FBHEIGHT - blockcount;

    if (blockcount)
    {
        rb->memmove(lcd_fb + blockcount * LCD_FBWIDTH,
                          lcd_fb,
                          blocklen * LCD_FBWIDTH * sizeof(fb_data));
    }
    if (bitcount)
    {
#if LCD_PIXELFORMAT == VERTICAL_PACKING

#if defined(CPU_COLDFIRE) && (LCD_DEPTH == 2)
        asm (
            "move.l  %[wide],%%d3\n"  /* columns = width */

        ".sd_cloop:              \n"  /* repeat for every column */
            "move.l  %[addr],%%a1\n"  /* get start address */
            "move.l  %[rows],%%d2\n"  /* rows = row_count */
            "move.l  %[bkg],%%d1 \n"  /* fill with background */

        ".sd_iloop:              \n"  /* repeat for all rows */
            "lsr.l   #8,%%d1     \n"  /* shift right to get residue */
            "clr.l   %%d0        \n"
            "move.b  (%%a1),%%d0 \n"  /* get data byte */
            "lsl.l   %[cnt],%%d0 \n"
            "or.l    %%d0,%%d1   \n"  /* combine with last residue */
            "move.b  %%d1,(%%a1) \n"  /* store data */

            "add.l   %[wide],%%a1\n"  /* address += width */
            "subq.l  #1,%%d2     \n"  /* rows-- */
            "bne.b   .sd_iloop   \n"

            "lea.l   (1,%[addr]),%[addr] \n"  /* start_address++ */
            "subq.l  #1,%%d3     \n"  /* columns-- */
            "bne.b   .sd_cloop   \n"
            : /* outputs */
            : /* inputs */
            [wide]"r"(LCD_WIDTH),
            [rows]"r"(blocklen),
            [addr]"a"(lcd_fb + blockcount * LCD_FBWIDTH),
            [cnt] "d"(bitcount),
            [bkg] "d"((0x55 * (~rb->lcd_get_background() & 3)) << bitcount)
            : /* clobbers */
            "a1", "d0", "d1", "d2", "d3"
        );
#else /* C version */
        int x, by;
        unsigned char *addr = lcd_fb + blockcount * LCD_FBWIDTH;
#if LCD_DEPTH == 2
        unsigned fill = (0x55 * (~rb->lcd_get_background() & 3)) << bitcount;
#else
        const unsigned fill = 0;
#endif

        for (x = 0; x < LCD_WIDTH; x++)
        {
            unsigned char *col_addr = addr++;
            unsigned data = fill;

            for (by = 0; by < blocklen; by++)
            {
                data = (data >> 8) | (*col_addr << bitcount);
                *col_addr = data;
                col_addr += LCD_FBWIDTH;
            }
        }
#endif /* CPU, LCD_DEPTH */

#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED

#if LCD_DEPTH == 2
        int x, by;
        fb_data *addr = lcd_fb + blockcount * LCD_FBWIDTH;
        unsigned fill, mask;

        fill = patterns[rb->lcd_get_background() & 3] >> (8 - bitcount);
        mask = (0xFFu >> bitcount) << bitcount;
        mask |= mask << 8;

        for (x = 0; x < LCD_WIDTH; x++)
        {
            fb_data *col_addr = addr++;
            unsigned olddata = fill;
            unsigned data;

            for (by = 0; by < blocklen; by++)
            {
                data = *col_addr << bitcount;
                *col_addr = olddata ^ ((data ^ olddata) & mask);
                olddata = data >> 8;
                col_addr += LCD_FBWIDTH;
            }
        }
#endif /* LCD_DEPTH == 2 */

#endif /* LCD_PIXELFORMAT */
    }
    oldmode = rb->lcd_get_drawmode();
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(0, 0, LCD_WIDTH, count);
    rb->lcd_set_drawmode(oldmode);
}

#endif /* LCD_PIXELFORMAT, LCD_DEPTH */
#endif /* defined(LCD_STRIDEFORMAT) && LCD_STRIDEFORMAT == VERTICAL_STRIDE */
