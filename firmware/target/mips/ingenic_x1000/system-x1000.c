/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
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

#include "system.h"
#include "mips.h"
#include "panic.h"
#include "button.h"
#include "gpio-x1000.h"
#include "dma-x1000.h"
#include "irq-x1000.h"
#include "clk-x1000.h"
#include "boot-x1000.h"
#include "x1000/cpm.h"
#include "x1000/ost.h"
#include "x1000/tcu.h"
#include "x1000/wdt.h"
#include "x1000/intc.h"
#include "x1000/msc.h"
#include "x1000/aic.h"

#ifdef X1000_CPUIDLE_STATS
int __cpu_idle_avg = 0;
int __cpu_idle_cur = 0;
uint32_t __cpu_idle_ticks = 0;
uint32_t __cpu_idle_reftick = 0;
#endif

/* Prepare the CPU to process interrupts, but don't enable them yet */
static void system_init_irq(void)
{
    /* Mask all interrupts */
    jz_set(INTC_MSK(0), 0xffffffff);
    jz_set(INTC_MSK(1), 0xffffffff);

    /* It's safe to unmask these unconditionally */
    jz_clr(INTC_MSK(0), (1 << IRQ0_GPIO0) | (1 << IRQ0_GPIO1) |
                        (1 << IRQ0_GPIO2) | (1 << IRQ0_GPIO3) |
                        (1 << IRQ0_TCU1));

    /* Setup CP0 registers */
    write_c0_status(M_StatusCU0 | M_StatusIM2 | M_StatusIM3);
    write_c0_cause(M_CauseIV);
}

/* First function called by crt0.S */
void system_early_init(void)
{
#if defined(FIIO_M3K) && !defined(BOOTLOADER)
    /* HACK for compatibility: CPM scratchpad has undefined contents at
     * time of reset and old bootloader revisions don't initialize it.
     * Therefore we can't rely on its contents on the FiiO M3K. This does
     * kind of break the entire point of boot flags, but right now they
     * are really only used by the bootloader so it's not a huge issue.
     * This hack should keep everything working as usual. */
    if(jz_readf(CPM_MPCR, ON) == 0) {
        init_boot_flags();
        set_boot_flag(BOOT_FLAG_CLK_INIT);
    }
#endif

    /* Finish up clock init */
    clk_init();
}

