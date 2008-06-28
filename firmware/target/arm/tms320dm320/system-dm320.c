/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
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
#include "cpu.h"
#include "mmu-arm.h"
#include "kernel.h"
#include "system.h"
#include "panic.h"
#include "uart-target.h"
#include "system-arm.h"
#include "spi.h"
#ifdef CREATIVE_ZVx
#include "dma-target.h"
#endif

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIRQ"))) void name (void)

void irq_handler(void) __attribute__((interrupt ("IRQ"), naked));
void fiq_handler(void) __attribute__((interrupt ("FIQ"), naked));

default_interrupt(TIMER0);
default_interrupt(TIMER1);
default_interrupt(TIMER2);
default_interrupt(TIMER3);
default_interrupt(CCD_VD0);
default_interrupt(CCD_VD1);
default_interrupt(CCD_WEN);
default_interrupt(VENC);
default_interrupt(SERIAL0);
default_interrupt(SERIAL1);
default_interrupt(EXT_HOST);
default_interrupt(DSPHINT);
default_interrupt(UART0);
default_interrupt(UART1);
default_interrupt(USB_DMA);
default_interrupt(USB_CORE);
default_interrupt(VLYNQ);
default_interrupt(MTC0);
default_interrupt(MTC1);
default_interrupt(SD_MMC);
default_interrupt(SDIO_MS);
default_interrupt(GIO0);
default_interrupt(GIO1);
default_interrupt(GIO2);
default_interrupt(GIO3);
default_interrupt(GIO4);
default_interrupt(GIO5);
default_interrupt(GIO6);
default_interrupt(GIO7);
default_interrupt(GIO8);
default_interrupt(GIO9);
default_interrupt(GIO10);
default_interrupt(GIO11);
default_interrupt(GIO12);
default_interrupt(GIO13);
default_interrupt(GIO14);
default_interrupt(GIO15);
default_interrupt(PREVIEW0);
default_interrupt(PREVIEW1);
default_interrupt(WATCHDOG);
default_interrupt(I2C);
default_interrupt(CLKC);
default_interrupt(ICE);
default_interrupt(ARMCOM_RX);
default_interrupt(ARMCOM_TX);
default_interrupt(RESERVED);

/* The entry address is equal to base address plus an offset.
 * The offset is based on the priority of the interrupt. So if
 * the priority of an interrupt is changed, the user should also
 * change the offset for the interrupt in the entry table.
 */

static const unsigned short const irqpriority[] = 
{
    IRQ_TIMER0,IRQ_TIMER1,IRQ_TIMER2,IRQ_TIMER3,IRQ_CCD_VD0,IRQ_CCD_VD1,
    IRQ_CCD_WEN,IRQ_VENC,IRQ_SERIAL0,IRQ_SERIAL1,IRQ_EXT_HOST,IRQ_DSPHINT,
    IRQ_UART0,IRQ_UART1,IRQ_USB_DMA,IRQ_USB_CORE,IRQ_VLYNQ,IRQ_MTC0,IRQ_MTC1,
    IRQ_SD_MMC,IRQ_SDIO_MS,IRQ_GIO0,IRQ_GIO1,IRQ_GIO2,IRQ_GIO3,IRQ_GIO4,IRQ_GIO5,
    IRQ_GIO6,IRQ_GIO7,IRQ_GIO8,IRQ_GIO9,IRQ_GIO10,IRQ_GIO11,IRQ_GIO12,IRQ_GIO13,
    IRQ_GIO14,IRQ_GIO15,IRQ_PREVIEW0,IRQ_PREVIEW1,IRQ_WATCHDOG,IRQ_I2C,IRQ_CLKC,
    IRQ_ICE,IRQ_ARMCOM_RX,IRQ_ARMCOM_TX,IRQ_RESERVED
}; /* IRQ priorities, ranging from highest to lowest */

static void (* const irqvector[])(void) =
{
    TIMER0,TIMER1,TIMER2,TIMER3,CCD_VD0,CCD_VD1,
    CCD_WEN,VENC,SERIAL0,SERIAL1,EXT_HOST,DSPHINT,
    UART0,UART1,USB_DMA,USB_CORE,VLYNQ,MTC0,MTC1,
    SD_MMC,SDIO_MS,GIO0,GIO1,GIO2,GIO3,GIO4,GIO5,
    GIO6,GIO7,GIO8,GIO9,GIO10,GIO11,GIO12,GIO13,
    GIO14,GIO15,PREVIEW0,PREVIEW1,WATCHDOG,I2C,CLKC,
    ICE,ARMCOM_RX,ARMCOM_TX,RESERVED
};

static const char * const irqname[] =
{
    "TIMER0","TIMER1","TIMER2","TIMER3","CCD_VD0","CCD_VD1",
    "CCD_WEN","VENC","SERIAL0","SERIAL1","EXT_HOST","DSPHINT",
    "UART0","UART1","USB_DMA","USB_CORE","VLYNQ","MTC0","MTC1",
    "SD_MMC","SDIO_MS","GIO0","GIO1","GIO2","GIO3","GIO4","GIO5",
    "GIO6","GIO7","GIO8","GIO9","GIO10","GIO11","GIO12","GIO13",
    "GIO14","GIO15","PREVIEW0","PREVIEW1","WATCHDOG","I2C","CLKC",
    "ICE","ARMCOM_RX","ARMCOM_TX","RESERVED"
};

static void UIRQ(void)
{
    unsigned int offset = (IO_INTC_IRQENTRY0>>2)-1;
    panicf("Unhandled IRQ %02X: %s", offset, irqname[offset]);
}

