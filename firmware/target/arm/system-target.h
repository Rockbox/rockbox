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

#ifdef CPU_PP
/* TODO: This header is actually portalplayer specific, and should be
 * moved into an appropriate subdir (or even split in 2). */

#if CONFIG_CPU == PP5002
#define CPUFREQ_SLEEP      32768
#define CPUFREQ_DEFAULT 24000000
#define CPUFREQ_NORMAL  30000000
#define CPUFREQ_MAX     80000000

#else /* PP5022, PP5024 */
#define CPUFREQ_SLEEP      32768
#define CPUFREQ_DEFAULT 24000000
#define CPUFREQ_NORMAL  30000000
#define CPUFREQ_MAX     80000000
#endif

#define inl(a) (*(volatile unsigned long *) (a))
#define outl(a,b) (*(volatile unsigned long *) (b) = (a))
#define inb(a) (*(volatile unsigned char *) (a))
#define outb(a,b) (*(volatile unsigned char *) (b) = (a))
#define inw(a) (*(volatile unsigned short *) (a))
#define outw(a,b) (*(volatile unsigned short *) (b) = (a))

#if defined(HAVE_ADJUSTABLE_CPU_FREQ) && NUM_CORES > 1
extern struct spinlock boostctrl_spin;
#endif

static inline void udelay(unsigned usecs)
{
    unsigned stop = USEC_TIMER + usecs;
    while (TIME_BEFORE(USEC_TIMER, stop));
}

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
        "ldrb   %0, [%1]       \n" /* Just load the LSB */
        "mov    %0, %0, lsr #7 \n" /* Bit 7 => index    */
        : "=r"(core)              /* CPU=0, COP=1      */
        : "r"(&PROCESSOR_ID)
    );
    return core;
}

/* Return the actual ID instead of core index */
static inline unsigned int processor_id(void)
{
    unsigned int id;

    asm volatile (
        "ldrb   %0, [%1] \n"
        : "=r"(id)
        : "r"(&PROCESSOR_ID)
    );

    return id;
}

#if CONFIG_CPU == PP5002
static inline void sleep_core(int core)
{
    asm volatile (
        /* Sleep: PP5002 crashes if the instruction that puts it to sleep is
         * located at 0xNNNNNNN0. 4/8/C works. This sequence makes sure
         * that the correct alternative is executed. Don't change the order
         * of the next 4 instructions! */
        "tst    pc, #0x0c       \n"
        "mov    r0, #0xca       \n"
        "strne  r0, [%[ctl]]    \n"
        "streq  r0, [%[ctl]]    \n"
        "nop                    \n" /* nop's needed because of pipeline */
        "nop                    \n"
        "nop                    \n"
        :
        : [ctl]"r"(&PROC_CTL(core))
        : "r0"
    );
}
static inline void wake_core(int core)
{
    asm volatile (
        "mov    r0, #0xce       \n"
        "str    r0, [%[ctl]]    \n"
        :
        : [ctl]"r"(&PROC_CTL(core))
        : "r0"
    );
}
#else /* PP502x */
static inline void sleep_core(int core)
{
    asm volatile (
        "mov    r0, #0x80000000  \n"
        "str    r0, [%[ctl]]     \n"
        "nop                     \n"
        :
        : [ctl]"r"(&PROC_CTL(core))
        : "r0"
    );
}
static inline void wake_core(int core)
{
    asm volatile (
        "mov    r0, #0           \n"
        "str    r0, [%[ctl]]     \n"
        :
        : [ctl]"r"(&PROC_CTL(core))
        : "r0"
    );
}
#endif

#ifdef BOOTLOADER
/* All addresses within rockbox are in IRAM in the bootloader so
   are therefore uncached */
#define UNCACHED_ADDR(a) (a)

#else /* !BOOTLOADER */

#if CONFIG_CPU == PP5002
#define UNCACHED_BASE_ADDR 0x28000000
#else /* PP502x */
#define UNCACHED_BASE_ADDR 0x10000000
#endif

#define UNCACHED_ADDR(a) \
    ((typeof (a))((uintptr_t)(a) | UNCACHED_BASE_ADDR))
#endif /* BOOTLOADER */

/* Certain data needs to be out of the way of cache line interference
 * such as data for COP use or for use with UNCACHED_ADDR */
#define PROC_NEEDS_CACHEALIGN
#define CACHEALIGN_BITS (4) /* 2^4 = 16 bytes */

/** cache functions **/
#ifndef BOOTLOADER
#define CACHE_FUNCTIONS_AS_CALL

#define HAVE_INVALIDATE_ICACHE
void invalidate_icache(void);

#define HAVE_FLUSH_ICACHE
void flush_icache(void);
#endif

#endif /* CPU_PP */

#endif /* SYSTEM_TARGET_H */
