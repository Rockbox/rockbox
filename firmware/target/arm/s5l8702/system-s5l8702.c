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
#include "i2c-s5l8702.h"
#include "pmu-target.h"
#include "uart-target.h"
#include "gpio-s5l8702.h"
#include "dma-s5l8702.h"
#include "clocking-s5l8702.h"

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIRQ"))) void name (void)

void irq_handler(void) __attribute__((interrupt ("IRQ"), naked));
void fiq_handler(void) __attribute__((interrupt ("FIQ"), naked, \
                                      weak, alias("fiq_dummy")));

default_interrupt(INT_EXT0);    /* GPIOIC group 6 (GPIO 0..31) */
default_interrupt(INT_EXT1);    /* GPIOIC group 5 (GPIO 32..63) */
default_interrupt(INT_EXT2);    /* GPIOIC group 4 (GPIO 64..95) */
default_interrupt(INT_EXT3);    /* GPIOIC group 3 (GPIO 96..123) */
default_interrupt(INT_IRQ4);
default_interrupt(INT_IRQ5);
default_interrupt(INT_IRQ6);
default_interrupt(INT_TIMERE);  /* IRQ7: 32-bit timers */
default_interrupt(INT_TIMERF);
default_interrupt(INT_TIMERG);
default_interrupt(INT_TIMERH);
default_interrupt(INT_TIMERA);  /* IRQ8: 16-bit timers */
default_interrupt(INT_TIMERB);
default_interrupt(INT_TIMERC);
default_interrupt(INT_TIMERD);
default_interrupt(INT_IRQ9);
default_interrupt(INT_IRQ10);
default_interrupt(INT_IRQ11);
default_interrupt(INT_IRQ12);
default_interrupt(INT_IRQ13);
default_interrupt(INT_IRQ14);
default_interrupt(INT_IRQ15);
default_interrupt(INT_DMAC0);
default_interrupt(INT_DMAC1);
default_interrupt(INT_IRQ18);
default_interrupt(INT_USB_FUNC);
default_interrupt(INT_IRQ20);
default_interrupt(INT_IRQ21);
default_interrupt(INT_IRQ22);
default_interrupt(INT_WHEEL);
default_interrupt(INT_UART0);
default_interrupt(INT_UART1);
default_interrupt(INT_UART2);
default_interrupt(INT_UART3);
default_interrupt(INT_IRQ28);   /* obsolete/not implemented UART4 ??? */
default_interrupt(INT_ATA);
default_interrupt(INT_IRQ30);
default_interrupt(INT_EXT4);    /* GPIOIC group 2 (not used) */
default_interrupt(INT_EXT5);    /* GPIOIC group 1 (not used) */
default_interrupt(INT_EXT6);    /* GPIOIC group 0 */
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
}

void INT_TIMER32(void) ICODE_ATTR;
void INT_TIMER32()
{
    uint32_t tstat = TSTAT;
    if ((TECON >> 12) & 0x7 & (tstat >> 24)) INT_TIMERE();
    if ((TFCON >> 12) & 0x7 & (tstat >> 16)) INT_TIMERF();
    if ((TGCON >> 12) & 0x7 & (tstat >> 8)) INT_TIMERG();
    if ((THCON >> 12) & 0x7 & tstat) INT_TIMERH();
}

