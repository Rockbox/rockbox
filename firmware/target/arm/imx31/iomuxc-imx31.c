/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2010 Michael Sevakis
 *
 * i.MX31 IOMUXC helper routines
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
#include "iomuxc-imx31.h"


/* Set the pin multiplexing configuration (functional, GPIO, etc.) */
void iomuxc_set_pin_mux(enum IMX31_IOMUXC_PINS pin,
                        unsigned long mux)
{
    unsigned long index = pin / 4;
    unsigned int shift = 8*(pin % 4);

    imx31_regmod32((unsigned long *)(IOMUXC_BASE_ADDR + 0xc) + index,
                   mux << shift, IOMUXC_MUX_MASK << shift);
}


/* Set the pin pad configuration (keeper, drive strength, etc.) */
void iomuxc_set_pad_config(enum IMX31_IOMUXC_PINS pin,
                           unsigned long config)
{
    unsigned long padoffs = pin + 2;
    unsigned long index = padoffs / 3;
    unsigned int shift = 10*(padoffs % 3);

    imx31_regmod32((unsigned long *)(IOMUXC_BASE_ADDR + 0x154) + index,
                   config << shift, IOMUXC_PAD_MASK << shift);
}
