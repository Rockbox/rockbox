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
 * Copyright © 2008 Rafaël Carré
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

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIRQ"))) void name (void)

void irq_handler(void) __attribute__((interrupt ("IRQ"), naked));
void fiq_handler(void) __attribute__((interrupt ("FIQ"), naked));

default_interrupt(INT_WATCHDOG);
default_interrupt(INT_TIMER1);
default_interrupt(INT_TIMER2);
default_interrupt(INT_USB);
default_interrupt(INT_DMAC);
default_interrupt(INT_NAND);
default_interrupt(INT_IDE);
default_interrupt(INT_MCI0);
default_interrupt(INT_MCI1);
default_interrupt(INT_AUDIO);
default_interrupt(INT_SSP);
default_interrupt(INT_I2C_MS);
default_interrupt(INT_I2C_AUDIO);
default_interrupt(INT_I2SIN);
default_interrupt(INT_I2SOUT);
default_interrupt(INT_UART);
default_interrupt(INT_GPIOD);
default_interrupt(RESERVED1); /* Interrupt 17 : unused */
default_interrupt(INT_CGU);
default_interrupt(INT_MEMORY_STICK);
default_interrupt(INT_DBOP);
default_interrupt(RESERVED2); /* Interrupt 21 : unused */
default_interrupt(RESERVED3); /* Interrupt 22 : unused */
default_interrupt(RESERVED4); /* Interrupt 23 : unused */
default_interrupt(RESERVED5); /* Interrupt 24 : unused */
default_interrupt(RESERVED6); /* Interrupt 25 : unused */
default_interrupt(RESERVED7); /* Interrupt 26 : unused */
default_interrupt(RESERVED8); /* Interrupt 27 : unused */
default_interrupt(RESERVED9); /* Interrupt 28 : unused */
default_interrupt(INT_GPIOA);
default_interrupt(INT_GPIOB);
default_interrupt(INT_GPIOC);



static void (* const irqvector[])(void) =
{
    INT_WATCHDOG, INT_TIMER1, INT_TIMER2, INT_USB, INT_DMAC, INT_NAND, INT_IDE, INT_MCI0,
    INT_MCI1, INT_AUDIO, INT_SSP, INT_I2C_MS, INT_I2C_AUDIO, INT_I2SIN, INT_I2SOUT,
    INT_UART, INT_GPIOD, RESERVED1 /* 17 */ ,INT_CGU, INT_MEMORY_STICK, INT_DBOP,
    RESERVED2 /* 21 */, RESERVED3 /* 22 */, RESERVED4 /* 23 */, RESERVED5 /* 24 */,
    RESERVED6 /* 25 */, RESERVED7 /* 26 */, RESERVED8 /* 27 */, RESERVED9 /* 28 */,
    INT_GPIOA, INT_GPIOB, INT_GPIOC
};

static const char * const irqname[] =
{
    "INT_WATCHDOG", "INT_TIMER1", "INT_TIMER2", "INT_USB", "INT_DMAC", "INT_NAND",
    "INT_IDE", "INT_MCI0", "INT_MCI1", "INT_AUDIO", "INT_SSP", "INT_I2C_MS",
    "INT_I2C_AUDIO", "INT_I2SIN", "INT_I2SOUT", "INT_UART", "INT_GPIOD", "RESERVED1",
    "INT_CGU", "INT_MEMORY_STICK", "INT_DBOP", "RESERVED2", "RESERVED3", "RESERVED4",
    "RESERVED5", "RESERVED6", "RESERVED7", "RESERVED8", "RESERVED9", "INT_GPIOA",
    "INT_GPIOB", "INT_GPIOC"
};

static void UIRQ(void)
{
    unsigned int irq_no = 0;
    int status = VIC_IRQ_STATUS;
    while((status >>= 1))
        irq_no++;

    panicf("Unhandled IRQ %02X: %s", irq_no, irqname[irq_no]);
}

void irq_handler(void)
{
    /*
     * Based on: linux/arch/arm/kernel/entry-armv.S and system-meg-fx.c
     */

    asm volatile(   "stmfd sp!, {r0-r7, ip, lr} \n"   /* Store context */
                    "sub   sp, sp, #8           \n"); /* Reserve stack */

    unsigned int irq_no = 0;
    int status = VIC_IRQ_STATUS;
    while((status >>= 1))
        irq_no++;

    irqvector[irq_no]();

    asm volatile(   "add   sp, sp, #8           \n"   /* Cleanup stack   */
                    "ldmfd sp!, {r0-r7, ip, lr} \n"   /* Restore context */
                    "subs  pc, lr, #4           \n"); /* Return from IRQ */
}

void fiq_handler(void)
{
    asm volatile (
        "subs   pc, lr, #4   \r\n"
    );
}

static void sdram_delay(void)
{
    int delay = 1024; /* arbitrary */
    while (delay--) ;
}

