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
#include "x1000/cpm.h"
#include "x1000/ost.h"
#include "x1000/tcu.h"
#include "x1000/wdt.h"
#include "x1000/intc.h"
#include "x1000/msc.h"
#include "x1000/aic.h"

unsigned clk_rate_cache[X1000_CLK_COUNT] = {
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
    1,
};

/* status bits checked changing CPU/bus clock mux or divider */
#define CSR_MUX_BITS jz_orm(CPM_CSR, SRC_MUX, CPU_MUX, AHB0_MUX, AHB2_MUX)
#define CSR_DIV_BITS jz_orm(CPM_CSR, H2DIV_BUSY, H0DIV_BUSY, CDIV_BUSY)

/* Clock utility functions for system init */
static void system_set_clk_div(int c, int l2, int h0, int h2, int p)
{
    jz_writef(CPM_CCR, CDIV(c), L2DIV(l2), H0DIV(h0), H2DIV(h2), PDIV(p),
              CE_CPU(1), CE_AHB0(1), CE_AHB2(1));
    while(REG_CPM_CSR & CSR_DIV_BITS);
    jz_writef(CPM_CCR, CE_CPU(0), CE_AHB0(0), CE_AHB2(0));
}

static void system_set_clk_mux(int src, int c, int h0, int h2)
{
    jz_writef(CPM_CCR, SEL_SRC(src), SEL_CPLL(c),
              SEL_H0PLL(h0), SEL_H2PLL(h2));
    while((REG_CPM_CSR & CSR_MUX_BITS) != CSR_MUX_BITS);
}

static void system_set_ddr_clk(int div, int src)
{
    jz_writef(CPM_DDRCDR, CLKDIV(div), CLKSRC(src), CE(1));
    while(jz_readf(CPM_CSR, DDR_MUX) == 0);
    while(jz_readf(CPM_DDRCDR, BUSY));
    jz_writef(CPM_DDRCDR, CE(0));
}

static void system_init_clk(void)
{
    /* Gate all clocks except CPU/bus/memory */
    REG_CPM_CLKGR = ~jz_orm(CPM_CLKGR, CPU_BIT, DDR, AHB0, APB0);

    /* Switch to EXCLK */
    system_set_clk_mux(1, 1, 1, 1);
    system_set_clk_div(0, 0, 0, 0, 0);

#ifdef FIIO_M3K
    /* APLL -> 1008 MHz, this is the default setting on original firmware */
    jz_writef(CPM_APCR, BS(1), PLLM(41), PLLN(0), PLLOD(0), ENABLE(1));
    while(jz_readf(CPM_APCR, ON) == 0);

    system_set_ddr_clk(4, 1);
    system_set_clk_div(0, 1, 4, 4, 9);
    system_set_clk_mux(2, 1, 1, 1);

    /* Shut off MPLL, since nobody should be using it now */
    jz_writef(CPM_MPCR, ENABLE(0));
#else
# error "Please define system clock configuration for target"
#endif
}

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

