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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* Initialise the 64 MiB SiP LPDDR2 at 350 MHz using vendor SPL parameters.
 * X1600 uses an Innosilicon PHY, so the X1000 PHY sequence does not apply.
 * This driver configures MPLL and DDRCDR, checks the OST timebase and
 * bounds register polling. */

#include "config.h"
#include "x1600.h"
#include "system.h"
#include "ddr-x1600.h"
#include "x1600/cpm.h"

#define REG32(a)    (*(volatile uint32_t*)(a))
#define REG32P(a)   ((volatile uint32_t*)(a))

/* Polling budgets in microseconds; register-read overhead adds to the
 * delay. */
#define TMO_PLL_LOCK_US      10000   /* CPM PLL / PHY PLL lock                */
#define TMO_CLK_BUSY_US      10000   /* CPM_DDRCDR.DDR_BUSY                   */
#define TMO_DFI_US           10000   /* DFI handshake through PHY_INIT        */
#define TMO_LMR_US            1000   /* DDRC_LMR.START self-clear             */
#define TMO_CALIB_US        100000   /* Innosilicon RX calibration            */

/* The vendor SPL's MPLL word: M=175 N=3 OD=1,1 -> 24 MHz * 175/3 = 1400 MHz */
#define X1600_DDR_CPMPCR    0x0af0c9c1u
#define X1600_DDR_MPLL_HZ   1400000000u

/* Verbatim from the vendor SPL's table; none of it is derived */
const struct x1600_ddr_param x1600_ddr_param_hibyr1 = {
    .name         = "M54D5121632A",
    .id           = 0x0000000a,
    .type         = X1600_LPDDR2,
    .freq         = 350000000,    /* +0x28 = 0x14dc9380 */
    .cfg          = 0x0a6a8a40,
    .ctrl         = 0x00000002,
    .cs           = 0x00000000,
    .mmap0        = 0x000020fc,   /* base 0x20 mask 0xfc: 64 MiB on CS0 */
    .mmap1        = 0x00002400,   /* base 0x24 mask 0x00: CS1 disabled  */
    .refcnt       = 0x00540001,   /* CON=0x54, CLK_DIV=0, REF_EN=1      */
    .timing       = { 0x030b0603, 0x020f0706, 0x20080417,
                      0x0f640030, 0xff0b0403, 0x09120505 },
    .phy_mem_cfg  = 0x00000013,   /* MEMSEL=3 (LPDDR2) | BSTSEL (burst 8) */
    .phy_cl       = 0x00000006,   /* DDRPHY_CL  = 6, must equal SDRAM RL   */
    .phy_cwl      = 0x00000003,   /* DDRPHY_CWL = 3, must equal SDRAM WL   */
    /* LPDDR2 mode registers, encoded (MA << 8) | OP. MR0 is read-only on
     * LPDDR2 and is used only by the vendor's DDR3 path. */
    .mr0          = 0x00000000,
    .mr1          = 0x00000183,
    .mr2          = 0x00000204,
    .mr3          = 0x00000302,
    .mr10         = 0x00000aff,
    .mr63         = 0x00003f00,
    .remap        = { 0x03020d0c, 0x07060504, 0x0b0a0908,
                      0x0f0e0100, 0x13121110 },
};

/* In .bss, which the USB stage1 path does not clear, so x1600_ddr_init()
 * rewrites every field -- do not add one it leaves untouched */
struct ddr_status ddr_last_status;

/* The OST ticks at least once every four core cycles even at 24 MHz, so after
 * this many reads it is not going to move */
#define DDR_OST_PROBE_ITERS  100000u

static int ddr_timebase_alive(void)
{
    uint32_t start = __ost_read32();
    uint32_t i;

    for (i = 0; i < DDR_OST_PROBE_ITERS; ++i)
        if (__ost_read32() != start)
            return 1;

    return 0;
}

