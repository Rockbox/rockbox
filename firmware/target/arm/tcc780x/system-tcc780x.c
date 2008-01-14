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

#if !defined(BOOTLOADER)

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIRQ"))) void name (void)

void irq_handler(void) __attribute__((interrupt ("IRQ"), naked));
void fiq_handler(void) __attribute__((interrupt ("FIQ"), naked));

default_interrupt(EXT0);
default_interrupt(EXT1);
default_interrupt(EXT2);
default_interrupt(EXT3);
default_interrupt(IRQ4);
default_interrupt(IRQ5);
default_interrupt(TIMER);
default_interrupt(IRQ7);
default_interrupt(IRQ8);
default_interrupt(IRQ9);
default_interrupt(IRQ10);
default_interrupt(IRQ11);
default_interrupt(IRQ12);
default_interrupt(IRQ13);
default_interrupt(DAI_RX);
default_interrupt(DAI_TX);
default_interrupt(IRQ16);
default_interrupt(IRQ17);
default_interrupt(IRQ18);
default_interrupt(IRQ19);
default_interrupt(IRQ20);
default_interrupt(IRQ21);
default_interrupt(IRQ22);
default_interrupt(IRQ23);
default_interrupt(IRQ24);
default_interrupt(IRQ25);
default_interrupt(IRQ26);
default_interrupt(IRQ27);
default_interrupt(IRQ28);
default_interrupt(IRQ29);
default_interrupt(IRQ30);
default_interrupt(IRQ31);

static void (* const irqvector[])(void) =
{
    EXT0,EXT1,EXT2,EXT3,IRQ4,IRQ5,TIMER,IRQ7,
    IRQ8,IRQ9,IRQ10,IRQ11,IRQ12,IRQ13,DAI_RX,DAI_TX,
    IRQ16,IRQ17,IRQ18,IRQ19,IRQ20,IRQ21,IRQ22,IRQ23,
    IRQ24,IRQ25,IRQ26,IRQ27,IRQ28,IRQ29,IRQ30,IRQ31
};

static const char * const irqname[] =
{
    "EXT0","EXT1","EXT2","EXT3","IRQ4","IRQ5","TIMER","IRQ7",
    "IRQ8","IRQ9","IRQ10","IRQ11","IRQ12","IRQ13","DAI_RX","DAI_TX",
    "IRQ16","IRQ17","IRQ18","IRQ19","IRQ20","IRQ21","IRQ22","IRQ23",
    "IRQ24","IRQ25","IRQ26","IRQ27","IRQ28","IRQ29","IRQ30","IRQ31"
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
    irqvector[VNIRQ]();
    asm volatile(   "add   sp, sp, #8           \n"   /* Cleanup stack   */
                    "ldmfd sp!, {r0-r7, ip, lr} \n"   /* Restore context */
                    "subs  pc, lr, #4           \n"); /* Return from FIQ */
}

void fiq_handler(void)
{
    asm volatile (
        "sub    lr, lr, #4   \r\n"
        "movs   lr,pc        \r\n"
    );
}
#endif /* !defined(BOOTLOADER) */


/* TODO:
  a) this is not the place for this function
  b) it currently ignores the supplied frequency and uses default values
  c) if the PLL being set drives any PCKs, an appropriate new clock divider
     will have to be re-calculated for those PCKs (the OF maintains a list of
     PCK frequencies for this purpose).
*/
void set_pll_frequency(unsigned int pll_number, unsigned int frequency)
{
    int i = 0;

    if (pll_number > 1) return;

    /* The frequency parameter is currently ignored and temporary values are
       used (PLL0=192Mhz, PLL1=216Mhz). The D2 firmware uses a lookup table
       to derive the values of PLLxCFG from a the supplied frequency.
       Presumably we will need to do something similar. */
    if (pll_number == 0)
    {
        /* drive CPU off Xin while switching */
        CLKCTRL = 0xB00FF014;   /* Xin enable, Fsys driven by Xin, Fbus = Fsys,
                                   MCPU=Fbus, SCPU=Fbus */

        asm volatile (
            "nop      \n\t"
            "nop      \n\t"
        );
    
        PLL0CFG |= (1<<31);                /* power down */
        CLKDIVC = CLKDIVC &~ (0xff << 24); /* disable PLL0 divider */
        PLL0CFG = 0x80019808;              /* set for 192Mhz (with power down) */
        PLL0CFG = PLL0CFG &~ (1<<31);      /* power up */

        CLKCTRL = (CLKCTRL & ~0x1f) | 0x800FF010;
        
        asm volatile (
            "nop      \n\t"
            "nop      \n\t"
        );
    }
    else if (pll_number == 1)
    {
        PLL1CFG |= (1<<31);                /* power down */
        CLKDIVC = CLKDIVC &~ (0xff << 16); /* disable PLL1 divider */
        PLL1CFG = 0x80002503;              /* set for 216Mhz (with power down)*/
        PLL1CFG = PLL1CFG &~ (1<<31);      /* power up */
    }

    i = 0x1000;
    while (--i) {};
}


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
    CLKCTRL = (CLKCTRL & ~0xff) | 0x14;
    
    PCLK_RFREQ = 0x1401002d; /* RAM refresh source = Xin (4) / 0x2d = 266kHz */
    
    MCFG |= 1;
    SDCFG = (SDCFG &~ 0x7000) | 0x2000;
    
    MCFG1 |= 1;
    SDCFG1 = (SDCFG &~ 0x7000) | 0x2000;

    PLL0CFG |= 0x80000000;  /* power down */
    PLL0CFG  = 0x14010000;  /* power up, source = Xin (4) undivided = 12Mhz */

    i = 0x8000;
    while (--i) {};
    
    CLKCTRL = (CLKCTRL &~ 0x1f) | 0x800FF010; /* CPU and COP driven by PLL0 */

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
    MIRQ = -1;

    gpio_init();
    clock_init();

    /* TODO: these almost certainly shouldn't be here */
    set_pll_frequency(0, 192000000);    /* drives CPU */
    set_pll_frequency(1, 216000000);    /* drives LCD PXCLK - divided by 2 */
}
#endif


void system_reboot(void)
{
    #warning function not implemented
}

int system_memory_guard(int newmode)
{
    #warning function not implemented
    
    (void)newmode;
    return 0;
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ

void set_cpu_frequency(long frequency)
{
    #warning function not implemented
    (void)frequency;
}

#endif
