/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 by Aidan MacDonald
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
#ifndef SYSTEM_ARM_MICRO_H
#define SYSTEM_ARM_MICRO_H

#define IRQ_ENABLED         0x00
#define IRQ_DISABLED        0x01
#define IRQ_STATUS          0x01
#define HIGHEST_IRQ_LEVEL   IRQ_DISABLED

#define disable_irq_save() \
    set_irq_level(IRQ_DISABLED)

/* For compatibility with ARM classic */
#define CPU_MODE_THREAD_CONTEXT 0

#define is_thread_context() \
    (get_interrupt_number() == CPU_MODE_THREAD_CONTEXT)

/* Assert that the processor is in the desired execution mode
 * mode:       Processor mode value to test for
 * rstatus...: Ignored; for compatibility with ARM classic only.
 */
#define ASSERT_CPU_MODE(mode, rstatus...) \
    ({ unsigned long __massert = (mode);                         \
       unsigned long __mproc = get_interrupt_number();           \
       if (__mproc != __massert)                                 \
           panicf("Incorrect CPU mode in %s (0x%02lx!=0x%02lx)", \
                  __func__, __mproc, __massert); })

/* Core-level interrupt masking */

static inline int set_irq_level(int primask)
{
    int oldvalue;

    asm volatile ("mrs %0, primask\n"
                  "msr primask, %1\n"
                  : "=r"(oldvalue) : "r"(primask));

    return oldvalue;
}

static inline void restore_irq(int primask)
{
    asm volatile ("msr primask, %0" :: "r"(primask));
}

static inline void enable_irq(void)
{
    asm volatile ("cpsie i");
}

static inline void disable_irq(void)
{
    asm volatile ("cpsid i");
}

static inline bool irq_enabled(void)
{
    int primask;
    asm volatile ("mrs %0, primask" : "=r"(primask));

    return !(primask & 1);
}

static inline unsigned long get_interrupt_number(void)
{
    unsigned long ipsr;
    asm volatile ("mrs %0, ipsr" : "=r"(ipsr));

    return ipsr & 0x1ff;
}

static inline void core_sleep(void)
{
    asm volatile ("wfi");
    enable_irq();
}

#endif /* SYSTEM_ARM_MICRO_H */