/* First thing called from Rockbox main() */
void system_init(void)
{
    /* Gate all clocks except CPU/bus/memory/RTC */
    REG_CPM_CLKGR = ~jz_orm(CPM_CLKGR, CPU_BIT, DDR, AHB0, APB0, RTC);

    /* Ungate timers and turn them all off by default */
    jz_writef(CPM_CLKGR, TCU(0), OST(0));
    jz_clrf(OST_ENABLE, OST1, OST2);
    jz_write(OST_1MSK, 1);
    jz_write(OST_1FLG, 0);
    jz_clr(TCU_ENABLE, 0x80ff);
    jz_set(TCU_MASK, 0xff10ff);
    jz_clr(TCU_FLAG, 0xff10ff);
    jz_set(TCU_STOP, 0x180ff);

    /* Start OST2, needed for delay timer */
    jz_writef(OST_CTRL, PRESCALE2_V(BY_4));
    jz_writef(OST_CLEAR, OST2(1));
    jz_write(OST_2CNTH, 0);
    jz_write(OST_2CNTL, 0);
    jz_setf(OST_ENABLE, OST2);

    /* Ensure CPU sleep mode is IDLE and not SLEEP */
    jz_writef(CPM_LCR, LPM_V(IDLE));

    /* All other init */
    gpio_init();
    system_init_irq();
    dma_init();
    mmu_init();
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency(long tgt_freq)
{
    /* Clamp target frequency to "sane" values */
    if(tgt_freq < 0)        tgt_freq = 0;
    if(tgt_freq > CPU_FREQ) tgt_freq = CPU_FREQ;

    /* Find out input clock */
    uint32_t in_freq;
    switch(jz_readf(CPM_CCR, SEL_CPLL)) {
    case 1: in_freq = clk_get(X1000_CLK_SCLK_A); break;
    case 2: in_freq = clk_get(X1000_CLK_MPLL); break;
    default: return;
    }

    /* Clamp to valid range */
    if(tgt_freq < 1)
        tgt_freq = 1;
    if(tgt_freq > (long)in_freq)
        tgt_freq = in_freq;

    /* Calculate CPU clock divider */
    uint32_t cdiv = clk_calc_div(in_freq, tgt_freq);
    if(cdiv > 16) cdiv = 16;
    if(cdiv < 1)  cdiv = 1;

    /* Calculate L2 cache clock. */
    uint32_t l2div = cdiv;
    if(cdiv == 1)
        l2div = 2;

    /* Change CPU/L2 frequency */
    jz_writef(CPM_CCR, CE_CPU(1), L2DIV(l2div - 1), CDIV(cdiv - 1));
    while(jz_readf(CPM_CSR, CDIV_BUSY));
    jz_writef(CPM_CCR, CE_CPU(0));

    /* Update value for Rockbox */
    cpu_frequency = in_freq / cdiv;
}
#endif

void system_reboot(void)
{
    jz_clr(TCU_STOP, 0x10000);
    jz_writef(WDT_CTRL, PRESCALE_V(BY_4), SOURCE_V(EXT));
    jz_write(WDT_COUNT, 0);
    jz_write(WDT_DATA, X1000_EXCLK_FREQ / 1000);
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
    int irq = disable_irq_save();
    uint64_t lcnt = REG_OST_2CNTL;
    uint64_t hcnt = REG_OST_2CNTHB;
    restore_irq(irq);
    return (hcnt << 32) | lcnt;
}

/* IRQ handling */
static int irq = 0;
static unsigned ipr0 = 0, ipr1 = 0;

static void UIRQ(void)
{
    panicf("Unhandled interrupt occurred: %d", irq);
}

#define intr(name) extern __attribute__((weak, alias("UIRQ"))) void name(void)
/* DWC2 USB interrupt */
#define OTG INT_USB_FUNC

/* Main interrupts */
intr(DMIC); intr(AIC);   intr(SFC);   intr(SSI0);  intr(OTG);   intr(AES);
intr(TCU2); intr(TCU1);  intr(TCU0);  intr(CIM);   intr(LCD);   intr(RTC);
intr(MSC1); intr(MSC0);  intr(SCC);   intr(PCM0);  intr(HARB2); intr(HARB0);
intr(CPM);  intr(UART2); intr(UART1); intr(UART0); intr(DDR);   intr(EFUSE);
intr(MAC);  intr(I2C2);  intr(I2C1);  intr(I2C0);  intr(JPEG);
intr(PDMA); intr(PDMAD); intr(PDMAM);
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
/* GPIO C - 26 pins */
intr(GPIOC00); intr(GPIOC01); intr(GPIOC02); intr(GPIOC03); intr(GPIOC04);
intr(GPIOC05); intr(GPIOC06); intr(GPIOC07); intr(GPIOC08); intr(GPIOC09);
intr(GPIOC10); intr(GPIOC11); intr(GPIOC12); intr(GPIOC13); intr(GPIOC14);
intr(GPIOC15); intr(GPIOC16); intr(GPIOC17); intr(GPIOC18); intr(GPIOC19);
intr(GPIOC20); intr(GPIOC21); intr(GPIOC22); intr(GPIOC23); intr(GPIOC24);
intr(GPIOC25);
/* GPIO D - 6 pins */
intr(GPIOD00); intr(GPIOD01); intr(GPIOD02); intr(GPIOD03); intr(GPIOD04);
intr(GPIOD05);

/* OST interrupt -- has no IRQ number since it's got special handling */
intr(OST);

#undef intr

static void(*irqvector[])(void) = {
    /* ICSR0: 0 - 31 */
    DMIC,   AIC,    UIRQ,   UIRQ,   UIRQ,   UIRQ,   UIRQ,   SFC,
    SSI0,   UIRQ,   PDMA,   PDMAD,  UIRQ,   UIRQ,   UIRQ,   UIRQ,
    UIRQ,   UIRQ,   UIRQ,   UIRQ,   UIRQ,   OTG,    UIRQ,   AES,
    UIRQ,   TCU2,   TCU1,   TCU0,   UIRQ,   UIRQ,   CIM,    LCD,
    /* ICSR1: 32 - 63 */
    RTC,    UIRQ,   UIRQ,   UIRQ,   MSC1,   MSC0,   SCC,    UIRQ,
    PCM0,   UIRQ,   UIRQ,   UIRQ,   HARB2,  UIRQ,   HARB0,  CPM,
    UIRQ,   UART2,  UART1,  UART0,  DDR,    UIRQ,   EFUSE,  MAC,
    UIRQ,   UIRQ,   I2C2,   I2C1,   I2C0,   PDMAM,  JPEG,   UIRQ,
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
    GPIOC24, GPIOC25, UIRQ,    UIRQ,    UIRQ,    UIRQ,    UIRQ,    UIRQ,
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
        jz_clr(INTC_MSK(0), 1 << IRQ_TO_GROUP0(irq));
    } else if(IRQ_IS_GROUP1(irq)) {
        jz_clr(INTC_MSK(1), 1 << IRQ_TO_GROUP1(irq));
    }
}

