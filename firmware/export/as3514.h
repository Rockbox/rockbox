/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Daniel Ankers
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

#ifndef _AS3514_H
#define _AS3514_H

#include "config.h"
#include <stdbool.h>

#if 0
#define AUDIOHW_CAPS    (LINEOUT_CAP | LIN_GAIN_CAP | MIC_GAIN_CAP)
#endif

#define AUDIOHW_CAPS    (LIN_GAIN_CAP | MIC_GAIN_CAP)

/*different volume ranges for different AMS chips*/
#ifdef HAVE_AS3543
AUDIOHW_SETTING(VOLUME,     "dB",   0,   1, -82,   6, -25)
#else /* !HAVE_AS3543 */
AUDIOHW_SETTING(VOLUME,     "dB",   0,   1, -74,   6, -25)
#endif /* HAVE_AS3543 */

#ifdef HAVE_RECORDING
/* Line in   : 0 .. 23 .. 31 =>  Volume -34.5 .. +00.0 .. +12.0 dB
 * Mic (left): 0 .. 23 .. 39 =>  Volume -34.5 .. +00.0 .. +24.0 dB
 */
AUDIOHW_SETTING(MIC_GAIN,   "dB",   1,   1,   0,  39,  23, (val - 23) * 15)
AUDIOHW_SETTING(LEFT_GAIN,  "dB",   1,   1,   0,  31,  23, (val - 23) * 15)
AUDIOHW_SETTING(RIGHT_GAIN, "dB",   1,   1,   0,  31,  23, (val - 23) * 15)
#endif /* HAVE_RECORDING */

void audiohw_set_sampr_dividers(int fsel);

#if defined(SANSA_FUZEV2)
extern void audiohw_enable_lineout(bool enable);
#endif

/* Register Descriptions */

#ifndef HAVE_AS3543
#define AS3514_LINE_OUT_R 0x00
#define AS3514_LINE_OUT_L 0x01
#endif

#define AS3514_HPH_OUT_R  0x02
#define AS3514_HPH_OUT_L  0x03

#ifndef HAVE_AS3543
#define AS3514_LSP_OUT_R  0x04
#define AS3514_LSP_OUT_L  0x05
#endif

#define AS3514_MIC1_R     0x06
#define AS3514_MIC1_L     0x07

#ifndef HAVE_AS3543
#define AS3514_MIC2_R     0x08
#define AS3514_MIC2_L     0x09
#endif

#define AS3514_LINE_IN1_R 0x0a
#define AS3514_LINE_IN1_L 0x0b

#ifndef HAVE_AS3543
#define AS3514_LINE_IN2_R 0x0c
#define AS3514_LINE_IN2_L 0x0d
#endif

#define AS3514_DAC_R      0x0e
#define AS3514_DAC_L      0x0f
#define AS3514_ADC_R      0x10
#define AS3514_ADC_L      0x11

#ifdef HAVE_AS3543
#define AS3543_DAC_IF     0x12
#endif

#define AS3514_AUDIOSET1  0x14
#define AS3514_AUDIOSET2  0x15
#define AS3514_AUDIOSET3  0x16
#define AS3515_USB_UTIL   0x17  /* only in as3515 */

#ifdef HAVE_AS3543
#define AS3543_BACKLIGHT  0x1b
#define AS3543_PMU_ENABLE 0x1c
#endif

#ifndef HAVE_AS3543
#define AS3514_PLLMODE    0x1d
#endif

#ifdef HAVE_AS3543
#define AS3543_CHARGER    0x19  /* PMU: 2 sub registers */
#endif

#define AS3514_SYSTEM     0x20

#ifndef HAVE_AS3543
#define AS3514_CVDD_DCDC3 0x21
#define AS3514_CHARGER    0x22
#define AS3514_DCDC15     0x23
#define AS3514_SUPERVISOR 0x24
#endif

#ifdef HAVE_AS3543
#define AS3543_WAKEUP     0x22
#endif

/* AS3543 has 2 IRQ_ENRD registers at 0x23 and 0x24, but we don't use them
 * We call the real IRQ_ENRD2 register, IRQ_ENRD0, to stay compatible with
 * as3514, because the bits we use are the same
 */
