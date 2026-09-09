/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 *
 * X1600 DRAM controller / Innosilicon DDR PHY bring-up.
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

#ifndef __DDR_X1600_H__
#define __DDR_X1600_H__

#include <stdint.h>
#include "x1600/ddrc.h"
#include "x1600/ddrc_apb.h"
#include "x1600/ddrphy.h"
#include "x1600/cpm.h"

/* DDR initialisation parameters from the vendor SPL and PM chapter 3. */

/* DDRC and PHY addresses come from the generated register headers. PHY
 * setup values follow the vendor SPL where the PM is incomplete. */
#define DDRC_STATUS     JA_DDRC_STATUS      /* read-only */
#define DDRC_CFG        JA_DDRC_CFG
#define DDRC_CTRL       JA_DDRC_CTRL        /* reset 0x00800000 */
#define DDRC_LMR        JA_DDRC_LMR         /* command engine */
#define DDRC_REFCNT     JA_DDRC_REFCNT
#define DDRC_MMAP0      JA_DDRC_MMAP(0)
#define DDRC_MMAP1      JA_DDRC_MMAP(1)
#define DDRC_TIMING(n)  JA_DDRC_TIMING(n)   /* n = 0..5 -> TIMING1..6 */
#define DDRC_REMAP(n)   JA_DDRC_REMAP(n)    /* n = 0..4 -> REMAP1..5  */
#define DDRC_DLP        JA_DDRC_DLP
#define DDRC_AUTOSR_EN  JA_DDRC_AUTOSR_EN   /* see note below */
#define DDRC_AUTOSR_CNT JA_DDRC_AUTOSR_CNT  /* see note below */
/* The PM contradicts itself on where AUTOSR_EN/AUTOSR_CNT live; the vendor SPL
 * writes the Table 3-1 placement, so that is the one real on this silicon */

/* DDRC_STATUS is read-only. Reading clears MISS, so retain the first
 * sample. */
#define DDRC_STATUS_DFI_INIT_C  (1u << 31)
#define DDRC_STATUS_MISS        (1u << 6)   /* an access missed the mapping */
#define DDRC_STATUS_SREF        (1u << 2)
#define DDRC_STATUS_CKE0        (1u << 0)

/* Clear DFI_RST before PHY initialisation. The vendor sequence also uses
 * the three reset bits marked reserved in the PM. */
#define DDRC_CTRL_DFI_RST       (1u << 23)
#define DDRC_CTRL_DLL_RST       (1u << 22)
#define DDRC_CTRL_CTL_RST       (1u << 21)
#define DDRC_CTRL_CFG_RST       (1u << 20)
#define DDRC_CTRL_ALL_RST       (DDRC_CTRL_DFI_RST | DDRC_CTRL_DLL_RST | \
                                 DDRC_CTRL_CTL_RST | DDRC_CTRL_CFG_RST)
#define DDRC_CTRL_CKE_PULSE     (1u << 1)
#define DDRC_CTRL_RESET         (1u << 0)

/* DDRC_LMR: [31:12] DDR_ADDR drives A[13:0], [10:8] BA drives BA[2:0], [5:3]
 * CMD -- three bits, not two at [5:4] -- and [0] START, positive-edge
 * triggered, cleared by hardware once the command has issued. Software must
 * check START is 0 before writing 1. */
#define DDRC_LMR_START          0x00000001
#define DDRC_LMR_CMD_PRECHARGE  (0u << 3)
#define DDRC_LMR_CMD_AREF       (1u << 3)
#define DDRC_LMR_CMD_MRS        (2u << 3)
#define DDRC_LMR_CMD_ZQCS       (3u << 3) /* DDR3 only */
#define DDRC_LMR_CMD_ZQCL       (4u << 3) /* DDR3 only */
#define DDRC_LMR_BA(n)          (((n) & 7) << 8)

/* The PHY uses 8-bit register values in 32-bit words; use 32-bit accesses.
 * Address discrepancies in the PM are documented in x1600.reggen. */
