/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#ifndef __EMI_IMX233_H__
#define __EMI_IMX233_H__

#include "cpu.h"
#include "system.h"
#include "system-target.h"

#define HW_EMI_BASE     0x80020000

#define HW_EMI_CTRL     (*(volatile uint32_t *)(HW_EMI_BASE + 0x0))
#define HW_EMI_CTRL__DLL_SHIFT_RESET    (1 << 25)
#define HW_EMI_CTRL__DLL_RESET          (1 << 24)

/* this register is undocumented but exists, I put the whole doc here */
#define HW_EMI_STAT     (*(volatile uint32_t *)(HW_EMI_BASE + 0x10))
#define HW_EMI_STAT__DRAM_PRESENT       (1 << 31)
#define HW_EMI_STAT__NOR_PRESENT        (1 << 30)
#define HW_EMI_STAT__LARGE_DRAM_ENABLED (1 << 29)
#define HW_EMI_STAT__DRAM_HALTED        (1 << 1)
#define HW_EMI_STAT__NOR_BUSY           (1 << 0)

/* another undocumented registers (there are some more) */
#define HW_EMI_TIME     (*(volatile uint32_t *)(HW_EMI_BASE + 0x20))
#define HW_EMI_TIME__THZ_BP     24
#define HW_EMI_TIME__THZ_BM     (0xf << 24)
#define HW_EMI_TIME__TDH_BP     16
#define HW_EMI_TIME__TDH_BM     (0xf << 16)
#define HW_EMI_TIME__TDS_BP     8
#define HW_EMI_TIME__TDS_BM     (0x1f << 8)
#define HW_EMI_TIME__TAS_BP     0
#define HW_EMI_TIME__TAS_BM     (0xf << 0)

/** WARNING: the HW_DRAM_* registers don't have a SCT variant ! */
#define HW_DRAM_BASE    0x800E0000

#define HW_DRAM_CTLxx(xx) (*(volatile uint32_t *)(HW_DRAM_BASE + 0x4 * (xx)))

#define HW_DRAM_CTL00   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x0))
#define HW_DRAM_CTL01   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x4))
#define HW_DRAM_CTL02   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x8))
#define HW_DRAM_CTL03   (*(volatile uint32_t *)(HW_DRAM_BASE + 0xc))

#define HW_DRAM_CTL04   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x10))
#define HW_DRAM_CTL04__DLL_BYPASS_MODE  (1 << 24)
#define HW_DRAM_CTL04__DLLLOCKREG       (1 << 16)

#define HW_DRAM_CTL05   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x14))
#define HW_DRAM_CTL05__EN_LOWPOWER_MODE (1 << 0)

#define HW_DRAM_CTL06   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x18))
#define HW_DRAM_CTL07   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x1c))

#define HW_DRAM_CTL08   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x20))
#define HW_DRAM_CTL08__SREFRESH (1 << 8)

#define HW_DRAM_CTL09   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x24))

#define HW_DRAM_CTL10   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x28))
#define HW_DRAM_CTL10__TEMRS_BP 8
#define HW_DRAM_CTL10__TEMRS_BM (0x3 << 8)

#define HW_DRAM_CTL11   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x2c))
#define HW_DRAM_CTL11__CASLAT_BP    0
#define HW_DRAM_CTL11__CASLAT_BM    (0x7 << 0)

#define HW_DRAM_CTL12   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x30))
#define HW_DRAM_CTL12__TCKE_BP      0
#define HW_DRAM_CTL12__TCKE_BM      (0x7 << 0)
#define HW_DRAM_CTL12__TRRD_BP      16
#define HW_DRAM_CTL12__TRRD_BM      (0x7 << 16)
#define HW_DRAM_CTL12__TWR_INT_BP   24
#define HW_DRAM_CTL12__TWR_INT_BM   (0x7 << 24)

#define HW_DRAM_CTL13   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x34))
#define HW_DRAM_CTL13__TWTR_BP      0
#define HW_DRAM_CTL13__TWTR_BM      (0x7 << 0)
#define HW_DRAM_CTL13__CASLAT_LIN_BP    16
#define HW_DRAM_CTL13__CASLAT_LIN_BM    (0xf << 16)
#define HW_DRAM_CTL13__CASLAT_LIN_GATE_BP   24
#define HW_DRAM_CTL13__CASLAT_LIN_GATE_BM   (0xf << 24)

