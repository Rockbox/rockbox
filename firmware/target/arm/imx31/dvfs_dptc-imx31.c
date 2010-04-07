/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Michael Sevakis
 *
 * i.MX31 DVFS and DPTC drivers
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
#include "ccm-imx31.h"
#include "mc13783.h"

/* Most of the code in here is based upon the Linux BSP provided by Freescale
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved. */

void dvfs_dptc_start(void)
{
    /* For now, just set the regulator voltage off of overdrive mode */
    /* For 264 MHz, DPTC is not needed and lower V can be used */

    mc13783_write_masked(MC13783_SWITCHERS0,
                         MC13783_SW_1_350 << MC13783_SW1A_POS,
                         MC13783_SW1A);
    imx31_regmod32(&CCM_PMCR0, CCM_PMCR0_DVS1_0_DVS0_0,
                   CCM_PMCR0_DVSUP_DVS);           
}


void dvfs_dptc_stop(void)
{
    /* Nothing for now */
}

