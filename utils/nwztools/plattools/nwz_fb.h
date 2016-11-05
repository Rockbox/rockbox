/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#ifndef __NWZ_FB_H__
#define __NWZ_FB_H__

#define NWZ_FB_LCD_DEV  "/dev/fb/0"
#define NWZ_FB_TV_DEV   "/dev/fb/1"

#define NWZ_FB_TYPE 'N'

/* How backlight works:
 *
 * The brightness interface is a bit strange. There 6 levels: 0 throught 5.
 * Level 0 means backlight off. When changing brightness, one sets level to the
 * target brightness. The driver is gradually change the brightness to reach the
 * target level. The step parameters control how many hardware steps will be done.
 * For example, setting step to 1 will brutally change the level in one step.
 * Setting step to 2 will change brightness in two steps: one intermediate and
 * finally the target one. The more steps, the more gradual the transition. The
 * period parameters controls the speed to changes between steps. Using this
 * interface, one can achieve fade in/out at various speeds. */
#define NWZ_FB_BL_MIN_LEVEL     0
#define NWZ_FB_BL_MAX_LEVEL     5
#define NWZ_FB_BL_MIN_STEP      1
#define NWZ_FB_BL_MAX_STEP      100
#define NWZ_FB_BL_MIN_PERIOD    10

struct nwz_fb_brightness
{
    int level; /* brightness level: 0-5 */
    int step; /* number of hardware steps to do when changing: 1-100 */
    int period; /* period in ms between steps when changing: >=10 */
};

/* FB extensions:
 *
 * Sony added relatively complicated extensions to the framebuffer. They allow
 * better control of framebuffer refresh, double-buffering and mixing with DSP
 * (v4l2). Each outout (LCD and TV) has two buffers, called page 0 and 1 (or A
 * and B). Each page has its own attributes (image info) that control
 * transparency, rotation and updates. At any point in time, the LCD is drawing
 * a page and one can select the next page to draw. Unless an UPDATE ioctl()
 * is made to change it, the next page will be the same as the one being drawn.
 *
 * FIXME I don't know what the timer is, it seems irrelevant for the LCD but
 * the OF uses it for TV, maybe this controls the refresh rate of the TV output?
 *
 * On a side note, this information only applies to a subset of LCD types (the
 * LCD type can be gathered from icx_sysinfo):
 * - BB(0): AQUILA BB LCD
 * - SW(1): SWAN or FIJI LCD
 * - FC(2): FALCON OLED
 * - GM(3): GUAM and ROTA LCD
 * - FR(5): FURANO LCD ---> /!\ DOES NOT APPLY /!\
 * - SD(6): SPICA_D LCD
 * - AQ(7): AQUILA LCD
 */

/* Image infomation:
 * SET_MODE will change the attributes of the requested page (ie .page)
 * GET_MODE will return the attributes of the currently being displayed page
 * UPDATE will do the same thing as SET_MODE but immediately refreshes the screen */
struct nwz_fb_image_info
{
    int tc_enable; /* enable(1)/disable(0) transparent color */
    int t_color; /* transparent color (16bpp RGB565) */
    int alpha; /* alpha ratio (0 - 255) */
    int page; /* 2D framebuffer page(0/1) */
    int rot; /* LCD image rotation(0/1=180deg.) */
    int update; /* only use with NWZ_FB_UPDATE, ignored for others */
};

/* update type */
#define NWZ_FB_ONLY_2D_MODE     0
#define NWZ_FB_DSP_AND_2D_MODE  1

/* frame buffer page infomation: when NWZ_FB_WAIT_REFREHS is called, the driver
 * will wait until the next refresh or the timeout, whichever comes first. It
 * will then fill this structure with the page status. */
struct nwz_fb_status
{
    int timeout; /* waiting time for any frame ready (in units of 10 ms) */
    int page0; /* page 0 is out of display or waiting to be displayed */
    int page1; /* page 0 is out of display or waiting to be displayed */
};

/* frame buffer page status */
#define NWZ_FB_OUT_OF_DISPLAY           0
#define NWZ_FB_WAITING_FOR_ON_DISPLAY   1

/* frame buffer update timer infomation (use I/F fb <-> 2D API) */
struct nwz_fb_update_timer
{
    int timerflag; /* auto update off(0) / auto update on(1) */
    int timeout; /* timeout timer value (ms) */
};

/* timer flags */
#define NWZ_FB_TIMER_ON     1
#define NWZ_FB_TIMER_OFF    0

/* default and minimum timeout value */
#define NWZ_FB_DEFAULT_TIMEOUT  60
#define NWZ_FB_MIN_TIMEOUT      33

/* mmap offsets for page 1 (page 0 is always at address 0) */
#define NWZ_FB_LCD_PAGE_OFFSET  0x2f000

/* NOTE: I renamed those from Sony's header, because their original names were
 * pure crap */
/* FIXME: Sony uses _IOR for NWZ_FB_WAIT_REFRESH but it should be _IORW */
#define NWZ_FB_WAIT_REFRESH     _IORW(NWZ_FB_TYPE, 0x00, struct nwz_fb_status)
#define NWZ_FB_UPDATE           _IOW(NWZ_FB_TYPE, 0x01, struct nwz_fb_image_info)
#define NWZ_FB_SET_MODE         _IOW(NWZ_FB_TYPE, 0x02, struct nwz_fb_image_info)
#define NWZ_FB_GET_MODE         _IOR(NWZ_FB_TYPE, 0x03, struct nwz_fb_image_info)
#define NWZ_FB_UPDATE_TIMER     _IOR(NWZ_FB_TYPE, 0x04, struct nwz_fb_update_timer)
#define NWZ_FB_SET_BRIGHTNESS   _IOW(NWZ_FB_TYPE, 0x07, struct nwz_fb_brightness)
#define NWZ_FB_GET_BRIGHTNESS   _IOR(NWZ_FB_TYPE, 0x08, struct nwz_fb_brightness)


#endif /* __NWZ_FB_H__ */
