/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for TSC2100 audio codec
 *
 * Copyright (c) 2008 Jonathan Gordon
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
#include "cpu.h"
#include "debug.h"
#include "system.h"
#include "audio.h"

#include "audiohw.h"
#include "sound.h"
#include "tsc2100.h"

static bool is_muted = false;
/* convert tenth of dB volume to volume register value */
static int vol_tenthdb2hw(int db)
{
    /* 0 to -63.0dB in 1dB steps, tsc2100 can goto -63.5 in 0.5dB steps */
    if (db <= -640) {
        return 0x7E;
    } else if (db >= 0) {
        return 0x00;
    } else {
        return(-((db)/5));
    }
}

void audiohw_init(void)
{
    short val = tsc2100_readreg(TSAC4_PAGE, TSAC4_ADDRESS);
    /* disable DAC PGA soft-stepping */
    val |= TSAC4_DASTDP;
    
    tsc2100_writereg(TSAC4_PAGE, TSAC4_ADDRESS, val);
}

static void audiohw_mute(bool mute)
{
    short vol = tsc2100_readreg(TSDACGAIN_PAGE, TSDACGAIN_ADDRESS);
    /* left  mute bit == 1<<15
       right mute bit == 1<<7
     */
    if (mute) 
    {
        vol |= (1<<15)|(1<<7);
    } else 
    {
        vol &= ~((1<<15)|(1<<7));
    }
    is_muted = mute;
    tsc2100_writereg(TSDACGAIN_PAGE, TSDACGAIN_ADDRESS, vol);
}

void audiohw_postinit(void)
{
    audiohw_mute(false);
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    vol_l = vol_tenthdb2hw(vol_l);
    vol_r = vol_tenthdb2hw(vol_r);
    tsc2100_writereg(TSDACGAIN_PAGE, TSDACGAIN_ADDRESS,
                     (short)((vol_l<<8) | vol_r) );
}

void audiohw_close(void)
{
    /* mute headphones */
    audiohw_mute(true);
}

void audiohw_set_frequency(int fsel)
{
    int reg_val;
    reg_val = tsc2100_readreg(CONTROL_PAGE2, TSAC1_ADDRESS);
    
    reg_val &= ~(0x07<<3);
    
    switch(fsel) 
    {
    case HW_FREQ_8:
        reg_val |= (0x06<<3);
        break;
    case HW_FREQ_11:
        reg_val |= (0x04<<3);
        break;
    case HW_FREQ_22:
        reg_val |= (0x02<<3);
        break;
    case HW_FREQ_44:
    default:
        break;
    }

    tsc2100_writereg(CONTROL_PAGE2, TSAC1_ADDRESS, reg_val);
}
