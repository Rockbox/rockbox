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
#else
#include "usb-mr500.h"
#endif

static unsigned short clock_arm_slow = 0xFFFF;
static unsigned short clock_arm_fast = 0xFFFF;

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIRQ"))) void name (void)

void irq_handler(void) __attribute__((interrupt ("IRQ"), section(".icode")));
void fiq_handler(void) __attribute__((interrupt ("FIQ"), section(".icode")));

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

static void (* const irqvector[])(void) __attribute__ ((section(".idata"))) =
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
    unsigned short addr = IO_INTC_IRQENTRY0>>2;
    if(addr != 0)
    {
        addr--;
        irqvector[addr]();
    }
}

void fiq_handler(void)
{
    /*
     * Based on: linux/arch/arm/kernel/entry-armv.S and system-meg-fx.c
     */
    unsigned short addr = IO_INTC_FIQENTRY0>>2;
    if(addr != 0)
    {
        addr--;
        irqvector[addr]();
    }
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

void system_exception_wait(void)
{
    /* Mask all Interrupts. */
    IO_INTC_EINT0 = 0;
    IO_INTC_EINT1 = 0;
    IO_INTC_EINT2 = 0;
    while ((IO_GIO_BITSET0&0x01) != 0); /* Wait for power button */
}

void system_init(void)
{
    unsigned int vector_addr;
    /* Pin 33 is connected to a buzzer, for an annoying sound set 
     *  PWM0C == 0x3264
     *  PWM0H == 0x1932
     *  Function to 1
     *  Since this is not used in the FW, set it to a normal output at a zero
     *  level. */

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

    /* Only initially needed clocks should be turned on */
    IO_CLK_MOD0 =   CLK_MOD0_HPIB | CLK_MOD0_DSP  | CLK_MOD0_SDRAMC | 
                    CLK_MOD0_EMIF | CLK_MOD0_INTC | CLK_MOD0_AIM    | 
                    CLK_MOD0_AHB  | CLK_MOD0_BUSC | CLK_MOD0_ARM;
    IO_CLK_MOD1 =   CLK_MOD1_CPBUS;
    IO_CLK_MOD2 =   CLK_MOD2_GIO;

#if 0
    if (IO_BUSC_REVR == REVR_ES11)
    {
        /* Agressive clock setup for newer parts (ES11) - this is actually lower
         *  power also.
         */

        /* Setup the EMIF interface timings */

        /* ATA interface:
         * If this is the newer silicon the timings need to be slowed down some
         * for reliable access due to the faster ARM clock.
         */
        /* OE width, WE width, CS width, Cycle width */
        IO_EMIF_CS3CTRL1 = (8 << 12) | (8 << 8) | (14 << 4) | 15;
        /* 14: Width (16), 12: Idles, 8: OE setup, 4: WE Setup, CS setup */
        IO_EMIF_CS3CTRL2 = (1<<14) | (1 << 12) | (3 << 8) | (3 << 4) | 1;

        /* USB interface:
         * The following EMIF timing values are from the OF:
         *      IO_EMIF_CS4CTRL1 = 0x66AB;
         *      IO_EMIF_CS4CTRL2 = 0x4220;
         *
         * More agressive numbers may be possible, but it depends on the clocking
         *  setup. 
         */
        IO_EMIF_CS4CTRL1 = 0x66AB;
        IO_EMIF_CS4CTRL2 = 0x4220; 

        /* 27 MHz input clock:
         *  PLLA: 27 * 15 / 2 = 202.5 MHz
         *  PLLB: 27 *  9 / 2 = 121.5 MHz (off: bit 12)
         */
        IO_CLK_PLLA = (14 <<  4) | 1;
        IO_CLK_PLLB = ( 1 << 12) | ( 8 <<  4) | 1;         

        /* Set the slow and fast clock speeds used for boosting
         * Slow Setup:
         *  ARM div  = 4    ( 50.625 MHz )
         *  AHB div  = 1    ( 50.625 MHz )
         * Fast Setup:
         *  ARM div  = 1    ( 202.5  MHz )
         *  AHB div  = 2    ( 101.25 MHz )
         */
        clock_arm_slow = (0 << 8) | 3;
        clock_arm_fast = (1 << 8) | 0;

        IO_CLK_DIV0 = clock_arm_slow;
        
        /* SDRAM div= 2     ( 101.25 MHz )
         * AXL div  = 1     ( 202.5  MHz )
         */
        IO_CLK_DIV1 = (0 << 8) | 1;
        
        /* MS div   = 15    ( 13.5 MHz )
         * DSP div  = 4     ( 50.625 MHz - could be double, but this saves power)
         */ 
        IO_CLK_DIV2 = (3 << 8) | 14;
        
        /* MMC div  = 256   ( slow )
         * VENC div = 32    ( 843.75 KHz )
         */
        IO_CLK_DIV3 = (31 << 8) | 255;
        
        /* I2C div  = 1     ( 48 MHz if M48XI is running )
         * VLNQ div = 32
         */
        IO_CLK_DIV4 = (31 << 8) | 0;
        
        /* Feed everything from PLLA */
        IO_CLK_SEL0=0x007E;
        IO_CLK_SEL1=0x1000;
        IO_CLK_SEL2=0x0000; 
    }
    else
#endif
    {
        /* Set the slow and fast clock speeds used for boosting
         * Slow Setup:
         *  ARM div  = 4    ( 87.5 MHz )
         *  AHB div  = 1    ( 87.5 MHz )
         * Fast Setup:
         *  ARM div  = 2    ( 175  MHz )
         *  AHB div  = 2    ( 87.5 MHz )
         */
        clock_arm_slow = (0 << 8) | 3;
        clock_arm_fast = (1 << 8) | 1;
    }
    
    /* M48XI disabled, USB buffer powerdown */
    IO_CLK_LPCTL1 = 0x11; /* I2C wodn't work with this disabled */

    /* IRQENTRY only reflects enabled interrupts */
    IO_INTC_RAW = 0;

    vector_addr = (unsigned int) irqvector;
    IO_INTC_ENTRY_TBA0 = 0;//(short) vector_addr & ~0x000F;
    IO_INTC_ENTRY_TBA1 = 0;//(short) (vector_addr >> 16);

    int i;
    /* Set interrupt priorities to predefined values */
    for(i = 0; i < 23; i++)
        DM320_REG(0x0540+i*2) = ((irqpriority[i*2+1] & 0x3F) << 8) | 
            (irqpriority[i*2] & 0x3F); /* IO_INTC_PRIORITYx */
    
    /* Turn off all timers */
    IO_TIMER0_TMMD = CONFIG_TIMER0_TMMD_STOP;
    IO_TIMER1_TMMD = CONFIG_TIMER1_TMMD_STOP;
    IO_TIMER2_TMMD = CONFIG_TIMER2_TMMD_STOP;
    IO_TIMER3_TMMD = CONFIG_TIMER3_TMMD_STOP;

    uart_init();
    spi_init();

    /* Initialization is done so shut the front LED off so that the battery
     * can charge.
     */
    IO_GIO_BITCLR2 = 0x0001;
    
#ifdef CREATIVE_ZVx
    dma_init();
#endif
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency(long frequency)
{
    /* If these variables have not been changed since startup then boosting
     *  should not be used.
     */
    if(clock_arm_slow == 0xFFFF || clock_arm_fast == 0xFFFF)
    {
        return;
    }

    if (frequency == CPUFREQ_MAX) 
    {
        IO_CLK_DIV0 = clock_arm_fast;
        FREQ = CPUFREQ_MAX;
    } 
    else 
    {
        IO_CLK_DIV0 = clock_arm_slow;
        FREQ = CPUFREQ_NORMAL;
    }
}
#endif

/* This function is pretty crude.  It is not acurate to a usec, but errors on
 *  longer.
 */
void udelay(int usec) {
    volatile int temp=usec*(175000/200);
    
    while(temp) {
        temp--;
    }
}


