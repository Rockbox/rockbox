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
#include "backlight-target.h"

static int fb_fd = 0;
fb_data *nwz_framebuffer = 0; /* global variable, see lcd-target.h */

/* select which page (0 or 1) to display, disable DSP, transparency and rotation */
static int nwz_fb_set_page(int page)
{
    /* set page mode to no transparency and no rotation */
    struct nwz_fb_image_info mode_info;
    mode_info.tc_enable = 0;
    mode_info.t_color = 0;
    mode_info.alpha = 0;
    mode_info.rot = 0;
    mode_info.page = page;
    mode_info.update = NWZ_FB_ONLY_2D_MODE;
    if(ioctl(fb_fd, NWZ_FB_UPDATE, &mode_info) < 0)
        return -1;
    return 0;
}

/* make sure framebuffer is in standard state so rendering works */
static int nwz_fb_set_standard_mode(void)
{
    /* disable timer (apparently useless with LCD) */
    struct nwz_fb_update_timer update_timer;
    update_timer.timerflag = NWZ_FB_TIMER_OFF;
    update_timer.timeout = NWZ_FB_DEFAULT_TIMEOUT;
    if(ioctl(fb_fd, NWZ_FB_UPDATE_TIMER, &update_timer) < 0)
        return -1;
    return nwz_fb_set_page(0);
}

void backlight_hw_brightness(int brightness)
{
    struct nwz_fb_brightness bl;
    bl.level = brightness; /* brightness level: 0-5 */
    bl.step = 25; /* number of hardware steps to do when changing: 1-100 (smooth transition) */
    bl.period = NWZ_FB_BL_MIN_PERIOD; /* period in ms between steps when changing: >=10 */

    ioctl(fb_fd, NWZ_FB_SET_BRIGHTNESS, &bl);
}

bool backlight_hw_init(void)
{
    backlight_hw_brightness(DEFAULT_BRIGHTNESS_SETTING);
    return true;
}

void backlight_hw_on(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd + visible display */
#endif
    /* don't do anything special, the core will set the brightness */
}

void backlight_hw_off(void)
{
    /* there is no real on/off but we can set to 0 brightness */
    backlight_hw_brightness(0);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false); /* power off visible display */
#endif
}

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
    /* make sure rendering state is correct */
    nwz_fb_set_standard_mode();
}