/* Backstopped, so a stopped OST costs a failed init rather than the device */
static void ddr_delay_ticks(uint32_t ticks)
{
    uint32_t start = __ost_read32();
    uint32_t guard = ticks * 64u + 4096u;

    ticks += 1;
    while (guard--) {
        if (__ost_read32() - start >= ticks)
            return;
    }
}

#define ddr_udelay(n)   ddr_delay_ticks((uint32_t)(n) * OST_TICKS_PER_US)
#define ddr_mdelay(n)   ddr_delay_ticks((uint32_t)(n) * 1000u * OST_TICKS_PER_US)

/* 0 on success, -1 on timeout; there is deliberately no unbounded variant */
static int ddr_wait(volatile uint32_t* reg, uint32_t mask, uint32_t want,
                    int max_us)
{
    int i;
    for (i = 0; i <= max_us; ++i) {
        if ((*reg & mask) == want)
            return 0;
        ddr_udelay(1);
    }
    return -1;
}

/* Status reporting */

static void ddr_step(uint32_t step)
{
    ddr_last_status.step = step;
}

/* Before the DDR clock is known good. CPM only: a DDRC or PHY read without
 * one locks the bus. */
static int ddr_fail_clk(uint32_t err, uint32_t mpll_hz, uint32_t want_hz,
                        uint32_t div)
{
    ddr_last_status.err = err;
    ddr_last_status.reg_snapshot[0] = REG_CPM_CPCCR;
    ddr_last_status.reg_snapshot[1] = REG_CPM_CPMPCR;
    ddr_last_status.reg_snapshot[2] = REG_CPM_DDRCDR;
    ddr_last_status.reg_snapshot[3] = REG_CPM_CLKGR;
    ddr_last_status.reg_snapshot[4] = mpll_hz;
    ddr_last_status.reg_snapshot[5] = want_hz;
    ddr_last_status.reg_snapshot[6] = div;
    ddr_last_status.reg_snapshot[7] = 0;
    return (int)err;
}

/* Snapshot once the DDR clock is up. Reading DDRC_STATUS clears MISS. */
static void ddr_snapshot(void)
{
    ddr_last_status.reg_snapshot[0] = REG32(DDRC_STATUS);
    ddr_last_status.reg_snapshot[1] = REG32(DDRC_CTRL);
    ddr_last_status.reg_snapshot[2] = REG32(DDRC_APB_PHY_INIT);
    ddr_last_status.reg_snapshot[3] = REG32(DDRPHY_PLL_LOCK);
    ddr_last_status.reg_snapshot[4] = REG32(DDRPHY_CALIB_DONE);
    ddr_last_status.reg_snapshot[5] = (REG32(DDRPHY_CALIB_RES_L) & 0xff)
                                    | ((REG32(DDRPHY_CALIB_RES_R) & 0xff) << 8)
                                    | ((REG32(DDRPHY_GATE_DELAY) & 0xff) << 16);
    ddr_last_status.reg_snapshot[6] = (REG32(DDRPHY_RXDLL_DQS0) & 0xff)
                                    | ((REG32(DDRPHY_RXDLL_DQS1) & 0xff) << 8);
    ddr_last_status.reg_snapshot[7] = REG_CPM_DDRCDR;
}

static int ddr_fail(uint32_t err)
{
    ddr_last_status.err = err;
    ddr_snapshot();
    return (int)err;
}

/* Clocks */

/* rate = ((M * 24) / N / OD1 / OD0) MHz, truncating; 0 if not running */
static uint32_t ddr_pll_rate(uint32_t reg)
{
    uint32_t m, n, od1, od0, mhz;

    if ((reg & BM_CPM_CPMPCR_ON) == 0)
        return 0;

    m   = (reg & BM_CPM_CPMPCR_M)   >> BP_CPM_CPMPCR_M;
    n   = (reg & BM_CPM_CPMPCR_N)   >> BP_CPM_CPMPCR_N;
    od1 = (reg & BM_CPM_CPMPCR_OD1) >> BP_CPM_CPMPCR_OD1;
    od0 = (reg & BM_CPM_CPMPCR_OD0) >> BP_CPM_CPMPCR_OD0;

    if (n == 0 || od1 == 0 || od0 == 0)
        return 0;

    mhz = (m * (X1600_EXCLK_FREQ / 1000000)) / n / od1 / od0;
    return mhz * 1000000;
}

