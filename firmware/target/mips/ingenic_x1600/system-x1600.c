/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 *
 * Based on firmware/target/mips/ingenic_x1000/system-x1000.c,
 * Copyright (C) 2021 Aidan MacDonald
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

/* X1600 uses CP0 IP4 for the OST and a different INTC interrupt map. */

#include "system.h"
#include "boot-x1600.h"
#include "mips.h"
#include "panic.h"
#include "gpio-x1600.h"
#include "dma-ingenic.h"
#include "irq-x1600.h"
#include "clk-x1600.h"
#include "x1600/cpm.h"
#include "x1600/ost.h"
#include "x1600/tcu.h"
#include "x1600/wdt.h"
#include "x1600/intc.h"

#ifdef INGENIC_CPUIDLE_STATS
int __cpu_idle_avg = 0;
int __cpu_idle_cur = 0;
uint32_t __cpu_idle_ticks = 0;
uint32_t __cpu_idle_reftick = 0;
#endif

/* 8 channels plus the legacy in-TCU OST and WDT flags */
#define TCU_CHANNEL_BITS    0x000000ff
#define TCU_HALF_BITS       0x00ff0000
#define TCU_STOP_OST_BIT    (1u << 15)
#define TCU_STOP_WDT_BIT    (1u << 16)

/* Prepare the CPU to process interrupts, but don't enable them yet */
static void system_init_irq(void) INIT_ATTR;
static void system_init_irq(void)
{
    /* Mask all interrupts -- a warm boot will not have reset ICMRn */
    jz_set(INTC_MSK(0), 0xffffffff);
    jz_set(INTC_MSK(1), 0xffffffff);

    /* Safe to unmask unconditionally. Not the TCU, unlike the X1000: the PM
     * does not say which line a channel raises, so timer-x1600.c enables it. */
    jz_clr(INTC_MSK(0), IRQ_TO_BIT(IRQ_GPIO0) | IRQ_TO_BIT(IRQ_GPIO1) |
                        IRQ_TO_BIT(IRQ_GPIO2) | IRQ_TO_BIT(IRQ_GPIO3));

    /* Setup CP0 registers. IM4, not the X1000's IM3: the OST reaches CP0
     * directly rather than through the INTC. */
    write_c0_status(M_StatusCU0 | M_StatusIM2 | M_StatusIM4);
    write_c0_cause(M_CauseIV);
}

/* First function called by crt0.S */
void system_early_init(void)
{
    /* Finish up clock init, unless the bootloader already did it */
    if(get_boot_flag(BOOT_FLAG_CLK_INIT))
        return;

    clk_init();
    set_boot_flag(BOOT_FLAG_CLK_INIT);
}

/* First thing called from Rockbox main() */
void system_init(void)
{
    /* Gate all clocks except DDR/bus/RTC; gating APB0 makes every APB register
     * read garbage. No CPU gate here -- the X1000's CPU_BIT is Reserved. */
    REG_CPM_CLKGR = ~jz_orm(CPM_CLKGR, DDR, AHB0, APB0, RTC);

    /* Ungate the INTC: it comes out of reset gated and nothing else does it,
     * and while gated every register reads alike and mask writes are lost */
    jz_writef(CPM_CLKGR1, INTC(0));

    /* Ungate timers and turn them all off by default */
    jz_writef(CPM_CLKGR, TCU(0), OST(0));

    jz_clrf(OST_ENABLE, OST1, OST2);
    jz_write(OST_1MSK, 1);
    jz_write(OST_1FLG, 0);

    jz_clr(TCU_ENABLE, TCU_CHANNEL_BITS | BM_TCU_ENABLE_OSTEN);
    jz_set(TCU_MASK, TCU_CHANNEL_BITS | TCU_HALF_BITS |
                     BM_TCU_MASK_OSTMASK | BM_TCU_MASK_HMASKW);
    jz_clr(TCU_FLAG, TCU_CHANNEL_BITS | TCU_HALF_BITS |
                     BM_TCU_FLAG_OSTFLAG | BM_TCU_FLAG_WDTFLAG);
    jz_set(TCU_STOP, TCU_CHANNEL_BITS | TCU_STOP_WDT_BIT);

    /* The X1000 sets this bit; clear it, since two things here are called OST
     * and the PM does not say which one it stops */
    jz_clr(TCU_STOP, TCU_STOP_OST_BIT);

    /* Start OST2, needed for delay timer; /4 must agree with OST_FREQUENCY */
    jz_writef(OST_CTRL, PRESCALE2_V(BY_4));
    jz_writef(OST_CLEAR, OST2(1));
    jz_setf(OST_ENABLE, OST2);

    /* Ensure CPU sleep mode is IDLE and not SLEEP */
    jz_writef(CPM_LCR, LPM_V(IDLE));

    /* All other init */
    gpio_init();
    system_init_irq();
    dma_init();
    mmu_init();
}

