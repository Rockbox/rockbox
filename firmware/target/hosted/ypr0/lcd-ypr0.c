/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: lcd-bitmap.c 29248 2011-02-08 20:05:25Z thomasjfox $
 *
 * Copyright (C) 2011 Lorenzo Miori, Thomas Martitz
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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "string.h"
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "file.h"
#include "debug.h"
#include "system.h"
#include "screendump.h"
#include "lcd.h"

/* eqivalent to fb + y*width + x */
#define LCDADDR(x, y) (&lcd_framebuffer[(y)][(x)])

static int dev_fd = 0;
static fb_data *dev_fb = 0;

void lcd_update(void)
{
    /* update the entire display */
    memcpy(dev_fb, lcd_framebuffer, sizeof(lcd_framebuffer));
}

/* Copy Rockbox frame buffer to the mmapped lcd device */
void lcd_update_rect(int x, int y, int width, int height)
{
    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= LCD_WIDTH) ||
            (y >= LCD_HEIGHT) || (x + width <= 0) || (y + height <= 0))
        return;

    /* do the necessary clipping */
    if (x < 0)
    {   /* clip left */
        width += x;
        x = 0;
    }
    if (y < 0)
    {   /* clip top */
        height += y;
        y = 0;
    }
    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x; /* clip right */
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y; /* clip bottom */

    fb_data* src  = LCDADDR(x, y);
    fb_data* dst = dev_fb + y*LCD_WIDTH + x;

    if (LCD_WIDTH == width)
    {   /* optimized full-width update */
        memcpy(dst, src, width * height * sizeof(fb_data));
    }
    else
    {   /* row by row */
        do
        {
            memcpy(dst, src, width * sizeof(fb_data));
            src += LCD_WIDTH;
            dst += LCD_WIDTH;
        } while(--height > 0);
    }
}

void lcd_shutdown(void)
{
    printf("FB closed.");
    munmap(dev_fb, sizeof(lcd_framebuffer));
    close(dev_fd);
}

void lcd_init_device(void)
{
    size_t screensize;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    /* Open the framebuffer device */
    dev_fd = open("/dev/fb0", O_RDWR);
    if (dev_fd == -1) {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }
    printf("The framebuffer device was opened successfully.\n");

    /* Get the fixed properties */
    if (ioctl(dev_fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("Error reading fixed information");
        exit(2);
    }

    /* Now we get the settable settings, and we set 16 bit bpp */
    if (ioctl(dev_fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("Error reading variable information");
        exit(3);
    }

    vinfo.bits_per_pixel = 16;

    if (ioctl(dev_fd, FBIOPUT_VSCREENINFO, &vinfo)) {
            perror("fbset(ioctl)");
            exit(4);
    }

    printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    /* Figure out the size of the screen in bytes */
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    if (screensize != sizeof(lcd_framebuffer))
    {
        exit(4);
        perror("Display and framebuffer mismatch!\n");
    }

    /* Map the device to memory */
    dev_fb = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, 0);
    if ((int)dev_fb == -1) {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }
    printf("The framebuffer device was mapped to memory successfully.\n");
}