/* True if the CPU or either AHB bus runs from MPLL, where retuning it moves
 * the clock under our feet */
static int ddr_mpll_in_use(void)
{
    uint32_t ccr = REG_CPM_CPCCR;

    return jz_vreadf(ccr, CPM_CPCCR, SEL_CPLL)  == BV_CPM_CPCCR_SEL_CPLL__MPLL
        || jz_vreadf(ccr, CPM_CPCCR, SEL_H0PLL) == BV_CPM_CPCCR_SEL_H0PLL__MPLL
        || jz_vreadf(ccr, CPM_CPCCR, SEL_H2PLL) == BV_CPM_CPCCR_SEL_H2PLL__MPLL;
}

/* Ungate the DDR clock, guarantee an MPLL, and point DDRCDR at it. DCS,
 * divider and CE go out together so the result does not depend on the CE bit
 * the BootROM left. */
static int ddr_set_clock(uint32_t freq)
{
    uint32_t mpll, div, v;

    /* Belt and braces: the gate is already clear at BootROM time */
    ddr_step(DDR_STEP_UNGATE);
    jz_writef(CPM_CLKGR, DDR(0));

    /* --- MPLL ---------------------------------------------------------- */
    ddr_step(DDR_STEP_MPLL);
    mpll = ddr_pll_rate(REG_CPM_CPMPCR);

    if (mpll != X1600_DDR_MPLL_HZ) {
        /* MPLL is off out of the BootROM, so this is the normal path */
        if (ddr_mpll_in_use())
            return ddr_fail_clk(DDR_ERR_CLK_MPLL_BUSY, mpll, freq, 0);

        /* Disable first, then write M/N/OD, then wait for lock */
        jz_writef(CPM_CPMPCR, EN(0));
        ddr_udelay(10);
        REG_CPM_CPMPCR = X1600_DDR_CPMPCR;

        if (ddr_wait(REG32P(JA_CPM_CPMPCR), BM_CPM_CPMPCR_ON,
                     BM_CPM_CPMPCR_ON, TMO_PLL_LOCK_US))
            return ddr_fail_clk(DDR_ERR_CLK_MPLL_LOCK, 0, freq, 0);

        mpll = ddr_pll_rate(REG_CPM_CPMPCR);
        if (mpll != X1600_DDR_MPLL_HZ)
            return ddr_fail_clk(DDR_ERR_CLK_MPLL_LOCK, mpll, freq, 0);
    }

    /* --- DDRCDR -------------------------------------------------------- */
    ddr_step(DDR_STEP_DDRCDR);

    div = (mpll + freq - 1) / freq;             /* vendor's rounding */
    if (div < 1 || div > 16)                    /* PM 11.1.2.3: ratio 1..16 */
        return ddr_fail_clk(DDR_ERR_CLK_SRC, mpll, freq, div);

    /* A mistuned DDR clock gives memory that mostly works; refuse it */
    if (mpll / div > freq + freq / 64 || mpll / div < freq - freq / 64)
        return ddr_fail_clk(DDR_ERR_CLK_SRC, mpll, freq, div);

    /* Clearing [7:4] as well drops any junk the BootROM left there */
    v  = REG_CPM_DDRCDR;
    v &= ~(BM_CPM_DDRCDR_DCS | BM_CPM_DDRCDR_CE_DDR |
           BM_CPM_DDRCDR_DDR_STOP | 0xffu);
    v |= BF_CPM_DDRCDR_DCS_V(MPLL)
       | BF_CPM_DDRCDR_CE_DDR(1)
       | BF_CPM_DDRCDR_DDRCDR(div - 1);
    REG_CPM_DDRCDR = v;

    if (ddr_wait(REG32P(JA_CPM_DDRCDR), BM_CPM_DDRCDR_DDR_BUSY, 0,
                 TMO_CLK_BUSY_US))
        return ddr_fail_clk(DDR_ERR_CLK_DDR_BUSY, mpll, freq, div);

    /* Clear CE_DDR so a later stray write is harmless, as the X1000 does */
    jz_writef(CPM_DDRCDR, CE_DDR(0));

    if (jz_readf(CPM_DDRCDR, DCS) != BV_CPM_DDRCDR_DCS__MPLL)
        return ddr_fail_clk(DDR_ERR_CLK_SRC, mpll, freq, div);

    return DDR_OK;
}

