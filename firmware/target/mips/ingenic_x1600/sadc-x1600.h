/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

#ifndef __SADC_X1600_H__
#define __SADC_X1600_H__

#include <stdint.h>

/* Four channels, 12-bit, V = ADATA * VREF / 4096. Channel assignment is the
 * board's, in adc-target.h. The extras to adc.h's own API: */

/* Read a channel and convert to millivolts using the board's ADC_VREF_MV */
extern int adc_read_mv(int channel);

/* Each enabled channel lengthens the round-robin period */
extern void adc_enable_channel(int channel, bool enable);

#endif /* __SADC_X1600_H__ */
