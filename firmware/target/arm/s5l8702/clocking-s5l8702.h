/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:
 *
 * Copyright (C) 2015 by Cástor Muñoz
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
#ifndef __CLOCKING_S5L8702_H
#define __CLOCKING_S5L8702_H

/*
 *  - This is work in progress, results are mainly based on experimental
 *    tests using emCORE and/or iPod Classic OF reverse engineering.
 *  - Things marked as TBC are 'somewhat' speculations, so might be
 *    inconplete, inaccurate or erroneous.
 *  - Things not marked as TBC could be also incomplete, inaccurate or
 *    erroneous.
 */

/* S5L8702 _figured_ clocking:
 *
 *                                         CG16_SEL
 *                                      CG16_UNKOSC_BIT
 *                                            ||
 *                                            ||
 *                                     UNKOSC ||
 *                       OSCSEL        ------>||
 *                         |\                 ||
 *  OSC0 -+--------------->| |         OSCClk ||
 *        |                | |--------------->||      ______
 *  OSC1 -)+-------------->| |                ||     |      |            FClk
 *        ||               |/                 ||---->| DIV1 |---------------->
 *        ||     ______                       ||     |______|
 *        ||    |      | Pll0Freq |\          ||     CG16_SYS
 *        ++===>| PMS0 |--------->| |         ||
 *        ||    |______|          | | PLL0Clk ||      ____________
 *        ||      PLL0            | |-------->||     |            |   U2L_Clk
 *        ||                 OSC1 | |         ||---->| DIV1,UNK14 |---------->
 *        |+--------------------->| |         ||     |____________|
 *        ||                      |/          ||     CG16_2L
 *        ||                   PLLOUT0        ||
 *        ||                                  ||      ______
 *        ||     ______                       ||     |      |        SVID_Clk
 *        ||    |      | Pll1Freq |\          ||---->| DIV1 |---------------->
 *        ++===>| PMS1 |--------->| |         ||     |______|
 *         |    |______|          | | PLL1Clk ||     CG16_SVID (TBC)
 *         |      PLL1            | |-------->||
 *         |                 OSC1 | |         ||      ___________
 *         +--------------------->| |         ||     |           |   AUDx_Clk
 *         |                      |/          ||---->| DIV1,DIV2 |----------->
 *         |                   PLLOUT1        ||     |___________|
 *         |                                  ||     CG16_AUDx
 *         |     ______                       ||
 *         |    |      | Pll2Freq |\          ||      ___________
 *         +--->| PMS2 |--------->| |         ||     |           |       EClk
 *         |    |______|          | | PLL2Clk ||---->| DIV1,DIV2 |--+-------->
 *         |      PLL2            | |-------->||     |___________|  |
 *         |                 OSC1 | |         ||     CG16_RTIME     | MIU_Clk
 *         +--------------------->| |         ||                    +-------->
 *                                |/          ||      ______
 *                             PLLOUT2        ||     |      |         U5L_Clk
 *                                            ||---->| DIV1 |---------------->
 *                                            ||     |______|
 *                                            ||     CG16_5L
 *
 *               ______
 *              |      | CClk
 *  FClk --+--->| CDIV |------------> ARM_Clk
 *         |    |______|
 *         |
 *         |     ______
 *         |    |      | HClk
 *         +--->| HDIV |---------> CLKCON0[31] --> SDR_Clk
 *         |    |______|      |
 *         |                  |    PWRCON_AHB
 *         |                  +-----> [0] ----> Clk_SHA1
 *         |                          [1] ----> Clk_LCD
 *         |                          [2] ----> Clk_USBOTG
 *         |                          [3] ----> Clk_SMx     TBC: Static Memory
 *         |                          [4] ----> Clk_SM1          (ctrl) related
 *         |                          [5] ----> Clk_ATA
 *         |                          [6] ----> Clk_UNK
 *         |                          [7] ----> Clk_UNK
 *         |                          [8] ----> Clk_NAND    TBC
 *         |                          [9] ----> Clk_SDCI
 *         |                          [10] ---> Clk_AES
 *         |                          [11] ---> Clk_UNK
 *         |                          [12] ---> Clk_ECC     TBC
 *         |                          [13] ---> Clk_UNK
 *         |                          [14] ---> Clk_EV0     TBC: Ext. Video
 *         |                          [15] ---> Clk_EV1     TBC: Ext. Video
 *         |                          [16] ---> Clk_EV2     TBC: Ext. Video
 *         |                          [17] ---> Clk_UNK
 *         |                          [18] ---> Clk_UNK
 *         |                          [19] ---> Clk_UNK
 *         |                          [20] ---> Clk_UNK
 *         |                          [21] ---> Clk_UNK
 *         |                          [22] ---> Clk_UNK
 *         |                          [23] ---> Clk_UNK
 *         |                          [24] ---> Clk_UNK
 *         |                          [25] ---> Clk_DMA0
 *         |                          [26] ---> Clk_DMA1
 *         |                          [27] ---> Clk_UNK
 *         |                          [28] ---> Clk_UNK
 *         |                          [29] ---> Clk_UNK
 *         |                          [30] ---> Clk_ROM
 *         |                          [31] ---> Clk_UNK
 *         |
 *         |     ______
 *         |    |      | PClk      PWRCON_APB
 *         +--->| PDIV |------------> [0] ----> Clk_RTC
 *              |______|              [1] ----> Clk_CWHEEL
 *                                    [2] ----> Clk_SPI0
 *                                    [3] ----> Clk_USBPHY
 *                                    [4] ----> Clk_I2C0
 *                                    [5] ----> Clk_TIMER   16/32-bit timer
 *                                    [6] ----> Clk_I2C1
 *                                    [7] ----> Clk_I2S0
 *                                    [8] ----> Clk_UNK     TBC: SPDIF Out
 *                                    [9] ----> Clk_UART
 *                                    [10] ---> Clk_I2S1
 *                                    [11] ---> Clk_SPI1
 *                                    [12] ---> Clk_GPIO
 *                                    [13] ---> Clk_SBOOT   TBC: Secure Boot
 *                                    [14] ---> Clk_CHIPID
 *                                    [15] ---> Clk_SPI2
 *                                    [16] ---> Clk_I2S2
 *                                    [17] ---> Clk_UNK
 *
 * IRAM notes:
 * - IRAM0 (1st. 128 Kb) and IRAM1 (2nd. 128 Kb) uses diferent clocks,
 *   maximum rd/wr speed for IRAM1 is about a half of maximum rd/wr
 *   speed for IRAM0, it is unknown but probably they are different
 *   HW memory.
 * - masking Clk_SMx disables access to IRAM0 and IRAM1
 * - masking Clk_SM1 disables access to IRAM1
 * - IRAM1 rd/wr speed is scaled by SM1_DIV, so it could be related
 *   with Clk_SM1 (TBC)
 */