/* Assert the PHY DLL reset, then release. Bit 4 is documented reserved but
 * the vendor sets it, and the X1000 SPL writes the same words. */
static void ddr_dll_reset(void)
{
    REG32(CPM_DRCG_ADDR) = 0x73;
    ddr_mdelay(1);
    REG32(CPM_DRCG_ADDR) = 0x71;
    ddr_mdelay(1);
}

/* PHY */

/* PHY steps 2-5: reset, configure memory type / CL / AL / CWL / DQ width,
 * release the digital core */
static void ddr_phy_init(const struct x1600_ddr_param* p)
{
    uint32_t saved;

    /* PHY_RST is active low; save and restore rather than write its reset */
    saved = REG32(DDRPHY_PHY_RST);
    REG32(DDRPHY_PHY_RST) = 0;
    ddr_mdelay(1);
    REG32(DDRPHY_PHY_RST) = saved;
    ddr_mdelay(1);

    REG32(DDRPHY_DQ_WIDTH) = 3;                 /* PM: 3 = 16-bit bus */
    REG32(DDRPHY_MEM_CFG)  = p->phy_mem_cfg;    /* 0x13 = LPDDR2 + burst 8 */

    REG32(DDRPHY_CL)  = (REG32(DDRPHY_CL)  & ~0xf) | (p->phy_cl  & 0xf);
    REG32(DDRPHY_CWL) = (REG32(DDRPHY_CWL) & ~0xf) | (p->phy_cwl & 0xf);
    REG32(DDRPHY_AL)  =  REG32(DDRPHY_AL)  & ~0xf;  /* keep zero */

    /* 0x18 = CMDDLL_en plus the CMD/CK DLL bypass select, then CMD_BYPASS */
    REG32(DDRPHY_CMD_DLL)     = (REG32(DDRPHY_CMD_DLL) & ~0xff) | 0x18;
    REG32(DDRPHY_CMD_BYPASS) |= 0x10;

    /* Release the PHY configure-port reset; the PM calls DCTRL[20] reserved */
    REG32(DDRC_CTRL) &= ~DDRC_CTRL_CFG_RST;
}

/* PHY step 6: enable and lock the PLL. Divider values come from the vendor
 * SPL and must be revisited if the DDR frequency changes. */
static int ddr_phy_pll(void)
{
    REG32(DDRPHY_PLL_FBDIV) = (REG32(DDRPHY_PLL_FBDIV) & ~0xff) | 0x10;
    REG32(DDRPHY_PLL_CTRL) |= 0x2;      /* PLL_PowDown = 1 while retuning */
    REG32(DDRPHY_PLL_PDIV)  = (REG32(DDRPHY_PLL_PDIV) & ~0xff) | 0x24;
    ddr_udelay(5);
    REG32(DDRPHY_PLL_CTRL) &= ~0x2;     /* power up */
    ddr_udelay(5);

    if (ddr_wait(REG32P(DDRPHY_PLL_LOCK), DDRPHY_PLL_LOCKED,
                 DDRPHY_PLL_LOCKED, TMO_PLL_LOCK_US))
        return ddr_fail(DDR_ERR_PHY_PLL_LOCK);

    ddr_udelay(5);

    /* Second witness: the controller's own view of the PHY PLL */
    if (ddr_wait(REG32P(DDRC_APB_PHY_INIT), DDRC_APB_PHY_INIT_PLLLOCK,
                 DDRC_APB_PHY_INIT_PLLLOCK, TMO_PLL_LOCK_US))
        return ddr_fail(DDR_ERR_PHY_DFI_PLL);

    ddr_udelay(5);
    ddr_udelay(5);
    return DDR_OK;
}

