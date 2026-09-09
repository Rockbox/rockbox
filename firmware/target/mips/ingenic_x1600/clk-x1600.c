/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 *
 * Loosely based on firmware/target/mips/ingenic_x1000/clk-x1000.c,
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

/* X1600 clocks use fractional-N PLLs and separate mux encodings for core,
 * peripheral and I2S clocks. Report zero for a PLL that has not locked. */

#include "system.h"
#include "clk-x1600.h"
#include "spin-x1600.h"
#include "x1600/cpm.h"

/* Fractional-N PLL decode */

/* Decode one of CPAPCR/CPMPCR/CPEPCR into Hz (PM 11.1.3.1):
 *      FVCO = FREF * (PLLM + FRAC/2^24) / PLLN
 *      FOUT = FVCO / PLLOD0 / PLLOD1
 * frac is passed separately because the FRAC registers sit at unrelated
 * offsets (0x84/0x88/0x8c) from the control registers (0x10/0x14/0x18). */
static uint32_t clk_decode_pll_frac(uint32_t reg, uint32_t frac)
{
    /* ON indicates that the PLL output is stable. */
    if((reg & BM_CPM_CPAPCR_ON) == 0)
        return 0;

    uint32_t m   = jz_vreadf(reg, CPM_CPAPCR, M);
    uint32_t n   = jz_vreadf(reg, CPM_CPAPCR, N);
    uint32_t od1 = jz_vreadf(reg, CPM_CPAPCR, OD1);
    uint32_t od0 = jz_vreadf(reg, CPM_CPAPCR, OD0);

    /* M, N and both post-dividers must be nonzero (PM 11.1.3.1). */
    if(m == 0 || n == 0 || od0 == 0 || od1 == 0)
        return 0;

    /* All three PLL control registers use the same field layout. */
    uint64_t num = (uint64_t)X1600_EXCLK_FREQ *
                   (((uint64_t)m << 24) + (frac & BM_CPM_CPAPACR_FRAC));
    uint64_t den = (uint64_t)n * od0 * od1;
    den <<= 24;

    return (uint32_t)(num / den);
}

uint32_t clk_decode_pll(uint32_t pllreg)
{
    /* Integer-only decode, for callers that have a register word but not the
     * matching FRAC register (eg. a debug screen dumping a saved value). */
    return clk_decode_pll_frac(pllreg, 0);
}

static uint32_t clk_get_apll(void)
{
    return clk_decode_pll_frac(REG_CPM_CPAPCR, REG_CPM_CPAPACR);
}

static uint32_t clk_get_mpll(void)
{
    return clk_decode_pll_frac(REG_CPM_CPMPCR, REG_CPM_CPMPACR);
}

static uint32_t clk_get_epll(void)
{
    return clk_decode_pll_frac(REG_CPM_CPEPCR, REG_CPM_CPEPACR);
}

/* Enable EPLL with the existing divider settings. Returns zero if it is
 * unconfigured or fails to lock. */
static uint32_t clk_enable_epll(void)
{
    uint32_t rate = clk_get_epll();
    if(rate != 0)
        return rate;                    /* already running */

    /* Do not enable a PLL with invalid divider settings. */
    if(jz_readf(CPM_CPEPCR, M) == 0 || jz_readf(CPM_CPEPCR, N) == 0 ||
       jz_readf(CPM_CPEPCR, OD1) == 0 || jz_readf(CPM_CPEPCR, OD0) == 0)
        return 0;

    jz_writef(CPM_CPEPCR, EN(1));

    /* Wait up to 1000 polling iterations for the PLL to lock. */
    for(int i = 0; i < 1000; ++i) {
        if(jz_readf(CPM_CPEPCR, ON))
            return clk_get_epll();
        udelay(1);
    }

    return 0;
}

