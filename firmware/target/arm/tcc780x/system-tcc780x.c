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

#if !defined(BOOTLOADER)

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIRQ"))) void name (void)

void irq_handler(void) __attribute__((interrupt ("IRQ"), naked));
void fiq_handler(void) __attribute__((interrupt ("FIQ"), naked));

default_interrupt(EXT0);
default_interrupt(EXT1);
default_interrupt(EXT2);
default_interrupt(EXT3);
default_interrupt(RTC);
default_interrupt(GPSB0);
default_interrupt(TIMER0);
default_interrupt(TIMER1);
default_interrupt(SCORE);
default_interrupt(SPDTX);
default_interrupt(VIDEO);
default_interrupt(GSIO);
default_interrupt(SCALER);
default_interrupt(I2C);
default_interrupt(DAI_RX);
default_interrupt(DAI_TX);
default_interrupt(CDRX);
default_interrupt(HPI);
default_interrupt(UART0);
default_interrupt(UART1);
default_interrupt(G2D);
default_interrupt(USB_DEVICE);
default_interrupt(USB_HOST);
default_interrupt(DMA);
default_interrupt(HDD);
default_interrupt(MSTICK);
default_interrupt(NFC);
default_interrupt(SDMMC);
default_interrupt(CAM);
default_interrupt(LCD);
default_interrupt(ADC);
default_interrupt(GPSB1);

/* TODO: Establish IRQ priorities (0 = highest priority) */
static const char irqpriority[] =
{
    0,  /* EXT0 */
    1,  /* EXT1 */
    2,  /* EXT2 */
    3,  /* EXT3 */
    4,  /* RTC */
    5,  /* GPSB0 */
    6,  /* TIMER0 */
    7,  /* TIMER1 */
    8,  /* SCORE */
    9,  /* SPDTX */
    10, /* VIDEO */
    11, /* GSIO */
    12, /* SCALER */
    13, /* I2C */
    14, /* DAI_RX */
    15, /* DAI_TX */
    16, /* CDRX */
    17, /* HPI */
    18, /* UART0 */
    19, /* UART1 */
    20, /* G2D */
    21, /* USB_DEVICE */
    22, /* USB_HOST */
    23, /* DMA */
    24, /* HDD */
    25, /* MSTICK */
    26, /* NFC */
    27, /* SDMMC */
    28, /* CAM */
    29, /* LCD */
    30, /* ADC */
    31, /* GPSB */
};

static void (* const irqvector[])(void) =
{
    EXT0,EXT1,EXT2,EXT3,RTC,GPSB0,TIMER0,TIMER1,
    SCORE,SPDTX,VIDEO,GSIO,SCALER,I2C,DAI_RX,DAI_TX,
    CDRX,HPI,UART0,UART1,G2D,USB_DEVICE,USB_HOST,DMA,
    HDD,MSTICK,NFC,SDMMC,CAM,LCD,ADC,GPSB1
};

static const char * const irqname[] =
{
    "EXT0","EXT1","EXT2","EXT3","RTC","GPSB0","TIMER0","TIMER1",
    "SCORE","SPDTX","VIDEO","GSIO","SCALER","I2C","DAI_RX","DAI_TX",
    "CDRX","HPI","UART0","UART1","G2D","USB_DEVICE","USB_HOST","DMA",
    "HDD","MSTICK","NFC","SDMMC","CAM","LCD","ADC","GPSB1"
};

static void UIRQ(void)
{
    unsigned int offset = VNIRQ;
    panicf("Unhandled IRQ %02X: %s", offset, irqname[offset]);
}

void irq_handler(void)
{
    /*
     * Based on: linux/arch/arm/kernel/entry-armv.S and system-meg-fx.c
     */

    asm volatile(   "stmfd sp!, {r0-r7, ip, lr} \n"   /* Store context */
                    "sub   sp, sp, #8           \n"); /* Reserve stack */

    int irq_no = VNIRQ;   /* Read clears the corresponding IRQ status */
    
    if ((irq_no & (1<<31)) == 0)  /* Ensure invalid flag is not set */
    {
        irqvector[irq_no]();
    }
    
    asm volatile(   "add   sp, sp, #8           \n"   /* Cleanup stack   */
                    "ldmfd sp!, {r0-r7, ip, lr} \n"   /* Restore context */
                    "subs  pc, lr, #4           \n"); /* Return from IRQ */
}

#endif /* !defined(BOOTLOADER) */


/* TODO - these should live in the target-specific directories and
   once we understand what all the GPIO pins do, move the init to the
   specific driver for that hardware.   For now, we just perform the 
   same GPIO init as the original firmware - this makes it easier to
   investigate what the GPIO pins do.
*/

#ifdef COWON_D2
static void gpio_init(void)
{
    /* Do what the original firmware does */
    GPIOA = 0x07000C83;
    GPIOA_DIR = 0x0F010CE3;
    GPIOB = 0;
    GPIOB_DIR = 0x00080000;
    GPIOC = 0x39000000;
    GPIOC_DIR = 0xB9000000;
    GPIOD = 0;
    GPIOD_DIR = 0;
    GPIOD = 0;
    GPIOD_DIR = 0x00480000;
    
    PORTCFG0 = 0x00034540;
    PORTCFG1 = 0x0566A000;
    PORTCFG2 = 0x000004C0;
    PORTCFG3 = 0x0AA40455;
}
#endif