/* DRAM mode registers */

/* START is positive-edge triggered and self-clearing; checking it first
 * doubles as a liveness test */
static int ddr_lmr(uint32_t word)
{
    if (ddr_wait(REG32P(DDRC_LMR), DDRC_LMR_START, 0, TMO_LMR_US))
        return -1;

    REG32(DDRC_LMR) = word;

    /* The vendor's delay; tINIT-class waits hide in here */
    ddr_mdelay(1);
    return 0;
}

/* LPDDR2 mode-register encoding follows the vendor SPL. Only chip select
 * zero has been verified. */
static int ddr_lpddr2_mrw(const struct x1600_ddr_param* p, uint32_t mr)
{
    return ddr_lmr(((mr & 0xff) << 24)
                 | (((mr >> 8) & 0xff) << 16)
                 | p->cs
                 | DDRC_LMR_CMD_MRS
                 | DDRC_LMR_START);
}

/* PHY step 7 (DFI handshake), then step 8 onwards (initialise the SDRAM) */
static int ddr_mode_init(const struct x1600_ddr_param* p)
{
    ddr_step(DDR_STEP_DFI);

    /* A 1 -> 0 pulse: the falling edge of dfi_init_start begins it */
    REG32(DDRC_APB_PHY_INIT) = DDRC_APB_PHY_INIT_START;
    REG32(DDRC_APB_PHY_INIT) = 0;

    if (ddr_wait(REG32P(DDRC_APB_PHY_INIT), DDRC_APB_PHY_INIT_COMPLETE,
                 DDRC_APB_PHY_INIT_COMPLETE, TMO_DFI_US))
        return ddr_fail(DDR_ERR_DFI_INIT);

    /* DFI_RST resets to 1 and must be 0 before the PHY is initialised */
    REG32(DDRC_CTRL) &= ~DDRC_CTRL_DFI_RST;
    ddr_udelay(500);

    REG32(DDRC_CFG)  = p->cfg;
    REG32(DDRC_CTRL) = DDRC_CTRL_CKE_PULSE;   /* CKE high */

    ddr_step(DDR_STEP_MODE_REGS);

    /* Only LPDDR2 is transcribed; do not issue DDR3 commands to this die */
    if (p->type != X1600_LPDDR2)
        return ddr_fail(DDR_ERR_TYPE);

    ddr_udelay(200);        /* LPDDR2 tINIT, vendor value; do not shorten */

    /* Not a standard LPDDR2 "precharge all"; the vendor issues this word on
     * all three type paths, so it is likely a controller NOP idiom */
    if (ddr_lmr(DDRC_LMR_CMD_PRECHARGE | DDRC_LMR_BA(7) | DDRC_LMR_START))
        return ddr_fail(DDR_ERR_MR);

    /* JEDEC power-up order: MR63 reset, MR10 ZQ init, then MR1, MR2, MR3 */
    if (ddr_lpddr2_mrw(p, p->mr63) ||
        ddr_lpddr2_mrw(p, p->mr10) ||
        ddr_lpddr2_mrw(p, p->mr1)  ||
        ddr_lpddr2_mrw(p, p->mr2)  ||
        ddr_lpddr2_mrw(p, p->mr3))
        return ddr_fail(DDR_ERR_MR);

    /* Confirm the last command actually completed */
    if (ddr_wait(REG32P(DDRC_LMR), DDRC_LMR_START, 0, TMO_LMR_US))
        return ddr_fail(DDR_ERR_MR);

    return DDR_OK;
}

