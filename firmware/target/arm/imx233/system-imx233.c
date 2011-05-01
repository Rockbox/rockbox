/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by amaury Pouly
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

#include "kernel.h"
#include "system.h"
#include "gcc_extensions.h"
#include "system-target.h"
#include "cpu.h"
#include "clkctrl-imx233.h"
#include "pinctrl-imx233.h"
#include "timrot-imx233.h"
#include "lcd.h"
#include "backlight-target.h"

#define HW_POWER_BASE           0x80044000

#define HW_POWER_RESET          (*(volatile uint32_t *)(HW_POWER_BASE + 0x100))
#define HW_POWER_RESET__UNLOCK  0x3E770000
#define HW_POWER_RESET__PWD     0x1

#define HW_ICOLL_BASE           0x80000000

#define HW_ICOLL_VECTOR         (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x0))

#define HW_ICOLL_LEVELACK       (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x10))
#define HW_ICOLL_LEVELACK__LEVEL0   0x1

#define HW_ICOLL_CTRL           (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x20))
#define HW_ICOLL_CTRL__IRQ_FINAL_ENABLE (1 << 16)
#define HW_ICOLL_CTRL__ARM_RSE_MODE     (1 << 18)

#define HW_ICOLL_VBASE          (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x40))
#define HW_ICOLL_INTERRUPT(i)   (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x120 + (i) * 0x10))
#define HW_ICOLL_INTERRUPT__PRIORITY_BM 0x3
#define HW_ICOLL_INTERRUPT__ENABLE      0x4
#define HW_ICOLL_INTERRUPT__SOFTIRQ     0x8
#define HW_ICOLL_INTERRUPT__ENFIQ       0x10

#define default_interrupt(name) \
    extern __attribute__((weak, alias("UIRQ"))) void name(void)

static void UIRQ (void) __attribute__((interrupt ("IRQ")));
void irq_handler(void) __attribute__((interrupt("IRQ")));
void fiq_handler(void) __attribute__((interrupt("FIQ")));

default_interrupt(INT_USB_CTRL);
default_interrupt(INT_TIMER0);
default_interrupt(INT_TIMER1);
default_interrupt(INT_TIMER2);
default_interrupt(INT_TIMER3);
default_interrupt(INT_LCDIF_DMA);
default_interrupt(INT_LCDIF_ERROR);

typedef void (*isr_t)(void);

static isr_t isr_table[INT_SRC_NR_SOURCES] =
{
    [INT_SRC_USB_CTRL] = INT_USB_CTRL,
    [INT_SRC_TIMER(0)] = INT_TIMER0,
    [INT_SRC_TIMER(1)] = INT_TIMER1,
    [INT_SRC_TIMER(2)] = INT_TIMER2,
    [INT_SRC_TIMER(3)] = INT_TIMER3,
    [INT_SRC_LCDIF_DMA] = INT_LCDIF_DMA,
    [INT_SRC_LCDIF_ERROR] = INT_LCDIF_ERROR,
};

static void UIRQ(void)
{
    panicf("Unhandled IRQ %02X",
        (unsigned int)(HW_ICOLL_VECTOR - (uint32_t)isr_table) / 4);
}

void irq_handler(void)
{
    HW_ICOLL_VECTOR = HW_ICOLL_VECTOR; /* notify icoll that we entered ISR */
    (*(isr_t *)HW_ICOLL_VECTOR)();
    /* acknowledge completion of IRQ (all use the same priority 0 */
    HW_ICOLL_LEVELACK = HW_ICOLL_LEVELACK__LEVEL0;
}

void fiq_handler(void)
{
}

static void imx233_chip_reset(void)
{
    HW_CLKCTRL_RESET = HW_CLKCTRL_RESET_CHIP;
}

void system_reboot(void)
{
    _backlight_off();

    disable_irq();

    /* use watchdog to reset */
    imx233_chip_reset();
    while(1);
}

void system_exception_wait(void)
{
    /* what is this supposed to do ? */
}

void imx233_enable_interrupt(int src, bool enable)
{
    if(enable)
        __REG_SET(HW_ICOLL_INTERRUPT(src)) = HW_ICOLL_INTERRUPT__ENABLE;
    else
        __REG_CLR(HW_ICOLL_INTERRUPT(src)) = HW_ICOLL_INTERRUPT__ENABLE;
}

void imx233_softirq(int src, bool enable)
{
    if(enable)
        __REG_SET(HW_ICOLL_INTERRUPT(src)) = HW_ICOLL_INTERRUPT__SOFTIRQ;
    else
        __REG_CLR(HW_ICOLL_INTERRUPT(src)) = HW_ICOLL_INTERRUPT__SOFTIRQ;
}

void system_init(void)
{
    /* disable all interrupts */
    for(int i = 0; i < INT_SRC_NR_SOURCES; i++)
    {
        /* priority = 0, disable, disable fiq */
        HW_ICOLL_INTERRUPT(i) = 0;
    }
    /* setup vbase as isr_table */
    HW_ICOLL_VBASE = (uint32_t)&isr_table;
    /* enable final irq bit */
    __REG_SET(HW_ICOLL_CTRL) = HW_ICOLL_CTRL__IRQ_FINAL_ENABLE;

    imx233_pinctrl_init();
    imx233_timrot_init();
}

void power_off(void)
{
    /* power down */
    HW_POWER_RESET = HW_POWER_RESET__UNLOCK | HW_POWER_RESET__PWD;
    while(1);
}

bool imx233_us_elapsed(uint32_t ref, unsigned us_delay)
{
    uint32_t cur = HW_DIGCTL_MICROSECONDS;
    if(ref + us_delay <= ref)
        return !(cur > ref) && !(cur < (ref + us_delay));
    else
        return (cur < ref) || cur >= (ref + us_delay);
}

void imx233_reset_block(volatile uint32_t *block_reg)
{
    __REG_CLR(*block_reg) = __BLOCK_SFTRST;
    while(*block_reg & __BLOCK_SFTRST);
    __REG_CLR(*block_reg) = __BLOCK_CLKGATE;
    __REG_SET(*block_reg) = __BLOCK_SFTRST;
    while(!(*block_reg & __BLOCK_CLKGATE));
    __REG_CLR(*block_reg) = __BLOCK_SFTRST;
    while(*block_reg & __BLOCK_SFTRST);
    __REG_CLR(*block_reg) = __BLOCK_CLKGATE;
    while(*block_reg & __BLOCK_CLKGATE);
}

void udelay(unsigned us)
{
    uint32_t ref = HW_DIGCTL_MICROSECONDS;
    while(!imx233_us_elapsed(ref, us));
}