static void (* const irqvector[])(void) =
{
    INT_EXT0,INT_EXT1,INT_EXT2,INT_EXT3,INT_IRQ4,INT_IRQ5,INT_IRQ6,INT_TIMER32,
    INT_TIMER,INT_IRQ9,INT_IRQ10,INT_IRQ11,INT_IRQ12,INT_IRQ13,INT_IRQ14,INT_IRQ15,
    INT_DMAC0,INT_DMAC1,INT_IRQ18,INT_USB_FUNC,INT_IRQ20,INT_IRQ21,INT_IRQ22,INT_WHEEL,
    INT_UART0,INT_UART1,INT_UART2,INT_UART3,INT_IRQ28,INT_ATA,INT_IRQ30,INT_EXT4,
    INT_EXT5,INT_EXT6,INT_IRQ34,INT_IRQ35,INT_IRQ36,INT_IRQ37,INT_IRQ38,INT_IRQ39,
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
    (void)dummy;
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

static struct clocking_mode clk_modes[] =
{
   /* cdiv  hdiv  hprat  hsdiv */    /* CClk  HClk  PClk  SM1Clk  FPS */
    { 1,    2,    2,     4 },        /* 216   108   54    27      42  */
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    { 4,    4,    2,     2 },        /* 54    54    27    27      21  */
#endif
};
#define N_CLK_MODES (sizeof(clk_modes) / sizeof(struct clocking_mode))

enum {
    CLK_BOOST = 0,
    CLK_UNBOOST = N_CLK_MODES - 1,
};

void system_init(void)
{
    /*
     * Bootloader seems to give a blank screen when IRAM1 is disabled
     * - FW 10/13/19
     */
#ifndef BOOTLOADER
    /* disable IRAM1 (not used because it is slower than DRAM) */
    clockgate_enable(CLOCKGATE_SM1, false);
#endif

    clocking_init(clk_modes, 0);
#ifndef BOOTLOADER
    gpio_preinit();
    i2c_preinit(0);
    pmu_preinit();
#endif
    gpio_init();
    eint_init();
    dma_init();
#ifdef HAVE_SERIAL
    uart_init();
#endif
    VIC0INTENABLE = 1 << IRQ_WHEEL;
    VIC0INTENABLE = 1 << IRQ_ATA;
    VIC1INTENABLE = 1 << (IRQ_MMC - 32);
    VIC0INTENABLE = 1 << IRQ_TIMER;
    VIC0INTENABLE = 1 << IRQ_TIMER32;
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

    if (frequency == CPUFREQ_MAX)
    {
        pmu_write(0x1e, 0x13);  /* Vcore = 1100 mV */
        set_clocking_level(CLK_BOOST);
    }
    else
    {
        set_clocking_level(CLK_UNBOOST);
        pmu_write(0x1e, 0xf);   /* Vcore = 1000 mV */
    }

    cpu_frequency = frequency;
}
#endif

static void set_page_tables(void)
{
    /* map RAM to itself and enable caching for it */
    map_section(0, 0, 0x380, CACHE_ALL);

    /* disable caching for I/O area */
    map_section(0x38000000, 0x38000000, 0x80, CACHE_NONE);

    /* map RAM uncached addresses */
    map_section(0, S5L8702_UNCACHED_ADDR(0x0), 0x380, CACHE_NONE);
}

void memory_init(void)
{
    ttb_init();
    set_page_tables();
    enable_mmu();
}

#ifdef BOOTLOADER
#include <stdbool.h>

static void syscon_preinit(void)
{
    /* after ROM boot, CG16_SYS is using PLL0 @108 MHz
       CClk = 108 MHz, HClk = 54 MHz, PClk = 27 MHz */

    CLKCON0 &= ~CLKCON0_SDR_DISABLE_BIT;

    PLLMODE &= ~PLLMODE_OSCSEL_BIT; /* CG16_SEL_OSC = OSC0 */
    cg16_config(&CG16_SYS, true, CG16_SEL_OSC, 1, 1);
    soc_set_system_divs(1, 1, 1);

    /* stop all PLLs */
    for (int pll = 0; pll < 3; pll++)
        pll_onoff(pll, false);

    pll_config(2, PLLOP_DM, 1, 36, 1, 32400);
    pll_onoff(2, true);
    soc_set_system_divs(1, 2, 2 /*hprat*/);
    cg16_config(&CG16_SYS,   true,  CG16_SEL_PLL2, 1, 1);
    cg16_config(&CG16_2L,    false, CG16_SEL_OSC,  1, 1);
    cg16_config(&CG16_SVID,  false, CG16_SEL_OSC,  1, 1);
    cg16_config(&CG16_AUD0,  false, CG16_SEL_OSC,  1, 1);
    cg16_config(&CG16_AUD1,  false, CG16_SEL_OSC,  1, 1);
    cg16_config(&CG16_AUD2,  false, CG16_SEL_OSC,  1, 1);
    cg16_config(&CG16_RTIME, true,  CG16_SEL_OSC,  1, 1);
    cg16_config(&CG16_5L,    false, CG16_SEL_OSC,  1, 1);

    soc_set_hsdiv(1);

    PWRCON_AHB = ~((1 << CLOCKGATE_SMx) |
                   (1 << CLOCKGATE_SM1));
    PWRCON_APB = ~((1 << (CLOCKGATE_TIMER - 32)) |
                   (1 << (CLOCKGATE_GPIO - 32)));
}

static void miu_preinit(bool selfrefreshing)
{
    if (selfrefreshing)
        MIUCON = 0x11;      /* TBC: self-refresh -> IDLE */

    MIUCON = 0x80D;         /* remap = 1 (IRAM mapped to 0x0),
                               TBC: SDRAM bank and column configuration */
    MIU_REG(0xF0) = 0x0;

    MIUAREF = 0x6105D;      /* Auto-Refresh enabled,
                               Row refresh interval = 0x5d/12MHz = 7.75 uS */
    MIUSDPARA = 0x1FB621;

    MIU_REG(0x200) = 0x1845;
    MIU_REG(0x204) = 0x1845;
    MIU_REG(0x210) = 0x1800;
    MIU_REG(0x214) = 0x1800;
    MIU_REG(0x220) = 0x1845;
    MIU_REG(0x224) = 0x1845;
    MIU_REG(0x230) = 0x1885;
    MIU_REG(0x234) = 0x1885;
    MIU_REG(0x14) = 0x19;       /* 2^19 = 0x2000000 = SDRAMSIZE (32Mb) */
    MIU_REG(0x18) = 0x19;       /* 2^19 = 0x2000000 = SDRAMSIZE (32Mb) */
    MIU_REG(0x1C) = 0x790682B;
    MIU_REG(0x314) &= ~0x10;

    for (int i = 0; i < 0x24; i++)
        MIU_REG(0x2C + i*4) &= ~(1 << 24);

    MIU_REG(0x1CC) = 0x540;
    MIU_REG(0x1D4) |= 0x80;

    MIUCOM = 0x33;     /* No action CMD */
    MIUCOM = 0x33;
    MIUCOM = 0x233;    /* Precharge all banks CMD */
    MIUCOM = 0x33;
    MIUCOM = 0x33;
    MIUCOM = 0x33;
    MIUCOM = 0x333;    /* Auto-refresh CMD */
    MIUCOM = 0x33;
    MIUCOM = 0x33;
    MIUCOM = 0x33;
    MIUCOM = 0x333;    /* Auto-refresh CMD */
    MIUCOM = 0x33;
    MIUCOM = 0x33;
    MIUCOM = 0x33;

    if (!selfrefreshing)
    {
        MIUMRS = 0x33;    /* MRS: Bust Length = 8, CAS = 3 */
        MIUCOM = 0x133;   /* Mode Register Set CMD */
        MIUCOM = 0x33;
        MIUCOM = 0x33;
        MIUCOM = 0x33;
        MIUMRS = 0x8040;  /* EMRS: Strength = 1/4, Self refresh area = Full */
        MIUCOM = 0x133;   /* Mode Register Set CMD */
        MIUCOM = 0x33;
        MIUCOM = 0x33;
        MIUCOM = 0x33;
    }

    MIUAREF |= 0x61000;   /* Auto-refresh enabled */
}

/* Preliminary HW initialization */
void system_preinit(void)
{
    bool gpio3out, coldboot;

    syscon_preinit();
    gpio_preinit();
    i2c_preinit(0);

    /* get (previously) configured output selection for GPIO3 */
    gpio3out = (pmu_rd(PCF5063X_REG_GPIO3CFG) & 7);
    /* coldboot: when set, device has been in NoPower state */
    coldboot = (pmu_rd(PCF5063X_REG_OOCSHDWN) & PCF5063X_OOCSHDWN_COLDBOOT);
    pmu_preinit();

    miu_preinit(!coldboot && !gpio3out);
}
#endif
