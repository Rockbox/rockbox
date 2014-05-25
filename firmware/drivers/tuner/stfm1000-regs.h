/*
 * Copyright 2008-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2016 Amaury Pouly
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef STFM1000_REGS_H
#define STFM1000_REGS_H

/* NOTE: this header only contains useful fields, a more complete documentation
 * of the STFM1000 registers is available in utils/regtools/desc/regs-stfm1000.xml */

/* registers */
#define REG_TUNE1              0x00
#define REG_SDNOMINAL          0x04
#define REG_PILOTTRACKING      0x08
#define REG_INITIALIZATION1    0x10
#define REG_INITIALIZATION2    0x14
#define REG_INITIALIZATION3    0x18
#define REG_INITIALIZATION4    0x1C
#define REG_INITIALIZATION5    0x20
#define REG_INITIALIZATION6    0x24
#define REG_REF                0x28
#define REG_LNA                0x2C
#define REG_MIXFILT            0x30
#define REG_CLK1               0x34
#define REG_CLK2               0x38
#define REG_ADC                0x3C
#define REG_AGC_CONTROL1       0x44
#define REG_AGC_CONTROL2       0x48
#define REG_DATAPATH           0x5C
#define REG_RMS                0x60
#define REG_AGC_STAT           0x64
#define REG_SIGNALQUALITY      0x68
#define REG_DCEST              0x6C
#define REG_RSSI_TONE          0x70
#define REG_PILOTCORRECTION    0x74
#define REG_ATTENTION          0x78
#define REG_CLK3               0x7C
#define REG_CHIPID             0x80

/* number of registers */
#define STFM1000_NUM_REGS           ((0x80 + 4) / 4)

/* frequency range: the tuning is stored in a table in stfm1000-tunetable.c */
#define FREQUENCY_100KHZ_MIN     758
#define FREQUENCY_100KHZ_RANGE   325
#define FREQUENCY_100KHZ_MAX     (FREQUENCY_100KHZ_MIN + FREQUENCY_100KHZ_RANGE)

/* TUNE1 */
#define TUNE1_B2_MIX_MASK           0x001C0000
#define TUNE1_B2_MIX_SHIFT          18
#define TUNE1_CICOSR_MASK           0x00007E00
#define TUNE1_CICOSR_SHIFT          9
#define TUNE1_PLL_DIV_MASK          0x000001FF
#define TUNE1_PLL_DIV_SHIFT         0

/* CHIPID */
#define CHIP_REV_TA1       0x00000001
#define CHIP_REV_TA2       0x00000002
#define CHIP_REV_TB1       0x00000011
#define CHIP_REV_TB2       0x00000012

/* DATAPATH bits we use */
#define DP_EN              0x01000000
#define DB_ACCEPT          0x00010000
#define SAI_CLK_DIV_MASK   0x7c
#define SAI_CLK_DIV_SHIFT  2
#define SAI_CLK_DIV(x) \
      (((x) << SAI_CLK_DIV_SHIFT) & SAI_CLK_DIV_MASK)
#define SAI_EN             0x00000001

/* AGC_CONTROL1 bits we use */
#define B2_BYPASS_AGC_CTL  0x00004000
#define B2_BYPASS_FILT_MASK      0x0000000C
#define B2_BYPASS_FILT_SHIFT     2
#define B2_BYPASS_FILT(x) \
      (((x) << B2_BYPASS_FILT_SHIFT) & B2_BYPASS_FILT_MASK)
#define B2_LNATH_MASK            0x001F0000
#define B2_LNATH_SHIFT           16
#define B2_LNATH(x) \
      (((x) << B2_LNATH_SHIFT) & B2_LNATH_MASK)

/* AGC_STAT bits we use */
#define AGCOUT_STAT_MASK   0x1F000000
#define AGCOUT_STAT_SHIFT  24
#define LNA_RMS_MASK       0x00001F00
#define LNA_RMS_SHIFT            8

