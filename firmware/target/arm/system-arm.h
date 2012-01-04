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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef SYSTEM_ARM_H
#define SYSTEM_ARM_H

/* Common to all ARM_ARCH */
#define nop \
  asm volatile ("nop")

void __div0(void);

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

#define disable_irq_save() \
    disable_interrupt_save(IRQ_STATUS)
#define disable_fiq_save() \
    disable_interrupt_save(FIQ_STATUS)

#define restore_irq(cpsr) \
    restore_interrupt(cpsr)
#define restore_fiq(cpsr) \
    restore_interrupt(cpsr)

#define disable_irq() \
    disable_interrupt(IRQ_STATUS)
#define enable_irq() \
    enable_interrupt(IRQ_STATUS)
#define disable_fiq() \
    disable_interrupt(FIQ_STATUS)
#define enable_fiq() \
    enable_interrupt(FIQ_STATUS)

#define irq_enabled() \
    interrupt_enabled(IRQ_STATUS)
#define fiq_enabled() \
    interrupt_enabled(FIQ_STATUS)
#define ints_enabled() \
    interrupt_enabled(IRQ_FIQ_STATUS)

#define irq_enabled_checkval(val) \
    (((val) & IRQ_STATUS) == 0)
#define fiq_enabled_checkval(val) \
    (((val) & FIQ_STATUS) == 0)
#define ints_enabled_checkval(val) \
    (((val) & IRQ_FIQ_STATUS) == 0)

/* We run in SYS mode */
#define is_thread_context() \
    (get_processor_mode() == 0x1f)

/* Core-level interrupt masking */

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

static inline void restore_interrupt(int cpsr)
{
    /* Set cpsr_c from value returned by disable_interrupt_save
     * or set_interrupt_status */
    asm volatile ("msr cpsr_c, %0" : : "r"(cpsr));
}

static inline bool interrupt_enabled(int status)
{
    unsigned long cpsr;
    asm ("mrs %0, cpsr" : "=r"(cpsr));
    return (cpsr & status) == 0;
}

static inline unsigned long get_processor_mode(void)
{
    unsigned long cpsr;
    asm ("mrs %0, cpsr" : "=r"(cpsr));
    return cpsr & 0x1f;
}

/* ARM_ARCH version section for architecture*/

#if ARM_ARCH >= 6
static inline uint16_t swap16_hw(uint16_t value)
    /*
      result[15..8] = value[ 7..0];
      result[ 7..0] = value[15..8];
    */
{
    uint32_t retval;
    asm ("revsh %0, %1"                         /* xxAB */
        : "=r"(retval) : "r"((uint32_t)value)); /* xxBA */
    return retval;
}

static inline uint32_t swap32_hw(uint32_t value)
    /*
      result[31..24] = value[ 7.. 0];
      result[23..16] = value[15.. 8];
      result[15.. 8] = value[23..16];
      result[ 7.. 0] = value[31..24];
    */
{
    uint32_t retval;
    asm ("rev %0, %1"                 /* ABCD */
        : "=r"(retval) : "r"(value)); /* DCBA */
    return retval;
}

static inline uint32_t swap_odd_even32_hw(uint32_t value)
{
    /*
      result[31..24],[15.. 8] = value[23..16],[ 7.. 0]
      result[23..16],[ 7.. 0] = value[31..24],[15.. 8]
    */
    uint32_t retval;
    asm ("rev16 %0, %1"               /* ABCD */
        : "=r"(retval) : "r"(value)); /* BADC */
    return retval;
}

static inline void enable_interrupt(int mask)
{
    /* Clear I and/or F disable bit */
    /* mask is expected to be constant and so only relevent branch
     * is preserved */
    switch (mask & IRQ_FIQ_STATUS)
    {
    case IRQ_STATUS:
        asm volatile ("cpsie i");
        break;
    case FIQ_STATUS:
        asm volatile ("cpsie f");
        break;
    case IRQ_FIQ_STATUS:
        asm volatile ("cpsie if");
        break;
    }
}

static inline void disable_interrupt(int mask)
{
    /* Set I and/or F disable bit */
    /* mask is expected to be constant and so only relevent branch
     * is preserved */
    switch (mask & IRQ_FIQ_STATUS)
    {
    case IRQ_STATUS:
        asm volatile ("cpsid i");
        break;
    case FIQ_STATUS:
        asm volatile ("cpsid f");
        break;
    case IRQ_FIQ_STATUS:
        asm volatile ("cpsid if");
        break;
    }
}

