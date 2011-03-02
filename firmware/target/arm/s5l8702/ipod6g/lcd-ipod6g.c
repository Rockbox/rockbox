/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: lcd-nano2g.c 28868 2010-12-21 06:59:17Z Buschel $
 *
 * Copyright (C) 2009 by Dave Chapman
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
#include "config.h"

#include "hwcompat.h"
#include "kernel.h"
#include "lcd.h"
#include "system.h"
#include "cpu.h"
#include "pmu-target.h"
#include "power.h"
#include "string.h"


#define R_HORIZ_GRAM_ADDR_SET     0x200
#define R_VERT_GRAM_ADDR_SET      0x201
#define R_WRITE_DATA_TO_GRAM      0x202
#define R_HORIZ_ADDR_START_POS    0x210
#define R_HORIZ_ADDR_END_POS      0x211
#define R_VERT_ADDR_START_POS     0x212
#define R_VERT_ADDR_END_POS       0x213


/* LCD type 1 register defines */

#define R_COLUMN_ADDR_SET         0x2a
#define R_ROW_ADDR_SET            0x2b
#define R_MEMORY_WRITE            0x2c


/** globals **/

int lcd_type; /* also needed in debug-s5l8702.c */
static struct dma_lli lcd_lli[(LCD_WIDTH * LCD_HEIGHT - 1) / 0xfff] CACHEALIGN_ATTR;
static struct semaphore lcd_wakeup;
static struct mutex lcd_mutex;
static uint16_t lcd_dblbuf[LCD_HEIGHT][LCD_WIDTH];


static inline void s5l_lcd_write_cmd_data(int cmd, int data)
{
    while (LCD_STATUS & 0x10);
    LCD_WCMD = cmd;

    while (LCD_STATUS & 0x10);
    LCD_WDATA = (data & 0xff) | ((data & 0x7f00) << 1);
}

static inline void s5l_lcd_write_cmd(unsigned short cmd)
{
    while (LCD_STATUS & 0x10);
    LCD_WCMD = cmd;
}

static inline void s5l_lcd_write_data(unsigned short data)
{
    while (LCD_STATUS & 0x10);
    LCD_WDATA = data;
}

/*** hardware configuration ***/

int lcd_default_contrast(void)
{
    return 0x1f;
}

void lcd_set_contrast(int val)
{
    (void)val;
}

void lcd_set_invert_display(bool yesno)
{
    (void)yesno;
}

void lcd_set_flip(bool yesno)
{
    (void)yesno;
}

bool lcd_active(void)
{
    return true;
}

void lcd_shutdown(void)
{
    mutex_lock(&lcd_mutex);
    pmu_write(0x2b, 0);  /* Kill the backlight, instantly. */
    pmu_write(0x29, 0);

    if (lcd_type & 2)
    {
        s5l_lcd_write_cmd_data(0x7, 0x172);
        s5l_lcd_write_cmd_data(0x30, 0x3ff);
        sleep(HZ / 10);
        s5l_lcd_write_cmd_data(0x7, 0x120);
        s5l_lcd_write_cmd_data(0x30, 0x0);
        s5l_lcd_write_cmd_data(0x100, 0x780);
        s5l_lcd_write_cmd_data(0x7, 0x0);
        s5l_lcd_write_cmd_data(0x101, 0x260);
        s5l_lcd_write_cmd_data(0x102, 0xa9);
        sleep(HZ / 30);
        s5l_lcd_write_cmd_data(0x100, 0x700);
        s5l_lcd_write_cmd_data(0x100, 0x704);
    }
    else if (lcd_type == 1)
    {
        s5l_lcd_write_cmd(0x28);
        s5l_lcd_write_cmd(0x10);
        sleep(HZ / 10);
    }
    else
    {
        s5l_lcd_write_cmd(0x28);
        sleep(HZ / 20);
        s5l_lcd_write_cmd(0x10);
        sleep(HZ / 20);
    }
    mutex_unlock(&lcd_mutex);
}

void lcd_sleep(void)
{
    lcd_shutdown();
}

/* LCD init */
void lcd_init_device(void)
{
    /* Detect lcd type */
    semaphore_init(&lcd_wakeup, 1, 0);
    mutex_init(&lcd_mutex);
    lcd_type = (PDAT6 & 0x30) >> 4;
}

/*** Update functions ***/

