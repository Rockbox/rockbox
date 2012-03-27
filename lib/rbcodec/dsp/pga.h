/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 Michael Sevakis
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
#ifndef PGA_H
#define PGA_H

#define PGA_UNITY ((int32_t)0x01000000) /* s7.24 */

/* Various gains supported by pre-gain amp */
enum pga_gain_ids
{
    PGA_EQ_PRECUT = 0,
    PGA_REPLAYGAIN,
#ifdef HAVE_SW_VOLUME_CONTROL
    PGA_VOLUME,
#endif
    PGA_NUM_GAINS,
};

void pga_set_gain(enum pga_gain_ids id, int32_t value);
void pga_enable_gain(enum pga_gain_ids id, bool enable);

#endif /* PGA_H */
