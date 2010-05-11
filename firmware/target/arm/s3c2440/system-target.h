/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Greg White
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
#ifndef SYSTEM_TARGET_H
#define SYSTEM_TARGET_H

#include "system-arm.h"
#include "mmu-arm.h"

/* NB: These values must match the register settings in s3c2440/crt0.S */

#ifdef GIGABEAT_F
    /* MPLLCON = 0x000C9042, 16.9344 MHz refclk, therefore:
     * MPLL = 294940800 = 2*(201 + 8)*16934400 / ((4 + 2) * 2^2) */
    #define CPUFREQ_DEFAULT 98313600
    #define CPUFREQ_NORMAL  98313600
    #define CPUFREQ_MAX    294940800

    /* Uses 1:3:6 */
    #define FCLK    CPUFREQ_MAX
    #define HCLK    (FCLK/3)   /* = 98,313,600 */
    #define PCLK    (HCLK/2)   /* = 49,156,800 */

    #ifdef BOOTLOADER
        /* All addresses within rockbox are in IRAM in the bootloader so
           are therefore uncached */
        #define UNCACHED_ADDR(a) (a)
    #else /* !BOOTLOADER */
        #define UNCACHED_BASE_ADDR 0x30000000
        #define UNCACHED_ADDR(a)  ((typeof(a))((unsigned int)(a) | UNCACHED_BASE_ADDR ))
    #endif /* BOOTLOADER */

#elif defined(MINI2440)

    /* Uses 1:4:8 */
    #define FCLK 406000000
    #define HCLK (FCLK/4)   /* = 101,250,000 */
    #define PCLK (HCLK/2)   /* =  50,625,000 */

    #define CPUFREQ_DEFAULT FCLK    /* 406 MHz */
    #define CPUFREQ_NORMAL  (FCLK/4)/* 101.25 MHz */   
    #define CPUFREQ_MAX     FCLK    /* 406 MHz */
    
    #define UNCACHED_BASE_ADDR 0x30000000
    #define UNCACHED_ADDR(a)  ((typeof(a))((unsigned int)(a) | UNCACHED_BASE_ADDR ))

#else
    #error Unknown target
#endif


void system_prepare_fw_start(void);
void tick_stop(void);

/* Functions to set and clear register bits atomically */

/* Set and clear register bits */
void s3c_regmod32(volatile unsigned long *reg, unsigned long bits,
                  unsigned long mask);
/* Set register bits */
void s3c_regset32(volatile unsigned long *reg, unsigned long bits);
/* Clear register bits */
void s3c_regclr32(volatile unsigned long *reg, unsigned long bits);

#endif /* SYSTEM_TARGET_H */