void system_reboot(void)
{
    /* The WDT shares the TCU block, so clear its stop bit before it counts.
     * The PM contradicts itself on EXT_EN's position -- see x1600/wdt.h. */
    jz_clr(TCU_STOP, TCU_STOP_WDT_BIT);
    jz_writef(WDT_CTRL, PRESCALE_V(BY_4), SOURCE_V(EXT));
    jz_write(WDT_COUNT, 0);
    jz_write(WDT_DATA, X1600_EXCLK_FREQ / 1000);
    jz_write(WDT_ENABLE, 1);
    while(1);
}

int system_memory_guard(int mode)
{
    /* unused */
    (void)mode;
    return 0;
}

/* Simple delay API -- slow path functions */

static int irq = 0;
static unsigned ipr0 = 0, ipr1 = 0;

static void UIRQ(void)
{
    panicf("Unhandled interrupt occurred: %d", irq);
}

/* Main interrupts. Weak aliases to UIRQ, so a driver defining the matching
 * name takes over and anything unclaimed panics with its number. */
#define intr(name) extern __attribute__((weak, alias("UIRQ"))) void name(void)

/* DWC2 USB interrupt */
#define OTG INT_USB_FUNC

/* Group 0 sources, in ICSR0 bit order. Names match x1600/intc.h. */
intr(AUDIO); intr(OTG);   intr(PDMA);  intr(PDMAD); intr(PDMAM); intr(PWM);
intr(SFC);   intr(SSI0);  intr(SADC);  intr(HASH);  intr(AES);   intr(TCU2);
intr(TCU1);  intr(TCU0);  intr(MIPI_CSI); intr(CIM); intr(LCD);

/* Group 1 sources, in ICSR1 bit order */
intr(RTC);   intr(SOFT);  intr(DTRNG); intr(MSC1);  intr(MSC0);  intr(CAN0);
intr(CAN1);  intr(CDBUS); intr(UART3); intr(UART2); intr(UART1); intr(UART0);
intr(HARB2); intr(HARB0); intr(CPM);   intr(DDR);   intr(EFUSE); intr(GMAC);
intr(I2C1);  intr(I2C0);  intr(SSI_SLV);

/* GPIO A - 32 pins */
intr(GPIOA00); intr(GPIOA01); intr(GPIOA02); intr(GPIOA03); intr(GPIOA04);
intr(GPIOA05); intr(GPIOA06); intr(GPIOA07); intr(GPIOA08); intr(GPIOA09);
intr(GPIOA10); intr(GPIOA11); intr(GPIOA12); intr(GPIOA13); intr(GPIOA14);
intr(GPIOA15); intr(GPIOA16); intr(GPIOA17); intr(GPIOA18); intr(GPIOA19);
intr(GPIOA20); intr(GPIOA21); intr(GPIOA22); intr(GPIOA23); intr(GPIOA24);
intr(GPIOA25); intr(GPIOA26); intr(GPIOA27); intr(GPIOA28); intr(GPIOA29);
intr(GPIOA30); intr(GPIOA31);
/* GPIO B - 32 pins */
intr(GPIOB00); intr(GPIOB01); intr(GPIOB02); intr(GPIOB03); intr(GPIOB04);
intr(GPIOB05); intr(GPIOB06); intr(GPIOB07); intr(GPIOB08); intr(GPIOB09);
intr(GPIOB10); intr(GPIOB11); intr(GPIOB12); intr(GPIOB13); intr(GPIOB14);
intr(GPIOB15); intr(GPIOB16); intr(GPIOB17); intr(GPIOB18); intr(GPIOB19);
intr(GPIOB20); intr(GPIOB21); intr(GPIOB22); intr(GPIOB23); intr(GPIOB24);
intr(GPIOB25); intr(GPIOB26); intr(GPIOB27); intr(GPIOB28); intr(GPIOB29);
intr(GPIOB30); intr(GPIOB31);
/* GPIO C has 32 pins where the X1000 has 26; PC31 is KEY_POWER here */
intr(GPIOC00); intr(GPIOC01); intr(GPIOC02); intr(GPIOC03); intr(GPIOC04);
intr(GPIOC05); intr(GPIOC06); intr(GPIOC07); intr(GPIOC08); intr(GPIOC09);
intr(GPIOC10); intr(GPIOC11); intr(GPIOC12); intr(GPIOC13); intr(GPIOC14);
intr(GPIOC15); intr(GPIOC16); intr(GPIOC17); intr(GPIOC18); intr(GPIOC19);
intr(GPIOC20); intr(GPIOC21); intr(GPIOC22); intr(GPIOC23); intr(GPIOC24);
intr(GPIOC25); intr(GPIOC26); intr(GPIOC27); intr(GPIOC28); intr(GPIOC29);
intr(GPIOC30); intr(GPIOC31);
/* GPIO D - 6 pins (PD00..PD05, the MSC1/microSD group) */
intr(GPIOD00); intr(GPIOD01); intr(GPIOD02); intr(GPIOD03); intr(GPIOD04);
intr(GPIOD05);