#define DDRPHY_PHY_RST      JA_DDRPHY_PHY_RST
#define DDRPHY_MEM_CFG      JA_DDRPHY_MEM_CFG
#define DDRPHY_TRAIN_CTRL   JA_DDRPHY_TRAIN_CTRL
#define DDRPHY_CL           JA_DDRPHY_CL
#define DDRPHY_AL           JA_DDRPHY_AL /* keep 0 */
#define DDRPHY_CWL          JA_DDRPHY_CWL
#define DDRPHY_GATE_DELAY   JA_DDRPHY_GATE_DELAY /* [3:1] */
#define DDRPHY_CMD_BYPASS   JA_DDRPHY_CMD_BYPASS /* see note */
#define DDRPHY_CMD_DLL      JA_DDRPHY_CMD_DLL
#define DDRPHY_DQ_WIDTH     JA_DDRPHY_DQ_WIDTH /* 3 = 16 bit */
#define DDRPHY_PLL_FBDIV    JA_DDRPHY_PLL_FBDIV
#define DDRPHY_PLL_CTRL     JA_DDRPHY_PLL_CTRL /* [1] = PowDown */
#define DDRPHY_PLL_PDIV     JA_DDRPHY_PLL_PDIV /* [7:5] postdiv */
#define DDRPHY_PLL_LOCK     JA_DDRPHY_PLL_LOCK /* [3] = locked */
#define DDRPHY_CALIB_DONE   JA_DDRPHY_CALIB_DONE /* [1:0] = done */
#define DDRPHY_RXDLL_DQS0   JA_DDRPHY_RXDLL_DQS0 /* "CALIB_DELAY_AL" */
#define DDRPHY_RXDLL_DQS1   JA_DDRPHY_RXDLL_DQS1 /* "CALIB_DELAY_AH" */
#define DDRPHY_CALIB_RES_L  JA_DDRPHY_CALIB_RES_L
#define DDRPHY_CALIB_RES_R  JA_DDRPHY_CALIB_RES_R

/* PHY+0x050 follows the vendor CMD/CK DLL-bypass usage; the PM gives
 * conflicting definitions. */

/* [1:0] MEMSEL: 3 = LPDDR2.  [4] BSTSEL = burst 8. Bit [6] is undocumented
 * but both the PM and the vendor SPL set it after RX calibration. */
#define DDRPHY_MEM_CFG_CALIB_APPLY  0x40

/* [0] DSACE enables DQS-gating calibration, [1] DSCSE selects
 * register-bypass mode (0 = automatic) */
#define DDRPHY_TRAIN_CTRL_START     0x01
#define DDRPHY_TRAIN_CTRL_BYPASS    0x02

/* [0] and [1] are the low and high byte lanes done. The vendor waits for
 * both, plus an undocumented bit 4 clear. */
#define DDRPHY_CALIB_DONE_MASK      0x13
#define DDRPHY_CALIB_DONE_VALUE     0x03

#define DDRPHY_PLL_LOCKED           0x08

/* DDRC APB group.  +0x08c is PHY_INIT, the DFI handshake register, not
 * PHYRST_CFG, which is a different register at +0x080. */
#define DDRC_APB_PHY_INIT   JA_DDRC_APB_PHY_INIT
#define DDRC_APB_PHY_INIT_START     0x01  /* [0] dfi_init_start,    RW */
#define DDRC_APB_PHY_INIT_COMPLETE  0x02  /* [1] dfi_init_complete, R  */
#define DDRC_APB_PHY_INIT_PLLLOCK   0x04  /* [2] ddr_plllock,       R  */

/* CPM registers this driver programs itself */
#define CPM_DRCG_ADDR   JA_CPM_DRCG /* DDR clk gate */
/* Vendor DRAM type identifiers. The X1600 DDRC supports DDR2 and LPDDR2;
 * X1600E uses SiP LPDDR2. */
enum x1600_ddr_type {
    X1600_DDR3   = 0,
    X1600_LPDDR  = 1,
    X1600_LPDDR2 = 2,
    X1600_LPDDR3 = 3,
    X1600_DDR2   = 4,
};

/* Initialisation errors use 0x01-0x0f; memory-test errors use 0x10-0x1f. */
enum x1600_ddr_err {
    DDR_OK                  = 0x00,
    DDR_ERR_CLK_MPLL_BUSY   = 0x01,
    DDR_ERR_CLK_MPLL_LOCK   = 0x02,
    DDR_ERR_CLK_DDR_BUSY    = 0x03,
    DDR_ERR_CLK_SRC         = 0x04,
    DDR_ERR_PHY_PLL_LOCK    = 0x05,
    DDR_ERR_PHY_DFI_PLL     = 0x06,
    DDR_ERR_DFI_INIT        = 0x07,
    DDR_ERR_TYPE            = 0x08,
    DDR_ERR_MR              = 0x09,
    DDR_ERR_PHY_CALIB       = 0x0a,
    DDR_ERR_NO_TIMEBASE     = 0x0b,
    DDR_ERR_MEMTEST_DATA    = 0x10,
    DDR_ERR_MEMTEST_ADDR    = 0x11,
    DDR_ERR_MEMTEST_SPARSE  = 0x12,
    DDR_ERR_MEMTEST_BLOCK   = 0x13,
};

