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
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
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

void memory_init(void) {
    ttb_init();
    set_page_tables();
    enable_mmu();
}

void s3c_regmod(volatile int *reg, unsigned int set, unsigned int clr)
{
    int oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);
    unsigned int val = *reg;
    *reg = (val | set) & ~clr;
    restore_interrupt(oldstatus);
}

void s3c_regset(volatile int *reg, unsigned int mask)
{
    s3c_regmod(reg, mask, 0);
}

void s3c_regclr(volatile int *reg, unsigned int mask)
{
    s3c_regmod(reg, 0, mask);
}

void system_init(void)
{
    /* Disable interrupts and set all to IRQ mode */
    INTMSK = -1;
    INTMOD =  0;
    SRCPND = -1;
    INTPND = -1;
    INTSUBMSK = -1;
    SUBSRCPND = -1;

    /* TODO: do something with PRIORITY */
    
    
    /* Turn off currently-not or never-needed devices  */

    CLKCON &= ~(
        /* Turn off AC97 and Camera */
        (1<<19) | (1<<20)

        /* Turn off SPI */
        | (1 << 18)

        /* Turn off IIS */
        | (1 << 17)

        /* Turn off I2C */
        | (1 << 16)

        /* Turn off all of the UARTS */
        | ( (1<<10) | (1<<11) |(1<<12) )

        /* Turn off MMC/SD/SDIO Controller (SDI) */
        | (1 << 9)

        /* Turn off USB device */
        | (1 << 7)

        /* Turn off USB host */
        | (1 << 6)

        /* Turn off NAND flash controller */
        | (1 << 4)

        );
    
    /* Turn off the USB PLL */
    CLKSLOW |= (1 << 7);
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