#define AS3514_IRQ_ENRD0  0x25
#define AS3514_IRQ_ENRD1  0x26
#define AS3514_IRQ_ENRD2  0x27

#define AS3514_RTCV       0x28
#define AS3514_RTCT       0x29
#define AS3514_RTC_0      0x2a
#define AS3514_RTC_1      0x2b
#define AS3514_RTC_2      0x2c
#define AS3514_RTC_3      0x2d
#define AS3514_ADC_0      0x2e
#define AS3514_ADC_1      0x2f

#ifdef HAVE_AS3543
#define AS3514_UID_0      0x38
#define AS3514_UID_LEN    8
#else
#define AS3514_UID_0      0x30
#define AS3514_UID_LEN    16
#endif

/*** Audio Registers ***/

/* 00h (LINE_OUT_R) to 16h (AUDIOSET3) */
#define AS3514_NUM_AUDIO_REGS   (0x17)

/* Common registers masks */
#define AS3514_VOL_MASK         (0x1f << 0)

/* LINE_OUT_R (0x00) */
/* Only has volume control bits */

/* LINE_OUT_L (0x01) */
#define LINE_OUT_L_LO_SES_DM    (0x3 << 6)
    #define LINE_OUT_L_LO_SES_DM_MUTE   (0x0 << 6)
    #define LINE_OUT_L_LO_SES_DM_DF_MO  (0x1 << 6)
    #define LINE_OUT_L_LO_SES_DM_SE_ST  (0x2 << 6)
/* Use AS3514_VOL_MASK */

/* HPH_OUT_R (0x02) */
#define HPH_OUT_R_HP_OVC_TO     (0x3 << 6)
    /* AS3514/AS3515 */
    #define HPH_OUT_R_HP_OVC_TO_0MS     (0x3 << 6)
    #define HPH_OUT_R_HP_OVC_TO_128MS   (0x1 << 6)
    #define HPH_OUT_R_HP_OVC_TO_256MS   (0x0 << 6)
    #define HPH_OUT_R_HP_OVC_TO_512MS   (0x2 << 6)
    /* AS3543 */
    #define HPH_OUT_R_LINEOUT           (0x1 << 7)
    #define HPH_OUT_R_HEADPHONES        (0x0 << 7)
    #define HPH_OUT_R_HP_OUT_SUM        (0x0 << 5)
    #define HPH_OUT_R_HP_OUT_DAC        (0x1 << 5)
    #define HPH_OUT_R_HP_OUT_LINE       (0x2 << 5)
    #define HPH_OUT_R_HP_OUT_MIC        (0x3 << 5)
    #define HPH_OUT_R_HP_OUT_MASK       (0x3 << 5)
/* Use AS3514_VOL_MASK */

/* HPH_OUT_L (0x03) */
#define HPH_OUT_L_HP_MUTE       (0x1 << 7)
#define HPH_OUT_L_HP_ON         (0x1 << 6)
#define HPH_OUT_L_HP_DET_ON     (0x1 << 5)
/* Use AS3514_VOL_MASK */

/* LSP_OUT_R (0x04) */
#define LSP_OUT_R_SP_OVC_TO     (0x3 << 6)
    #define LSP_OUT_R_SP_OVC_TO_256MS (0x0 << 6)
    #define LSP_OUT_R_SP_OVC_TO_128MS (0x1 << 6)
    #define LSP_OUT_R_SP_OVC_TO_512MS (0x2 << 6)
    #define LSP_OUT_R_SP_OVC_TO_0MS   (0x3 << 6)
/* Use AS3514_VOL_MASK */

/* LSP_OUT_L (0x05) */
#define LSP_OUT_L_SP_MUTE       (0x1 << 7)
#define LSP_OUT_L_SP_ON         (0x1 << 6)
/* Use AS3514_VOL_MASK */