#include <inttypes.h>
#include "config.h"

#define CLOCKING_DEBUG

#if defined(IPOD_6G)
/* iPod Classic target */
#define S5L8702_OSC0_HZ         12000000  /* external OSC */
#define S5L8702_OSC1_HZ         32768     /* from PMU */

#define S5L8702_ALTOSC0_HZ      0   /* TBC */
#define S5L8702_ALTOSC1_HZ      0   /* TBC */

/* this clock is selected when CG16_UNKOSC_BIT is set,
   ignoring PLLMODE_CLKSEL and CG16_SEL settings */
/* TBC: OSC0*2 ???, 24 MHz Xtal ???, USB ??? */
#define S5L8702_UNKOSC_HZ       24000000

#else
/* s5l8702 ROMBOOT */
#define S5L8702_OSC0_HZ         (soc_get_osc0())  /* external OSC */
#define S5L8702_OSC1_HZ         32768             /* from PMU */

#define S5L8702_ALTOSC0_HZ      1800000
#define S5L8702_ALTOSC1_HZ      27000000
#endif

/* TODO: join all these definitions in an unique place */
#if 1
#include "s5l8702.h"
#else
#define CLKCON0     (*((volatile uint32_t*)(0x3C500000)))
#define CLKCON1     (*((volatile uint32_t*)(0x3C500004)))
#define CLKCON2     (*((volatile uint32_t*)(0x3C500008)))
#define CLKCON3     (*((volatile uint32_t*)(0x3C50000C)))
#define CLKCON4     (*((volatile uint32_t*)(0x3C500010)))
#define CLKCON5     (*((volatile uint32_t*)(0x3C500014)))
#define PLL0PMS     (*((volatile uint32_t*)(0x3C500020)))
#define PLL1PMS     (*((volatile uint32_t*)(0x3C500024)))
#define PLL2PMS     (*((volatile uint32_t*)(0x3C500028)))
#define PLL0LCNT    (*((volatile uint32_t*)(0x3C500030)))
#define PLL1LCNT    (*((volatile uint32_t*)(0x3C500034)))
#define PLL2LCNT    (*((volatile uint32_t*)(0x3C500038)))
#define PLLLOCK     (*((volatile uint32_t*)(0x3C500040)))
#define PLLMODE     (*((volatile uint32_t*)(0x3C500044)))
#define PWRCON(i)   (*((volatile uint32_t*)(0x3C500048 + ((i)*4)))) /*i=1,2*/
#endif

