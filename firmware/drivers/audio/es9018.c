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
#include "es9018.h"
#include "config.h"
#include "audio.h"
#include "audiohw.h"

unsigned char reg0  = 0x00; /* System settings. Default value of register 0 */
unsigned char reg1  = 0x80; /* Input settings. Manual input, I2S, 32-bit (?) */
unsigned char reg4  = 0x00; /* Automute time. Default = disabled */
unsigned char reg5  = 0x68; /* Automute level. Default is some level */
unsigned char reg6  = 0x4A; /* Deemphasis. Default = disabled */
unsigned char reg7  = 0x83; /* General settings. Default sharp fir, pcm iir and muted */
unsigned char reg8  = 0x10; /* GPIO configuration */
unsigned char reg10 = 0x05; /* Master Mode Control. Default value: master mode off */
unsigned char reg11 = 0x02; /* Channel Mapping. Default stereo is Ch1=left, Ch2=right */
unsigned char reg12 = 0x50; /* DPLL Settings. Default = 005 for I2S, OFF for DSD */
unsigned char reg13 = 0x40; /* THD Compensation */
unsigned char reg14 = 0x8A; /* Soft Start Settings */
unsigned char reg21 = 0x00; /* Oversampling filter. Default: oversampling ON */

#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))

static int vol_tenthdb2hw(const int tdb)
{
    if (tdb < ES9018_VOLUME_MIN) {
        return 0xff;
    } else if (tdb > ES9018_VOLUME_MAX) {
        return 0x00;
    } else {
        return (-tdb/5);
    }
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    es9018_write_reg(15, vol_tenthdb2hw(vol_l));
    es9018_write_reg(16, vol_tenthdb2hw(vol_r));
}

void audiohw_mute(void)
{
    bitSet(reg7, 0); /* Mute Channel 1 */
    bitSet(reg7, 1); /* Mute Channel 2 */
    es9018_write_reg(0x07, reg7);
}

void audiohw_unmute(void)
{
    bitClear(reg7, 0); /* Unmute Channel 1 */
    bitClear(reg7, 1); /* Unmute Channel 2 */
    es9018_write_reg(0x07, reg7);
}

void audiohw_init(void)
{
    es9018_write_reg(0x00, reg0);
    es9018_write_reg(0x01, reg1);
    es9018_write_reg(0x04, reg4);
    es9018_write_reg(0x05, reg5);
    es9018_write_reg(0x06, reg6);
    es9018_write_reg(0x07, reg7);
    es9018_write_reg(0x08, reg8);
    es9018_write_reg(0x0A, reg10);
    es9018_write_reg(0x0B, reg11);
    es9018_write_reg(0x0C, reg12);
    es9018_write_reg(0x0D, reg13);
    es9018_write_reg(0x0E, reg14);
    es9018_write_reg(0x15, reg21);
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
    /* 0 = Sharp (Default)
       1 = Slow 
       2 = Short
       3 = Bypass */
    switch(value)
    {
        case 0:
            bitClear(reg7, 5);
            bitClear(reg7, 6);
            bitClear(reg21, 0);
            break;
        case 1:
            bitSet(reg7, 5);
            bitClear(reg7, 6);
            bitClear(reg21, 0);
            break;
        case 2:
            bitClear(reg7, 5);
            bitSet(reg7, 6);
            bitClear(reg21, 0);
            break;
        case 3:
            bitSet(reg21, 0);
            break;
    }
    es9018_write_reg(0x07, reg7);
    es9018_write_reg(0x15, reg21);
}