/* MIC1_R (0x06) */
#define MIC1_R_M1_AGC_off       (0x1 << 7)
#define MIC1_R_M1_GAIN          (0x3 << 5)
    #define MIC1_R_M1_GAIN_28DB (0x0 << 5)
    #define MIC1_R_M1_GAIN_34DB (0x1 << 5)
    #define MIC1_R_M1_GAIN_40DB (0x2 << 5)
/* Use AS3514_VOL_MASK */

/* MIC1_L (0x07) */
#define MIC1_L_M1_SUP_off       (0x1 << 7)
#define MIC1_L_M1_MUTE_off      (0x1 << 6)
/* Use AS3514_VOL_MASK */

/* MIC2_R (0x08) */
#define MIC2_R_M2_AGC_off       (0x1 << 7)
#define MIC2_R_M2_GAIN          (0x3 << 5)
    #define MIC2_R_M2_GAIN_28DB (0x0 << 5)
    #define MIC2_R_M2_GAIN_34DB (0x1 << 5)
    #define MIC2_R_M2_GAIN_40DB (0x2 << 5)
/* Use AS3514_VOL_MASK */

/* MIC2_L (0x09) */
#define MIC2_L_M2_SUP_off       (0x1 << 7)
#define MIC2_L_M2_MUTE_off      (0x1 << 6)
/* Use AS3514_VOL_MASK */

/* LINE_IN1_R (0Ah) */
#ifdef HAVE_AS3543
#define LINE_IN_R_LINE_SELECT       (0x1 << 6)
#endif
#define LINE_IN1_R_LI1R_MUTE_off    (0x1 << 5)
/* Use AS3514_VOL_MASK */

/* LINE_IN1_L (0Bh) */
#define LINE_IN1_L_LI1_MODE      (0x3 << 6)
    #define LINE_IN1_L_LI1_MODE_SE_ST   (0x0 << 6)
    #define LINE_IN1_L_LI1_MODE_DF_MO   (0x1 << 6)
    #define LINE_IN1_L_LI1_MODE_SE_MO   (0x2 << 6)
#define LINE_IN1_L_LI1L_MUTE_off (0x1 << 5)
/* Use AS3514_VOL_MASK */

/* LINE_IN2_R (0Ch) */
#define LINE_IN2_R_LI2R_MUTE_off (0x1 << 5)
/* Use AS3514_VOL_MASK */

/* LINE_IN2_L (0Dh) */
#define LINE_IN2_L_LI2_MODE      (0x3 << 6)
    #define LINE_IN2_L_LI2_MODE_SE_ST   (0x0 << 6)
    #define LINE_IN2_L_LI2_MODE_DF_MO   (0x1 << 6)
    #define LINE_IN2_L_LI2_MODE_SE_MO   (0x2 << 6)
#define LINE_IN2_L_LI2L_MUTE_off (0x1 << 5)
/* Use AS3514_VOL_MASK */

/* DAC_R (0Eh) */
/* Only has volume control bits */
/* Use AS3514_VOL_MASK */

/* DAC_L (0Fh) */
#ifdef HAVE_AS3543
#define DAC_L_DAC_MUTE_off      (0x1 << 5)
#else
#define DAC_L_DAC_MUTE_off      (0x1 << 6)
#endif
/* Use AS3514_VOL_MASK */

/* ADC_R (10h) */
#define ADC_R_ADCMUX            (0x3 << 6)
    #define ADC_R_ADCMUX_ST_MIC     (0x0 << 6)
    #define ADC_R_ADCMUX_LINE_IN1   (0x1 << 6)
    #define ADC_R_ADCMUX_LINE_IN2   (0x2 << 6)
    #define ADC_R_ADCMUX_AUDIO_SUM  (0x3 << 6)
/* Use AS3514_VOL_MASK */

/* ADC_L (11h) */
#ifdef HAVE_AS3543
#define ADC_L_ADC_MUTE_off      (0x1 << 5)
#else
#define ADC_L_FS_2              (0x1 << 7)
#define ADC_L_ADC_MUTE_off      (0x1 << 6)
#endif
/* Use AS3514_VOL_MASK */

