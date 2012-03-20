/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: system-s5l8700.c 28935 2010-12-30 20:23:46Z Buschel $
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
#include "pmu-target.h"

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIRQ"))) void name (void)

void irq_handler(void) __attribute__((interrupt ("IRQ"), naked));
void fiq_handler(void) __attribute__((interrupt ("FIQ"), naked, \
                                      weak, alias("fiq_dummy")));

default_interrupt(INT_IRQ0);
default_interrupt(INT_IRQ1);
default_interrupt(INT_IRQ2);
default_interrupt(INT_IRQ3);
default_interrupt(INT_IRQ4);
default_interrupt(INT_IRQ5);
default_interrupt(INT_IRQ6);
default_interrupt(INT_IRQ7);
default_interrupt(INT_TIMERA);
default_interrupt(INT_TIMERB);
default_interrupt(INT_TIMERC);
default_interrupt(INT_TIMERD);
default_interrupt(INT_TIMERE);
default_interrupt(INT_TIMERF);
default_interrupt(INT_TIMERG);
default_interrupt(INT_TIMERH);
default_interrupt(INT_IRQ9);
default_interrupt(INT_IRQ10);
default_interrupt(INT_IRQ11);
default_interrupt(INT_IRQ12);
default_interrupt(INT_IRQ13);
default_interrupt(INT_IRQ14);
default_interrupt(INT_IRQ15);
default_interrupt(INT_DMAC0C0);
default_interrupt(INT_DMAC0C1);
default_interrupt(INT_DMAC0C2);
default_interrupt(INT_DMAC0C3);
default_interrupt(INT_DMAC0C4);
default_interrupt(INT_DMAC0C5);
default_interrupt(INT_DMAC0C6);
default_interrupt(INT_DMAC0C7);
default_interrupt(INT_DMAC1C0);
default_interrupt(INT_DMAC1C1);
default_interrupt(INT_DMAC1C2);
default_interrupt(INT_DMAC1C3);
default_interrupt(INT_DMAC1C4);
default_interrupt(INT_DMAC1C5);
default_interrupt(INT_DMAC1C6);
default_interrupt(INT_DMAC1C7);
default_interrupt(INT_IRQ18);
default_interrupt(INT_USB_FUNC);
default_interrupt(INT_IRQ20);
default_interrupt(INT_IRQ21);
default_interrupt(INT_IRQ22);
default_interrupt(INT_WHEEL);
default_interrupt(INT_IRQ24);
default_interrupt(INT_IRQ25);
default_interrupt(INT_IRQ26);
default_interrupt(INT_IRQ27);
default_interrupt(INT_IRQ28);
default_interrupt(INT_ATA);
default_interrupt(INT_IRQ30);
default_interrupt(INT_IRQ31);
default_interrupt(INT_IRQ32);
default_interrupt(INT_IRQ33);
default_interrupt(INT_IRQ34);
default_interrupt(INT_IRQ35);
default_interrupt(INT_IRQ36);
default_interrupt(INT_IRQ37);
default_interrupt(INT_IRQ38);
default_interrupt(INT_IRQ39);
default_interrupt(INT_IRQ40);
default_interrupt(INT_IRQ41);
default_interrupt(INT_IRQ42);
default_interrupt(INT_IRQ43);
default_interrupt(INT_MMC);
default_interrupt(INT_IRQ45);
default_interrupt(INT_IRQ46);
default_interrupt(INT_IRQ47);
default_interrupt(INT_IRQ48);
default_interrupt(INT_IRQ49);
default_interrupt(INT_IRQ50);
default_interrupt(INT_IRQ51);
default_interrupt(INT_IRQ52);
default_interrupt(INT_IRQ53);
default_interrupt(INT_IRQ54);
default_interrupt(INT_IRQ55);
default_interrupt(INT_IRQ56);
default_interrupt(INT_IRQ57);
default_interrupt(INT_IRQ58);
default_interrupt(INT_IRQ59);
default_interrupt(INT_IRQ60);
default_interrupt(INT_IRQ61);
default_interrupt(INT_IRQ62);
default_interrupt(INT_IRQ63);


static int current_irq;


void INT_TIMER(void) ICODE_ATTR;
void INT_TIMER()
{
    if (TACON & (TACON >> 4) & 0x7000) INT_TIMERA();
    if (TBCON & (TBCON >> 4) & 0x7000) INT_TIMERB();
    if (TCCON & (TCCON >> 4) & 0x7000) INT_TIMERC();
    if (TDCON & (TDCON >> 4) & 0x7000) INT_TIMERD();
    if (TFCON & (TFCON >> 4) & 0x7000) INT_TIMERF();
    if (TGCON & (TGCON >> 4) & 0x7000) INT_TIMERG();
    if (THCON & (THCON >> 4) & 0x7000) INT_TIMERH();
}

void INT_DMAC0(void) ICODE_ATTR;
void INT_DMAC0()
{
    uint32_t intsts = DMAC0INTSTS;
    if (intsts & 1) INT_DMAC0C0();
    if (intsts & 2) INT_DMAC0C1();
    if (intsts & 4) INT_DMAC0C2();
    if (intsts & 8) INT_DMAC0C3();
    if (intsts & 0x10) INT_DMAC0C4();
    if (intsts & 0x20) INT_DMAC0C5();
    if (intsts & 0x40) INT_DMAC0C6();
    if (intsts & 0x80) INT_DMAC0C7();
}

