/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Dave Chapman
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

#define CPUFREQ_DEFAULT 98784000
#define CPUFREQ_NORMAL  98784000
#define CPUFREQ_MAX    120000000

#define inl(a) (*(volatile unsigned long *) (a))
#define outl(a,b) (*(volatile unsigned long *) (b) = (a))
#define inb(a) (*(volatile unsigned char *) (a))
#define outb(a,b) (*(volatile unsigned char *) (b) = (a))
#define inw(a) (*(volatile unsigned short *) (a))
#define outw(a,b) (*(volatile unsigned short *) (b) = (a))

/* TC32 is configured to 1MHz in clock_init() */
#define USEC_TIMER  TC32MCNT

static inline void udelay(unsigned usecs)
{
    unsigned stop = USEC_TIMER + usecs;
    while (TIME_BEFORE(USEC_TIMER, stop));
}


#define TCC77X_CSCFG_BW8       0
#define TCC77X_CSCFG_BW16      1

/* Due to hardware bug or "feature" this hack is needed to set bus width bits */
static inline
unsigned long tcc77x_cscfg_bw(int bw) {
    if (bw == TCC77X_CSCFG_BW8)
        return (((MCFG >> 11) & 3) ^ 3) << 28;
    else
        return (((MCFG >> 11) & 3) ^ 2) << 28;
}

#endif /* SYSTEM_TARGET_H */