/* First thing called from Rockbox main() */
void system_init(void)
{
    /* Setup system clocks */
    system_init_clk();

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
    jz_writef(OST_CTRL, PRESCALE2_V(BY_1));
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
void set_cpu_frequency(long in_freq)
{
    (void)in_freq;
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

/* Clock query functions */
static unsigned clk_get_ccrclk(unsigned selbit, unsigned divbit)
{
    unsigned reg = REG_CPM_CCR;
    unsigned sel = (reg >> selbit) & 0x3;
    unsigned div = (reg >> divbit) & 0xf;
    switch(sel) {
    case 1: return clk_get_sclk_a() / (div + 1);
    case 2: return clk_get_mpll() / (div + 1);
    default: return 0;
    }
}

static unsigned clk_get_pllclk(unsigned pllreg, int onbit)
{
    if((pllreg & (1 << onbit)) == 0)
        return 0;

    unsigned long long rate = X1000_EXCLK_FREQ;
    rate *= jz_vreadf(pllreg, CPM_APCR, PLLM) + 1;
    rate /= jz_vreadf(pllreg, CPM_APCR, PLLN) + 1;
    rate >>= jz_vreadf(pllreg, CPM_APCR, PLLOD);
    return rate & 0xffffffff;
}

static unsigned clk_get_mscclk(int msc)
{
    unsigned rate;
    if(jz_readf(CPM_MSC0CDR, CLKSRC) == 0)
        rate = clk_get_sclk_a();
    else
        rate = clk_get_mpll();

    if(msc == 0)
        rate /= 2 * (jz_readf(CPM_MSC0CDR, CLKDIV) + 1);
    else
        rate /= 2 * (jz_readf(CPM_MSC1CDR, CLKDIV) + 1);

    rate >>= REG_MSC_CLKRT(msc);
    return rate;
}

static unsigned clk_get_i2s_mclk(void)
{
    unsigned long long rate;
    unsigned reg = REG_CPM_I2SCDR;
    if(jz_vreadf(reg, CPM_I2SCDR, CS) == 0)
        rate = X1000_EXCLK_FREQ;
    else {
        if(jz_vreadf(reg, CPM_I2SCDR, PCS) == 0)
            rate = clk_get_sclk_a();
        else
            rate = clk_get_mpll();

        rate *= jz_vreadf(reg, CPM_I2SCDR, DIV_M);
        rate /= jz_vreadf(reg, CPM_I2SCDR, DIV_N);
    }

    return rate;
}

unsigned clk_get(enum x1000_clk_t clk)
{
    if(clk_rate_cache[clk] != 1)
        return clk_rate_cache[clk];

    unsigned reg;
    unsigned rate;
    switch(clk) {
    case X1000_CLK_APLL:
        rate = clk_get_pllclk(REG_CPM_APCR, BP_CPM_APCR_ON);
        break;
    case X1000_CLK_MPLL:
        rate = clk_get_pllclk(REG_CPM_MPCR, BP_CPM_MPCR_ON);
        break;
    case X1000_CLK_SCLK_A:
        switch(jz_readf(CPM_CCR, SEL_SRC)) {
        case 1: rate = X1000_EXCLK_FREQ; break;
        case 2: rate = clk_get_apll(); break;
        default: rate = 0; break;
        }
        break;
    case X1000_CLK_CPU:
        rate = clk_get_ccrclk(BP_CPM_CCR_SEL_CPLL, BP_CPM_CCR_CDIV);
        break;
    case X1000_CLK_L2CACHE:
        rate = clk_get_ccrclk(BP_CPM_CCR_SEL_CPLL, BP_CPM_CCR_L2DIV);
        break;
    case X1000_CLK_AHB0:
        rate = clk_get_ccrclk(BP_CPM_CCR_SEL_H0PLL, BP_CPM_CCR_H0DIV);
        break;
    case X1000_CLK_AHB2:
        rate = clk_get_ccrclk(BP_CPM_CCR_SEL_H2PLL, BP_CPM_CCR_H2DIV);
        break;
    case X1000_CLK_PCLK:
        rate = clk_get_ccrclk(BP_CPM_CCR_SEL_H2PLL, BP_CPM_CCR_PDIV);
        break;
    case X1000_CLK_LCD:
        reg = REG_CPM_LPCDR;
        if(jz_vreadf(reg, CPM_LPCDR, CLKSRC))
            rate = clk_get_mpll();
        else
            rate = clk_get_sclk_a();
        rate /= jz_vreadf(reg, CPM_LPCDR, CLKDIV) + 1;
        break;
    case X1000_CLK_MSC0:
        rate = clk_get_mscclk(0);
        break;
    case X1000_CLK_MSC1:
        rate = clk_get_mscclk(1);
        break;
    case X1000_CLK_I2S_MCLK:
        rate = clk_get_i2s_mclk();
        break;
    case X1000_CLK_I2S_BCLK:
        rate = clk_get(X1000_CLK_I2S_MCLK);
        rate /= REG_AIC_I2SDIV + 1;
        break;
    default:
        return 0;
    }

    clk_rate_cache[clk] = rate;
    return rate;
}

/* IRQ handling */
static int irq = 0;
static unsigned ipr0 = 0, ipr1 = 0;

static void UIRQ(void)
{
    panicf("Unhandled interrupt occurred: %d", irq);
}

#define intr(name) extern __attribute__((weak, alias("UIRQ"))) void name(void)

/* Main interrupts */
intr(DMIC); intr(AIC0);  intr(SFC);   intr(SSI0);  intr(OTG);   intr(AES);
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

static void(*const irqvector[])(void) = {
    /* ICSR0: 0 - 31 */
    DMIC,   AIC0,   UIRQ,   UIRQ,   UIRQ,   UIRQ,   UIRQ,   SFC,
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
