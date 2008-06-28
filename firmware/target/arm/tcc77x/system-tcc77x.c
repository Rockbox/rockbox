/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Dave Chapman
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

/* Externally defined interrupt handlers */
extern void TIMER(void);
extern void ADC(void);

void irq(void)
{
    int irq = IREQ & 0x7fffffff;
    CREQ = irq;     /* Clears the corresponding IRQ status */

    if (irq & TIMER0_IRQ_MASK)
        TIMER();
    else if (irq & ADC_IRQ_MASK)
        ADC();
    else
        panicf("Unhandled IRQ 0x%08X", irq);
}

void fiq_handler(void)
{
    /* TODO */
}

void system_reboot(void)
{
}

/* TODO - these should live in the target-specific directories and
   once we understand what all the GPIO pins do, move the init to the
   specific driver for that hardware.   For now, we just perform the 
   same GPIO init as the original firmware - this makes it easier to
   investigate what the GPIO pins do.
*/

#ifdef LOGIK_DAX
static void gpio_init(void)
{
    /* Do what the original firmware does */
    GPIOD_FUNC = 0;
    GPIOD_DIR = 0x3f0;
    GPIOD = 0xe0;
	
    GPIOE_FUNC = 0;
    GPIOE_DIR = 0xe0;
    GPIOE = 0;
	
    GPIOA_FUNC = 0;
    GPIOA_DIR = 0xffff1000;   /* 0 - 0xf000 */
    GPIOA = 0x1080;
	
    GPIOB_FUNC = 0x16a3;
    GPIOB_DIR = 0x6ffff;
    GPIOB = 0;
	
    GPIOC_FUNC = 1;
    GPIOC_DIR = 0x03ffffff;  /* mvn r2, 0xfc000000 */
    GPIOC = 0;
}
#elif defined(IAUDIO_7)
static void gpio_init(void)
{
    /* Do what the original firmware does */
    GPIOA_FUNC = 0;
    GPIOB_FUNC = 0x1623;
    GPIOC_FUNC = 1;
    GPIOD_FUNC = 0;
    GPIOE_FUNC = 0;
    GPIOA = 0x30;
    GPIOB = 0x80000;
    GPIOC = 0;
    GPIOD = 0x180;
    GPIOE = 0;
    GPIOA_DIR = 0x84b0
    GPIOB_DIR = 0x80800;
    GPIOC_DIR = 0x2000000;
    GPIOD_DIR = 0x3e3;
    GPIOE_DIR = 0x88;
}
#elif defined(SANSA_M200)
static void gpio_init(void)
{
    /* TODO - Implement for M200 */
}
#elif defined(SANSA_C100)
static void gpio_init(void)
{
    /* Do what the original firmware does */
    GPIOA_FUNC = 0;
    GPIOB_FUNC = 0x16A3;
    GPIOC_FUNC = 1;
    GPIOD_FUNC |= 2; 
    GPIOE_FUNC = 0;
	
    GPIOA_DIR = 0xFFFF0E00;  
    GPIOB_DIR = 0x6FFFF; 
    GPIOC_DIR = 0x03FFFFFF; 
    GPIOD_DIR = 0x3F7; 
    GPIOE_DIR = 0x9B; 
    
    GPIOA = 0x80; 
    GPIOB = 0;
    GPIOC = 0;
    GPIOD |= 0xC0; 
    GPIOE = 0x9B; 
}
#endif

/* Second function called in the original firmware's startup code - we just
   set up the clocks in the same way as the original firmware for now. */
static void clock_init(void)
{
    unsigned int i;

    /* STP = 0x1, PW = 0x04 , HLD = 0x0 */
    CSCFG3 = (CSCFG3 &~ 0x3fff) | 0x820;

    /* XIN=External main, Fcpu=Fsys, BCKDIV=1 (Fbus = Fsys / 2) */
    CLKCTRL = (CLKCTRL & ~0xff) | 0x14;

    if (BMI & 0x20)
      PCLKCFG0 = 0xc82d7000; /* EN1 = 1, XIN=Ext. main, DIV1 = 0x2d, P1 = 1 */
    else
      PCLKCFG0 = 0xc8ba7000; /* EN1 = 1, XIN=Ext. main, DIV1 = 0xba, P1 = 1 */

    MCFG |= 0x2000;

#ifdef LOGIK_DAX
    /* Only seen in the Logik DAX original firmware */
    SDCFG = (SDCFG & ~0x7000) | 0x2000;
#endif

    /* Disable PLL */
    PLL0CFG |= 0x80000000;

    /* Enable PLL, M=0xcf, P=0x13. m=M+8, p=P+2, S = 0
       Fout = (215/21)*12MHz = 122857142Hz */
    PLL0CFG = 0x0000cf13;

    i = 8000;
    while (--i) {};

    /* Enable PLL0 */
    CLKDIVC = 0x81000000;

    /* Fsys = PLL0, Fcpu = Fsys, Fbus=Fsys / 2 */
    CLKCTRL = 0x80000010;

    asm volatile (
        "nop      \n\t"
        "nop      \n\t"
    );
}

