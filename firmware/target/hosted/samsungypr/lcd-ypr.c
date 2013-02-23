/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "lcd-target.h"

static int dev_fd = 0;
fb_data *dev_fb = 0;

/* FIXME: never used! Place this into final teardown */
void lcd_shutdown(void)
{
    printf("FB closed.");
    munmap(dev_fb, FRAMEBUFFER_SIZE);
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
    if (screensize != FRAMEBUFFER_SIZE)
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

/*TODO: implemented in a better way -> ie using LCD_ENABLE */

/* Turn off display
 * This translates - in most of the cases - in power supply off
 */
void _backlight_lcd_sleep(void)
{
    int fp = open("/sys/class/graphics/fb0/blank", O_RDWR);
    write(fp, "1", 1);
    close(fp);
}
/* Turn on LCD screen
 * This translates - in most of the cases - in power supply on
 */
void _backlight_lcd_wake(void)
{
    int fp = open("/sys/class/graphics/fb0/blank", O_RDWR);
    write(fp, "0", 1);
    close(fp);
}