static inline void lcd_write_pixel(fb_data pixel)
{
    mutex_lock(&lcd_mutex);
    LCD_WDATA = pixel;
    mutex_unlock(&lcd_mutex);
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

/* Line write helper function. */
extern void lcd_write_line(const fb_data *addr, 
                           int pixelcount,
                           const unsigned int lcd_base_addr);

static void displaylcd_setup(int x, int y, int width, int height) ICODE_ATTR;
static void displaylcd_setup(int x, int y, int width, int height)
{
    mutex_lock(&lcd_mutex);
    while (DMAC0C4CONFIG & 1) semaphore_wait(&lcd_wakeup, HZ / 10);

    int xe = (x + width) - 1;           /* max horiz */
    int ye = (y + height) - 1;          /* max vert */

    if (lcd_type & 2) {
        s5l_lcd_write_cmd_data(R_HORIZ_ADDR_START_POS, x);
        s5l_lcd_write_cmd_data(R_HORIZ_ADDR_END_POS,   xe);
        s5l_lcd_write_cmd_data(R_VERT_ADDR_START_POS,  y);
        s5l_lcd_write_cmd_data(R_VERT_ADDR_END_POS,    ye);

        s5l_lcd_write_cmd_data(R_HORIZ_GRAM_ADDR_SET,  x);
        s5l_lcd_write_cmd_data(R_VERT_GRAM_ADDR_SET,   y);

        s5l_lcd_write_cmd(R_WRITE_DATA_TO_GRAM);
    } else {
        s5l_lcd_write_cmd(R_COLUMN_ADDR_SET);
        s5l_lcd_write_data(x >> 8);
        s5l_lcd_write_data(x & 0xff);
        s5l_lcd_write_data(xe >> 8);
        s5l_lcd_write_data(xe & 0xff);

        s5l_lcd_write_cmd(R_ROW_ADDR_SET);
        s5l_lcd_write_data(y >> 8);
        s5l_lcd_write_data(y & 0xff);
        s5l_lcd_write_data(ye >> 8);
        s5l_lcd_write_data(ye & 0xff);

        s5l_lcd_write_cmd(R_MEMORY_WRITE);
    }
}

static void displaylcd_dma(int pixels) ICODE_ATTR;
static void displaylcd_dma(int pixels)
{
    int i;
    void* data = lcd_dblbuf;
    for (i = -1; i < (int)ARRAYLEN(lcd_lli) && pixels > 0; i++, pixels -= 0xfff)
    {
        bool last = i + 1 >= (int)ARRAYLEN(lcd_lli) || pixels <= 0xfff;
        struct dma_lli* lli = i < 0 ? (struct dma_lli*)((int)&DMAC0C4LLI) : &lcd_lli[i];
        lli->srcaddr = data;
        lli->dstaddr = (void*)((int)&LCD_WDATA);
        lli->nextlli = last ? NULL : &lcd_lli[i + 1];
        lli->control = 0x70240000 | (last ? pixels : 0xfff)
                     | (last ? 0x80000000 : 0) | 0x4000000;
        data += 0x1ffe;
    }
    clean_dcache();
    DMAC0C4CONFIG = 0x88c1;
    mutex_unlock(&lcd_mutex);
}

void INT_DMAC0C4(void) ICODE_ATTR;
void INT_DMAC0C4(void)
{
    DMAC0INTTCCLR = 0x10;
    semaphore_release(&lcd_wakeup);
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    int pixels = width * height;
    fb_data* p = &lcd_framebuffer[y][x];
    uint16_t* out = lcd_dblbuf[0];
    
    displaylcd_setup(x, y, width, height);

    /* Copy display bitmap to hardware */
    if (LCD_WIDTH == width) {
        /* Write all lines at once */
        memcpy(out, p, pixels * 2);
    } else {
        do {
            /* Write a single line */
            memcpy(out, p, width * 2);
            p += LCD_WIDTH;
            out += width;
        } while (--height);
    }

    displaylcd_dma(pixels);
}

/* Line write helper function for lcd_yuv_blit. Writes two lines of yuv420. */
extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
                                   uint16_t* outbuf,
                                   int width,
                                   int stride);

/* Blit a YUV bitmap directly to the LCD */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height) ICODE_ATTR;
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    unsigned int z;
    unsigned char const * yuv_src[3];
    
    width = (width + 1) & ~1;       /* ensure width is even */

    int pixels = width * height;
    uint16_t* out = lcd_dblbuf[0];

    z = stride * src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    displaylcd_setup(x, y, width, height);

    height >>= 1;

    do {
        lcd_write_yuv420_lines(yuv_src, out, width, stride);
        yuv_src[0] += stride << 1;
        yuv_src[1] += stride >> 1; /* Skip down one chroma line */
        yuv_src[2] += stride >> 1;
        out += width << 1;
    } while (--height);

    displaylcd_dma(pixels);
}