static void cpu_init(void)
{
 /* Memory protection - see page 48 of ARM946 TRM
http://infocenter.arm.com/help/topic/com.arm.doc.ddi0201d/DDI0201D_arm946es_r1p1_trm.pdf
 */
    asm volatile (
        /* Region 0 - addr=0, size=4GB, enabled */
        "mov     r0, #0x3f              \n\t"
        "mcr     p15, 0, r0, c6, c0, 0  \n\t"
        "mcr     p15, 0, r0, c6, c0, 1  \n\t"

#if defined(LOGIK_DAX) || defined(SANSA_C100)
        /* Address region 1 - addr 0x2fff0000, size=64KB, enabled*/
        "ldr     r0, =0x2fff001f        \n\t"
#elif defined(IAUDIO_7)
        /* Address region 1 - addr 0x20000000, size=8KB, enabled*/
        "mov     r0, #0x19              \n\t"
        "add     r0, r0, #0x20000000    \n\t"
#elif defined(SANSA_M200)
        /* Address region 1 - addr 0x20000000, size=256MB, enabled*/
        "mov     r0, #0x37              \n\t"
        "add     r0, r0, #0x20000000    \n\t"
#endif
        "mcr     p15, 0, r0, c6, c1, 0  \n\t"
        "mcr     p15, 0, r0, c6, c1, 1  \n\t"

        /* Address region 2 - addr 0x30000000, size=256MB, enabled*/
        "mov     r0, #0x37              \n\t"
        "add     r0, r0, #0x30000000    \n\t"
        "mcr     p15, 0, r0, c6, c2, 0  \n\t"
        "mcr     p15, 0, r0, c6, c2, 1  \n\t"

        /* Address region 2 - addr 0x40000000, size=512MB, enabled*/
        "mov     r0, #0x39              \n\t"
        "add     r0, r0, #0x40000000    \n\t"
        "mcr     p15, 0, r0, c6, c3, 0  \n\t"
        "mcr     p15, 0, r0, c6, c3, 1  \n\t"

        /* Address region 4 - addr 0x60000000, size=256MB, enabled*/
        "mov     r0, #0x37              \n\t"
        "add     r0, r0, #0x60000000    \n\t"
        "mcr     p15, 0, r0, c6, c4, 0  \n\t"
        "mcr     p15, 0, r0, c6, c4, 1  \n\t"

        /* Address region 5 - addr 0x10000000, size=256MB, enabled*/
        "mov     r0, #0x37              \n\t"
        "add     r0, r0, #0x10000000    \n\t"
        "mcr     p15, 0, r0, c6, c5, 0  \n\t"
        "mcr     p15, 0, r0, c6, c5, 1  \n\t"

        /* Address region 6 - addr 0x80000000, size=2GB, enabled*/
        "mov     r0, #0x37              \n\t"
        "add     r0, r0, #0x80000006    \n\t"
        "mcr     p15, 0, r0, c6, c6, 0  \n\t"
        "mcr     p15, 0, r0, c6, c6, 1  \n\t"

        /* Address region 7 - addr 0x3000f000, size=4KB, enabled*/
        "ldr     r0, =0x3000f017        \n\t"
        "mcr     p15, 0, r0, c6, c7, 0  \n\t"
        "mcr     p15, 0, r0, c6, c7, 1  \n\t"

		
        /* Register 5 - Access Permission Registers */

        "ldr     r0, =0xffff            \n\t"
        "mcr     p15, 0, r0, c5, c0, 0  \n\t"  /* write data access permission bits */
        "mcr     p15, 0, r0, c5, c0, 1  \n\t"  /* write instruction access permission bits */

        "mov     r0, #0xa7              \n\t"
        "mcr     p15, 0, r0, c3, c0, 0  \n\t"  /* set write buffer control register */

#if defined(LOGIK_DAX) || defined(SANSA_C100)
        "mov     r0, #0xa5              \n\t"
#elif defined(IAUDIO_7) || defined(SANSA_M200) 
        "mov     r0, #0xa7              \n\t"
#elif
    #error NOT DEFINED FOR THIS TARGET!
#endif
        "mcr     p15, 0, r0, c2, c0, 0  \n\t"
        "mcr     p15, 0, r0, c2, c0, 1  \n\t"

        "mov     r0, #0xa0000006        \n\t"
        "mcr     p15, 0, r0, c9, c1, 0  \n\t"

        "ldr     r1, =0x1107d           \n\t"
        "mov     r0, #0x0               \n\t"
        "mcr     p15, 0, r0, c7, c5, 0  \n\t" /* Flush instruction cache */
        "mcr     p15, 0, r0, c7, c6, 0  \n\t" /* Flush data cache */

        "mcr     p15, 0, r1, c1, c0, 0  \n\t" /* CPU control bits */
        : : : "r0", "r1"
    );
}



void system_init(void)
{
    /* mask all interrupts */
    IEN = 0;

    /* Set all interrupts as IRQ for now - some may need to be FIQ in future */
    IRQSEL = 0xffffffff; 

    /* Set master enable bit */
    IEN = 0x80000000;

    cpu_init();
    clock_init();
    gpio_init();

    enable_irq();
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ

void set_cpu_frequency(long frequency)
{
}

#endif
