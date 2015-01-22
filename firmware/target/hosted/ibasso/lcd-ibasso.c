/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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


#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "config.h"
#include "debug.h"
#include "events.h"
#include "panic.h"

#include "debug-ibasso.h"
#include "lcd-target.h"
#include "sysfs-ibasso.h"


fb_data *dev_fb = 0;


/* Framebuffer device handle. */
static int dev_fd = 0;


void lcd_init_device(void)
{
    TRACE;

    dev_fd = open("/dev/graphics/fb0", O_RDWR);
    if(dev_fd == -1)
    {
        DEBUGF("ERROR %s: open failed on /dev/graphics/fb0, errno: %d.", __func__, errno);
        exit(errno);
    }

    /* Get the changeable information. */
    struct fb_var_screeninfo vinfo;
    if(ioctl(dev_fd, FBIOGET_VSCREENINFO, &vinfo) < 0)
    {
        DEBUGF("ERROR %s: ioctl FBIOGET_VSCREENINFO failed on /dev/graphics/fb0, errno: %d.", __func__, errno);
        exit(errno);
    }

    DEBUGF("DEBUG %s: bits_per_pixel: %u, width: %u, height: %u.", __func__, vinfo.bits_per_pixel, vinfo.width, vinfo.height);

    /*
        Framebuffer does not fit the screen, a bug of iBassos Firmware, not Rockbox.
        Cannot be solved with parameters.
    */
    /*vinfo.bits_per_pixel = LCD_DEPTH;
    vinfo.xres           = LCD_WIDTH;
    vinfo.xres_virtual   = LCD_WIDTH;
    vinfo.width          = LCD_WIDTH;
    vinfo.yres           = LCD_HEIGHT;
    vinfo.yres_virtual   = LCD_HEIGHT;
    vinfo.height         = LCD_HEIGHT;
    vinfo.activate       = FB_ACTIVATE_NOW;
    if(ioctl(dev_fd, FBIOPUT_VSCREENINFO, &vinfo) == -1)
    {
        DEBUGF("ERROR %s: ioctl FBIOPUT_VSCREENINFO failed on /dev/graphics/fb0, errno: %d.", __func__, errno);
        exit(EXIT_FAILURE);
    }*/


    /* Sanity check: Does framebuffer config match Rockbox config? */
    size_t screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    if(screensize != FRAMEBUFFER_SIZE)
    {
        DEBUGF("ERROR %s: Screen size does not match config: %d != %d.", __func__, screensize, FRAMEBUFFER_SIZE);
        exit(EXIT_FAILURE);
    }

    /* Map the device to memory. */
    dev_fb = mmap(0, FRAMEBUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, 0);
    if(dev_fb == MAP_FAILED)
    {
        DEBUGF("ERROR %s: mmap failed on /dev/graphics/fb0, errno: %d.", __func__, errno);
        exit(errno);
    }

    /* Activate Rockbox LCD. */
    lcd_enable(true);
}


void lcd_shutdown(void)
{
    TRACE;

    lcd_set_active(false);
    munmap(dev_fb, FRAMEBUFFER_SIZE);
    close(dev_fd) ;
}


/*
    Left as reference. Unblanking does not work as expected, will not enable LCD after a few
    seconds of power down.
    Instead the backlight power is toggled.
*/
/*void lcd_power_on(void)
{
    TRACE;

    if(ioctl(dev_fd, FBIOBLANK, VESA_NO_BLANKING) == -1)
    {
        DEBUGF("ERROR %s: ioctl FBIOBLANK failed on /dev/graphics/fb0, errno: %d.", __func__, errno);
        panicf("ERROR %s: ioctl FBIOBLANK failed on /dev/graphics/fb0, errno: %d.", __func__, errno);
        return;
    }

    lcd_set_active(true);
    send_event(LCD_EVENT_ACTIVATION, NULL);
}


void lcd_power_off(void)
{
    TRACE;

    lcd_set_active(false);

    if(ioctl(dev_fd, FBIOBLANK, VESA_POWERDOWN) == -1)
    {
        DEBUGF("ERROR %s: ioctl FBIOBLANK failed on /dev/graphics/fb0, errno: %d.", __func__, errno);
        panicf("ERROR %s: ioctl FBIOBLANK failed on /dev/graphics/fb0, errno: %d.", __func__, errno);
        return;
    }
}*/


void lcd_enable(bool on)
{
    TRACE;

    lcd_set_active(on);

    if(on)
    {
        /*
            /sys/power/state
            on: Cancel suspend.
        */
        if(! sysfs_set_string(SYSFS_POWER_STATE, "on"))
        {
            DEBUGF("ERROR %s: Can not set power state.", __func__);
        }

        send_event(LCD_EVENT_ACTIVATION, NULL);
    }
}


void lcd_sleep(void)
{
    TRACE;

    /*
        See system_init(). Without suspend blocker und mute prevention this will interrupt playback.
        Essentially, we are turning off the touch screen.
        /sys/power/state
        mem: Suspend to RAM.
    */
    if(! sysfs_set_string(SYSFS_POWER_STATE, "mem"))
    {
        DEBUGF("ERROR %s: Can not set power state.", __func__);
    }
}
