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

#define NWZ_FB_SET_BRIGHTNESS _IOW(NWZ_FB_TYPE, 0x07, struct nwz_fb_brightness)
#define NWZ_FB_GET_BRIGHTNESS _IOR(NWZ_FB_TYPE, 0x08, struct nwz_fb_brightness)

#endif /* __NWZ_FB_H__ */
