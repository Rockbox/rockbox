/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Rob Purchase
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
#ifdef IPOD_NANO2G
#include "storage.h"
#include "pmu-target.h"
#endif

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIRQ"))) void name (void)

void irq_handler(void) __attribute__((interrupt ("IRQ"), naked));
void fiq_handler(void) __attribute__((interrupt ("FIQ"), naked, \
                                      weak, alias("fiq_dummy")));

default_interrupt(EXT0);
default_interrupt(EXT1);
default_interrupt(EXT2);
default_interrupt(EINT_VBUS);
default_interrupt(EINTG);
default_interrupt(INT_TIMERA);
default_interrupt(INT_WDT);
default_interrupt(INT_TIMERB);
default_interrupt(INT_TIMERC);
default_interrupt(INT_TIMERD);
default_interrupt(INT_DMA);
default_interrupt(INT_ALARM_RTC);
default_interrupt(INT_PRI_RTC);
default_interrupt(RESERVED1);
default_interrupt(INT_UART);
default_interrupt(INT_USB_HOST);
default_interrupt(INT_USB_FUNC);
default_interrupt(INT_LCDC_0);
default_interrupt(INT_LCDC_1);
default_interrupt(INT_ECC);
default_interrupt(INT_CALM);
default_interrupt(INT_ATA);
default_interrupt(INT_UART0);
default_interrupt(INT_SPDIF_OUT);
default_interrupt(INT_SDCI);
default_interrupt(INT_LCD);
default_interrupt(INT_WHEEL);
default_interrupt(INT_IIC);
default_interrupt(RESERVED2);
default_interrupt(INT_MSTICK);
default_interrupt(INT_ADC_WAKEUP);
default_interrupt(INT_ADC);
default_interrupt(INT_UNK1);
default_interrupt(INT_UNK2);
default_interrupt(INT_UNK3);


void INT_TIMER(void)
{
    if (TACON & 0x00038000) INT_TIMERA();
    if (TBCON & 0x00038000) INT_TIMERB();
    if (TCCON & 0x00038000) INT_TIMERC();
    if (TDCON & 0x00038000) INT_TIMERD();
}


#if CONFIG_CPU==S5L8701
static void (* const irqvector[])(void) =
{ /* still 90% unverified and probably incorrect */
    EXT0,EXT1,EXT2,EINT_VBUS,EINTG,INT_TIMER,INT_WDT,INT_UNK1,
    INT_UNK2,INT_UNK3,INT_DMA,INT_ALARM_RTC,INT_PRI_RTC,RESERVED1,INT_UART,INT_USB_HOST,
    INT_USB_FUNC,INT_LCDC_0,INT_LCDC_1,INT_CALM,INT_ATA,INT_UART0,INT_SPDIF_OUT,INT_ECC,
    INT_SDCI,INT_LCD,INT_WHEEL,INT_IIC,RESERVED2,INT_MSTICK,INT_ADC_WAKEUP,INT_ADC
};
#else
static void (* const irqvector[])(void) =
{
    EXT0,EXT1,EXT2,EINT_VBUS,EINTG,INT_TIMERA,INT_WDT,INT_TIMERB,
    INT_TIMERC,INT_TIMERD,INT_DMA,INT_ALARM_RTC,INT_PRI_RTC,RESERVED1,INT_UART,INT_USB_HOST,
    INT_USB_FUNC,INT_LCDC_0,INT_LCDC_1,INT_ECC,INT_CALM,INT_ATA,INT_UART0,INT_SPDIF_OUT,
    INT_SDCI,INT_LCD,INT_WHEEL,INT_IIC,RESERVED2,INT_MSTICK,INT_ADC_WAKEUP,INT_ADC
};
#endif

