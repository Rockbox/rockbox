/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __FIIOM3K_CODEC__
#define __FIIOM3K_CODEC__

#define AUDIOHW_CAPS (FILTER_ROLL_OFF_CAP)

/*  Max HW volume:  -32 dB to  +9 dB
 * With SW volume: -105 dB to +15 dB
 */
AUDIOHW_SETTING(VOLUME,          "dB", 1, 5, -600, 90, -200)
AUDIOHW_SETTING(FILTER_ROLL_OFF,   "", 1, 1, 0, 3, 0)

#endif /* __FIIOM3K_CODEC__ */