/* SCLK_A: CPCCR.SEL_SRC, PM 11.1.2.1.  00 = stop, 01 = EXCLK, 10 = APLL. */
static uint32_t clk_get_sclk_a(void)
{
    switch(jz_readf(CPM_CPCCR, SEL_SRC)) {
    case BV_CPM_CPCCR_SEL_SRC__EXCLK: return X1600_EXCLK_FREQ;
    case BV_CPM_CPCCR_SEL_SRC__APLL:  return clk_get_apll();
    default:                          return 0; /* stopped */
    }
}

/* Core muxes use 0=stop, 1=SCLK_A, 2=MPLL. GATE_SCLKA affects only the
 * peripheral domains (PM 11.1.2.1). */
static uint32_t clk_get_ccr_src(uint32_t sel)
{
    switch(sel) {
    case BV_CPM_CPCCR_SEL_CPLL__SCLK_A: return clk_get_sclk_a();
    case BV_CPM_CPCCR_SEL_CPLL__MPLL:   return clk_get_mpll();
    default:                            return 0; /* stopped */
    }
}

/* Peripheral muxes use 0=SCLK_A, 1=MPLL, 2=EPLL (PM 11.1.2.7-13). */
static uint32_t clk_get_periph_src(uint32_t sel)
{
    switch(sel) {
    case 0:
        /* PM 11.1.2.1 bit 23: "GATE_SCLKA ... 1: SCLK_A is gating.  Gating
         * modules: LCD MSC MAC AIC USB SSI PCM CIM etc" */
        if(jz_readf(CPM_CPCCR, GATE_SCLKA))
            return 0;
        return clk_get_sclk_a();
    case 1: return clk_get_mpll();
    case 2: return clk_get_epll();
    default: return 0;
    }
}

/* I2S / AIC transmit clock */

/* AIC transmit divider, accessed without including the full AIC register
 * header. */
#define X1600_AIC_I2SDIV_ADDR   0xb0079030

/* BCLK = MCLK / I2SDIV, not MCLK / (I2SDIV + 1) as on the X1000 -- measured
 * at 64fs on hardware. Do not copy clk-x1000.c here. */
static uint32_t clk_get_i2s_bclk(uint32_t mclk)
{
    if(mclk == 0)
        return 0;

    uint32_t div = *(const volatile uint32_t*)X1600_AIC_I2SDIV_ADDR;
    div &= 0xff;
    if(div == 0)
        return 0;

    return mclk / div;
}

static uint32_t clk_get_i2s_mclk(void)
{
    /* Two gates: the AUDIO block (CLKGR bit 11) and the AIC transmit clock
     * (CLKGR1 bit 9, I2S0_dev_tclk). PM 11.2.2.6/7. */
    if(jz_readf(CPM_CLKGR, AUDIO) || jz_readf(CPM_CLKGR1, I2S0_TCLK))
        return 0;

    uint32_t reg = REG_CPM_I2S1CDR;
    if(jz_vreadf(reg, CPM_I2S1CDR, CE_I2S) == 0)
        return 0;

    uint32_t parent;
    if(jz_vreadf(reg, CPM_I2S1CDR, I2PCS) == BV_CPM_I2S1CDR_I2PCS__EPLL)
        parent = clk_get_epll();
    else
        parent = clk_get_periph_src(0); /* SCLK_A, honouring GATE_SCLKA */

    uint32_t m = jz_vreadf(reg, CPM_I2S1CDR, I2SDIV_M);
    uint32_t n = jz_vreadf(reg, CPM_I2S1CDR, I2SDIV_N);
    if(m == 0 || n == 0)
        return 0;

    /* PM 11.1.2.5: "Fout = Fin * M / N". */
    return (uint32_t)(((uint64_t)parent * m) / n);
}

/* Program the AIC transmit M/N divider for a sample rate. The search below
 * finds the best rational M/N with M <= I2S_M_MAX; ties go to the smaller M.
 * M is 9 bits and N is 20 bits (PM 11.1.2.5). */