/* RX (DQS gating) calibration */

/* PHY steps 9 and 10: find the phase at which the generated gating signal
 * covers the incoming DQS. Must run after SDRAM init and before any real
 * access. Results are [6:4] dll_dly, [3] oph_dly, [2:0] cyc_dly per half. */
static int ddr_phy_calib(void)
{
    uint32_t al, ah, gate;

    /* TRAIN_CTRL: DSCSE = 0 (automatic, not register-bypass), DSACE = 1 */
    REG32(DDRPHY_TRAIN_CTRL) =
        (REG32(DDRPHY_TRAIN_CTRL) & ~DDRPHY_TRAIN_CTRL_BYPASS)
        | DDRPHY_TRAIN_CTRL_START;

    /* Both byte lanes done, undocumented bit 4 clear. The vendor's loop
     * fails open on expiry; return an error instead. */
    if (ddr_wait(REG32P(DDRPHY_CALIB_DONE), DDRPHY_CALIB_DONE_MASK,
                 DDRPHY_CALIB_DONE_VALUE, TMO_CALIB_US))
        return ddr_fail(DDR_ERR_PHY_CALIB);

    REG32(DDRPHY_TRAIN_CTRL) = 0;
    ddr_mdelay(5);
    REG32(DDRPHY_TRAIN_CTRL) &= ~DDRPHY_TRAIN_CTRL_START;

    /* The vendor discards two reads first; replicated in case the register
     * needs a dummy read to latch */
    (void)REG32(DDRPHY_CALIB_RES_L);
    (void)REG32(DDRPHY_CALIB_RES_R);

    al = (REG32(DDRPHY_CALIB_RES_L) >> 4) & 0x7;   /* dll_dly, left  */
    ah = (REG32(DDRPHY_CALIB_RES_R) >> 4) & 0x7;   /* dll_dly, right */

    gate = (al > ah ? al : ah) + 1;

    /* At 7 the vendor's max+1 overflows the [3:1] field and sets an
     * undocumented bit instead; clamp to stay inside the field */
    if (gate > 7)
        gate = 7;

    REG32(DDRPHY_GATE_DELAY) = (REG32(DDRPHY_GATE_DELAY) & ~0xe) | (gate << 1);

    REG32(DDRPHY_MEM_CFG) |= DDRPHY_MEM_CFG_CALIB_APPLY;

    /* The PM also names 0xa4/0xa5 and makes the fix-up conditional; the
     * vendor's unconditional max+1 is what boots this board */
    return DDR_OK;
}

/* Post-init memory test, entirely through KSEG1: a cached test is answered out
 * of the D-cache, and cached writes evict into the BootROM's working area */

#define DDR_TEST_LO     0x00100000u     /* first byte we are allowed to touch */
#define DDR_TEST_BASE   0x00200000u     /* also clear of X1600_BOOT_LOAD_ADDR */
#define DDR_TEST_TOP    ((uint32_t)X1600_SDRAM_SIZE)
#define DDR_TEST_STRIDE 0x00010000u     /* sparse test: one word per 64 KiB   */
#define DDR_TEST_BLOCK  1024u           /* bytes per contiguous block probe   */

static volatile uint32_t* ddr_at(uint32_t off)
{
    return (volatile uint32_t*)UNCACHEDADDR(X1600_SDRAM_BASE + off);
}

static int ddr_memtest_fail(uint32_t err, uint32_t off, uint32_t expect,
                            uint32_t actual)
{
    ddr_last_status.err = err;
    ddr_snapshot();
    ddr_last_status.reg_snapshot[2] = (uint32_t)ddr_at(off);
    ddr_last_status.reg_snapshot[3] = expect;
    ddr_last_status.reg_snapshot[4] = actual;
    return (int)err;
}