#define HW_DRAM_CTL14   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x38))

#define HW_DRAM_CTL15   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x3c))
#define HW_DRAM_CTL15__TDAL_BP      16
#define HW_DRAM_CTL15__TDAL_BM      (0xf << 16)
#define HW_DRAM_CTL15__TRP_BP       24
#define HW_DRAM_CTL15__TRP_BM       (0xf << 24)

#define HW_DRAM_CTL16   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x40))
#define HW_DRAM_CTL16__TMRD_BP       24
#define HW_DRAM_CTL16__TMRD_BM       (0x1f << 24)

#define HW_DRAM_CTL17   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x44))
#define HW_DRAM_CTL17__TRC_BP        0
#define HW_DRAM_CTL17__TRC_BM        (0x1f << 0)
#define HW_DRAM_CTL17__DLL_INCREMENT_BP     8
#define HW_DRAM_CTL17__DLL_INCREMENT_BM     (0xff << 0)
#define HW_DRAM_CTL17__DLL_START_POINT_BP   24
#define HW_DRAM_CTL17__DLL_START_POINT_BM   (0xff << 24)

#define HW_DRAM_CTL18   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x48))
#define HW_DRAM_CTL17__DLL_DQS_DELAY_0_BP   16
#define HW_DRAM_CTL17__DLL_DQS_DELAY_0_BM   (0x7f << 16)
#define HW_DRAM_CTL17__DLL_DQS_DELAY_1_BP   24
#define HW_DRAM_CTL17__DLL_DQS_DELAY_1_BM   (0x7f << 24)

#define HW_DRAM_CTL19   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x4c))
#define HW_DRAM_CTL19__DQS_OUT_SHIFT_BP     16
#define HW_DRAM_CTL19__DQS_OUT_SHIFT_BM     (0x7f << 16)

#define HW_DRAM_CTL20   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x50))
#define HW_DRAM_CTL20__WR_DQS_SHIFT_BP  0
#define HW_DRAM_CTL20__WR_DQS_SHIFT_BM  (0x7f << 0)
#define HW_DRAM_CTL20__TRAS_MIN_BP      16
#define HW_DRAM_CTL20__TRAS_MIN_BM      (0xff << 16)
#define HW_DRAM_CTL20__TRCD_INT_BP      24
#define HW_DRAM_CTL20__TRCD_INT_BM      (0xff << 24)

#define HW_DRAM_CTL21   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x54))
#define HW_DRAM_CTL21__TRFC_BP      0
#define HW_DRAM_CTL21__TRFC_BM      (0xff << 0)

#define HW_DRAM_CTL22   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x58))
#define HW_DRAM_CTL23   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x5c))
#define HW_DRAM_CTL24   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x60))
#define HW_DRAM_CTL25   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x64))

#define HW_DRAM_CTL26   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x68))
#define HW_DRAM_CTL26__TREF_BP      0
#define HW_DRAM_CTL26__TREF_BM      (0xfff << 0)

#define HW_DRAM_CTL27   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x6c))
#define HW_DRAM_CTL28   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x70))
#define HW_DRAM_CTL29   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x74))
#define HW_DRAM_CTL30   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x78))

#define HW_DRAM_CTL31   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x7c))
#define HW_DRAM_CTL31__TDLL_BP      16
#define HW_DRAM_CTL31__TDLL_BM      (0xffff << 16)

#define HW_DRAM_CTL32   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x80))
#define HW_DRAM_CTL32__TRAS_MAX_BP  0
#define HW_DRAM_CTL32__TRAS_MAX_BM  (0xffff << 0)
#define HW_DRAM_CTL32__TXSNR_BP     16
#define HW_DRAM_CTL32__TXSNR_BM     (0xffff << 16)

#define HW_DRAM_CTL33   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x84))
#define HW_DRAM_CTL33__TXSR_BP      0
#define HW_DRAM_CTL33__TXSR_BM      (0xffff << 0)