#define I2S_M_MAX   0x1ff
#define I2S_N_MAX   0xfffff

#define I2S_PARENT_SEL  I2PCS_V(EPLL)

uint32_t clk_set_i2s_mclk(uint32_t fs, uint32_t mult)
{
    uint32_t target = fs * mult;
    if(target == 0)
        return 0;

    /* EPLL supplies the audio clock. */
    /* Start EPLL on demand; the other clock domains do not require it. */
    uint32_t parent = clk_enable_epll();
    if(parent == 0)
        return 0;

    uint32_t best_m = 0, best_n = 0, best_err = 0xffffffff, best_rate = 0;

    for(uint32_t m = 1; m <= I2S_M_MAX; ++m) {
        /* Nearest N for this M. */
        uint64_t nn = ((uint64_t)m * parent + target/2) / target;
        if(nn > I2S_N_MAX)
            break;  /* N only grows with M, so we are done */

        uint32_t n = (uint32_t)nn;

        /* PM 11.1.2.5 note: "N should not be less than M*2." */
        if(n < 2*m)
            continue;

        uint32_t rate = (uint32_t)(((uint64_t)parent * m) / n);
        uint32_t err = rate > target ? rate - target : target - rate;

        if(err < best_err) {
            best_err  = err;
            best_m    = m;
            best_n    = n;
            best_rate = rate;

            if(err == 0)
                break;
        }
    }

    if(best_m == 0)
        return 0;

    /* Preserve I2S1CDR1: the PM gives a conflicting auto-calculation
     * formula and marks bits 30/31 as misconnected. Existing settings were
     * used for hardware clock measurements. */
    jz_overwritef(CPM_I2S1CDR, I2S_PARENT_SEL, CE_I2S(1),
                  I2SDIV_M(best_m), I2SDIV_N(best_n));

    return best_rate;
}

/* clk_get() */