/* PILOTTRACKING bits we use */
#define B2_PILOTTRACKING_EN            0x00008000
#define B2_PILOTLPF_TIMECONSTANT_MASK  0x00000f00
#define B2_PILOTLPF_TIMECONSTANT_SHIFT 8
#define B2_PILOTLPF_TIMECONSTANT(x)    \
      (((x) << B2_PILOTLPF_TIMECONSTANT_SHIFT) & \
            B2_PILOTLPF_TIMECONSTANT_MASK)
#define B2_PFDSCALE_MASK         0x000000f0
#define B2_PFDSCALE_SHIFT        4
#define B2_PFDSCALE(x)     \
      (((x) << B2_PFDSCALE_SHIFT) & B2_PFDSCALE_MASK)
#define B2_PFDFILTER_SPEEDUP_MASK      0x0000000f
#define B2_PFDFILTER_SPEEDUP_SHIFT     0
#define B2_PFDFILTER_SPEEDUP(x)  \
      (((x) << B2_PFDFILTER_SPEEDUP_SHIFT) & \
            B2_PFDFILTER_SPEEDUP_MASK)

/* PILOTCORRECTION bits we use */
#define A2_PILOTEST_MASK    0xff000000
#define A2_PILOTEST_SHIFT   24
#define B2_PILOTEST_MASK    0xfe000000
#define B2_PILOTEST_SHIFT   25

/* INITIALIZATION1 bits we use */
#define B2_BYPASS_FILT_MASK      0x0000000C
#define B2_BYPASS_FILT_SHIFT     2
#define B2_BYPASS_FILT(x)  \
      (((x) << B2_BYPASS_FILT_SHIFT) & B2_BYPASS_FILT_MASK)

/* INITIALIZATION2 bits we use */
#define DRI_CLK_EN         0x80000000
#define DEEMPH_50_75B            0x00000100
#define RDS_ENABLE         0x00100000
#define RDS_MIXOFFSET            0x00200000

/* INITIALIZATION3 bits we use */
#define B2_NEAR_CHAN_MIX_MASK    0x1c000000
#define B2_NEAR_CHAN_MIX_SHIFT   26
#define B2_NEAR_CHAN_MIX(x)      \
      (((x) << B2_NEAR_CHAN_MIX_SHIFT) & \
            B2_NEAR_CHAN_MIX_MASK)

/* CLK1 bits we use */
#define ENABLE_TAPDELAYFIX 0x00000020

/* REF bits we use */
#define LNA_AMP1_IMPROVE_DISTORTION 0x08000000

/* LNA bits we use */
#define AMP2_IMPROVE_DISTORTION 0x08000000
#define ANTENNA_TUNECAP_MASK     0x001F0000
#define ANTENNA_TUNECAP_SHIFT    16
#define ANTENNA_TUNECAP(x) \
      (((x) << ANTENNA_TUNECAP_SHIFT) & \
            ANTENNA_TUNECAP_MASK)
#define IBIAS2_UP          0x00000008
#define IBIAS2_DN          0x00000004
#define IBIAS1_UP          0x00000002
#define IBIAS1_DN          0x00000001
#define USEATTEN_MASK            0x00600000
#define USEATTEN_SHIFT           21
#define USEATTEN(x)  \
      (((x) << USEATTEN_SHIFT) & USEATTEN_MASK)

/* SIGNALQUALITY bits we use */
#define NEAR_CHAN_AMPLITUDE_MASK 0x0000007F
#define NEAR_CHAN_AMPLITUDE_SHIFT      0
#define NEAR_CHAN_AMPLITUDE(x)   \
      (((x) << NEAR_CHAN_AMPLITUDE_SHIFT) & \
            NEAR_CHAN_AMPLITUDE_MASK)

/* precalc tables elements */
struct stfm1000_tune1 {
      unsigned int tune1;     /* at least 32 bit */
      unsigned int sdnom;
};

#endif /* STFM1000_REGS_H */