int x1600_ddr_memtest(void)
{
    uint32_t off, pat, got, i;

    /* Clear MISS by reading; a set MISS afterwards means the map is wrong */
    (void)REG32(DDRC_STATUS);

    /* --- 1. data bus: walking ones then walking zeros at one address ----- */
    for (i = 0; i < 32; ++i) {
        pat = 1u << i;
        *ddr_at(DDR_TEST_BASE) = pat;
        got = *ddr_at(DDR_TEST_BASE);
        if (got != pat)
            return ddr_memtest_fail(DDR_ERR_MEMTEST_DATA, DDR_TEST_BASE,
                                    pat, got);
    }
    for (i = 0; i < 32; ++i) {
        pat = ~(1u << i);
        *ddr_at(DDR_TEST_BASE) = pat;
        got = *ddr_at(DDR_TEST_BASE);
        if (got != pat)
            return ddr_memtest_fail(DDR_ERR_MEMTEST_DATA, DDR_TEST_BASE,
                                    pat, got);
    }

    /* 2. address bus: a unique value at every power-of-two offset, catching a
     * wrong DMMAP or DREMAP as the array aliasing onto itself. Write it all
     * before reading any back, or a write buffer can mask broken memory. */
    *ddr_at(DDR_TEST_BASE) = 0xa5a5a5a5;
    for (i = 2; i < 31 && (DDR_TEST_BASE + (1u << i)) < DDR_TEST_TOP; ++i)
        *ddr_at(DDR_TEST_BASE + (1u << i)) = i;

    got = *ddr_at(DDR_TEST_BASE);
    if (got != 0xa5a5a5a5)
        return ddr_memtest_fail(DDR_ERR_MEMTEST_ADDR, DDR_TEST_BASE,
                                0xa5a5a5a5, got);

    for (i = 2; i < 31 && (DDR_TEST_BASE + (1u << i)) < DDR_TEST_TOP; ++i) {
        off = DDR_TEST_BASE + (1u << i);
        got = *ddr_at(off);
        if (got != i)
            return ddr_memtest_fail(DDR_ERR_MEMTEST_ADDR, off, i, got);
    }

    /* 3. sparse sweep: one word per 64 KiB, all written before any is read
     * back, so refresh and gate-delay errors have time to bite. */
    for (off = DDR_TEST_LO; off < DDR_TEST_TOP; off += DDR_TEST_STRIDE)
        *ddr_at(off) = off ^ 0x5a5a5a5a;

    for (off = DDR_TEST_LO; off < DDR_TEST_TOP; off += DDR_TEST_STRIDE) {
        pat = off ^ 0x5a5a5a5a;
        got = *ddr_at(off);
        if (got != pat)
            return ddr_memtest_fail(DDR_ERR_MEMTEST_SPARSE, off, pat, got);
    }

    /* 4. three contiguous 1 KiB blocks: bursts across a row, which the sparse
     * pass never exercises. */
    {
        static const uint32_t blocks[3] = {
            DDR_TEST_BASE,          /* low  */
            0x01800000u,            /* mid  */
            0x03fff000u,            /* high (last 4 KiB page, 64 MiB part) */
        };

        for (i = 0; i < 3; ++i) {
            uint32_t b = blocks[i];
            uint32_t n;

            if (b + DDR_TEST_BLOCK > DDR_TEST_TOP)
                continue;

            for (n = 0; n < DDR_TEST_BLOCK; n += 4)
                *ddr_at(b + n) = (b + n) * 0x9e3779b1u;

            for (n = 0; n < DDR_TEST_BLOCK; n += 4) {
                pat = (b + n) * 0x9e3779b1u;
                got = *ddr_at(b + n);
                if (got != pat)
                    return ddr_memtest_fail(DDR_ERR_MEMTEST_BLOCK, b + n,
                                            pat, got);
            }
        }
    }

    /* A set MISS means DMMAP0/1 do not describe the array just written. Read
     * the status once and pass it along by hand, since reading clears MISS and
     * ddr_memtest_fail()'s own snapshot would show it clear again. */
    got = REG32(DDRC_STATUS);
    if (got & DDRC_STATUS_MISS)
        return ddr_memtest_fail(DDR_ERR_MEMTEST_ADDR, DDR_TEST_BASE,
                                DDRC_STATUS_MISS, got);

    return DDR_OK;
}