uint32_t clk_get(x1600_clk_t clk)
{
    switch(clk) {
    case X1600_CLK_EXCLK:
        return X1600_EXCLK_FREQ;

    case X1600_CLK_APLL:
        return clk_get_apll();
    case X1600_CLK_MPLL:
        return clk_get_mpll();
    case X1600_CLK_EPLL:
        return clk_get_epll();
    case X1600_CLK_SCLK_A:
        return clk_get_sclk_a();

    /* CPU and L2 share SEL_CPLL and use value+1 dividers. Neither has a
     * separate CLKGR gate. */
    case X1600_CLK_CPU:
        return clk_get_ccr_src(jz_readf(CPM_CPCCR, SEL_CPLL)) /
               (jz_readf(CPM_CPCCR, CDIV) + 1);
    case X1600_CLK_L2CACHE:
        return clk_get_ccr_src(jz_readf(CPM_CPCCR, SEL_CPLL)) /
               (jz_readf(CPM_CPCCR, L2CDIV) + 1);

    case X1600_CLK_AHB0:
        if(jz_readf(CPM_CLKGR, AHB0))
            return 0;
        return clk_get_ccr_src(jz_readf(CPM_CPCCR, SEL_H0PLL)) /
               (jz_readf(CPM_CPCCR, H0DIV) + 1);

    /* AHB2 and PCLK share SEL_H2PLL; PM 11.1.4 requires H2CLK to be 1x or 2x
     * PCLK, but we just report whatever is programmed. */
    case X1600_CLK_AHB2:
        return clk_get_ccr_src(jz_readf(CPM_CPCCR, SEL_H2PLL)) /
               (jz_readf(CPM_CPCCR, H2DIV) + 1);
    case X1600_CLK_PCLK:
        if(jz_readf(CPM_CLKGR, APB0))
            return 0;
        return clk_get_ccr_src(jz_readf(CPM_CPCCR, SEL_H2PLL)) /
               (jz_readf(CPM_CPCCR, PDIV) + 1);

    /* DDRCDR uses the CPCCR-style encoding (stop/SCLK_A/MPLL), PM 11.1.2.3,
     * and a 4-bit "+1" divider. */
    case X1600_CLK_DDR: {
        if(jz_readf(CPM_CLKGR, DDR))
            return 0;
        uint32_t src;
        switch(jz_readf(CPM_DDRCDR, DCS)) {
        case BV_CPM_DDRCDR_DCS__SCLK_A: src = clk_get_sclk_a(); break;
        case BV_CPM_DDRCDR_DCS__MPLL:   src = clk_get_mpll();   break;
        default:                        return 0;
        }
        return src / (jz_readf(CPM_DDRCDR, DDRCDR) + 1);
    }

    case X1600_CLK_LCD:
        if(jz_readf(CPM_CLKGR, LCD))
            return 0;
        return clk_get_periph_src(jz_readf(CPM_LPCDR, LPCS)) /
               (jz_readf(CPM_LPCDR, LPCDR) + 1);

    /* MSC division ratio is (MSCnCDR + 1) * 2 (PM 11.1.2.8-9). */
    case X1600_CLK_MSC0:
        if(jz_readf(CPM_CLKGR, MSC0))
            return 0;
        return clk_get_periph_src(jz_readf(CPM_MSC0CDR, MPCS)) /
               ((jz_readf(CPM_MSC0CDR, MSCCDR) + 1) * 2);
    case X1600_CLK_MSC1:
        if(jz_readf(CPM_CLKGR, MSC1))
            return 0;
        return clk_get_periph_src(jz_readf(CPM_MSC1CDR, MPCS)) /
               ((jz_readf(CPM_MSC1CDR, MSCCDR) + 1) * 2);

    case X1600_CLK_SFC:
        if(jz_readf(CPM_CLKGR, SFC))
            return 0;
        return clk_get_periph_src(jz_readf(CPM_SFCCDR, SFCS)) /
               (jz_readf(CPM_SFCCDR, SFCCDR) + 1);

    case X1600_CLK_SSI:
        if(jz_readf(CPM_CLKGR, SSI0))
            return 0;
        return clk_get_periph_src(jz_readf(CPM_SSICDR, SPCS)) /
               (jz_readf(CPM_SSICDR, SSICDR) + 1);

    case X1600_CLK_PWM:
        /* PWM is gated from CLKGR1, not CLKGR (PM 11.2.2.7 bit 5). */
        if(jz_readf(CPM_CLKGR1, PWM))
            return 0;
        return clk_get_periph_src(jz_readf(CPM_PWMCDR, PWMPCS)) /
               (jz_readf(CPM_PWMCDR, PWMCDR) + 1);

    case X1600_CLK_I2S_MCLK:
        return clk_get_i2s_mclk();
    case X1600_CLK_I2S_BCLK:
        return clk_get_i2s_bclk(clk_get_i2s_mclk());

    default:
        return 0;
    }
}

const char* clk_get_name(x1600_clk_t clk)
{
    switch(clk) {
#define CASE(x) case X1600_CLK_##x: return #x
        CASE(EXCLK);
        CASE(APLL);
        CASE(MPLL);
        CASE(EPLL);
        CASE(SCLK_A);
        CASE(CPU);
        CASE(L2CACHE);
        CASE(AHB0);
        CASE(AHB2);
        CASE(PCLK);
        CASE(DDR);
        CASE(LCD);
        CASE(MSC0);
        CASE(MSC1);
        CASE(SFC);
        CASE(SSI);
        CASE(PWM);
        CASE(I2S_MCLK);
        CASE(I2S_BCLK);
#undef CASE
    default:
        return "NONE";
    }
}

/* Measuring the real core clock */

/* CP0 Count advances once per two core cycles, measured against the OST. */
#define X1600_CP0_COUNT_DIVISOR 2

