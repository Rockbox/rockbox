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
#define VOLUME_MIN -1000
#define VOLUME_MAX  60

#define AUDIOHW_CAPS    (DEPTH_3D_CAP | BASS_CAP | TREBLE_CAP)

/* Work with half dB since the i.MX233 doesn't have a better resolution */
int tenthdb2master(int tdb);
void audiohw_set_headphone_vol(int vol_l, int vol_r);

#endif /* __IMX233_CODEC_H_ */
