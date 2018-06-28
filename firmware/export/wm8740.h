/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
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

#ifndef _WM8740_H
#define _WM8740_H

#define WM8740_VOLUME_MIN	-1270
#define WM8740_VOLUME_MAX	0

#define AUDIOHW_CAPS (FILTER_ROLL_OFF_CAP)
AUDIOHW_SETTING(VOLUME, "dB", 0, 1, WM8740_VOLUME_MIN/10, WM8740_VOLUME_MAX/10, 0)
AUDIOHW_SETTING(FILTER_ROLL_OFF, "", 0, 1, 0, 1, 0)

#define WM8740_REG0             0x0000
#define WM8740_REG1             0x0200
#define WM8740_REG2             0x0400
#define WM8740_REG3             0x0600
#define WM8740_REG4             0x0C00

/**
 * Register #0
 */
#define WM8740_LDL              (1<<8)

/**
 * Register #1
 */
#define WM8740_LDR              (1<<8)

/**
 * Register #2
 */
#define WM8740_MUT              (1<<0)
#define WM8740_DEM              (1<<1)
#define WM8740_OPE              (1<<2)
#define WM8740_IW0              (1<<3)
#define WM8740_IW1              (1<<4)

/**
 * Register #3
 */
#define WM8740_I2S              (1<<0)
#define WM8740_LRP              (1<<1)
#define WM8740_ATC              (1<<2)
#define WM8740_SR0              (1<<3)
#define WM8740_REV              (1<<4)
#define WM8740_SF0              (1<<6)
#define WM8740_SF1              (1<<7)
#define WM8740_IZD              (1<<8)

/**
 * Register #4
 */
#define WM8740_DIFF0            (1<<4)
#define WM8740_DIFF1            (1<<5)
#define WM8740_CDD              (1<<6)

void audiohw_mute(void);
void audiohw_unmute(void);

void wm8740_set_ml(const int);
void wm8740_set_mc(const int);
void wm8740_set_md(const int);

#endif
