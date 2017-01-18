/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#ifndef __HWSTUB_SYSTEM__
#define __HWSTUB_SYSTEM__

#ifdef ARM_ARCH
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

#ifndef __ASSEMBLER__
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
#endif /* __ASSEMBLER__ */
#endif /* ARM_ARCH */

/* Save the current context into a local buffer and return 0.
 * When an exception occurs, typically read/write at invalid address or invalid
 * instructions (the exact exceptions caught depend on the architecture), it will
 * restore the context to what it was when the function was called except that
 * it returns a nonzero value describing the error */
#define EXCEPTION_NONE  0   /* no exception, returned on the first call */
#define EXCEPTION_UNSP  1   /* some unspecified exception occured */
#define EXCEPTION_ADDR  2   /* read/write at an invalid address */
#define EXCEPTION_INSTR 3   /* invalid instruction */

#ifndef __ASSEMBLER__
int set_exception_jmp(void);
#endif /* __ASSEMBLER__ */

#endif /* __HWSTUB_SYSTEM__ */