void irq_handler(void)
{
    /*
     * Based on: linux/arch/arm/kernel/entry-armv.S and system-meg-fx.c
     */

    asm volatile(   "stmfd sp!, {r0-r7, ip, lr} \n"   /* Store context */
                    "sub   sp, sp, #8           \n"); /* Reserve stack */
    unsigned short addr = IO_INTC_IRQENTRY0>>2;
    if(addr != 0)
    {
        addr--;
        irqvector[addr]();
    }
    asm volatile(   "add   sp, sp, #8           \n"   /* Cleanup stack   */
                    "ldmfd sp!, {r0-r7, ip, lr} \n"   /* Restore context */
                    "subs  pc, lr, #4           \n"); /* Return from IRQ */
}

void fiq_handler(void)
{
    /*
     * Based on: linux/arch/arm/kernel/entry-armv.S and system-meg-fx.c
     */

    asm volatile (
        "sub    lr, lr, #4            \r\n"
        "stmfd  sp!, {r0-r3, ip, lr}  \r\n"
        "mov    r0, #0x00030000       \r\n"
        "ldr    r0, [r0, #0x510]      \r\n" /* Fetch value from IO_INTC_FIQENTRY0 */
        "sub    r0, r0, #1            \r\n"
        "ldr    r1, =irqvector        \r\n"
        "ldr    r1, [r1, r0, lsl #2]  \r\n" /* Divide value by 4 (TBA0/TBA1 is set to 0) and load appropriate pointer from the vector list */
        "blx    r1                    \r\n" /* Jump to handler */
        "ldmfd  sp!, {r0-r3, ip, pc}^ \r\n" /* Return from FIQ */
    );
}

void system_reboot(void)
{
    /* Code taken from linux/include/asm-arm/arch-itdm320-20/system.h at NeuroSVN */
    __asm__ __volatile__(                    
        "mov     ip, #0                                             \n"
        "mcr     p15, 0, ip, c7, c7, 0           @ invalidate cache \n"
        "mcr     p15, 0, ip, c7, c10,4           @ drain WB         \n"
        "mcr     p15, 0, ip, c8, c7, 0           @ flush TLB (v4)   \n"
        "mrc     p15, 0, ip, c1, c0, 0           @ get ctrl register\n"
        "bic     ip, ip, #0x000f                 @ ............wcam \n"
        "bic     ip, ip, #0x2100                 @ ..v....s........ \n"
        "mcr     p15, 0, ip, c1, c0, 0           @ ctrl register    \n"
        "mov     ip, #0xFF000000                                    \n"
        "orr     pc, ip, #0xFF0000               @ ip = 0xFFFF0000  \n"  
        :
        :
        : "cc"
    );
}

void system_init(void)
{
    /* taken from linux/arch/arm/mach-itdm320-20/irq.c */

    /* Clearing all FIQs and IRQs. */
    IO_INTC_IRQ0 = 0xFFFF;
    IO_INTC_IRQ1 = 0xFFFF;
    IO_INTC_IRQ2 = 0xFFFF;

    IO_INTC_FIQ0 = 0xFFFF;
    IO_INTC_FIQ1 = 0xFFFF;
    IO_INTC_FIQ2 = 0xFFFF;

    /* Masking all Interrupts. */
    IO_INTC_EINT0 = 0;
    IO_INTC_EINT1 = 0;
    IO_INTC_EINT2 = 0;

    /* Setting INTC to all IRQs. */
    IO_INTC_FISEL0 = 0;
    IO_INTC_FISEL1 = 0;
    IO_INTC_FISEL2 = 0;

    /* IRQENTRY only reflects enabled interrupts */
    IO_INTC_RAW = 0;

    IO_INTC_ENTRY_TBA0 = 0;
    IO_INTC_ENTRY_TBA1 = 0;
    
    int i;
    /* Set interrupt priorities to predefined values */
    for(i = 0; i < 23; i++)
        DM320_REG(0x0540+i*2) = ((irqpriority[i*2+1] & 0x3F) << 8) | (irqpriority[i*2] & 0x3F); /* IO_INTC_PRIORITYx */
    
    /* Turn off all timers */
    IO_TIMER0_TMMD = CONFIG_TIMER0_TMMD_STOP;
    IO_TIMER1_TMMD = CONFIG_TIMER1_TMMD_STOP;
    IO_TIMER2_TMMD = CONFIG_TIMER2_TMMD_STOP;
    IO_TIMER3_TMMD = CONFIG_TIMER3_TMMD_STOP;

#ifndef CREATIVE_ZVx
    /* set GIO26 (reset pin) to output and low */
    IO_GIO_BITCLR1=(1<<10);
    IO_GIO_DIR1&=~(1<<10);
#endif

    uart_init();
    spi_init();
    
#ifdef CREATIVE_ZVx
    dma_init();
#endif
 
    /* MMU initialization (Starts data and instruction cache) */
    ttb_init();
    /* Make sure everything is mapped on itself */
    map_section(0, 0, 0x1000, CACHE_NONE);
    /* Enable caching for RAM */
    map_section(CONFIG_SDRAM_START, CONFIG_SDRAM_START, MEM, CACHE_ALL);
    /* enable buffered writing for the framebuffer */
    map_section((int)FRAME, (int)FRAME, 1, BUFFERED);
#ifdef CREATIVE_ZVx
    /* mimic OF */
    map_section(0x00100000, 0x00100000,  4, CACHE_NONE);
    map_section(0x04700000, 0x04700000,  2,   BUFFERED);
    map_section(0x40000000, 0x40000000, 16, CACHE_NONE);
    map_section(0x50000000, 0x50000000, 16, CACHE_NONE);
    map_section(0x60000000, 0x60000000, 16, CACHE_NONE);
    map_section(0x80000000, 0x80000000,  1, CACHE_NONE);
#endif
    enable_mmu();
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