/* OST interrupt -- has no INTC number since it arrives on CP0 IP4 */
intr(OST);

#undef intr

/* Index by INTC source number; reserved entries use UIRQ. */
static void(*irqvector[IRQ_COUNT])(void) = {
    /* ICSR0: 0 - 31 */
    /*  0 */ AUDIO,  /*  1 */ OTG,    /*  2 */ UIRQ,   /*  3 */ PDMA,
    /*  4 */ PDMAD,  /*  5 */ PDMAM,  /*  6 */ PWM,    /*  7 */ SFC,
    /*  8 */ UIRQ,   /*  9 */ SSI0,   /* 10 */ UIRQ,   /* 11 */ SADC,
    /* 12 */ UIRQ,   /* 13 */ UIRQ,
    /* 14..17 are the GPIO port sources; vector_irq() replaces them with a
     * per-pin number, so reaching one here is spurious */
    /* 14 */ UIRQ,   /* 15 */ UIRQ,   /* 16 */ UIRQ,   /* 17 */ UIRQ,
    /* 18 */ UIRQ,   /* 19 */ UIRQ,
    /* 20 */ UIRQ,   /* 21 */ UIRQ,   /* 22 */ HASH,   /* 23 */ AES,
    /* 24 */ UIRQ,   /* 25 */ TCU2,   /* 26 */ TCU1,   /* 27 */ TCU0,
    /* 28 */ MIPI_CSI, /* 29 */ UIRQ, /* 30 */ CIM,    /* 31 */ LCD,
    /* ICSR1: 32 - 63 */
    /* 32 */ RTC,    /* 33 */ SOFT,   /* 34 */ DTRNG,  /* 35 */ UIRQ,
    /* 36 */ MSC1,   /* 37 */ MSC0,   /* 38 */ UIRQ,   /* 39 */ UIRQ,
    /* 40 */ CAN0,   /* 41 */ CAN1,   /* 42 */ CDBUS,  /* 43 */ UIRQ,
    /* 44 */ UART3,  /* 45 */ UART2,  /* 46 */ UART1,  /* 47 */ UART0,
    /* 48 */ UIRQ,   /* 49 */ HARB2,  /* 50 */ HARB0,  /* 51 */ CPM,
    /* 52 */ DDR,    /* 53 */ UIRQ,   /* 54 */ EFUSE,  /* 55 */ GMAC,
    /* 56 */ UIRQ,   /* 57 */ UIRQ,   /* 58 */ UIRQ,   /* 59 */ UIRQ,
    /* 60 */ I2C1,   /* 61 */ I2C0,   /* 62 */ SSI_SLV, /* 63 */ UIRQ,
    /* GPIO A: 64 - 95 */
    GPIOA00, GPIOA01, GPIOA02, GPIOA03, GPIOA04, GPIOA05, GPIOA06, GPIOA07,
    GPIOA08, GPIOA09, GPIOA10, GPIOA11, GPIOA12, GPIOA13, GPIOA14, GPIOA15,
    GPIOA16, GPIOA17, GPIOA18, GPIOA19, GPIOA20, GPIOA21, GPIOA22, GPIOA23,
    GPIOA24, GPIOA25, GPIOA26, GPIOA27, GPIOA28, GPIOA29, GPIOA30, GPIOA31,
    /* GPIO B: 96 - 127 */
    GPIOB00, GPIOB01, GPIOB02, GPIOB03, GPIOB04, GPIOB05, GPIOB06, GPIOB07,
    GPIOB08, GPIOB09, GPIOB10, GPIOB11, GPIOB12, GPIOB13, GPIOB14, GPIOB15,
    GPIOB16, GPIOB17, GPIOB18, GPIOB19, GPIOB20, GPIOB21, GPIOB22, GPIOB23,
    GPIOB24, GPIOB25, GPIOB26, GPIOB27, GPIOB28, GPIOB29, GPIOB30, GPIOB31,
    /* GPIO C: 128 - 159 */
    GPIOC00, GPIOC01, GPIOC02, GPIOC03, GPIOC04, GPIOC05, GPIOC06, GPIOC07,
    GPIOC08, GPIOC09, GPIOC10, GPIOC11, GPIOC12, GPIOC13, GPIOC14, GPIOC15,
    GPIOC16, GPIOC17, GPIOC18, GPIOC19, GPIOC20, GPIOC21, GPIOC22, GPIOC23,
    GPIOC24, GPIOC25, GPIOC26, GPIOC27, GPIOC28, GPIOC29, GPIOC30, GPIOC31,
    /* GPIO D: 160 - 165 */
    GPIOD00, GPIOD01, GPIOD02, GPIOD03, GPIOD04, GPIOD05,
};