static inline int disable_interrupt_save(int mask)
{
    /* Set I and/or F disable bit and return old cpsr value */
    int cpsr;
    /* mask is expected to be constant and so only relevent branch
     * is preserved */
    asm volatile("mrs %0, cpsr" : "=r"(cpsr));
    switch (mask & IRQ_FIQ_STATUS)
    {
    case IRQ_STATUS:
        asm volatile ("cpsid i");
        break;
    case FIQ_STATUS:
        asm volatile ("cpsid f");
        break;
    case IRQ_FIQ_STATUS:
        asm volatile ("cpsid if");
        break;
    }
    return cpsr;
}

#else /* ARM_ARCH < 6 */

static inline uint16_t swap16_hw(uint16_t value)
    /*
      result[15..8] = value[ 7..0];
      result[ 7..0] = value[15..8];
    */
{
    return (value >> 8) | (value << 8);
}

static inline uint32_t swap32_hw(uint32_t value)
    /*
      result[31..24] = value[ 7.. 0];
      result[23..16] = value[15.. 8];
      result[15.. 8] = value[23..16];
      result[ 7.. 0] = value[31..24];
    */
{
#ifdef __thumb__
    uint32_t mask = 0x00FF00FF;
    asm (                            /* val  = ABCD */
        "and %1, %0              \n" /* mask = .B.D */
        "eor %0, %1              \n" /* val  = A.C. */
        "lsl %1, #8              \n" /* mask = B.D. */
        "lsr %0, #8              \n" /* val  = .A.C */
        "orr %0, %1              \n" /* val  = BADC */
        "mov %1, #16             \n" /* mask = 16   */
        "ror %0, %1              \n" /* val  = DCBA */
        : "+l"(value), "+l"(mask));
#else
    uint32_t tmp;
    asm (
        "eor %1, %0, %0, ror #16 \n"
        "bic %1, %1, #0xff0000   \n"
        "mov %0, %0, ror #8      \n"
        "eor %0, %0, %1, lsr #8  \n"
        : "+r" (value), "=r" (tmp));
#endif
    return value;
}

static inline uint32_t swap_odd_even32_hw(uint32_t value)
{
    /*
      result[31..24],[15.. 8] = value[23..16],[ 7.. 0]
      result[23..16],[ 7.. 0] = value[31..24],[15.. 8]
    */
#ifdef __thumb__
    uint32_t mask = 0x00FF00FF;
    asm (                            /* val  = ABCD */
        "and %1, %0             \n"  /* mask = .B.D */
        "eor %0, %1             \n"  /* val  = A.C. */
        "lsl %1, #8             \n"  /* mask = B.D. */
        "lsr %0, #8             \n"  /* val  = .A.C */
        "orr %0, %1             \n"  /* val  = BADC */
        : "+l"(value), "+l"(mask));
#else
    uint32_t tmp;
    asm (                            /* ABCD      */
        "bic %1, %0, #0x00ff00  \n"  /* AB.D      */
        "bic %0, %0, #0xff0000  \n"  /* A.CD      */
        "mov %0, %0, lsr #8     \n"  /* .A.C      */
        "orr %0, %0, %1, lsl #8 \n"  /* B.D.|.A.C */
        : "+r" (value), "=r" (tmp)); /* BADC      */
#endif
    return value;
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

#endif /* ARM_ARCH */

static inline uint32_t swaw32_hw(uint32_t value)
{
    /*
      result[31..16] = value[15.. 0];
      result[15.. 0] = value[31..16];
    */
#ifdef __thumb__
    asm (
        "ror %0, %1"
        : "+l"(value) : "l"(16));
    return value;
#else
    uint32_t retval;
    asm (
        "mov %0, %1, ror #16"
        : "=r"(retval) : "r"(value));
    return retval;
#endif

}

#if defined(CPU_TCC780X) || defined(CPU_TCC77X) /* Single core only for now */ \
|| CONFIG_CPU == IMX31L || CONFIG_CPU == DM320 || CONFIG_CPU == AS3525 \
|| CONFIG_CPU == S3C2440 || CONFIG_CPU == S5L8701 || CONFIG_CPU == AS3525v2 \
|| CONFIG_CPU == S5L8702
/* Use the generic ARMv4/v5/v6 wait for IRQ */
static inline void core_sleep(void)
{
    asm volatile (
        "mcr p15, 0, %0, c7, c0, 4 \n" /* Wait for interrupt */
#if CONFIG_CPU == IMX31L
        "nop\n nop\n nop\n nop\n nop\n" /* Clean out the pipes */
#endif
        : : "r"(0)
    );
    enable_irq();
}
#else
/* Skip this if special code is required and implemented */
#if !(defined(CPU_PP)) && CONFIG_CPU != RK27XX && CONFIG_CPU != IMX233
static inline void core_sleep(void)
{
    /* TODO: core_sleep not implemented, battery life will be decreased */
    enable_irq();
}
#endif /* CPU_PP */
#endif

#endif /* SYSTEM_ARM_H */
