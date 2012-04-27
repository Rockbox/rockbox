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
default_interrupt(INT_PWM3);
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
    INT_RTC,INT_SCU,INT_SD,INT_SPI,INT_HDMA,INT_A2A_BRIDGE,INT_I2C,INT_I2S,
    INT_UDC,INT_UHC,INT_PWM0,INT_PWM1,INT_PWM2,INT_PWM3,INT_ADC,INT_GPIO1,
    INT_VIP,INT_DWDMA,INT_NANDC,INT_LCDC,INT_DSP,INT_SW_INT1,INT_SW_INT2,INT_SW_INT3
};

static const char * const irqname[] =
{
    "INT_UART0","INT_UART1","INT_TIMER0","INT_TIMER1","INT_TIMER2","INT_GPIO0","INT_SW_INT0","INT_AHB0_MAILBOX",
    "INT_RTC","INT_SCU","INT_SD","INT_SPI","INT_HDMA","INT_A2A_BRIDGE","INT_I2C","INT_I2S",
    "INT_UDC","INT_UHC","INT_PWM0","INT_PWM1","INT_PWM2","INT_PWM3","INT_ADC","INT_GPIO1",
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
#ifndef BOOTLOADER
    /* SDRAM tweaks */
    MCSDR_MODE = (2<<4)|3;         /* CAS=2, burst=8 */
    MCSDR_T_REF = (125*100) >> 3;  /* 125/8 = 15.625 autorefresh interval */
    MCSDR_T_RFC = (64*100) / 1000; /* autorefresh period */
    MCSDR_T_RP = 1;                /* precharge period */
    MCSDR_T_RCD = 1;               /* active to RD/WR delay */

    /* turn off clock for unused modules */
    SCU_CLKCFG |= (1<<31) |        /* WDT pclk */
                  (1<<30) |        /* RTC pclk */
                  (1<<26) |        /* HS_ADC clock */
                  (1<<25) |        /* HS_ADC HCLK */
                  (1<<21) |        /* SPI clock */
                  (1<<19) |        /* UART1 clock */
                  (1<<18) |        /* UART0 clock */
                  (1<<15) |        /* VIP clock */
                  (1<<14) |        /* VIP HCLK */
                  (1<<13) |        /* LCDC clock */
                   (1<<9) |        /* NAND HCLK */
                   (1<<5) |        /* USB host HCLK */
                   (1<<1) |        /* DSP clock */
                   (1<<0);         /* OTP clock (dunno what it is */

    /* turn off DSP pll */
    SCU_PLLCON2 |= (1<<22);

    /* turn off codec pll */
    SCU_PLLCON3 |= (1<<22);
#endif
}

/* not tested */
void system_reboot(void)
{
    /* use Watchdog to reset */
    SCU_CLKCFG &= ~(1<<31);
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

void commit_discard_idcache(void)
{
    /* invalidate cache way 0 */
    CACHEOP = 0x02;

    /* wait for invalidate process to complete */
    while (CACHEOP & 0x03);

    /* invalidate cache way 1 */
    CACHEOP = 0x80000002;

    /* wait for invalidate process to complete */
    while (CACHEOP & 0x03);   
}
void commit_discard_dcache (void) __attribute__((alias("commit_discard_idcache")));

void commit_discard_dcache_range (const void *base, unsigned int size)
{
    int cnt = size + ((unsigned long)base & 0x1f);
    unsigned long opcode = ((unsigned long)base & 0xffffffe0) | 0x01;

    while (cnt > 0)
    {
        CACHEOP = opcode;

        while (CACHEOP & 0x03);

        cnt -= 32;
        opcode += 32;
    }
}
