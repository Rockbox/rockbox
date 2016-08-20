/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#ifndef __IMX233_CODEC_H_
#define __IMX233_CODEC_H_

#define AUDIOHW_CAPS    (DEPTH_3D_CAP | BASS_CAP | TREBLE_CAP | \
                         LIN_GAIN_CAP | MIC_GAIN_CAP)

/* i.MX233 can boost up to 6dB in DAC mode and 12dB in line mode. Pretend we can
 * do 12dB (but we cap at 6dB in DAC mode). With chained DAC volume
 * and headphone volume, the i.MX233 can achieve < -100dB but stay at -100dB. */
AUDIOHW_SETTING(VOLUME,     "dB", 0, 1, -100,  12,  -25)
/* HAVE_SW_TONE_CONTROLS */
#ifdef HAVE_RECORDING
/* Depending on the input, we have three available volumes to tweak:
 * - adc volume: -100dB -> -0.5dB in 0.5dB steps
 * - mux gain: 0dB -> 22.5dB in 1.5dB steps
 * - mic gain: 0dB -> 40dB in 10dB steps (except for 10) */
AUDIOHW_SETTING(LEFT_GAIN,  "dB", 0, 1, -100,  22,   0)
AUDIOHW_SETTING(RIGHT_GAIN, "dB", 0, 1, -100,  22,   0)
AUDIOHW_SETTING(MIC_GAIN,   "dB", 0, 1, -100,  60,  20)
#endif /* HAVE_RECORDING */
/* i.MX233 has four settings: 0dB, 3dB, 4.5dB, 6dB */
/* depth_3d setting: 0=0dB, 1=3dB, 2=4.5dB, 3=6dB. Return value in tenth of dB */
static inline int imx233_depth_3d_val2phys(int val)
{
    if(val == 0)
        return 0; /* 0dB */
    else
        return 15 * (val + 1); /* 3dB + 1.5dB per step */
}
AUDIOHW_SETTING(DEPTH_3D,   "dB", 1, 1,    0,   3,   0, imx233_depth_3d_val2phys(val))


#endif /* __IMX233_CODEC_H_ */
