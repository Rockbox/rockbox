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
#include "tsc2100.h"

const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB",   0,   1, VOLUME_MIN/10, VOLUME_MAX/10, -25},
    /* HAVE_SW_TONE_CONTROLS */
    [SOUND_BASS]          = {"dB",   0,   1, -24,  24,   0},
    [SOUND_TREBLE]        = {"dB",   0,   1, -24,  24,   0},
    [SOUND_BALANCE]       = {"%",    0,   1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",     0,   1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",    0,   5,   0, 250, 100},
};
static bool is_muted = false;
/* convert tenth of dB volume to master volume register value */
int tenthdb2master(int db)
{
    /* 0 to -63.0dB in 1dB steps, tsc2100 can goto -63.5 in 0.5dB steps */
    if (db < VOLUME_MIN) {
        return 0x7E;
    } else if (db >= VOLUME_MAX) {
        return 0x00;
    } else {
        return(-((db)/5)); /* VOLUME_MIN is negative */
    }
}

int sound_val2phys(int setting, int value)
{
    int result;

    switch(setting)
    {
#if 0
    case SOUND_LEFT_GAIN:
    case SOUND_RIGHT_GAIN:
    case SOUND_MIC_GAIN:
        result = (value - 23) * 15;
        break;
#endif
    default:
        result = value;
        break;
    }

    return result;
}

void audiohw_init(void)
{
    short val = tsc2100_readreg(TSAC4_PAGE, TSAC4_ADDRESS);
    /* disable DAC PGA soft-stepping */
    val |= TSAC4_DASTDP;
    
    tsc2100_writereg(TSAC4_PAGE, TSAC4_ADDRESS, val);
}

void audiohw_postinit(void)
{
    audiohw_mute(0);
}

void audiohw_set_master_vol(int vol_l, int vol_r)
{
   tsc2100_writereg(TSDACGAIN_PAGE, TSDACGAIN_ADDRESS, (short)((vol_l<<8) | vol_r) );
}

void audiohw_mute(bool mute)
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

void audiohw_close(void)
{
    /* mute headphones */
    audiohw_mute(true);

}

void audiohw_set_frequency(int fsel)
{
    (void)fsel;
}
