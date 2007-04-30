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
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef SYSTEM_ARM_H
#define SYSTEM_ARM_H

#define nop \
  asm volatile ("nop")

/* This gets too complicated otherwise with all the ARM variation and would
   have conflicts with another system-target.h elsewhere so include a
   subheader from here. */

/* TODO: Implement set_irq_level and check CPU frequencies */

#if CONFIG_CPU != S3C2440 && CONFIG_CPU != PNX0101

/* TODO: Finish targeting this stuff */
#define CPUFREQ_DEFAULT_MULT 8
#define CPUFREQ_DEFAULT 24000000
#define CPUFREQ_NORMAL_MULT 10
#define CPUFREQ_NORMAL 30000000
#define CPUFREQ_MAX_MULT 25
#define CPUFREQ_MAX 75000000

#endif

static inline uint16_t swap16(uint16_t value)
    /*
      result[15..8] = value[ 7..0];
      result[ 7..0] = value[15..8];
    */
{
    return (value >> 8) | (value << 8);
}

static inline uint32_t swap32(uint32_t value)
    /*
      result[31..24] = value[ 7.. 0];
      result[23..16] = value[15.. 8];
      result[15.. 8] = value[23..16];
      result[ 7.. 0] = value[31..24];
    */
{
    uint32_t tmp;

    asm volatile (
        "eor %1, %0, %0, ror #16 \n\t"
        "bic %1, %1, #0xff0000   \n\t"
        "mov %0, %0, ror #8      \n\t"
        "eor %0, %0, %1, lsr #8  \n\t"
        : "+r" (value), "=r" (tmp)
    );
    return value;
}

static inline uint32_t swap_odd_even32(uint32_t value)
{
    /*
      result[31..24],[15.. 8] = value[23..16],[ 7.. 0]
      result[23..16],[ 7.. 0] = value[31..24],[15.. 8]
    */
    uint32_t tmp;

    asm volatile (                    /* ABCD      */
        "bic %1, %0, #0x00ff00  \n\t" /* AB.D      */
        "bic %0, %0, #0xff0000  \n\t" /* A.CD      */
        "mov %0, %0, lsr #8     \n\t" /* .A.C      */
        "orr %0, %0, %1, lsl #8 \n\t" /* B.D.|.A.C */
        : "+r" (value), "=r" (tmp)    /* BADC      */
    );
    return value;
}

#define HIGHEST_IRQ_LEVEL (0x80)

static inline int set_irq_level(int level)
{
    unsigned long cpsr;
    int oldlevel;
    /* Read the old level and set the new one */
    asm volatile (
        "mrs    %1, cpsr        \n"
        "bic    %0, %1, #0x80   \n"
        "and    %2, %2, #0x80   \n"
        "orr    %0, %0, %2      \n"
        "msr    cpsr_c, %0      \n"
        : "=&r"(cpsr), "=&r"(oldlevel), "+&r"(level)
    );
    return oldlevel;
}

static inline void set_fiq_handler(void(*fiq_handler)(void))
{
    /* Install the FIQ handler */
    *((unsigned int*)(15*4)) = (unsigned int)fiq_handler;
}

static inline void enable_fiq(void)
{
    /* Clear FIQ disable bit */
    asm volatile (
        "mrs     r0, cpsr         \n"\
        "bic     r0, r0, #0x40    \n"\
        "msr     cpsr_c, r0         "
        : : : "r0"
    );
}

static inline void disable_fiq(void)
{
    /* Set FIQ disable bit */
    asm volatile (
        "mrs     r0, cpsr         \n"\
        "orr     r0, r0, #0x40    \n"\
        "msr     cpsr_c, r0         "
        : : : "r0"
    );
}

#endif /* SYSTEM_ARM_H */
