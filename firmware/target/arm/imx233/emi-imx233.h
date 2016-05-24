/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#ifndef __EMI_IMX233_H__
#define __EMI_IMX233_H__

#include "cpu.h"
#include "system.h"
#include "system-target.h"

struct imx233_emi_info_t
{
    int cas; // 1/2 cycle unit
    int rows;
    int columns;
    int banks;
    int chips;
    int size;
};

/**
 * Absolute maximum EMI speed:  151.58 MHz (mDDR),  130.91 MHz (DDR)
 * Intermediate EMI speeds: 130.91 MHz,  120.00 MHz, 64 MHz, 24 MHz
 * Absolute minimum CPU speed: 24 MHz */
#define IMX233_EMIFREQ_151_MHz  151580
#define IMX233_EMIFREQ_130_MHz  130910
#define IMX233_EMIFREQ_64_MHz    64000

void imx233_emi_set_frequency(unsigned long freq);
struct imx233_emi_info_t imx233_emi_get_info(void);

#endif /* __EMI_IMX233_H__ */
