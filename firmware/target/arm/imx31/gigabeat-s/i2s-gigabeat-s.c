/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Michael Sevakis
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
#include "config.h"
#include "system.h"
#include "i2s.h"

void i2s_reset(void)
{
    /* How SYSCLK for codec is derived (USBPLL=338.688MHz).
     *
     * SSI post dividers (SSI2 PODF=4, SSI2 PRE PODF=0):
     * 338688000Hz / 5 = 67737600Hz = ssi1_clk
     * 
     * SSI bit clock dividers (DIV2=1, PSR=0, PM=0):
     * ssi1_clk / 4 = 16934400Hz = INT_BIT_CLK (MCLK)
     *
     * WM Codec post divider (MCLKDIV=1.5):
     * INT_BIT_CLK (MCLK) / 1.5 = 11289600Hz = 256*fs = SYSCLK
     */
    imx31_regmod32(&CCM_PDR1,
                   ((1-1) << CCM_PDR1_SSI1_PRE_PODF_POS) |
                   ((5-1) << CCM_PDR1_SSI1_PODF_POS) |
                   ((8-1) << CCM_PDR1_SSI2_PRE_PODF_POS) |
                   ((64-1) << CCM_PDR1_SSI2_PODF_POS),
                   CCM_PDR1_SSI1_PODF | CCM_PDR1_SSI2_PODF |
                   CCM_PDR1_SSI1_PRE_PODF | CCM_PDR1_SSI2_PRE_PODF);
}