/* Sample CP0 Count over a 2 ms OST window. The OST runs from EXCLK (PM 14.1),
 * so this is assumption free. */
/* Count PLL and mux timeouts for the debug screen. */
volatile uint32_t clk_timeouts;

#define clk_spin_while(cond)                                                \
    __extension__ ({                                                        \
        bool _timed_out = x1600_spin_while(cond);                           \
        if(_timed_out) clk_timeouts++;                                      \
        _timed_out;                                                         \
    })

static uint32_t clk_measure_count(void)
{
    const uint32_t window = 2 * 1000 * OST_TICKS_PER_US;

    int irq = disable_irq_save();

    uint32_t ost0 = __ost_read32();
    uint32_t cnt0 = read_c0_count();

    /* Spin on the OST, not a cycle count: the boot core clock is not
     * predictable (system-target.h). */
    (void)clk_spin_while(__ost_read32() - ost0 < window);

    uint32_t ost1 = __ost_read32();
    uint32_t cnt1 = read_c0_count();

    restore_irq(irq);

    uint32_t ost_delta = ost1 - ost0;
    uint32_t cnt_delta = cnt1 - cnt0;
    if(ost_delta == 0)
        return 0;

    return (uint32_t)(((uint64_t)cnt_delta * OST_FREQUENCY) / ost_delta);
}

uint32_t clk_measure_cpu(void)
{
    return clk_measure_count() * X1600_CP0_COUNT_DIVISOR;
}

/* Clock configuration */

#define CCR_MUX_BITS jz_orm(CPM_CPCCR, SEL_SRC, SEL_CPLL, SEL_H0PLL, SEL_H2PLL)
#define CCR_DIV_BITS jz_orm(CPM_CPCCR, CDIV, L2CDIV, H0DIV, H2DIV, PDIV)
#define CSR_MUX_BITS jz_orm(CPM_CPCSR, SRC_MUX, CPU_MUX, AHB0_MUX, AHB2_MUX)
#define CSR_DIV_BITS jz_orm(CPM_CPCSR, H2DIV_BUSY, H0DIV_BUSY, CDIV_BUSY)

void clk_set_ccr_mux(uint32_t muxbits)
{
    uint32_t reg = REG_CPM_CPCCR;
    reg &= ~CCR_MUX_BITS;
    reg |= muxbits & CCR_MUX_BITS;
    REG_CPM_CPCCR = reg;

    /* Wait for all mux switches to settle (PM 11.1.2.2). */
    (void)clk_spin_while((REG_CPM_CPCSR & CSR_MUX_BITS) != CSR_MUX_BITS);
}

void clk_set_ccr_div(uint32_t divbits)
{
    uint32_t reg = REG_CPM_CPCCR;
    reg &= ~CCR_DIV_BITS;
    reg |= divbits & CCR_DIV_BITS;
    /* Writes to the divider fields only take effect while the matching change
     * enable is set (PM 11.1.2.1, CE_CPU/CE_AHB0/CE_AHB2). */
    reg |= jz_orm(CPM_CPCCR, CE_CPU, CE_AHB0, CE_AHB2);
    REG_CPM_CPCCR = reg;

    (void)clk_spin_while(REG_CPM_CPCSR & CSR_DIV_BITS);

    jz_writef(CPM_CPCCR, CE_CPU(0), CE_AHB0(0), CE_AHB2(0));
}

void clk_set_ddr(x1600_clk_t src, uint32_t div)
{
    jz_writef(CPM_DDRCDR, CE_DDR(1), DDRCDR(div - 1),
              DCS(src == X1600_CLK_MPLL ? BV_CPM_DDRCDR_DCS__MPLL
                                        : BV_CPM_DDRCDR_DCS__SCLK_A));

    (void)clk_spin_while(jz_readf(CPM_CPCSR, DDR_MUX) == 0);
    (void)clk_spin_while(jz_readf(CPM_DDRCDR, DDR_BUSY));

    jz_writef(CPM_DDRCDR, CE_DDR(0));
}