/* Second function called in the original firmware's startup code - we just
   set up the clocks in the same way as the original firmware for now. */
#ifdef COWON_D2
static void clock_init(void)
{
    int i;
    
    CSCFG3  = (CSCFG3 &~ 0x3fff) | 0x841;

    /* Enable Xin (12Mhz), Fsys = Xin, Fbus = Fsys/2, MCPU=Fsys, SCPU=Fsys */
    CLKCTRL = 0x800FF014;

    asm volatile (
        "nop      \n\t"
        "nop      \n\t"
    );
    
    PCLK_RFREQ = 0x1401002d; /* RAM refresh source = Xin (4) / 0x2d = 266kHz */
    
    MCFG |= 1;
    SDCFG = (SDCFG &~ 0x7000) | 0x2000;
    
    MCFG1 |= 1;
    SDCFG1 = (SDCFG &~ 0x7000) | 0x2000;

    /* Configure PLL0 to 192Mhz, for CPU scaling */
    PLL0CFG |= (1<<31);                /* power down */
    CLKDIVC = CLKDIVC &~ (0xff << 24); /* disable PLL0 divider */
    PLL0CFG = 0x80019808;              /* set for 192Mhz (with power down) */
    PLL0CFG = PLL0CFG &~ (1<<31);      /* power up */

    /* Configure PLL1 to 216Mz, for LCD clock (when divided by 2) */
    PLL1CFG |= (1<<31);                /* power down */
    CLKDIVC = CLKDIVC &~ (0xff << 16); /* disable PLL1 divider */
    PLL1CFG = 0x80002503;              /* set for 216Mhz (with power down)*/
    PLL1CFG = PLL1CFG &~ (1<<31);      /* power up */

    i = 0x8000;
    while (--i) {};

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    set_cpu_frequency(CPUFREQ_NORMAL);
#else
    /* 48Mhz: Fsys = PLL0 (192Mhz) Fbus = Fsys/4 CPU = Fbus, COP = Fbus */
    CLKCTRL = (1<<31) | (3<<28) | (3<<4);
#endif

    asm volatile (
        "nop      \n\t"
        "nop      \n\t"
    );
    
    /* configure PCK_TCT to 2Mhz (clock source 4 (Xin) divided by 6) */
    PCLK_TCT = PCK_EN | (CKSEL_XIN<<24) | 5;
}
#endif


#ifdef COWON_D2
void system_init(void)
{
    MBCFG = 0x19;

    if (TCC780_VER == 0)
      ECFG0 = 0x309;
    else
      ECFG0 = 0x30d;

    /* mask all interrupts */
    IEN = 0;

#if !defined(BOOTLOADER)

    /* Set DAI interrupts as FIQ, all others are IRQ. */
    IRQSEL = ~(DAI_RX_IRQ_MASK | DAI_TX_IRQ_MASK);

    POL = 0x200108;     /* IRQs 3,8,21 active low (as OF) */
    TMODE = 0x20ce07c0; /* IRQs 6-10,17-19,22-23,29 level-triggered (as OF) */

    VCTRL |= (1<<31);   /* Reading from VNIRQ clears that interrupt */

    /* Write IRQ priority registers using ints - a freeze occurs otherwise */
    int i;
    for (i = 0; i < 7; i++)
    {
        IRQ_PRIORITY_TABLE[i] = ((int*)irqpriority)[i];
    }

    ALLMASK = 3;        /* Global FIQ/IRQ unmask */
    
#endif /* !defined(BOOTLOADER) */

    gpio_init();
    clock_init();
}
#endif


void system_reboot(void)
{
    disable_interrupt(IRQ_FIQ_DISABLED);
    
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    set_cpu_frequency(CPUFREQ_DEFAULT);
#endif

    /* TODO: implement reboot (eg. jump to boot ROM?) */
    while (1);
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
    
    /* CPU/COP frequencies can be scaled between Fbus (min) and Fsys (max).
       Fbus should not be set below ~32Mhz with LCD enabled or the display
       will be garbled. */
    if (frequency == CPUFREQ_MAX)
    {
        /* 192Mhz:
           Fsys = PLL0 (192Mhz)
           Fbus = Fsys/2
           CPU = Fsys, COP = Fsys */
        CLKCTRL = (1<<31) | (0xFF<<12) | (1<<4);
    }
    else if (frequency == CPUFREQ_NORMAL)
    {
        /* 48Mhz:
           Fsys = PLL0 (192Mhz)
           Fbus = Fsys/4
           CPU = Fbus, COP = Fbus */
        CLKCTRL = (1<<31) | (3<<28) | (3<<4);
    }
    else
    {
        /* 32Mhz:
           Fsys = PLL0 (192Mhz)
           Fbus = Fsys/6
           CPU = Fbus, COP = Fbus */
        CLKCTRL = (1<<31) | (3<<28) | (5<<4);
    }

    asm volatile (
        "nop      \n\t"
        "nop      \n\t"
        "nop      \n\t"
    );

    cpu_frequency = frequency;
}

#endif