/* DAC_IF (11h) */
#ifdef HAVE_AS3543
#define AS3543_DAC_INT_PLL      (0x0 << 7)
#define AS3543_DAC_EXT_MCLK     (0x1 << 7)
#endif

/* AUDIOSET1 (14h)*/
#ifdef HAVE_AS3543
#define AUDIOSET1_DAC_on        (0x1 << 6)
#define AUDIOSET1_DAC_GAIN_on   (0x1 << 5)
#else
#define AUDIOSET1_SUM_on        (0x1 << 6)
#define AUDIOSET1_DAC_on        (0x1 << 5)
#define AUDIOSET1_LOUT_on       (0x1 << 4)
#endif /* HAVE_AS3543 */
/* common bits */
#define AUDIOSET1_ADC_on        (0x1 << 7)
#define AUDIOSET1_LIN2_on       (0x1 << 3)
#define AUDIOSET1_LIN1_on       (0x1 << 2)
#define AUDIOSET1_MIC2_on       (0x1 << 1)
#define AUDIOSET1_MIC1_on       (0x1 << 0)

#define AUDIOSET1_INPUT_MASK    AUDIOSET1_MIC1_on | AUDIOSET1_MIC2_on | \
                                AUDIOSET1_LIN1_on | AUDIOSET1_LIN2_on


/* AUDIOSET2 (15h) */
#ifdef HAVE_AS3543
#define AUDIOSET2_BIAS_on      (0x0 << 7)
#define AUDIOSET2_BIAS_off      (0x1 << 7)
#define AUDIOSET2_SUM_off      (0x1 << 6)
#define AUDIOSET2_AGC_off       (0x1 << 5)
#define AUDIOSET2_HPH_QUALITY_LOW_POWER (0x0 << 4)
#define AUDIOSET2_HPH_QUALITY_HIGH      (0x1 << 4)
#else
#define AUDIOSET2_BIAS_off      (0x1 << 7)
#define AUDIOSET2_DITH_off      (0x1 << 6)
#define AUDIOSET2_AGC_off       (0x1 << 5)
#define AUDIOSET2_IBR_DAC       (0x3 << 3)
    #define AUDIOSET2_IBR_DAC_0     (0x0 << 3)
    #define AUDIOSET2_IBR_DAC_25    (0x1 << 3)
    #define AUDIOSET2_IBR_DAC_40    (0x2 << 3)
    #define AUDIOSET2_IBR_DAC_50    (0x3 << 3)
#define AUDIOSET2_LSP_LP        (0x1 << 2)
#define AUDIOSET2_IBR_LSP       (0x3 << 0)
    #define AUDIOSET2_IBR_LSP_0     (0x0 << 0)
    #define AUDIOSET2_IBR_LSP_17    (0x1 << 0)
    #define AUDIOSET2_IBR_LSP_34    (0x2 << 0)
    #define AUDIOSET2_IBR_LSP_50    (0x3 << 0)
#endif

/* AUDIOSET3 (16h) */
#ifdef HAVE_AS3543
#define AUDIOSET3_HP_FASTSTART  (0x1 << 2)
#define AUDIOSET3_HP_LONGSTART  (0x0 << 2)
#define AUDIOSET3_HP_BIAS_150   (0x1 << 1)
#define AUDIOSET3_HPCM_on       (0x1 << 0)
#define AUDIOSET3_HPCM_off      (0x0 << 0)
#else
#define AUDIOSET3_ZCU_off       (0x1 << 2)
#define AUDIOSET3_IBR_HPH       (0x1 << 1)
#define AUDIOSET3_HPCM_off      (0x1 << 0)
#endif

/* PLLMODE (1Dh) */
#define PLLMODE_LRCK             (0x3 << 1)
    #define PLLMODE_LRCK_24_48      (0x0 << 1)
    #define PLLMODE_LRCK_8_23       (0x2 << 1)

/* ADC channels */
#if CONFIG_CPU == AS3525v2
#define NUM_ADC_CHANNELS 16
#else
#define NUM_ADC_CHANNELS 13
#endif