/* TBC: ATM i am assuming that PWRCON_AHB/APB registers are clockgates
 * for SoC internal controllers sitting on AHB/APB buses, this is based
 * on other similar SoC documentation and experimental results for many
 * (not all) s5l8702 controllers.
 */
#define PWRCON_AHB  (*((uint32_t volatile*)(0x3C500048)))
#define PWRCON_APB  (*((uint32_t volatile*)(0x3C50004c)))

#define PLLPMS(i)   (*((volatile uint32_t*)(0x3C500020 + ((i) * 4))))
#define PLLCNT(i)   (*((volatile uint32_t*)(0x3C500030 + ((i) * 4))))
#define PLLMOD2     (*((volatile uint32_t*)(0x3C500060)))
#define PLLCNT_MSK  0x3fffff

/* TBC: Clk_SM1 = HClk / (SM1_DIV[3:0] + 1) */
#define SM1_DIV     (*((volatile uint32_t*)(0x38501000)))


/* CG16_x: for readability and debug, these gates are defined as
 * 16-bit registers, on HW they are really halves of 32-bit registers.
 * Some functionallity is not available on all CG16 gates (when so,
 * related bits are read-only and fixed to 0).
 *
 *                CLKCONx   DIV1    DIV2    UNKOSC   UNK14
 *  CG16_SYS      0L        +
 *  CG16_2L       2L        +               +(TBC)   +(TBC)
 *  CG16_SVID     2H        +               +(TBC)
 *  CG16_AUD0     3L        +       +
 *  CG16_AUD1     3H        +       +
 *  CG16_AUD2     4L        +       +
 *  CG16_RTIME    4H        +       +       +
 *  CG16_5L       5L        +
 *
 * Not all gates are fully tested, this information is mainly based
 * on experimental test using emCORE:
 *  - CG16_SYS and CG16_RTIME were tested mainly using time benchs.
 *  - EClk is used as a fixed clock (not depending on CPU/AHB/APB
 *    settings) for the timer contrller. MIU_Clk is used by the MIU
 *    controller to generate the DRAM refresh signals.
 *  - AUDxClk are a source selection for I2Sx modules, so they can
 *    can be scaled and routed to the I2S GPIO ports, where they
 *    were sampled (using emCORE) to inspect how they behave.
 *  - CG16_SVID seem to be used for external video, this info is
 *    based on OF diagnostics reverse engineering.
 *  - CG16_2L an CG16_5L usage is unknown.
 */
#define CG16_SYS        (*((volatile uint16_t*)(0x3C500000)))
#define CG16_2L         (*((volatile uint16_t*)(0x3C500008)))
#define CG16_SVID       (*((volatile uint16_t*)(0x3C50000A)))
#define CG16_AUD0       (*((volatile uint16_t*)(0x3C50000C)))
#define CG16_AUD1       (*((volatile uint16_t*)(0x3C50000E)))
#define CG16_AUD2       (*((volatile uint16_t*)(0x3C500010)))
#define CG16_RTIME      (*((volatile uint16_t*)(0x3C500012)))
#define CG16_5L         (*((volatile uint16_t*)(0x3C500014)))

/* CG16 output frequency =
   !DISABLE_BIT * SEL_x frequency / DIV1+1 / DIV2+1 */
#define CG16_DISABLE_BIT    (1 << 15)   /* mask clock output */
#define CG16_UNK14_BIT      (1 << 14)   /* writable on CG16_2L */

#define CG16_SEL_POS        12          /* source clock selection */
#define CG16_SEL_MSK        0x3
#define CG16_SEL_OSC        0
#define CG16_SEL_PLL0       1
#define CG16_SEL_PLL1       2
#define CG16_SEL_PLL2       3