/* Step markers, stored in ddr_last_status.step as the sequence advances: on a
 * hang, the last value written says which phase was running */
enum x1600_ddr_step {
    DDR_STEP_ENTRY          = 0x00,
    DDR_STEP_UNGATE         = 0x01,
    DDR_STEP_MPLL           = 0x02,
    DDR_STEP_DDRCDR         = 0x03,
    DDR_STEP_DRCG           = 0x04,
    DDR_STEP_CTRL_RESET     = 0x05,
    DDR_STEP_PHY_INIT       = 0x06,
    DDR_STEP_PHY_PLL        = 0x07,
    DDR_STEP_TIMING         = 0x08,
    DDR_STEP_DFI            = 0x09,
    DDR_STEP_MODE_REGS      = 0x0a,
    DDR_STEP_CALIB          = 0x0b,
    DDR_STEP_ENABLE         = 0x0c,
    DDR_STEP_MEMTEST        = 0x0d,
    DDR_STEP_DONE           = 0x0e,
};

/* Host-readable result block. Snapshot slots are assigned by
 * ddr_fail_clk(), ddr_snapshot() and ddr_memtest_fail(). Before DDR is
 * clocked, only CPM registers may be read. */
#define DDR_STATUS_MAGIC    0xdd120001

struct ddr_status {
    uint32_t magic;             /* DDR_STATUS_MAGIC once x1600_ddr_init ran  */
    uint32_t step;              /* enum x1600_ddr_step, last phase entered   */
    uint32_t err;               /* enum x1600_ddr_err, 0 on success          */
    uint32_t reg_snapshot[8];
};

extern struct ddr_status ddr_last_status;

/* Parameters used by DDR initialisation. Comments give offsets in the
 * vendor structure at mtd0+0x4060. */
struct x1600_ddr_param {
    const char* name;       /* +0x00, ASCII part number                       */
    uint32_t id;            /* +0x20, "burned ddr id"                         */
    uint32_t type;          /* +0x24, enum x1600_ddr_type                     */
    uint32_t freq;          /* +0x28, DDR clock in Hz                         */
    uint32_t cfg;           /* +0x2c, -> DDRC_CFG                             */
    uint32_t ctrl;          /* +0x30, OR'd into DDRC_CTRL masked with 0xf804  */
    uint32_t cs;            /* +0x34, chip-select bits OR'd into every LMR    */
    uint32_t mmap0;         /* +0x3c, -> DDRC_MMAP0                           */
    uint32_t mmap1;         /* +0x40, -> DDRC_MMAP1                           */
    uint32_t refcnt;        /* +0x44, -> DDRC_REFCNT                          */
    uint32_t timing[6];     /* +0x48..+0x5c, -> DDRC_TIMING1..6               */
    uint32_t phy_mem_cfg;   /* +0x64, -> DDRPHY_MEM_CFG                       */
    uint32_t phy_cl;        /* +0x68, -> DDRPHY_CL  [3:0]                     */
    uint32_t phy_cwl;       /* +0x6c, -> DDRPHY_CWL [3:0]                     */
    uint32_t mr0;           /* +0x70, (MA << 8) | OP; DDR3 path only          */
    uint32_t mr1;           /* +0x74                                          */
    uint32_t mr2;           /* +0x78                                          */
    uint32_t mr3;           /* +0x7c                                          */
    uint32_t mr10;          /* +0x80                                          */
    uint32_t mr63;          /* +0x88                                          */
    uint32_t remap[5];      /* +0x94..+0xa4, -> DDRC_REMAP1..5                */
};

extern const struct x1600_ddr_param x1600_ddr_param_hibyr1;

/* Initialise DRAM. Returns DDR_OK or an x1600_ddr_err and records details
 * in ddr_last_status. Requires a running OST. */
int x1600_ddr_init(const struct x1600_ddr_param* p);

/* Post-init memory test, exposed so a stage1 or debug menu can re-run it.
 * Writes ~8 KiB through KSEG1, above the first megabyte of DRAM, and reports
 * the first mismatch in ddr_last_status. */
int x1600_ddr_memtest(void);

#endif /* __DDR_X1600_H__ */
