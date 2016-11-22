/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2016 Amaury Pouly
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
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "lcd.h"
#include "lcd-target.h"

static int fb_fd = 0;
fb_data *nwz_framebuffer = 0; /* global variable, see lcd-target.h */

void lcd_shutdown(void)
{
    munmap(nwz_framebuffer, FRAMEBUFFER_SIZE);
    close(fb_fd);
}

void lcd_init_device(void)
{
    fb_fd = open("/dev/fb/0", O_RDWR);
    if(fb_fd < 0)
    {
        perror("Cannot open framebuffer");
        exit(1);
    }
    printf("Framebuffer opened\n");

    /* get fixed and variable information */
    struct fb_fix_screeninfo finfo;
    if(ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) < 0)
    {
        perror("Cannot read framebuffer fixed information");
        exit(2);
    }
    struct fb_var_screeninfo vinfo;
    if(ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0)
    {
        perror("Cannot read framebuffer variable information");
        exit(2);
    }

    /* check resolution and framebuffer size */
    if(vinfo.xres != LCD_WIDTH || vinfo.yres != LCD_HEIGHT || vinfo.bits_per_pixel != LCD_DEPTH)
    {
        printf("Unexpected framebuffer resolution: %dx%d\n", vinfo.xres, vinfo.yres);
        exit(3);
    }
    size_t screensize = finfo.line_length * vinfo.yres_virtual;
    if(screensize != FRAMEBUFFER_SIZE)
    {
        printf("Unexpected framebuffer size: %d (line_length: %d, Y-res-virt: %d)\n",
            screensize, finfo.line_length, vinfo.yres_virtual);
        exit(4);
    }

    /* map framebuffer */
    nwz_framebuffer = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if((void *)nwz_framebuffer == (void *)-1)
    {
        perror("Cannot map framebuffer");
        exit(5);
    }
}
