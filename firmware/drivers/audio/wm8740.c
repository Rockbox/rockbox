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

#include "system.h"
#include "wm8740.h"
#include "config.h"
#include "audio.h"
#include "audiohw.h"

static void wm8740_write_reg(const int reg, const unsigned int value)
{
    int i;

    for (i = (1<<15); i; i >>= 1) {
        udelay(1);
        wm8740_set_mc(0);
        if ((reg|value) & i) {
            wm8740_set_md(1);
        } else {
            wm8740_set_md(0);
        }
        udelay(1);
        wm8740_set_mc(1);
    }
    udelay(1);
    wm8740_set_ml(0);
    udelay(1);
    wm8740_set_mc(0);
    udelay(1);
    wm8740_set_ml(1);
    udelay(1);
}

static int vol_tenthdb2hw(const int tdb)
{
    if (tdb < WM8740_VOLUME_MIN) {
        return 0x00;
    } else if (tdb > WM8740_VOLUME_MAX) {
        return 0xff;
    } else {
        return ((tdb / 5 + 0xff) & 0xff);
    }
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    wm8740_write_reg(WM8740_REG0, vol_tenthdb2hw(vol_l));
    wm8740_write_reg(WM8740_REG1, vol_tenthdb2hw(vol_r) | WM8740_LDR);
}

void audiohw_mute(void)
{
    wm8740_write_reg(WM8740_REG2, WM8740_MUT);
}

void audiohw_unmute(void)
{
    wm8740_write_reg(WM8740_REG2, 0x00);
}

void audiohw_init(void)
{
    wm8740_write_reg(WM8740_REG0, 0x00);
    wm8740_write_reg(WM8740_REG1, 0x00);
    wm8740_write_reg(WM8740_REG2, WM8740_MUT);
    wm8740_write_reg(WM8740_REG3, WM8740_I2S);
    wm8740_write_reg(WM8740_REG4, 0x00);
}

void audiohw_preinit(void)
{
}

void audiohw_set_frequency(int fsel)
{
    (void)fsel;
}

void audiohw_set_filter_roll_off(int value)
{
    /* 0 = fast (sharp); 
       1 = slow */
    if (value == 0) {
        wm8740_write_reg(WM8740_REG3, WM8740_I2S);
    } else {
        wm8740_write_reg(WM8740_REG3, WM8740_I2S | WM8740_SR0);
    }
}
