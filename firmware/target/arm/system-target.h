/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 * Copyright (C) 2007 by Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef SYSTEM_TARGET_H
#define SYSTEM_TARGET_H

#include "system-arm.h"

/* TODO: This header is actually portalplayer specific, and should be
 * moved into an appropriate subdir (or even split in 2). */

#if CONFIG_CPU == PP5002
#define CPUFREQ_DEFAULT_MULT 4
#define CPUFREQ_DEFAULT 24000000
#define CPUFREQ_NORMAL_MULT 5
#define CPUFREQ_NORMAL 30000000
#define CPUFREQ_MAX_MULT 13
#define CPUFREQ_MAX 78000000

#else /* PP5022, PP5024 */
#define CPUFREQ_DEFAULT 24000000
#define CPUFREQ_NORMAL 30000000
#define CPUFREQ_MAX 80000000
#endif

#define inl(a) (*(volatile unsigned long *) (a))
#define outl(a,b) (*(volatile unsigned long *) (b) = (a))
#define inb(a) (*(volatile unsigned char *) (a))
#define outb(a,b) (*(volatile unsigned char *) (b) = (a))
#define inw(a) (*(volatile unsigned short *) (a))
#define outw(a,b) (*(volatile unsigned short *) (b) = (a))

static inline void udelay(unsigned usecs)
{
    unsigned stop = USEC_TIMER + usecs;
    while (TIME_BEFORE(USEC_TIMER, stop));
}

#ifdef CPU_PP502x
static inline unsigned int current_core(void)
{
    /*
     * PROCESSOR_ID seems to be 32-bits:
     * CPU = 0x55555555 = |01010101|01010101|01010101|01010101|
     * COP = 0xaaaaaaaa = |10101010|10101010|10101010|10101010|
     *                                                ^
     */
    unsigned int core;
    asm volatile (
        "mov    %0, #0x60000000 \r\n" /* PROCESSOR_ID      */
        "ldrb   %0, [%0]        \r\n" /* Just load the LSB */
        "mov    %0, %0, lsr #7  \r\n" /* Bit 7 => index    */
        : "=&r"(core)                 /* CPU=0, COP=1      */
    );
    return core;
}
#else
unsigned int current_core(void);
#endif

#if CONFIG_CPU != PP5002

#define HAVE_INVALIDATE_ICACHE
static inline void invalidate_icache(void)
{
    outl(inl(0xf000f044) | 0x6, 0xf000f044);
    while ((CACHE_CTL & 0x8000) != 0);
}

#define HAVE_FLUSH_ICACHE
static inline void flush_icache(void)
{
    outl(inl(0xf000f044) | 0x2, 0xf000f044);
    while ((CACHE_CTL & 0x8000) != 0);
}

#endif /* CONFIG_CPU */

#endif /* SYSTEM_TARGET_H */
