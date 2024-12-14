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
#include <stdbool.h>
#include "config.h"

#define CLOCKING_DEBUG

#if defined(IPOD_6G) || defined(IPOD_NANO3G)
#define S5L8702_OSC0_HZ         12000000  /* crystal */
#define S5L8702_OSC1_HZ         32768     /* crystal (n3g), from PMU (6g) */

#define S5L8702_ALTOSC0_HZ      0   /* TBC */
#define S5L8702_ALTOSC1_HZ      0   /* TBC */

/* this clock is selected when CG16_UNKOSC_BIT is set,
   ignoring PLLMODE_CLKSEL and CG16_SEL settings */
/* TBC: OSC0*2 ???, 24 MHz Xtal ???, USB ??? */
#define S5L8702_UNKOSC_HZ       24000000

#elif defined(IPOD_NANO4G)
#define S5L8702_OSC0_HZ         (soc_get_sec_epoch() ? 24000000 : 12000000)
#define S5L8702_OSC1_HZ         32768     /* from ??? */

// TBC:
#define S5L8702_ALTOSC0_HZ      0   /* TBC */
#define S5L8702_ALTOSC1_HZ      0   /* TBC */

// TBC:
/* this clock is selected when CG16_UNKOSC_BIT is set,
   ignoring PLLMODE_CLKSEL and CG16_SEL settings */
/* TBC: OSC0*2 ???, 24 MHz Xtal ???, USB ??? */
#define S5L8702_UNKOSC_HZ       48000000

#else
/* s5l8702 ROMBOOT */
#define S5L8702_OSC0_HZ         (soc_get_sec_epoch() ? 24000000 : 12000000)
#define S5L8702_OSC1_HZ         32768

#define S5L8702_ALTOSC0_HZ      1800000             // TODO: see if this depends on the sec_epoch
#define S5L8702_ALTOSC1_HZ      27000000

#define S5L8702_UNKOSC_HZ       (S5L8702_OSC0_HZ*2) /* TBC */
#endif

#include "s5l87xx.h"

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
#if CONFIG_CPU == S5L8720
int soc_get_sec_epoch(void);
#endif

/* debug */
unsigned pll_get_cfg_freq(int pll);
unsigned pll_get_out_freq(int pll);
unsigned soc_get_oscsel_freq(void);
int soc_get_hsdiv(void);

#ifdef BOOTLOADER
void usec_timer_init(void);

void soc_set_system_divs(unsigned cdiv, unsigned hdiv, unsigned hprat);
unsigned soc_get_system_divs(unsigned *cdiv, unsigned *hdiv, unsigned *pdiv);
void soc_set_hsdiv(int hsdiv);

#if CONFIG_CPU == S5L8702
void cg16_config(volatile uint16_t* cg16,
                    bool onoff, int clksel, int div1, int div2);
#elif CONFIG_CPU == S5L8720
void cg16_config(volatile uint16_t* cg16,
                    bool onoff, int clksel, int div1, int div2, int flags);
#endif

int pll_config(int pll, int op_mode, int p, int m, int s, int lock_time);
int pll_onoff(int pll, bool onoff);
#endif /* BOOTLOADER */

#endif /* __CLOCKING_S5L8702_H */
