/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Marcin Bukat
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
#include "panic.h"
#include "system-target.h"

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIRQ"))) void name (void)

void irq_handler(void) __attribute__((interrupt ("IRQ"), naked));
void fiq_handler(void) __attribute__((interrupt ("FIQ"), naked, \
                                      weak, alias("fiq_dummy")));

default_interrupt(INT_UART0);
default_interrupt(INT_UART1);
default_interrupt(INT_TIMER0);
default_interrupt(INT_TIMER1);
default_interrupt(INT_TIMER2);
default_interrupt(INT_GPIO0);
default_interrupt(INT_SW_INT0);
default_interrupt(INT_AHB0_MAILBOX);
default_interrupt(INT_RTC);
default_interrupt(INT_SCU);
default_interrupt(INT_SD);
default_interrupt(INT_SPI);
default_interrupt(INT_HDMA);
default_interrupt(INT_A2A_BRIDGE);
default_interrupt(INT_I2C);
default_interrupt(INT_I2S);
default_interrupt(INT_UDC);
default_interrupt(INT_UHC);
default_interrupt(INT_PWM0);
default_interrupt(INT_PWM1);
default_interrupt(INT_PWM2);
default_interrupt(INT_ADC);
default_interrupt(INT_GPIO1);
default_interrupt(INT_VIP);
default_interrupt(INT_DWDMA);
default_interrupt(INT_NANDC);
default_interrupt(INT_LCDC);
default_interrupt(INT_DSP);
default_interrupt(INT_SW_INT1);
default_interrupt(INT_SW_INT2);
default_interrupt(INT_SW_INT3);

static void (* const irqvector[])(void) =
{
    INT_UART0,INT_UART1,INT_TIMER0,INT_TIMER1,INT_TIMER2,INT_GPIO0,INT_SW_INT0,INT_AHB0_MAILBOX,
    INT_RTC,INT_SCU,INT_SD,INT_SPI,INT_HDMA,INT_A2A_BRIDGE,INT_I2C,
    INT_I2S,INT_UDC,INT_UHC,INT_PWM0,INT_PWM1,INT_PWM2,INT_ADC,INT_GPIO1,
    INT_VIP,INT_DWDMA,INT_NANDC,INT_LCDC,INT_DSP,INT_SW_INT1,INT_SW_INT2,INT_SW_INT3
};

static const char * const irqname[] =
{
    "INT_UART0","INT_UART1","INT_TIMER0","INT_TIMER1","INT_TIMER2","INT_GPIO0","INT_SW_INT0","INT_AHB0_MAILBOX",
    "INT_RTC","INT_SCU","INT_SD","INT_SPI","INT_HDMA","INT_A2A_BRIDGE","INT_I2C",
    "INT_I2S","INT_UDC","INT_UHC","INT_PWM0","INT_PWM1","INT_PWM2","INT_ADC","INT_GPIO1",
    "INT_VIP","INT_DWDMA","INT_NANDC","INT_LCDC","INT_DSP","INT_SW_INT1","INT_SW_INT2","INT_SW_INT3"
};

static void UIRQ(void)
{
    unsigned int offset = INTC_ISR & 0x1f;
    panicf("Unhandled IRQ %02X: %s", offset, irqname[offset]);
}

void irq_handler(void)
{
    /*
     * Based on: linux/arch/arm/kernel/entry-armv.S and system-meg-fx.c
     */

    asm volatile(   "stmfd sp!, {r0-r7, ip, lr} \n"   /* Store context */
                    "sub   sp, sp, #8           \n"); /* Reserve stack */

    int irq_no = INTC_ISR & 0x1f;
    
    irqvector[irq_no]();

    /* clear interrupt */
    INTC_ICCR = (1 << irq_no);

    asm volatile(   "add   sp, sp, #8           \n"   /* Cleanup stack   */
                    "ldmfd sp!, {r0-r7, ip, lr} \n"   /* Restore context */
                    "subs  pc, lr, #4           \n"); /* Return from IRQ */
}

void fiq_dummy(void)
{
    asm volatile (
        "subs   pc, lr, #4   \r\n"
    );
}


void system_init(void)
{
    return;
}

/* not tested */
void system_reboot(void)
{
    /* use Watchdog to reset */
    WDTLR = 1;
    WDTCON = (1<<4) | (1<<3);

    /* Wait for reboot to kick in */
    while(1);
}

void system_exception_wait(void)
{
    while(1);
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

/* usecs may be at most 2^32/200 (~21 seconds) for 200MHz max cpu freq */
void udelay(unsigned usecs)
{
    unsigned cycles_per_usec;
    unsigned delay;

    if (cpu_frequency == CPUFREQ_MAX) {
        cycles_per_usec = (CPUFREQ_MAX + 999999) / 1000000;
    } else {
        cycles_per_usec = (CPUFREQ_NORMAL + 999999) / 1000000;
    }

    delay = (usecs * cycles_per_usec + 3) / 4;

    asm volatile(
        "1: subs %0, %0, #1  \n"    /* 1 cycle  */
        "   bne  1b          \n"    /* 3 cycles */
        : : "r"(delay)
    );
}