/* Disable the PLL before changing its dividers, then re-enable it (PM
 * 11.1.3.1). */
#define PLL_SETUP(REG, m, n, od1, od0)                                      \
    do {                                                                    \
        jz_writef(REG, EN(0));                                              \
        (void)clk_spin_while(jz_readf(REG, ON));                            \
        jz_overwritef(REG, M(m), N(n), OD1(od1), OD0(od0),                  \
                      DSMPD(1), PHASEPD(1), EN(1));                         \
        (void)clk_spin_while(jz_readf(REG, ON) == 0);                       \
    } while(0)

void clk_init_early(void)
{
    /* x1600_ddr_init() configures the DDR clock during SPL startup. */
}

void clk_init(void)
{
    /* Retuning APLL temporarily switches SCLK_A to EXCLK. Do not do this
     * while DDR still uses SCLK_A. */
    if(jz_readf(CPM_DDRCDR, DCS) == BV_CPM_DDRCDR_DCS__SCLK_A)
        return;

    /* Park everything on EXCLK so that APLL can be reprogrammed. */
    clk_set_ccr_mux(CLKMUX_SCLK_A(EXCLK) |
                    CLKMUX_CPU(SCLK_A) |
                    CLKMUX_AHB0(SCLK_A) |
                    CLKMUX_AHB2(SCLK_A));
    clk_set_ccr_div(CLKDIV_CPU(1) |
                    CLKDIV_L2(1) |
                    CLKDIV_AHB0(1) |
                    CLKDIV_AHB2(1) |
                    CLKDIV_PCLK(1));

    /* Use the vendor APLL setting: 1104 MHz at the default core voltage. */
    PLL_SETUP(CPM_CPAPCR, 46, 1, 1, 1);

    /* CPU 1104, L2 552, AHB0/AHB2 138, PCLK 69 MHz. Keep bus clocks below
     * the measured BootROM rates until higher rates are tested. */
    clk_set_ccr_div(CLKDIV_CPU(1) |
                    CLKDIV_L2(2) |
                    CLKDIV_AHB0(8) |
                    CLKDIV_AHB2(8) |
                    CLKDIV_PCLK(16));
    clk_set_ccr_mux(CLKMUX_SCLK_A(APLL) |
                    CLKMUX_CPU(SCLK_A) |
                    CLKMUX_AHB0(SCLK_A) |
                    CLKMUX_AHB2(SCLK_A));

    /* Leave MPLL and EPLL unchanged. clk_set_i2s_mclk() enables EPLL on
     * demand; the display uses MPLL. */
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency(long tgt_freq)
{
    if(tgt_freq < 0)        tgt_freq = 0;
    if(tgt_freq > CPU_FREQ) tgt_freq = CPU_FREQ;

    uint32_t in_freq = clk_get_ccr_src(jz_readf(CPM_CPCCR, SEL_CPLL));
    if(in_freq == 0)
        return;

    if(tgt_freq < 1)
        tgt_freq = 1;
    if(tgt_freq > (long)in_freq)
        tgt_freq = in_freq;

    uint32_t cdiv = clk_calc_div(in_freq, tgt_freq);
    if(cdiv > 16) cdiv = 16;
    if(cdiv < 1)  cdiv = 1;

    /* Keep CCLK/L2CLK within the PM 11.1.4 ratios. At the maximum core
     * rate, run L2 at half speed. */
    uint32_t l2div = cdiv;
    if(cdiv == 1)
        l2div = 2;

    jz_writef(CPM_CPCCR, CE_CPU(1), L2CDIV(l2div - 1), CDIV(cdiv - 1));
    (void)clk_spin_while(jz_readf(CPM_CPCSR, CDIV_BUSY));
    jz_writef(CPM_CPCCR, CE_CPU(0));

    cpu_frequency = in_freq / cdiv;
}
#endif
