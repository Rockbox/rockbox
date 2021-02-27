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

#ifndef __AIC_X1000_H__
#define __AIC_X1000_H__

#include "clk-x1000.h"
#include <stdbool.h>

/* Set frequency of I2S master clock supplied by AIC. Has no use if an
 * external DAC is supplying the master clock. Must be called with the
 * bit clock disabled.
 *
 * - clksrc can be one of EXCLK, SCLK_A, MPLL.
 * - This function does not modify PLL settings. It's the caller's job
 *   to ensure the PLL is configured and runing.
 * - fs is the audio sampling frequency (8 KHz - 192 KHz)
 * - mult is multiplied by fs to get the master clock rate.
 * - mult must be a multiple of 64 due to AIC bit clock requirements.
 * - Note: EXCLK bypasses the decimal divider so it is not very flexible.
 *   If using EXCLK you must set mult=0. If EXCLK is not a multiple of
 *   the bit clock (= 64*fs), then the clock rate will be inaccurate.
 *
 * Returns zero on success and nonzero if the frequency is not achievable.
 */
extern int aic_i2s_set_mclk(x1000_clk_t clksrc, unsigned fs, unsigned mult);

#endif /* __AIC_X1000_H__ */