/* Entry point */

int x1600_ddr_init(const struct x1600_ddr_param* p)
{
    int rc, i;

    ddr_last_status.magic = DDR_STATUS_MAGIC;
    ddr_last_status.step  = DDR_STEP_ENTRY;
    ddr_last_status.err   = DDR_OK;
    for (i = 0; i < 8; ++i)
        ddr_last_status.reg_snapshot[i] = 0;

    /* Verify the timebase before using tick-based waits. */
    if (!ddr_timebase_alive())
        return ddr_fail_clk(DDR_ERR_NO_TIMEBASE, 0, p->freq, 0);

    /* DDRC steps 1-3. Nothing above may touch a DDRC or PHY register. */
    rc = ddr_set_clock(p->freq);
    if (rc != DDR_OK)
        return rc;

    ddr_step(DDR_STEP_DRCG);
    ddr_dll_reset();

    /* Assert all four DCTRL resets, then hold DFI_RST alone until
     * ddr_mode_init() releases it */
    ddr_step(DDR_STEP_CTRL_RESET);
    REG32(DDRC_CTRL) = DDRC_CTRL_ALL_RST;
    ddr_mdelay(5);
    REG32(DDRC_CTRL) = DDRC_CTRL_DFI_RST;
    ddr_mdelay(5);
    REG32(DDRC_AUTOSR_EN) = 0;

    ddr_step(DDR_STEP_PHY_INIT);
    ddr_phy_init(p);

    ddr_step(DDR_STEP_PHY_PLL);
    rc = ddr_phy_pll();
    if (rc != DDR_OK)
        return rc;

    /* DDRC steps 5-7: timings, then the memory map. DCFG comes later, in
     * ddr_mode_init(), where the vendor puts it. */
    ddr_step(DDR_STEP_TIMING);
    for (i = 0; i < 6; ++i)
        REG32(DDRC_TIMING(i)) = p->timing[i];
    REG32(DDRC_MMAP0) = p->mmap0;
    REG32(DDRC_MMAP1) = p->mmap1;

    /* Refresh and auto-self-refresh stay off for the whole of initialisation */
    REG32(DDRC_AUTOSR_EN)  = 0;
    REG32(DDRC_AUTOSR_CNT) = 0;
    REG32(DDRC_REFCNT)     = 0;

    for (i = 0; i < 5; ++i)
        REG32(DDRC_REMAP(i)) = p->remap[i];

    rc = ddr_mode_init(p);
    if (rc != DDR_OK)
        return rc;

    ddr_step(DDR_STEP_CALIB);
    rc = ddr_phy_calib();
    if (rc != DDR_OK)
        return rc;

    ddr_step(DDR_STEP_ENABLE);
    REG32(DDRC_REFCNT) = p->refcnt;

    /* A no-op with this board's p->ctrl; another table would need it */
    REG32(DDRC_CTRL) |= (p->ctrl & 0xf804);

    REG32(DDRC_AUTOSR_CNT) = 0x101;
    REG32(DDRC_AUTOSR_EN)  = 1;
    REG32(DDRC_DLP) |= 7;   /* FSR | FPD | LPEN, the DFI LP interface */

    /* Record the end state either way; ddr_memtest_fail() overwrites three */
    ddr_snapshot();

    ddr_step(DDR_STEP_MEMTEST);
    rc = x1600_ddr_memtest();
    if (rc != DDR_OK)
        return rc;

    ddr_step(DDR_STEP_DONE);
    return DDR_OK;
}