#define CG16_UNKOSC_BIT     (1 << 11)

#define CG16_DIV2_POS       4           /* 2nd divisor */
#define CG16_DIV2_MSK       0xf

#define CG16_DIV1_POS       0           /* 1st divisor */
#define CG16_DIV1_MSK       0xf

/*
 *  CLKCON0
 */
#define CLKCON0_SDR_DISABLE_BIT (1 << 31)

/*
 *  CLKCON1
 */
/* CPU/AHB/APB real_divisor =
   xDIV_EN_BIT ? 2*(reg_value+1) : 1 */
#define CLKCON1_CDIV_POS        24
#define CLKCON1_CDIV_MSK        0x1f
#define CLKCON1_CDIV_EN_BIT     (1 << 30)

#define CLKCON1_HDIV_POS        16
#define CLKCON1_HDIV_MSK        0x1f
#define CLKCON1_HDIV_EN_BIT     (1 << 22)

#define CLKCON1_PDIV_POS        8
#define CLKCON1_PDIV_MSK        0x1f
#define CLKCON1_PDIV_EN_BIT     (1 << 14)

/* AHB/APB ratio: must be written when HDIV and/or PDIV
   are modified, real_ratio = reg_value + 1 */
#define CLKCON1_HPRAT_POS       0
#define CLKCON1_HPRAT_MSK       0x3f

/*
 *  CLKCON5
 */
/* TBC: this bit selects a clock routed (at least) to all I2S modules
 * (AUDAUX_Clk, see i2s-s5l8702.h), it can be selected as a source
 * for CODEC_CLK (MCLK), on iPod Classic AUDAUX_Clk is:
 *  0 -> 12 MHz (TBC: OSC0 ???)
 *  1 -> 24 MHz (TBC: 2*OSC0 ???)
 */
#define CLKCON5_AUDAUXCLK_BIT   (1 << 31)

/*
 *  PLLnPMS
 */
#define PLLPMS_PDIV_POS         24      /* pre-divider */
#define PLLPMS_PDIV_MSK         0x3f
#define PLLPMS_MDIV_POS         8       /* main divider */
#define PLLPMS_MDIV_MSK         0xff
#define PLLPMS_SDIV_POS         0       /* post-divider (2^S) */
#define PLLPMS_SDIV_MSK         0x7

/*
 *  PLLLOCK
 */
/* Start status:
   0 -> in progress, 1 -> locked */
#define PLLLOCK_LCK_BIT(n)      (1 << (n))

/* Lock status for Divisor Mode (DM):
   0 -> DM unlocked, 1 -> DM locked */
#define PLLLOCK_DMLCK_BIT(n)    (1 << (4 + (n)))

/*
 *  PLLMODE
 */
/* Enable PLL0,1,2:
   0 -> turned off, 1 -> turned on */
#define PLLMODE_EN_BIT(n)       (1 << (n))

/* Select PMS mode for PLL0,1:
   0 -> mutiply mode (MM), 1 -> divide mode (DM) */
#define PLLMODE_PMSMOD_BIT(n)   (1 << (4 + (n)))

/* Select DMOSC for PLL2:
   0 -> DMOSC_STD, 1 -> DMOSC_ALT */
#define PLLMODE_PLL2DMOSC_BIT   (1 << 6)

/* Select oscilator for CG16_SEL_OSC source:
   0 -> S5L8702_OSC0, 1 -> S5L8702_OSC1 */
#define PLLMODE_OSCSEL_BIT      (1 << 8)

/* Select PLLxClk (a.k.a. "slow mode" (see s3c2440-DS) for PLL0,1,2:
   O -> S5L8702_OSC1, 1 -> PLLxFreq */
#define PLLMODE_PLLOUT_BIT(n)   (1 << (16 + (n)))

/*
 *  PLLMOD2
 */
/* Selects ALTOSCx for PLL0,1,2 when DMOSC == DMOSC_ALT:
   0 -> S5L8702_ALTOSC0, 1 -> S5L8702_ALTOSC1 */
#define PLLMOD2_ALTOSC_BIT(n)   (1 << (n))

/* Selects DMOSC for PLL0,1:
   0 -> DMOSC_STD, 1 -> DMOSC_ALT */