#define ADC_BVDD         0  /* Battery voltage of 4V LiIo accumulator */
#define ADC_RTCSUP       1  /* RTC backup battery voltage */
#define ADC_UVDD         2  /* USB host voltage */
#define ADC_CHG_IN       3  /* Charger input voltage */
#define ADC_CVDD         4  /* Charger pump output voltage */
#define ADC_BATTEMP      5  /* Battery charging temperature */
#define ADC_MICSUP1      6  /* Voltage on MicSup1 for remote control
                               or external voltage measurement */
#define ADC_MICSUP2      7  /* Voltage on MicSup1 for remote control
                               or external voltage measurement */
#define ADC_VBE1         8  /* Measuring junction temperature @ 2uA */
#define ADC_VBE2         9  /* Measuring junction temperature @ 1uA */
#define ADC_I_MICSUP1    10 /* Current of MicSup1 for remote control detection */
#define ADC_I_MICSUP2    11 /* Current of MicSup2 for remote control detection */
#define ADC_VBAT         12 /* Single cell battery voltage */

/* AS3514_CHARGER */
#define TMPSUP_OFF      (0x1 << 7)
#define CHG_I           (0x7 << 4)
#ifdef HAVE_AS3543  /* AS3543 uses charge current steps of 70 mA */
#define CHG_I_55MA      (0x0 << 4)
#define CHG_I_70MA      (0x1 << 4)
#define CHG_I_140MA     (0x2 << 4)
#define CHG_I_210MA     (0x3 << 4)
#define CHG_I_280MA     (0x4 << 4)
#define CHG_I_350MA     (0x5 << 4)
#define CHG_I_420MA     (0x6 << 4)
#define CHG_I_460MA     (0x7 << 4)
#else               /* AS3514 uses charge current steps of 50 mA */
#define CHG_I_50MA      (0x0 << 4)
#define CHG_I_100MA     (0x1 << 4)
#define CHG_I_150MA     (0x2 << 4)
#define CHG_I_200MA     (0x3 << 4)
#define CHG_I_250MA     (0x4 << 4)
#define CHG_I_300MA     (0x5 << 4)
#define CHG_I_350MA     (0x6 << 4)
#define CHG_I_400MA     (0x7 << 4)
#endif
#define CHG_V           (0x7 << 1)
#define CHG_V_4_25V     (0x7 << 1)
#define CHG_V_4_20V     (0x6 << 1)
#define CHG_V_4_15V     (0x5 << 1)
#define CHG_V_4_10V     (0x4 << 1)
#define CHG_V_4_05V     (0x3 << 1)
#define CHG_V_4_00V     (0x2 << 1)
#define CHG_V_3_95V     (0x1 << 1)
#define CHG_V_3_90V     (0x0 << 1)
#define CHG_OFF         (0x1 << 0)

/* AS3514_IRQ_ENRD0 */
#define CHG_TMPHIGH     (0x1 << 7)
#define CHG_ENDOFCH     (0x1 << 6)
#define CHG_STATUS      (0x1 << 5)
#define CHG_CHANGED     (0x1 << 4)
#define USB_STATUS      (0x1 << 3)
#define USB_CHANGED     (0x1 << 2)
#define RVDD_WASLOW     (0x1 << 1)
#define BVDD_ISLOW      (0x1 << 0)

#define IRQ_ENDOFCH     (0x1 << 6)
#define IRQ_CHGSTAT     (0x1 << 4)
#define IRQ_USBSTAT     (0x1 << 2)

/* AS3514_IRQ_ENRD2 */
#define DEBOUNCE_128MS  (0x2 << 6)
#define DEBOUNCE_256MS  (0x1 << 6)
#define DEBOUNCE_512MS  (0x0 << 6)
#define IRQ_HIGHACTIVE  (0x1 << 5)
#define IRQ_PUSHPULL    (0x1 << 4)
#define IRQ_REMDET2     (0x1 << 3)
#define IRQ_REMDET1     (0x1 << 2)
#define IRQ_RTC         (0x1 << 1)
#define IRQ_ADC         (0x1 << 0)

#define AS3514_I2C_ADDR 0x46

#endif /* _AS3514_H */
