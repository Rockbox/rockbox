#ifndef __PP6100_H__
#define __PP6100_H__
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Robert Keevil
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

/* We believe this is quite similar to the 5020 and for now we just use that
   completely and redefine any minor differences */
#include "pp5020.h"

#undef DRAM_START
#define DRAM_START       0x10f00000

#define GPIOM_ENABLE     (*(volatile unsigned long *)(0x6000d180))
#define GPION_ENABLE     (*(volatile unsigned long *)(0x6000d184))
#define GPIOO_ENABLE     (*(volatile unsigned long *)(0x6000d188))
#define GPIOP_ENABLE     (*(volatile unsigned long *)(0x6000d18c))
#define GPIOM_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d190))
#define GPION_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d194))
#define GPIOO_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d198))
#define GPIOP_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d19c))
#define GPIOM_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d1a0))
#define GPION_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d1a4))
#define GPIOO_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d1a8))
#define GPIOP_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d1ac))
#define GPIOM_INPUT_VAL  (*(volatile unsigned long *)(0x6000d1b0))
#define GPION_INPUT_VAL  (*(volatile unsigned long *)(0x6000d1b4))
#define GPIOO_INPUT_VAL  (*(volatile unsigned long *)(0x6000d1b8))
#define GPIOP_INPUT_VAL  (*(volatile unsigned long *)(0x6000d1bc))
#define GPIOM_INT_STAT   (*(volatile unsigned long *)(0x6000d1c0))
#define GPION_INT_STAT   (*(volatile unsigned long *)(0x6000d1c4))
#define GPIOO_INT_STAT   (*(volatile unsigned long *)(0x6000d1c8))
#define GPIOP_INT_STAT   (*(volatile unsigned long *)(0x6000d1cc))
#define GPIOM_INT_EN     (*(volatile unsigned long *)(0x6000d1d0))
#define GPION_INT_EN     (*(volatile unsigned long *)(0x6000d1d4))
#define GPIOO_INT_EN     (*(volatile unsigned long *)(0x6000d1d8))
#define GPIOP_INT_EN     (*(volatile unsigned long *)(0x6000d1dc))
#define GPIOM_INT_LEV    (*(volatile unsigned long *)(0x6000d1e0))
#define GPION_INT_LEV    (*(volatile unsigned long *)(0x6000d1e4))
#define GPIOO_INT_LEV    (*(volatile unsigned long *)(0x6000d1e8))
#define GPIOP_INT_LEV    (*(volatile unsigned long *)(0x6000d1ec))
#define GPIOM_INT_CLR    (*(volatile unsigned long *)(0x6000d1f0))
#define GPION_INT_CLR    (*(volatile unsigned long *)(0x6000d1f4))
#define GPIOO_INT_CLR    (*(volatile unsigned long *)(0x6000d1f8))
#define GPIOP_INT_CLR    (*(volatile unsigned long *)(0x6000d1fc))

#define GPIOQ_ENABLE     (*(volatile unsigned long *)(0x6000d200))
#define GPIOR_ENABLE     (*(volatile unsigned long *)(0x6000d204))
#define GPIOS_ENABLE     (*(volatile unsigned long *)(0x6000d208))
#define GPIOT_ENABLE     (*(volatile unsigned long *)(0x6000d20c))
#define GPIOQ_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d210))
#define GPIOR_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d214))
#define GPIOS_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d218))
#define GPIOT_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d21c))
#define GPIOQ_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d220))
#define GPIOR_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d224))
#define GPIOS_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d228))
#define GPIOT_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d22c))
#define GPIOQ_INPUT_VAL  (*(volatile unsigned long *)(0x6000d230))
#define GPIOR_INPUT_VAL  (*(volatile unsigned long *)(0x6000d234))
#define GPIOS_INPUT_VAL  (*(volatile unsigned long *)(0x6000d238))
#define GPIOT_INPUT_VAL  (*(volatile unsigned long *)(0x6000d23c))
#define GPIOQ_INT_STAT   (*(volatile unsigned long *)(0x6000d240))
#define GPIOR_INT_STAT   (*(volatile unsigned long *)(0x6000d244))
#define GPIOS_INT_STAT   (*(volatile unsigned long *)(0x6000d248))
#define GPIOT_INT_STAT   (*(volatile unsigned long *)(0x6000d24c))
#define GPIOQ_INT_EN     (*(volatile unsigned long *)(0x6000d250))
#define GPIOR_INT_EN     (*(volatile unsigned long *)(0x6000d254))
#define GPIOS_INT_EN     (*(volatile unsigned long *)(0x6000d258))
#define GPIOT_INT_EN     (*(volatile unsigned long *)(0x6000d25c))
#define GPIOQ_INT_LEV    (*(volatile unsigned long *)(0x6000d260))
#define GPIOR_INT_LEV    (*(volatile unsigned long *)(0x6000d264))
#define GPIOS_INT_LEV    (*(volatile unsigned long *)(0x6000d268))
#define GPIOT_INT_LEV    (*(volatile unsigned long *)(0x6000d26c))
#define GPIOQ_INT_CLR    (*(volatile unsigned long *)(0x6000d270))
#define GPIOR_INT_CLR    (*(volatile unsigned long *)(0x6000d274))
#define GPIOS_INT_CLR    (*(volatile unsigned long *)(0x6000d278))
#define GPIOT_INT_CLR    (*(volatile unsigned long *)(0x6000d27c))

#define GPIOM 12
#define GPION 13
#define GPIOO 14
#define GPIOP 15

#define GPIOQ 16
#define GPIOR 17
#define GPIOS 18
#define GPIOT 19

#define DEV_INIT3        (*(volatile unsigned long *)(0x70000014))

#endif