void system_disable_irq(int irq)
{
    if(IRQ_IS_GROUP0(irq)) {
        jz_set(INTC_MSK(0), 1 << IRQ_TO_GROUP0(irq));
    } else if(IRQ_IS_GROUP1(irq)) {
        jz_set(INTC_MSK(1), 1 << IRQ_TO_GROUP1(irq));
    }
}

static int vector_gpio_irq(int port)
{
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

    switch(n) {
    case IRQ0_GPIO0: n = vector_gpio_irq(GPIO_A); break;
    case IRQ0_GPIO1: n = vector_gpio_irq(GPIO_B); break;
    case IRQ0_GPIO2: n = vector_gpio_irq(GPIO_C); break;
    case IRQ0_GPIO3: n = vector_gpio_irq(GPIO_D); break;
    default: break;
    }

    return n;
}

void intr_handler(unsigned cause)
{
    /* OST interrupt is handled separately */
    if(cause & M_CauseIP3) {
        OST();
        return;
    }

    /* Gather pending interrupts */
    ipr0 |= REG_INTC_PND(0);
    ipr1 |= REG_INTC_PND(1);

    /* Process and dispatch interrupt */
    irq = vector_irq();
    if(irq < 0)
        return;

    irqvector[irq]();
}

void tlb_refill_handler(void)
{
    panicf("TLB refill handler at 0x%08lx! [0x%x]",
           read_c0_epc(), read_c0_badvaddr());
}

#define EXC(x,y) case (x): return (y);
static char* parse_exception(unsigned cause)
{
    switch(cause & M_CauseExcCode)
    {
        EXC(EXC_INT, "Interrupt");
        EXC(EXC_MOD, "TLB Modified");
        EXC(EXC_TLBL, "TLB Exception (Load or Ifetch)");
        EXC(EXC_ADEL, "Address Error (Load or Ifetch)");
        EXC(EXC_ADES, "Address Error (Store)");
        EXC(EXC_TLBS, "TLB Exception (Store)");
        EXC(EXC_IBE, "Instruction Bus Error");
        EXC(EXC_DBE, "Data Bus Error");
        EXC(EXC_SYS, "Syscall");
        EXC(EXC_BP, "Breakpoint");
        EXC(EXC_RI, "Reserved Instruction");
        EXC(EXC_CPU, "Coprocessor Unusable");
        EXC(EXC_OV, "Overflow");
        EXC(EXC_TR, "Trap Instruction");
        EXC(EXC_FPE, "Floating Point Exception");
        EXC(EXC_C2E, "COP2 Exception");
        EXC(EXC_MDMX, "MDMX Exception");
        EXC(EXC_WATCH, "Watch Exception");
        EXC(EXC_MCHECK, "Machine Check Exception");
        EXC(EXC_CacheErr, "Cache error caused re-entry to Debug Mode");
        default:
            return 0;
    }
}
#undef EXC

void exception_handler(unsigned cause, unsigned epc, unsigned stack_ptr)
{
    panicf("Exception occurred: %s [0x%08x] at 0x%08x (stack at 0x%08x)",
           parse_exception(cause), read_c0_badvaddr(), epc, stack_ptr);
}

void system_exception_wait(void)
{
#ifdef FIIO_M3K
    while(button_read_device() != (BUTTON_POWER|BUTTON_VOL_DOWN));
#else
    while(1);
#endif
}