#define PLLMOD2_DMOSC_BIT(n)    (1 << (4 + (n)))


/* See s3c2440-DS (figure 7.2) for similar SoC reference.
 *
 * There are two different PMS modes, PLLxFreq is:
 *   Divide Mode (DM):   (F_in * MDIV / PDIV) / 2^SDIV
 *   Multiply Mode (MM): (F_in * MDIV * PDIV) / 2^SDIV
 *
 * PLL0 and PLL1 supports DM and MM, PLL2 only supports DM.
 *
 * MM uses S5L8702_OSC1. DM oscillator is selected using DMOSC_BIT
 * and ALTOSC_BIT.
 *
 * PLLLOCK_LCK_BIT is not enabled when PLL gets locked, and being
 * enabled doesn't meant that the PLL is locked. When using MULTIPLY
 * mode, there is no (known) way to verify that the PLL is locked.
 * On DIVIDE mode, PLLLOCK_DMLCK_BIT is enabled when the PLL is
 * locked at the correct frequency.
 * PLLLOCK_LCK_BIT is enabled only when lock_time expires, lock_time
 * is configured in PLLCNT as ticks of PClk. The maximum needed time
 * to get a good lock is ~300nS (TBC).
 *
 * TODO: F_vco notes
 */
#define PMSMOD_MUL  0
#define PMSMOD_DIV  1
#define GET_PMSMOD(pll) (((pll) == 2) \
            ? PMSMOD_DIV \
            : ((PLLMODE & PLLMODE_PMSMOD_BIT(pll)) ? PMSMOD_DIV \
                                                   : PMSMOD_MUL))

#define DMOSC_STD  0
#define DMOSC_ALT  1
#define GET_DMOSC(pll) ((((pll) == 2) \
            ? (PLLMODE & PLLMODE_PLL2DMOSC_BIT) \
            : (PLLMOD2 & PLLMOD2_DMOSC_BIT(pll))) ? DMOSC_ALT \
                                                  : DMOSC_STD)

/* available PLL operation modes */
#define PLLOP_MM    0   /* Multiply Mode, F_in = S5L8702_OSC1 */
#define PLLOP_DM    1   /* Divisor Mode,  F_in = S5L8702_OSC0 */
#define PLLOP_ALT0  2   /* Divisor Mode,  F_in = S5L8702_ALTOSC0 */
#define PLLOP_ALT1  3   /* Divisor Mode,  F_in = S5L8702_ALTOSC1 */


/* These are real clock divisor values, to be encoded into registers
 * as required. We are using fixed FClk:
 *   FClk = CG16_SYS_SEL / fdiv, fdiv >= 1
 * On Classic CG16_SYS_SEL = 216 MHz from PLL2, fdiv = 1.
 */
struct clocking_mode
{
    uint8_t cdiv;   /* CClk = FClk / cdiv, cdiv = 1,2,4,6,.. */
    uint8_t hdiv;   /* HClk = FClk / hdiv, hdiv = 1,2,4,6,.. */
    uint8_t hprat;  /* PClk = HClk / hprat, hprat >= 1 */
    uint8_t hsdiv;  /* TBC: SM1_Clk = HClk / hsdiv, hsdiv >= 1 */
};

void clocking_init(struct clocking_mode *modes, int init_level);
void set_clocking_level(int level);
unsigned get_system_freqs(unsigned *cclk, unsigned *hclk, unsigned *pclk);
void clockgate_enable(int gate, bool enable);

/* debug */
unsigned pll_get_cfg_freq(int pll);
unsigned pll_get_out_freq(int pll);
unsigned soc_get_oscsel_freq(void);
int soc_get_hsdiv(void);

#ifdef BOOTLOADER
#include <stdbool.h>

void usec_timer_init(void);

void soc_set_system_divs(unsigned cdiv, unsigned hdiv, unsigned hprat);
unsigned soc_get_system_divs(unsigned *cdiv, unsigned *hdiv, unsigned *pdiv);
void soc_set_hsdiv(int hsdiv);

void cg16_config(volatile uint16_t* cg16,
                    bool onoff, int clksel, int div1, int div2);

int pll_config(int pll, int op_mode, int p, int m, int s, int lock_time);
int pll_onoff(int pll, bool onoff);
#endif

#endif /* __CLOCKING_S5L8702_H */
