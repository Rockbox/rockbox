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

/* i.MX233 can boost up to 6dB in DAC mode and 12dB in line mode. Since mic/line
 * already have adjustable gain, keep lowest of both. With chained DAC volume
 * and headphone volume, the i.MX233 can achieve < -100dB but stay at -100dB. */
#define AUDIOHW_CAPS    (DEPTH_3D_CAP | BASS_CAP | TREBLE_CAP | \
                         LIN_GAIN_CAP | MIC_GAIN_CAP)

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
/* i.MX233 has four settings: 0dB, 3dB, 4.5dB, 6dB so fake 1.5dB steps */
AUDIOHW_SETTING(DEPTH_3D,   "dB", 1,15,   0,   60,   0)

#endif /* __IMX233_CODEC_H_ */