#define HW_DRAM_CTL34   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x88))
#define HW_DRAM_CTL34__TINIT_BP 0
#define HW_DRAM_CTL34__TINIT_BM 0xffffff

#define HW_DRAM_CTL35   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x8c))
#define HW_DRAM_CTL36   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x90))
#define HW_DRAM_CTL37   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x94))
#define HW_DRAM_CTL38   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x98))
#define HW_DRAM_CTL39   (*(volatile uint32_t *)(HW_DRAM_BASE + 0x9a))

#define HW_DRAM_CTL40   (*(volatile uint32_t *)(HW_DRAM_BASE + 0xa0))
#define HW_DRAM_CTL40__TPDEX_BP     16
#define HW_DRAM_CTL40__TPDEX_BM     (0xffff << 16)

/** Interesting fields:
 * - TCKE: CTL12
 * - TDAL: CTL15
 * - TDLL: CTL31
 * - TEMRS: CTL10
 * - TINIT: CTL34
 * - TMRD: CTL16
 * - TPDEX: CTL40
 * - TRAS_MAX: CTL32
 * - TRAS_MIN: CTL20
 * - TRC: CTL17
 * - TRCD_INT: CTL20
 * - TREF: CTL26
 * - TRFC: CTL21
 * - TRP: CTL15
 * - TRRD: CTL12
 * - TWR_INT: CTL12
 * - TWTR: CTL13
 * - TXSNR: CTL32
 * - TXSR: CTL33
 * - DLL_DQS_DELAY_BYPASS_0
 * - DLL_DQS_DELAY_BYPASS_1
 * - DQS_OUT_SHIFT_BYPASS
 * - WR_DQS_SHIFT_BYPASS
 * - DLL_INCREMENT: CTL17
 * - DLL_START_POINT: CTL17
 * - DLL_DQS_DELAY_0: CTL18
 * - DLL_DQS_DELAY_1: CTL18
 * - DQS_OUT_SHIFT: CTL19
 * - WR_DQS_SHIFT: CTL20
 * - CAS: CTL11
 * - DLL_BYPASS_MODE: CTL04
 * - SREFRESH: CTL08
 * - CASLAT_LIN: CTL13
 * - CASLAT_LIN_GATE: CTL13
 *
 * Interesting registers:
 * - CTL04: DLL_BYPASS_MODE
 * - CTL08: SREFRESH
 * - CTL10: TEMRS
 * - CTL11: CASLAT
 * - CTL12: TCKE TRRD TWR_INT
 * - CTL13: TWTR CASLAT_LIN CASLAT_LIN_GATE
 * - CTL15: TDAL TRP
 * - CTL16: TMRD
 * - CTL17: TRC DLL_INCREMENT DLL_START_POINT
 * - CTL18: DLL_DQS_DELAY_0 DLL_DQS_DELAY_1
 * - CTL19: DQS_OUT_SHIFT
 * - CTL20: WR_DQS_SHIFT TRAS_MIN TRCD_INT
 * - CTL21 TRFC
 * - CTL26: TREF
 * - CTL31: TDLL
 * - CTL32: TRAS_MAX TXSNR TXSR: CTL33
 * - CTL34: TINIT
 * - CTL40: TPDEX

 * - DLL_DQS_DELAY_BYPASS_0
 * - DLL_DQS_DELAY_BYPASS_1
 * - DQS_OUT_SHIFT_BYPASS
 * - WR_DQS_SHIFT_BYPASS
 */

/**
 * Absolute maximum EMI speed:  151.58 MHz (mDDR),  130.91 MHz (DDR)
 * Intermediate EMI speeds: 130.91 MHz,  120.00 MHz, 64 MHz, 24 MHz
 * Absolute minimum CPU speed: 24 MHz */
#define IMX233_EMIFREQ_151_MHz  151580
#define IMX233_EMIFREQ_130_MHz  130910
#define IMX233_EMIFREQ_120_MHz  120000
#define IMX233_EMIFREQ_64_MHz    64000
#define IMX233_EMIFREQ_24_MHz    24000

void imx233_emi_set_frequency(unsigned long freq);

#endif /* __EMI_IMX233_H__ */