/* Use the same initialization than OF */
static void sdram_init(void)
{
    CGU_PERI &= ~(0xf<<2);  /* clear div0 (memclock) */
    CGU_PERI |= (1<<2);     /* divider = 2 */

    CGU_PERI |= (1<<26)|(1<<27); /* extmem & extmem intf clocks */

    MPMC_CONTROL = 0x1; /* enable MPMC */

    MPMC_DYNAMIC_CONTROL = 0x183; /* SDRAM NOP, all clocks high */
    sdram_delay();

    MPMC_DYNAMIC_CONTROL = 0x103; /* SDRAM PALL, all clocks high */
    sdram_delay();

    MPMC_DYNAMIC_REFRESH = 0x138; /* 0x138 * 16 HCLK ticks between SDRAM refresh cycles */

    MPMC_CONFIG = 0; /* little endian, HCLK:MPMCCLKOUT[3:0] ratio = 1:1 */

    if(MPMC_PERIPH_ID2 & 0xf0)
        MPMC_DYNAMIC_READ_CONFIG = 0x1; /* command delayed, clock out not delayed */

    /* timings */
    MPMC_DYNAMIC_tRP    = 2;
    MPMC_DYNAMIC_tRAS   = 4;
    MPMC_DYNAMIC_tSREX  = 5;
    MPMC_DYNAMIC_tAPR   = 0;
    MPMC_DYNAMIC_tDAL   = 4;
    MPMC_DYNAMIC_tWR    = 2;
    MPMC_DYNAMIC_tRC    = 5;
    MPMC_DYNAMIC_tRFC   = 5;
    MPMC_DYNAMIC_tXSR   = 5;
    MPMC_DYNAMIC_tRRD   = 2;
    MPMC_DYNAMIC_tMRD   = 2;

#if defined(SANSA_CLIP) || defined(SANSA_M200V2) || defined(SANSA_FUZE)
#   define MEMORY_MODEL 0x21
    /* 16 bits external bus, low power SDRAM, 16 Mbits = 2 Mbytes */
#elif defined(SANSA_E200V2)
#   define MEMORY_MODEL 0x5
    /* 16 bits external bus, high performance SDRAM, 64 Mbits = 8 Mbytes */
#else
#   error "The external memory in your player is unknown"
#endif

    MPMC_DYNAMIC_RASCAS_0 = (2<<8)|2; /* CAS & RAS latency = 2 clock cycles */
    MPMC_DYNAMIC_CONFIG_0 = (MEMORY_MODEL << 7);

    MPMC_DYNAMIC_RASCAS_1 = MPMC_DYNAMIC_CONFIG_1 =
    MPMC_DYNAMIC_RASCAS_2 = MPMC_DYNAMIC_CONFIG_2 =
    MPMC_DYNAMIC_RASCAS_3 = MPMC_DYNAMIC_CONFIG_3 = 0;

    MPMC_DYNAMIC_CONTROL = 0x82; /* SDRAM MODE, MPMCCLKOUT runs continuously */

    /* this part is required, if you know why please explain */
    unsigned int tmp = *(volatile unsigned int*)(0x30000000+0x2300*MEM);
    (void)tmp; /* we just need to read from this location */

    MPMC_DYNAMIC_CONTROL = 0x2; /* SDRAM NORMAL, MPMCCLKOUT runs continuously */

    MPMC_DYNAMIC_CONFIG_0 |= (1<<19); /* buffer enable */
}

void system_init(void)
{
#if 0 /* the GPIO clock is already enabled by the dualboot function */
    CGU_PERI |= CGU_GPIO_CLOCK_ENABLE;
#endif

    CGU_PROC = 0;           /* fclk 24 MHz */
    CGU_PERI &= ~0x7f;      /* pclk 24 MHz */

    asm volatile(
        "mrc p15, 0, r0, c1, c0  \n"
        "orr r0, r0, #0xC0000000 \n" /* asynchronous clocking */
        "mcr p15, 0, r0, c1, c0  \n"
        : : : "r0" );

    CGU_PLLA = 0x4330;      /* PLLA 384 MHz */
    while(!(CGU_INTCTRL & (1<<0))); /* wait until PLLA is locked */

    CGU_PROC = (3<<2)|0x01; /* fclk = PLLA*5/8 = 240 MHz */

    asm volatile(
        "mrs r0, cpsr             \n"
        "bic r0, r0, #0x80        \n" /* enable interrupts */
        "msr cpsr, r0             \n"
        "mov r0, #0               \n"
        "mcr p15, 0, r0, c7, c7   \n" /* invalidate icache & dcache */
        "mrc p15, 0, r0, c1, c0   \n" /* control register */
        "orr r0, r0, #0x1000      \n" /* enable icache */
        "orr r0, r0, #4           \n" /* enable dcache */
        "mcr p15, 0, r0, c1, c0   \n"
        : : : "r0" );

    sdram_init();

    CGU_PERI |= (5<<2)|0x01; /* pclk = PLLA / 6 = 64 MHz */

    /* enable timer interface for TIMER1 & TIMER2 */
    CGU_PERI |= CGU_TIMERIF_CLOCK_ENABLE;

    /* enable VIC */
    CGU_PERI |= CGU_VIC_CLOCK_ENABLE;
    VIC_INT_SELECT = 0; /* only IRQ, no FIQ */
}

void system_reboot(void)
{
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}
