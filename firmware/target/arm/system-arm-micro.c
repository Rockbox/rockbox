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
#include "config.h"
#include "system.h"

struct armv7m_exception_frame
{
    unsigned long r0;
    unsigned long r1;
    unsigned long r2;
    unsigned long r3;
    unsigned long r12;
    unsigned long lr;
    unsigned long pc;
    unsigned long xpsr;
};

void UIE(void)
{
    while (1);
}

void __div0(void)
{
    while (1);
}

void __aeabi_idiv0(void) __attribute__((alias("__div0")));
void __aeabi_ldiv0(void) __attribute__((alias("__div0")));

#define ATTR_IRQ_HANDLER __attribute__((weak, alias("UIE")))

void nmi_handler(void) ATTR_IRQ_HANDLER;
void hardfault_handler(void) ATTR_IRQ_HANDLER;
void pendsv_handler(void) ATTR_IRQ_HANDLER;
void svcall_handler(void) ATTR_IRQ_HANDLER;
void systick_handler(void) ATTR_IRQ_HANDLER;

#if ARCH_VERSION >= 7
void memmanage_handler(void) ATTR_IRQ_HANDLER;
void busfault_handler(void) ATTR_IRQ_HANDLER;
void usagefault_handler(void) ATTR_IRQ_HANDLER;
void debugmonitor_handler(void) ATTR_IRQ_HANDLER;
#endif

#if ARCH_VERSION >= 8
void securefault_handler(void) ATTR_IRQ_HANDLER;
#endif

/*
 * The weak alias attribute above only works if the alias is defined
 * in the same translation unit. So each platform should declare its
 * IRQ handlers in a file and include it below, so that unused ones
 * can be aliased to UIE.
 */
#if CONFIG_CPU == STM32H743
# include "irqhandlers-stm32h743.c"
#endif
