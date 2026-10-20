/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Driver for Cirrus Logic CS42L58 audio codec
 *
 * Copyright (C) 2026 Viktor Krapivin
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

#ifndef __CS42L58_H__
#define __CS42L58_H__

#ifdef HAVE_RECORDING
#define AUDIOHW_CAPS (LINEOUT_CAP | LIN_GAIN_CAP | MIC_GAIN_CAP)
#else
#define AUDIOHW_CAPS LINEOUT_CAP
#endif
/* Below -60 dB, software adds attenuation; see audiohw_set_volume(). */
AUDIOHW_SETTING(VOLUME, "dB", 0, 1, -100, 2, -25)
#ifdef HAVE_RECORDING
/* The recording path is not implemented yet: these settings and the
 * recording functions are stubs. */
AUDIOHW_SETTING(LEFT_GAIN,  "dB", 0, 1, -96, 0, 0)
AUDIOHW_SETTING(RIGHT_GAIN, "dB", 0, 1, -96, 0, 0)
AUDIOHW_SETTING(MIC_GAIN,   "dB", 0, 1, -96, 0, 0)
#endif

void audiohw_enable_lineout(bool enable);

#endif