#if CONFIG_CPU==S5L8701
static const char * const irqname[] =
{ /* still 90% unverified and probably incorrect */
    "EXT0","EXT1","EXT2","EINT_VBUS","EINTG","INT_TIMER","INT_WDT","INT_UNK1",
    "INT_UNK2","INT_UNK3","INT_DMA","INT_ALARM_RTC","INT_PRI_RTC","Reserved","INT_UART","INT_USB_HOST",
    "INT_USB_FUNC","INT_LCDC_0","INT_LCDC_1","INT_CALM","INT_ATA","INT_UART0","INT_SPDIF_OUT","INT_ECC",
    "INT_SDCI","INT_LCD","INT_WHEEL","INT_IIC","Reserved","INT_MSTICK","INT_ADC_WAKEUP","INT_ADC"
};
#else
static const char * const irqname[] =
{
    "EXT0","EXT1","EXT2","EINT_VBUS","EINTG","INT_TIMERA","INT_WDT","INT_TIMERB",
    "INT_TIMERC","INT_TIMERD","INT_DMA","INT_ALARM_RTC","INT_PRI_RTC","Reserved","INT_UART","INT_USB_HOST",
    "INT_USB_FUNC","INT_LCDC_0","INT_LCDC_1","INT_ECC","INT_CALM","INT_ATA","INT_UART0","INT_SPDIF_OUT",
    "INT_SDCI","INT_LCD","INT_WHEEL","INT_IIC","Reserved","INT_MSTICK","INT_ADC_WAKEUP","INT_ADC"
};
#endif

static void UIRQ(void)
{
    unsigned int offset = INTOFFSET;
    panicf("Unhandled IRQ %02X: %s", offset, irqname[offset]);
}

void irq_handler(void)
{
    /*
     * Based on: linux/arch/arm/kernel/entry-armv.S and system-meg-fx.c
     */

    asm volatile(   "stmfd sp!, {r0-r7, ip, lr} \n"   /* Store context */
                    "sub   sp, sp, #8           \n"); /* Reserve stack */

    int irq_no = INTOFFSET;
    
    irqvector[irq_no]();

    /* clear interrupt */
    SRCPND = (1 << irq_no);
    INTPND = INTPND;
    
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
#ifdef IPOD_NANO2G
    pmu_init();
#endif
}

void system_reboot(void)
{
#ifdef IPOD_NANO2G
#ifdef HAVE_STORAGE_FLUSH
    storage_flush();
#endif

    /* Reset the SoC */
    asm volatile("msr CPSR_c, #0xd3    \n"
                 "mov r5, #0x110000    \n"
                 "add r5, r5, #0xff    \n"
                 "add r6, r5, #0xa00   \n"
                 "mov r10, #0x3c800000 \n"
                 "str r6, [r10]        \n"
                 "mov r6, #0xff0       \n"
                 "str r6, [r10,#4]     \n"
                 "str r5, [r10]        \n");

    /* Wait for reboot to kick in */
    while(1);
#endif
}

extern void post_mortem_stub(void);

void system_exception_wait(void)
{
    post_mortem_stub();
    while(1);
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ

void set_cpu_frequency(long frequency)
{
    if (cpu_frequency == frequency)
        return;

    if (frequency == CPUFREQ_MAX)
    {
        /* Vcore = 1.000V */
        pmu_write(0x1e, 0xf);
        /* Allow for voltage to stabilize */
        udelay(100);
        /* FCLK_CPU = PLL0, HCLK = PLL0 / 2 */
        CLKCON = (CLKCON & ~0xFF00FF00) | 0x20003100;
        /* PCLK = HCLK / 2 */
        CLKCON2 |= 0x200;
        /* Switch to ASYNCHRONOUS mode => GCLK = FCLK_CPU */
        asm volatile(
            "mrc     p15, 0, r0,c1,c0    \n\t"
            "orr     r0, r0, #0xc0000000 \n\t"
            "mcr     p15, 0, r0,c1,c0    \n\t"
            ::: "r0"
          );
    }
    else
    {
        /* Switch to FASTBUS mode => GCLK = HCLK */
        asm volatile(
            "mrc     p15, 0, r0,c1,c0    \n\t"
            "bic     r0, r0, #0xc0000000 \n\t"
            "mcr     p15, 0, r0,c1,c0    \n\t"
            ::: "r0"
          );
        /* PCLK = HCLK */
        CLKCON2 &= ~0x200;
        /* FCLK_CPU = OFF, HCLK = PLL0 / 4 */
        CLKCON = (CLKCON & ~0xFF00FF00) | 0x80003300;
        /* Vcore = 0.900V */
        pmu_write(0x1e, 0xb);
    }

    cpu_frequency = frequency;
}

#endif
