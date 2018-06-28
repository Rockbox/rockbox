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

#ifndef _ES9018_H
#define _ES9018_H

#define ES9018_VOLUME_MIN	-1270
#define ES9018_VOLUME_MAX	0

#define AUDIOHW_CAPS (FILTER_ROLL_OFF_CAP)
#define AUDIOHW_HAVE_SHORT_ROLL_OFF
AUDIOHW_SETTING(VOLUME, "dB", 0, 1, ES9018_VOLUME_MIN/10, ES9018_VOLUME_MAX/10, 0)
AUDIOHW_SETTING(FILTER_ROLL_OFF, "", 0, 1, 0, 3, 0)

void es9018_write_reg(uint8_t reg, uint8_t val);
uint8_t es9018_read_reg(uint8_t reg);

void audiohw_mute(void);
void audiohw_unmute(void);

#endif
