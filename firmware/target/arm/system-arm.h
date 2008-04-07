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

/* Core-level interrupt masking */

/* This one returns the old status */
#define IRQ_ENABLED      0x00
#define IRQ_DISABLED     0x80
#define IRQ_STATUS       0x80
#define FIQ_ENABLED      0x00
#define FIQ_DISABLED     0x40
#define FIQ_STATUS       0x40
#define IRQ_FIQ_ENABLED  0x00
#define IRQ_FIQ_DISABLED 0xc0
#define IRQ_FIQ_STATUS   0xc0
#define HIGHEST_IRQ_LEVEL IRQ_DISABLED

#define set_irq_level(status) \
    set_interrupt_status((status), IRQ_STATUS)
#define set_fiq_status(status) \
    set_interrupt_status((status), FIQ_STATUS)

static inline int set_interrupt_status(int status, int mask)
{
    unsigned long cpsr;
    int oldstatus;
    /* Read the old levels and set the new ones */
    asm volatile (
        "mrs    %1, cpsr        \n"
        "bic    %0, %1, %[mask] \n"
        "orr    %0, %0, %2      \n"
        "msr    cpsr_c, %0      \n"
        : "=&r,r"(cpsr), "=&r,r"(oldstatus)
        : "r,i"(status & mask), [mask]"i,i"(mask));

    return oldstatus;
}

static inline void enable_interrupt(int mask)
{
    /* Clear I and/or F disable bit */
    int tmp;
    asm volatile (
        "mrs     %0, cpsr   \n"
        "bic     %0, %0, %1 \n"
        "msr     cpsr_c, %0 \n"
        : "=&r"(tmp) : "i"(mask));
}

static inline void disable_interrupt(int mask)
{
    /* Set I and/or F disable bit */
    int tmp;
    asm volatile (
        "mrs     %0, cpsr   \n"
        "orr     %0, %0, %1 \n"
        "msr     cpsr_c, %0 \n"
        : "=&r"(tmp) : "i"(mask));
}

#define disable_irq() \
    disable_interrupt(IRQ_STATUS)

#define enable_irq() \
    enable_interrupt(IRQ_STATUS)

#define disable_fiq() \
    disable_interrupt(FIQ_STATUS)

#define enable_fiq() \
    enable_interrupt(FIQ_STATUS)

static inline int disable_interrupt_save(int mask)
{
    /* Set I and/or F disable bit and return old cpsr value */
    int cpsr, tmp;
    asm volatile (
        "mrs     %1, cpsr   \n"
        "orr     %0, %1, %2 \n"
        "msr     cpsr_c, %0 \n"
        : "=&r"(tmp), "=&r"(cpsr)
        : "i"(mask));
    return cpsr;
}

#define disable_irq_save() \
    disable_interrupt_save(IRQ_STATUS)

#define disable_fiq_save() \
    disable_interrupt_save(FIQ_STATUS)

static inline void restore_interrupt(int cpsr)
{
    /* Set cpsr_c from value returned by disable_interrupt_save
     * or set_interrupt_status */
    asm volatile ("msr cpsr_c, %0" : : "r"(cpsr));
}

#define restore_irq(cpsr) \
    restore_interrupt(cpsr)

#define restore_fiq(cpsr) \
    restore_interrupt(cpsr)

#endif /* SYSTEM_ARM_H */