irq_handler_t system_set_irq_handler(int irq, irq_handler_t handler)
{
    irq_handler_t old_handler = irqvector[irq];
    irqvector[irq] = handler;
    return old_handler;
}

void system_enable_irq(int irq)
{
    if(IRQ_IS_GROUP0(irq)) {
        jz_clr(INTC_MSK(0), IRQ_TO_BIT(irq));
    } else if(IRQ_IS_GROUP1(irq)) {
        jz_clr(INTC_MSK(1), IRQ_TO_BIT(irq));
    }
    /* Pin interrupts are unmasked by gpio_enable_irq() */
}

void system_disable_irq(int irq)
{
    if(IRQ_IS_GROUP0(irq)) {
        jz_set(INTC_MSK(0), IRQ_TO_BIT(irq));
    } else if(IRQ_IS_GROUP1(irq)) {
        jz_set(INTC_MSK(1), IRQ_TO_BIT(irq));
    }
}

static int vector_gpio_irq(int port)
{
    /* PxFLG is read-only, PxFLGC write-1-to-clear; there is no PxFLGS here */
    int n = find_first_set_bit(REG_GPIO_FLAG(port));
    if(n & 32)
        return -1;

    jz_clr(GPIO_FLAG(port), 1 << n);
    return IRQ_GPIO(port, n);
}

static int vector_irq(void)
{
    int n = find_first_set_bit(ipr0);
    if(n & 32) {
        n = find_first_set_bit(ipr1);
        if(n & 32)
            return -1;
        ipr1 &= ~(1 << n);
        n += 32;
    } else {
        ipr0 &= ~(1 << n);
    }

    /* Demux the GPIO port sources, numbered high-to-low: port A is bit 17 */
    switch(n) {
    case IRQ_GPIO0: n = vector_gpio_irq(GPIO_A); break;
    case IRQ_GPIO1: n = vector_gpio_irq(GPIO_B); break;
    case IRQ_GPIO2: n = vector_gpio_irq(GPIO_C); break;
    case IRQ_GPIO3: n = vector_gpio_irq(GPIO_D); break;
    default: break;
    }

    return n;
}

void intr_handler(void)
{
    unsigned long cause = read_c0_cause();

    /* OST interrupt -- handled separately, and IP4 rather than IP3 */
    if(cause & M_CauseIP4) {
        OST();
        return;
    }

    /* Gather pending interrupts. ICPRn is the post-mask pending word. */
    ipr0 |= REG_INTC_PND(0);
    ipr1 |= REG_INTC_PND(1);

    /* Process and dispatch interrupt */
    irq = vector_irq();
    if(irq < 0)
        return;

    irqvector[irq]();
}

void system_exception_wait(void)
{
    /* The X1000 targets poll for a key combo here; two of the R1's five keys
     * are on the SADC, so hang instead */
    while(1);
}

/* Slow-path delay API. */
void __udelay(uint32_t us)
{
    while(us > MAX_UDELAY_ARG) {
        __ost_delay(MAX_UDELAY_ARG * OST_TICKS_PER_US);
        us -= MAX_UDELAY_ARG;
    }

    __ost_delay(us * OST_TICKS_PER_US);
}

void __mdelay(uint32_t ms)
{
    while(ms > MAX_MDELAY_ARG) {
        __ost_delay(MAX_MDELAY_ARG * 1000 * OST_TICKS_PER_US);
        ms -= MAX_MDELAY_ARG;
    }

    __ost_delay(ms * 1000 * OST_TICKS_PER_US);
}

uint64_t __ost_read64(void)
{
    /* Reading OST2CNTL latches the high half; do not let an IRQ split them */
    int irq = disable_irq_save();
    uint64_t lcnt = REG_OST_2CNTL;
    uint64_t hcnt = REG_OST_2CNTHB;
    restore_irq(irq);
    return (hcnt << 32) | lcnt;
}
