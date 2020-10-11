/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 Marcin Bukat
 * Copyright (C) 2016 Amaury Pouly
 * Copyright (C) 2019 Roman Stolyarov
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
#include <string.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "lcd.h"
#include "lcd-target.h"
#include "backlight-target.h"
#include "sysfs.h"
#include "panic.h"

static int fd = -1;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
fb_data *framebuffer = NULL; /* global variable, see lcd-target.h */

static void redraw(void)
{
    ioctl(fd, FBIOPAN_DISPLAY, &vinfo);
}

void lcd_init_device(void)
{
    const char * const fb_dev = "/dev/fb0";
    fd = open(fb_dev, O_RDWR | O_CLOEXEC);
    if(fd < 0)
    {
        panicf("Cannot open framebuffer: %s\n", fb_dev);
    }

    /* get fixed and variable information */
    if(ioctl(fd, FBIOGET_FSCREENINFO, &finfo) < 0)
    {
        panicf("Cannot read framebuffer fixed information");
    }

#if 0
    /* check resolution and framebuffer size */
    if(vinfo.xres != LCD_WIDTH || vinfo.yres != LCD_HEIGHT || vinfo.bits_per_pixel != LCD_DEPTH)
    {
        panicf("Unexpected framebuffer resolution: %dx%dx%d\n", vinfo.xres,
            vinfo.yres, vinfo.bits_per_pixel);
    }
#endif
    /* Note: we use a framebuffer size of width*height*bbp. We cannot trust the
     * values returned by the driver for line_length */

    /* map framebuffer */
    framebuffer = mmap(NULL, FRAMEBUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if((void *)framebuffer == MAP_FAILED)
    {
        panicf("Cannot map framebuffer");
    }

    if(ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0)
    {
        panicf("Cannot read framebuffer variable information");
    }

    memset(framebuffer, 0, finfo.smem_len);

#ifdef HAVE_LCD_ENABLE
    lcd_set_active(true);
#endif
}

#ifdef HAVE_LCD_SHUTDOWN
void lcd_shutdown(void)
{
    munmap(framebuffer, FRAMEBUFFER_SIZE);
    framebuffer = NULL;
    close(fd);
    fd = -1;
}
#endif

#ifdef HAVE_LCD_ENABLE
void lcd_enable(bool on)
{
    if (fd < 0) return;

    if (lcd_active() == on)
        return;

    lcd_set_active(on);

    if (on)
    {
        send_event(LCD_EVENT_ACTIVATION, NULL);
        ioctl(fd, FB_BLANK_UNBLANK);
    }
    else
    {
        memset(framebuffer, 0, finfo.smem_len);
        redraw();
        ioctl(fd, FB_BLANK_POWERDOWN);
    }
}
#endif

extern void lcd_copy_buffer_rect(fb_data *dst, const fb_data *src,
                                 int width, int height);

void lcd_update(void)
{
    if (fd < 0) return;

#ifdef HAVE_LCD_ENABLE
    if (lcd_active())
#endif
    {
        /* Copy the Rockbox framebuffer to the second framebuffer */
        lcd_copy_buffer_rect(LCD_FRAMEBUF_ADDR(0, 0), FBADDR(0,0),
                             LCD_WIDTH*LCD_HEIGHT, 1);
        redraw();
    }
}

void lcd_update_rect(int x, int y, int width, int height)
{
    if (fd < 0) return;

#ifdef HAVE_LCD_ENABLE
    if (lcd_active())
#endif
    {
        fb_data *dst = LCD_FRAMEBUF_ADDR(x, y);
        fb_data * src = FBADDR(x,y);

        /* Copy part of the Rockbox framebuffer to the second framebuffer */
        if (width < LCD_WIDTH)
        {
            /* Not full width - do line-by-line */
            lcd_copy_buffer_rect(dst, src, width, height);
        }
        else
        {
            /* Full width - copy as one line */
            lcd_copy_buffer_rect(dst, src, LCD_WIDTH*height, 1);
        }
        redraw();
    }
}