void INT_DMAC1(void) ICODE_ATTR;
void INT_DMAC1()
{
    uint32_t intsts = DMAC1INTSTS;
    if (intsts & 1) INT_DMAC1C0();
    if (intsts & 2) INT_DMAC1C1();
    if (intsts & 4) INT_DMAC1C2();
    if (intsts & 8) INT_DMAC1C3();
    if (intsts & 0x10) INT_DMAC1C4();
    if (intsts & 0x20) INT_DMAC1C5();
    if (intsts & 0x40) INT_DMAC1C6();
    if (intsts & 0x80) INT_DMAC1C7();
}

static void (* const irqvector[])(void) =
{
    INT_IRQ0,INT_IRQ1,INT_IRQ2,INT_IRQ3,INT_IRQ4,INT_IRQ5,INT_IRQ6,INT_IRQ7,
    INT_TIMER,INT_IRQ9,INT_IRQ10,INT_IRQ11,INT_IRQ12,INT_IRQ13,INT_IRQ14,INT_IRQ15,
    INT_DMAC0,INT_DMAC1,INT_IRQ18,INT_USB_FUNC,INT_IRQ20,INT_IRQ21,INT_IRQ22,INT_WHEEL,
    INT_IRQ24,INT_IRQ25,INT_IRQ26,INT_IRQ27,INT_IRQ28,INT_ATA,INT_IRQ30,INT_IRQ31,
    INT_IRQ32,INT_IRQ33,INT_IRQ34,INT_IRQ35,INT_IRQ36,INT_IRQ37,INT_IRQ38,INT_IRQ39,
    INT_IRQ40,INT_IRQ41,INT_IRQ42,INT_IRQ43,INT_MMC,INT_IRQ45,INT_IRQ46,INT_IRQ47,
    INT_IRQ48,INT_IRQ49,INT_IRQ50,INT_IRQ51,INT_IRQ52,INT_IRQ53,INT_IRQ54,INT_IRQ55,
    INT_IRQ56,INT_IRQ57,INT_IRQ58,INT_IRQ59,INT_IRQ60,INT_IRQ61,INT_IRQ62,INT_IRQ63
};

static void UIRQ(void)
{
    panicf("Unhandled IRQ %d!", current_irq);
}

void irq_handler(void)
{
    /*
     * Based on: linux/arch/arm/kernel/entry-armv.S and system-meg-fx.c
     */

    asm volatile(   "stmfd sp!, {r0-r7, ip, lr} \n"   /* Store context */
                    "sub   sp, sp, #8           \n"); /* Reserve stack */

    void* dummy = VIC0ADDRESS;
    dummy = VIC1ADDRESS;
    uint32_t irqs0 = VIC0IRQSTATUS;
    uint32_t irqs1 = VIC1IRQSTATUS;
    for (current_irq = 0; irqs0; current_irq++, irqs0 >>= 1)
        if (irqs0 & 1)
            irqvector[current_irq]();
    for (current_irq = 32; irqs1; current_irq++, irqs1 >>= 1)
        if (irqs1 & 1)
            irqvector[current_irq]();
    VIC0ADDRESS = NULL;
    VIC1ADDRESS = NULL;
        
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
    pmu_init();
    VIC0INTENABLE = 1 << IRQ_WHEEL;
    VIC0INTENABLE = 1 << IRQ_ATA;
    VIC1INTENABLE = 1 << (IRQ_MMC - 32);
}

void system_reboot(void)
{
    /* Reset the SoC */
    asm volatile("msr CPSR_c, #0xd3   \n"
                 "mov r0, #0x100000   \n"
                 "mov r1, #0x3c800000 \n"
                 "str r0, [r1]        \n");

    /* Wait for reboot to kick in */
    while(1);
}

//extern void post_mortem_stub(void);

void system_exception_wait(void)
{
//    post_mortem_stub();
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

    /*
     * CPU scaling parameters:
     * CPUFREQ_MAX:    CPU = 216MHz, AHB = 108MHz, Vcore = 1.200V
     * CPUFREQ_NORMAL: CPU =  54MHz, AHB =  54MHz, Vcore = 1.050V
     *
     * CLKCON0 sets PLL2->FCLK divider (CPU clock)
     * CLKCON1 sets FCLK->HCLK divider (AHB clock)
     *
     * HCLK is derived from FCLK, the system goes unstable if HCLK
     * is out of the range 54-108 MHz, so two stages are required to
     * switch FCLK (216 MHz <-> 54 MHz), adjusting HCLK in between
     * to ensure system stability.
     */
    if (frequency == CPUFREQ_MAX)
    {
        /* Vcore = 1.200V */
        pmu_write(0x1e, 0x17);

        /* FCLK = PLL2 / 2  (FCLK = 108MHz, HCLK = 108MHz) */
        CLKCON0 = 0x3011;
        udelay(50);

        /* HCLK = FCLK / 2  (HCLK = 54MHz) */
        CLKCON1 = 0x404101;
        udelay(50);

        /* FCLK = PLL2  (FCLK = 216MHz, HCLK = 108MHz) */
        CLKCON0 = 0x3000;
        udelay(100);
    }
    else
    {
        /* FCLK = PLL2 / 2  (FCLK = 108MHz, HCLK = 54MHz) */
        CLKCON0 = 0x3011;
        udelay(50);

        /* HCLK = FCLK  (HCLK = 108MHz) */
        CLKCON1 = 0x4001;
        udelay(50);

        /* FCLK = PLL2 / 4  (FCLK = 54MHz, HCLK = 54MHz) */
        CLKCON0 = 0x3013;
        udelay(100);

        /* Vcore = 1.050V */
        pmu_write(0x1e, 0x11);
    }

    cpu_frequency = frequency;
}

#endif
