/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2007 by Michael Sevakis
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
#include "mmu-arm.h"
#include "cpu.h"

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIRQ"))) void name (void)

default_interrupt(EINT0);
default_interrupt(EINT1);
default_interrupt(EINT2);
default_interrupt(EINT3);
default_interrupt(EINT4_7);
default_interrupt(EINT8_23);
default_interrupt(CAM);
default_interrupt(nBATT_FLT);
default_interrupt(TICK);
default_interrupt(WDT_AC97);
default_interrupt(TIMER0);
default_interrupt(TIMER1);
default_interrupt(TIMER2);
default_interrupt(TIMER3);
default_interrupt(TIMER4);
default_interrupt(UART2);
default_interrupt(LCD);
default_interrupt(DMA0);
default_interrupt(DMA1);
default_interrupt(DMA2);
default_interrupt(DMA3);
default_interrupt(SDI);
default_interrupt(SPI0);
default_interrupt(UART1);
default_interrupt(NFCON);
default_interrupt(USBD);
default_interrupt(USBH);
default_interrupt(IIC);
default_interrupt(UART0);
default_interrupt(SPI1);
default_interrupt(RTC);
default_interrupt(ADC);

static void (* const irqvector[32])(void) =
{
    EINT0, EINT1, EINT2, EINT3,
    EINT4_7, EINT8_23, CAM, nBATT_FLT, TICK, WDT_AC97,
    TIMER0, TIMER1, TIMER2, TIMER3, TIMER4, UART2,
    LCD, DMA0, DMA1, DMA2, DMA3, SDI,
    SPI0, UART1, NFCON, USBD, USBH, IIC,
    UART0, SPI1, RTC, ADC,
};

static const char * const irqname[32] =
{
    "EINT0", "EINT1", "EINT2", "EINT3",
    "EINT4_7", "EINT8_23", "CAM", "nBATT_FLT", "TICK", "WDT_AC97",
    "TIMER0", "TIMER1", "TIMER2", "TIMER3", "TIMER4", "UART2",
    "LCD", "DMA0", "DMA1", "DMA2", "DMA3", "SDI",
    "SPI0", "UART1", "NFCON", "USBD", "USBH", "IIC",
    "UART0", "SPI1", "RTC", "ADC"
};

static void UIRQ(void)
{
    unsigned int offset = INTOFFSET;
    panicf("Unhandled IRQ %02X: %s", offset, irqname[offset]);
}

void irq_handler(void) __attribute__((interrupt ("IRQ"), naked));
void irq_handler(void)
{
    asm volatile (
        "sub    lr, lr, #4            \r\n"
        "stmfd  sp!, {r0-r3, ip, lr}  \r\n"
        "mov    r0, #0x4a000000       \r\n" /* INTOFFSET = 0x4a000014 */
        "ldr    r0, [r0, #0x14]       \r\n"
        "ldr    r1, =irqvector        \r\n"
        "ldr    r1, [r1, r0, lsl #2]  \r\n"
        "mov    lr, pc                \r\n"
        "bx     r1                    \r\n"
        "ldmfd  sp!, {r0-r3, ip, pc}^ \r\n"
    );
}

void system_reboot(void)
{
    WTCON = 0;
    WTCNT = WTDAT = 1 ;
    WTCON = 0x21;
    for(;;)
        ;
}

static void set_page_tables(void)
{
    map_section(0, 0, 0x1000, CACHE_NONE); /* map every memory region to itself */
    map_section(0x30000000, 0, 32, CACHE_ALL); /* map RAM to 0 and enable caching for it */
    map_section((int)FRAME, (int)FRAME, 1, BUFFERED); /* enable buffered writing for the framebuffer */
}

void memory_init(void)
{
    ttb_init();
    set_page_tables();
    enable_mmu();
}

void s3c_regmod32(volatile unsigned long *reg, unsigned long bits,
                  unsigned long mask)
{
    int oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);
    *reg = (*reg & ~mask) | (bits & mask);
    restore_interrupt(oldstatus);
}

void s3c_regset32(volatile unsigned long *reg, unsigned long bits)
{
    s3c_regmod32(reg, bits, bits);
}

void s3c_regclr32(volatile unsigned long *reg, unsigned long bits)
{
    s3c_regmod32(reg, 0, bits);
}

#ifdef BOOTLOADER
void system_prepare_fw_start(void)
{
    tick_stop();
    disable_interrupt(IRQ_FIQ_STATUS);
    INTMSK = 0xFFFFFFFF;
}
#endif

void system_init(void)
{
    INTMSK = 0xFFFFFFFF;
    INTMOD =  0;
    SRCPND = 0xFFFFFFFF;
    INTPND = 0xFFFFFFFF;
    INTSUBMSK = 0xFFFFFFFF;
    SUBSRCPND = 0xFFFFFFFF;

    GPBCON |= 0x85;
    GPBDAT |= 0x07;
    GPBUP  |= 0x20F;
 
    /* Take care of flash related pins */
    GPCCON |= 0x1000;
    GPCDAT &= ~0x40;
    GPCUP  |= 0x51;

    GPDCON |= 0x05;
    GPDUP  |= 0x03;
    GPDDAT &= ~0x03;

    GPFCON |= 0x00000AAA;
    GPFUP  |= 0xFF;

    GPGCON |= 0x01001000;
    GPGUP  |= 0x70;

    GPHCON |= 0x4005;
    GPHDAT |= 0x03;   

    /* TODO: do something with PRIORITY */
 
    /* Turn off currently-not or never-needed devices.
     *  Be careful here, it is possible to freeze the device by disabling
     *  clocks at the wrong time.
     *
     * Turn off AC97, Camera, SPI, IIS, I2C, UARTS, MMC/SD/SDIO Controller 
     * USB device, USB host, NAND flash controller.
     *
     * IDLE, Sleep, LCDC, PWM timer, GPIO, RTC, and ADC are untouched (on)
     */
    CLKCON &= ~0xFF1ED0;
 
    CLKSLOW |= 0x80;
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ

void set_cpu_frequency(long frequency)
{
    if (frequency == CPUFREQ_MAX)
    {
        asm volatile("mov r0, #0\n"
            "mrc p15, 0, r0, c1, c0, 0\n"
            "orr r0, r0, #3<<30\n" /* set to Asynchronous mode*/
            "mcr p15, 0, r0, c1, c0, 0" : : : "r0");

        FREQ = CPUFREQ_MAX;
    }
    else
    {
        asm volatile("mov r0, #0\n"
            "mrc p15, 0, r0, c1, c0, 0\n"
            "bic r0, r0, #3<<30\n" /* set to FastBus mode*/
            "mcr p15, 0, r0, c1, c0, 0" : : : "r0");

        FREQ = CPUFREQ_NORMAL;
    }
}

#endif
