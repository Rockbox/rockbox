/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Michael Sevakis
 *
 * Header file for WM8978 codec
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
#ifndef _WM8978_H
#define _WM8978_H

#define VOLUME_MIN -900
#define VOLUME_MAX 60

int tenthdb2master(int db);
void audiohw_set_headphone_vol(int vol_l, int vol_r);
void audiohw_set_recsrc(int source, bool recording);

void wmc_set(unsigned int reg, unsigned int bits);
void wmc_clear(unsigned int reg, unsigned int bits);

#define WMC_I2C_ADDR 0x34

/* Registers */
#define WMC_SOFTWARE_RESET              0x00
#define WMC_POWER_MANAGEMENT1           0x01
#define WMC_POWER_MANAGEMENT2           0x02
#define WMC_POWER_MANAGEMENT3           0x03
#define WMC_AUDIO_INTERFACE             0x04
#define WMC_COMPANDING_CTRL             0x05
#define WMC_CLOCK_GEN_CTRL              0x06
#define WMC_ADDITIONAL_CTRL             0x07
#define WMC_GPIO                        0x08
#define WMC_JACK_DETECT_CONTROL1        0x09
#define WMC_DAC_CONTROL                 0x0a
#define WMC_LEFT_DAC_DIGITAL_VOL        0x0b
#define WMC_RIGHT_DAC_DIGITAL_VOL       0x0c
#define WMC_JACK_DETECT_CONTROL2        0x0d
#define WMC_ADC_CONTROL                 0x0e
#define WMC_LEFT_ADC_DIGITAL_VOL        0x0f
#define WMC_RIGHT_ADC_DIGITAL_VOL       0x10
#define WMC_EQ1_LOW_SHELF               0x12
#define WMC_EQ2_PEAK1                   0x13
#define WMC_EQ3_PEAK2                   0x14
#define WMC_EQ4_PEAK3                   0x15
#define WMC_EQ5_HIGH_SHELF              0x16
#define WMC_DAC_LIMITER1                0x18
#define WMC_DAC_LIMITER2                0x19
#define WMC_NOTCH_FILTER1               0x1b
#define WMC_NOTCH_FILTER2               0x1c
#define WMC_NOTCH_FILTER3               0x1d
#define WMC_NOTCH_FILTER4               0x1e
#define WMC_ALC_CONTROL1                0x20
#define WMC_ALC_CONTROL2                0x21
#define WMC_ALC_CONTROL3                0x22
#define WMC_NOISE_GATE                  0x23
#define WMC_PLL_N                       0x24
#define WMC_PLL_K1                      0x25
#define WMC_PLL_K2                      0x26
#define WMC_PLL_K3                      0x27
#define WMC_3D_CONTROL                  0x29
#define WMC_BEEP_CONTROL                0x2b
#define WMC_INPUT_CTRL                  0x2c
#define WMC_LEFT_INP_PGA_GAIN_CTRL      0x2d
#define WMC_RIGHT_INP_PGA_GAIN_CTRL     0x2e
#define WMC_LEFT_ADC_BOOST_CTRL         0x2f
#define WMC_RIGHT_ADC_BOOST_CTRL        0x30
#define WMC_OUTPUT_CTRL                 0x31
#define WMC_LEFT_MIXER_CTRL             0x32
#define WMC_RIGHT_MIXER_CTRL            0x33
#define WMC_LOUT1_HP_VOLUME_CTRL        0x34
#define WMC_ROUT1_HP_VOLUME_CTRL        0x35
#define WMC_LOUT2_SPK_VOLUME_CTRL       0x36
#define WMC_ROUT2_SPK_VOLUME_CTRL       0x37
#define WMC_OUT3_MIXER_CTRL             0x38
#define WMC_OUT4_MONO_MIXER_CTRL        0x39
#define WMC_NUM_REGISTERS               0x3a

/* Register bitmasks */

/* Volume update bit for volume registers */
#define WMC_VU                          (1 << 8)

/* Zero-crossing bit for volume registers */
#define WMC_ZC                          (1 << 7)

/* Mute bit for volume registers */
#define WMC_MUTE                        (1 << 6)

/* Volume masks and macros for digital volumes */
#define WMC_DVOL                        0xff
#define WMC_DVOLr(x)                    ((x) & WMC_DVOL)
#define WMC_DVOLw(x)                    ((x) & WMC_DVOL)

/* Volums masks and macros for analogue volumes */
#define WMC_AVOL                        0x3f
#define WMC_AVOLr(x)                    ((x) & WMC_AVOL)
#define WMC_AVOLw(x)                    ((x) & WMC_AVOL)

/* WMC_SOFTWARE_RESET (0x00) */
#define WMC_RESET
    /* Write any value */

/* WMC_POWER_MANAGEMENT1 (0x01) */
#define WMC_BUFDCOMPEN                  (1 << 8)
#define WMC_OUT4MIXEN                   (1 << 7)
#define WMC_OUT3MIXEN                   (1 << 6)
#define WMC_PLLEN                       (1 << 5)
#define WMC_MICBEN                      (1 << 4)
#define WMC_BIASEN                      (1 << 3)
#define WMC_BUFIOEN                     (1 << 2)
#define WMC_VMIDSEL                     (3 << 0)
    #define WMC_VMIDSEL_OFF             (0 << 0)
    #define WMC_VMIDSEL_75K             (1 << 0)
    #define WMC_VMIDSEL_300K            (2 << 0)
    #define WMC_VMIDSEL_5K              (3 << 0)

/* WMC_POWER_MANAGEMENT2 (0x02) */
#define WMC_ROUT1EN                     (1 << 8)
#define WMC_LOUT1EN                     (1 << 7)
#define WMC_SLEEP                       (1 << 6)
#define WMC_BOOSTENR                    (1 << 5)
#define WMC_BOOSTENL                    (1 << 4)
#define WMC_INPPGAENR                   (1 << 3)
#define WMC_INPPGAENL                   (1 << 2)
#define WMC_ADCENR                      (1 << 1)
#define WMC_ADCENL                      (1 << 0)

/* WMC_POWER_MANAGEMENT3 (0x03) */
#define WMC_OUT4EN                      (1 << 8)
#define WMC_OUT3EN                      (1 << 7)
#define WMC_LOUT2EN                     (1 << 6)
#define WMC_ROUT2EN                     (1 << 5)
#define WMC_RMIXEN                      (1 << 3)
#define WMC_LMIXEN                      (1 << 2)
#define WMC_DACENR                      (1 << 1)
#define WMC_DACENL                      (1 << 0)

/* WMC_AUDIO_INTERFACE (0x04) */
#define WMC_BCP                         (1 << 8)
#define WMC_LRP                         (1 << 7)
#define WMC_WL                          (3 << 5)
    #define WMC_WL_16                   (0 << 5)
    #define WMC_WL_20                   (1 << 5)
    #define WMC_WL_24                   (2 << 5)
    #define WMC_WL_32                   (3 << 5)
#define WMC_FMT                         (3 << 3)
    #define WMC_FMT_RJUST               (0 << 3)
    #define WMC_FMT_LJUST               (1 << 3)
    #define WMC_FMT_I2S                 (2 << 3)
    #define WMC_FMT_DSP_PCM             (3 << 3)
#define WMC_DACLRSWAP                   (1 << 2)
#define WMC_ADCLRSWAP                   (1 << 1)
#define WMC_MONO                        (1 << 0)

/* WMC_COMPANDING_CTRL (0x05) */
#define WMC_WL8                         (1 << 5)
#define WMC_DAC_COMP                    (3 << 3)
    #define WMC_DAC_COMP_OFF            (0 << 3)
    #define WMC_DAC_COMP_U_LAW          (2 << 3)
    #define WMC_DAC_COMP_A_LAW          (3 << 3)
#define WMC_ADC_COMP                    (3 << 1)
    #define WMC_ADC_COMP_OFF            (0 << 1)
    #define WMC_ADC_COMP_U_LAW          (2 << 1)
    #define WMC_ADC_COMP_A_LAW          (3 << 1)
#define WMC_LOOPBACK                    (1 << 0)

/* WMC_CLOCK_GEN_CTRL (0x06) */
#define WMC_CLKSEL                      (1 << 8)
#define WMC_MCLKDIV                     (7 << 5)
    #define WMC_MCLKDIV_1               (0 << 5)
    #define WMC_MCLKDIV_1_5             (1 << 5)
    #define WMC_MCLKDIV_2               (2 << 5)
    #define WMC_MCLKDIV_3               (3 << 5)
    #define WMC_MCLKDIV_4               (4 << 5)
    #define WMC_MCLKDIV_6               (5 << 5)
    #define WMC_MCLKDIV_8               (6 << 5)
    #define WMC_MCLKDIV_12              (7 << 5)
#define WMC_BCLKDIV                     (7 << 2)
    #define WMC_BCLKDIV_1               (0 << 2)
    #define WMC_BCLKDIV_2               (1 << 2)
    #define WMC_BCLKDIV_4               (2 << 2)
    #define WMC_BCLKDIV_8               (3 << 2)
    #define WMC_BCLKDIV_16              (4 << 2)
    #define WMC_BCLKDIV_32              (5 << 2)
#define WMC_MS                          (1 << 0)

/* WMC_ADDITIONAL_CTRL (0x07) */
/* This configure the digital filter coefficients - pick the closest
 * to what's really being used (greater than or equal). */
#define WMC_SR                          (7 << 1)
#define WMC_SR_48KHZ                    (0 << 1)
#define WMC_SR_32KHZ                    (1 << 1)
#define WMC_SR_24KHZ                    (2 << 1)
#define WMC_SR_16KHZ                    (3 << 1)
#define WMC_SR_12KHZ                    (4 << 1)
#define WMC_SR_8KHZ                     (5 << 1)
/* 110-111=reserved */
#define WMC_SLOWCLKEN                   (1 << 0)

/* WMC_GPIO (0x08) */
#define WMC_OPCLKDIV                    (3 << 4)
    #define WMC_OPCLKDIV_1              (0 << 4)
    #define WMC_OPCLKDIV_2              (1 << 4)
    #define WMC_OPCLKDIV_3              (2 << 4)
    #define WMC_OPCLKDIV_4              (3 << 4)
#define WMC_GPIO1POL                    (1 << 3)
#define WMC_GPIO1SEL                    (7 << 0)
    #define WMC_GPIO1SEL_TEMP_OK        (2 << 0)
    #define WMC_GPIO1SEL_AMUTE_ACTIVE   (3 << 0)
    #define WMC_GPIO1SEL_PLL_CLK_OP     (4 << 0)
    #define WMC_GPIO1SEL_PLL_LOCK       (5 << 0)
    #define WMC_GPIO1SEL_LOGIC_1        (6 << 0)
    #define WMC_GPIO1SEL_LOGIC_0        (7 << 0)

/* WMC_JACK_DETECT_CONTROL1 (0x09) */
#define WMC_JD_VMID                     (3 << 7)
    #define WMC_JD_VMID_EN_0            (1 << 7)
    #define WMC_JD_VMID_EN_1            (2 << 7)
#define WMC_JD_EN                       (1 << 6)
#define WMC_JD_SEL                      (3 << 4)
    #define WMC_JD_SEL_GPIO1            (0 << 4)
    #define WMC_JD_SEL_GPIO2            (1 << 4)
    #define WMC_JD_SEL_GPIO3            (2 << 4)

/* WMC_DAC_CONTROL (0x0a) */
#define WMC_SOFT_MUTE                   (1 << 6)
#define WMC_DACOSR_128                  (1 << 3)
#define WMC_AMUTE                       (1 << 2)
#define WMC_DACPOLR                     (1 << 1)
#define WMC_DACPOLL                     (1 << 0)

/* WMC_LEFT_DAC_DIGITAL_VOL (0x0b) */
/* WMC_RIGHT_DAC_DIGITAL_VOL (0x0c) */
    /* 00000000=mute, 00000001=-127dB...(0.5dB steps)...11111111=0dB */
    /* Use WMC_DVOL* macros */

/* WMC_JACK_DETECT_CONTROL2 (0x0d) */
#define WMC_JD_EN1                      (0xf << 4)
    #define WMC_OUT1_EN1                (1 << 4)
    #define WMC_OUT2_EN1                (2 << 4)
    #define WMC_OUT3_EN1                (4 << 4)
    #define WMC_OUT4_EN1                (8 << 4)
#define WMC_JD_EN0                      (0xf << 0)
    #define WMC_OUT1_EN0                (1 << 0)
    #define WMC_OUT2_EN0                (2 << 0)
    #define WMC_OUT3_EN0                (4 << 0)
    #define WMC_OUT4_EN0                (8 << 0)

/* WMC_ADC_CONTROL (0x0e) */
#define WMC_HPFEN                       (1 << 8)
#define WMC_HPFAPP                      (1 << 7)
#define WMC_HPFCUT                      (7 << 4)
#define WMC_ADCOSR                      (1 << 3)
#define WMC_ADCRPOL                     (1 << 1)
#define WMC_ADCLPOL                     (1 << 0)

/* WMC_LEFT_ADC_DIGITAL_VOL (0x0f) */
/* WMC_RIGHT_ADC_DITIGAL_VOL (0x10) */
    /* 0.5dB steps: Mute:0x00, -127dB:0x01...0dB:0xff */
    /*Use WMC_DVOL* macros */

/* Macros for EQ gain and cutoff */
#define WMC_EQGC                        0x1f
#define WMC_EQGCr(x)                    ((x) & WMC_EQGC)
#define WMC_EQGCw(x)                    ((x) & WMC_EQGC)

/* WMC_EQ1_LOW_SHELF (0x12) */
#define WMC_EQ3DMODE                    (1 << 8)
#define WMC_EQ1C                        (3 << 5) /* Cutoff */
    #define WMC_EQ1C_80HZ               (0 << 5) /* 80Hz */
    #define WMC_EQ1C_105HZ              (1 << 5) /* 105Hz */
    #define WMC_EQ1C_135HZ              (2 << 5) /* 135Hz */
    #define WMC_EQ1C_175HZ              (3 << 5) /* 175Hz */
    /* 00000=+12dB, 00001=+11dB...(-1dB steps)...11000=-12dB, 11001-11111=reserved */

/* WMC_EQ2_PEAK1 (0x13) */
#define WMC_EQ2BW                       (1 << 8)
#define WMC_EQ2C                        (3 << 5) /* Center */
    #define WMC_EQ2C_230HZ              (0 << 5) /* 230Hz */
    #define WMC_EQ2C_300HZ              (1 << 5) /* 300Hz */
    #define WMC_EQ2C_385HZ              (2 << 5) /* 385Hz */
    #define WMC_EQ2C_500HZ              (3 << 5) /* 500Hz */
    /* 00000=+12dB, 00001=+11dB...(-1dB steps)...11000=-12dB,
       11001-11111=reserved */

/* WMC_EQ3_PEAK2 (0x14) */
#define WMC_EQ3BW                       (1 << 8)
#define WMC_EQ3C                        (3 << 5) /* Center */
    #define WMC_EQ3C_650HZ              (0 << 5) /* 650Hz */
    #define WMC_EQ3C_850HZ              (1 << 5) /* 850Hz */
    #define WMC_EQ3C_1_1KHZ             (2 << 5) /* 1.1kHz */
    #define WMC_EQ3C_1_4KHZ             (3 << 5) /* 1.4kHz */
    /* 00000=+12dB, 00001=+11dB...(-1dB steps)...11000=-12dB,
       11001-11111=reserved */

/* WMC_EQ4_PEAK3 (0x15) */
#define WMC_EQ4BW                       (1 << 8)
#define WMC_EQ4C                        (3 << 5) /* Center */
    #define WMC_EQ4C_1_8KHZ             (0 << 5) /* 1.8kHz */
    #define WMC_EQ4C_2_4KHZ             (1 << 5) /* 2.4kHz */
    #define WMC_EQ4C_3_2KHZ             (2 << 5) /* 3.2kHz */
    #define WMC_EQ4C_4_1KHZ             (3 << 5) /* 4.1kHz */
    /* 00000=+12dB, 00001=+11dB...(-1dB steps)...11000=-12dB,
       11001-11111=reserved */

/* WMC_EQ5_HIGH_SHELF (0x16) */
#define WMC_EQ5C                        (3 << 5) /* Cutoff */
    #define WMC_EQ5C_5_3KHZ             (0 << 5) /* 5.3kHz */
    #define WMC_EQ5C_6_9KHZ             (1 << 5) /* 6.9kHz */
    #define WMC_EQ5C_9KHZ               (2 << 5) /* 9.0kHz */
    #define WMC_EQ5C_11_7KHZ            (3 << 5) /* 11.7kHz */
    /* 00000=+12dB, 00001=+11dB...(-1dB steps)...11000=-12dB,
       11001-11111=reserved */

/* WMC_DAC_LIMITER1 (0x18) */
#define WMC_LIMEN                       (1 << 8)
    /* 0000=750uS, 0001=1.5mS...(x2 each step)...1010-1111=768mS */
#define WMC_LIMDCY                      (0xf << 4)
    #define WMC_LIMDCYr(x)              (((x) & WMC_LIMDCY) >> 4)
    #define WMC_LIMDCYw(x)              (((x) << 4) & WMC_LIMDCY)
    /* 0000=94uS, 0001=188uS...(x2 each step)...1011-1111=192mS */
#define WMC_LIMATK                      (0xf << 0)
    #define WMC_LIMATKr(x)              ((x) & WMC_LIMATK)
    #define WMC_LIMATKw(x)              ((x) & WMC_LIMATK)

/* WMC_DAC_LIMITER2 (0x19) */
#define WMC_LIMLVL                      (7 << 4)
    /* 000=-1dB, 001=-2dB...(-1dB steps)...101-111:-6dB */
    #define WMC_LIMLVLr(x)              (((x) & WMC_LIMLVL) >> 4)
    #define WMC_LIMLVLw(x)              (((x) << 4) & WMC_LIMLVL)
#define WMC_LIMBOOST                    (0xf << 0)
    /* 0000=0dB, 0001=+1dB...1100=+12dB, 1101-1111=reserved */
    #define WMC_LIMBOOSTr(x)            (((x) & WMC_LIMBOOST)    
    #define WMC_LIMBOOSTw(x)            (((x) & WMC_LIMBOOST)    


/* Generic notch filter bits and macros */
#define WMC_NFU                         (1 << 8)
#define WMC_NFA                         (0x7f << 0)
#define WMC_NFAr(x)                     ((x) & WMC_NFA)
#define WMC_NFAw(x)                     ((x) & WMC_NFA)

/* WMC_NOTCH_FILTER1 (0x1b) */
#define WMC_NFEN                        (1 << 7)
/* WMC_NOTCH_FILTER2 (0x1c) */
/* WMC_NOTCH_FILTER3 (0x1d) */
/* WMC_NOTCH_FILTER4 (0x1e) */

/* WMC_ALC_CONTROL1 (0x20) */
#define WMC_ALCSEL                      (3 << 7)
    #define WMC_ALCSEL_OFF              (0 << 7)
    #define WMC_ALCSEL_RIGHT_ONLY       (1 << 7)
    #define WMC_ALCSEL_LEFT_ONLY        (2 << 7)
    #define WMC_ALCSEL_BOTH_ON          (3 << 7)
    /* 000=-6.75dB, 001=-0.75dB...(6dB steps)...111=+35.25dB */
#define WMC_ALCMAXGAIN                  (7 << 3)
    #define WMC_ALCMAXGAINr(x)          (((x) & WMC_ALCMAXGAIN) >> 3)
    #define WMC_ALCMAXGAINw(x)          (((x) << 3) & WMC_ALCMAXGAIN)
    /* 000:-12dB...(6dB steps)...111:+30dB */
#define WMC_ALCMINGAIN                  (7 << 0)
    #define WMC_ALCMINGAINr(x)          ((x) & WMC_ALCMINGAIN)
    #define WMC_ALCMINGAINw(x)          ((x) & WMC_ALCMINGAIN)

/* WMC_ALC_CONTROL2 (0x21) */
    /* 0000=0ms, 0001=2.67ms, 0010=5.33ms...
       (2x with every step)...43.691s */
#define WMC_ALCHLD                      (0xf << 4)
    #define WMC_ALCHLDr(x)              (((x) & WMC_ALCHLD) >> 4)
    #define WMC_ALCHLDw(x)              (((x) << 4) & WMC_ALCHLD)
    /* 1111:-1.5dBFS, 1110:-1.5dBFS, 1101:-3dBFS, 1100:-4.5dBFS...
       (-1.5dB steps)...0001:-21dBFS, 0000:-22.5dBFS */
#define WMC_ALCLVL                      (0xf << 0)
    #define WMC_ALCLVLr(x)              ((x) & WMC_ALCLVL)
    #define WMC_ALCLVLw(x)              ((x) & WMC_ALCLVL)

/* WMC_ALC_CONTROL3 (0x22) */
#define WMC_ALCMODE                     (1 << 8)
#define WMC_ALCDCY                      (0xf << 4)
#define WMC_ALCATK                      (0xf << 0)

/* WMC_NOISE_GATE (0x23) */
#define WMC_NGEN                        (1 << 3)
    /* 000=-39dB, 001=-45dB, 010=-51dB...(6dB steps)...111=-81dB */
#define WMC_NGTH                        (7 << 0)
    #define WMC_NGTHr(x)                ((x) & WMC_NGTH)
    #define WMC_NGTHw(x)                ((x) & WMC_NGTH)

/* WMC_PLL_N (0x24) */
#define WMC_PLL_PRESCALE                (1 << 4)
#define WMC_PLLN                        (0xf << 0)
    #define WMC_PLLNr(x)                ((x) & WMC_PLLN)
    #define WMC_PLLNw(x)                ((x) & WMC_PLLN)

/* WMC_PLL_K1 (0x25) */
#define WMC_PLLK_23_18                  (0x3f << 0)
    #define WMC_PLLK_23_18r(x)          ((x) & WMC_PLLK_23_18)
    #define WMC_PLLK_23_18w(x)          ((x) & WMC_PLLK_23_18)

/* WMC_PLL_K2 (0x26) */
#define WMC_PLLK_17_9                   (0x1ff << 0)
    #define WMC_PLLK_17_9r(x)           ((x) & WMC_PLLK_17_9)
    #define WMC_PLLK_17_9w(x)           ((x) & WMC_PLLK_17_9)

/* WMC_PLL_K3 (0x27) */
#define WMC_PLLK_8_0                    (0x1ff << 0)
    #define WMC_PLLK_8_0r(x)            ((x) & WMC_PLLK_8_0)
    #define WMC_PLLK_8_0w(x)            ((x) & WMC_PLLK_8_0)

/* WMC_3D_CONTROL (0x29) */
    /* 0000: 0%, 0001: 6.67%...1110: 93.3%, 1111: 100% */
#define WMC_DEPTH3D                     (0xf << 0)
    #define WMC_DEPTH3Dw(x)             ((x) & WMC_DEPTH3D)
    #define WMC_DEPTH3Dr(x)             ((x) & WMC_DEPTH3D)

/* WMC_BEEP_CONTROL (0x2b) */
#define WMC_MUTERPGA2INV                (1 << 5)
#define WMC_INVROUT2                    (1 << 4)
    /* 000=-15dB, 001=-12dB...111=+6dB */
#define WMC_BEEPVOL                     (7 << 1)
    #define WMC_BEEPVOLr(x)             (((x) & WMC_BEEPVOL) >> 1)
    #define WMC_BEEPVOLw(x)             (((x) << 1) & WMC_BEEPVOL)
#define WMC_BEEPEN                      (1 << 0)

/* WMC_INPUT_CTRL (0x2c) */
#define WMC_MBVSEL                      (1 << 8)
#define WMC_R2_2INPPGA                  (1 << 6)
#define WMC_RIN2INPPGA                  (1 << 5)
#define WMC_RIP2INPPGA                  (1 << 4)
#define WMC_L2_2INPPGA                  (1 << 2)
#define WMC_LIN2INPPGA                  (1 << 1)
#define WMC_LIP2INPPGA                  (1 << 0)

/* WMC_LEFT_INP_PGA_GAIN_CTRL (0x2d) */
    /* 000000=-12dB, 000001=-11.25dB...010000=0dB, 111111=+35.25dB */
    /* Uses WMC_AVOL* macros */

/* WMC_RIGHT_INP_PGA_GAIN_CTRL (0x2e) */
    /* 000000=-12dB, 000001=-11.25dB...010000=0dB, 111111=+35.25dB */
    /* Uses WMC_AVOL* macros */

/* WMC_LEFT_ADC_BOOST_CTRL (0x2f) */
#define WMC_PGABOOSTL                    (1 << 8)
    /* 000=disabled, 001=-12dB, 010=-9dB...111=+6dB */
#define WMC_L2_2BOOSTVOL                 (7 << 4)
    #define WMC_L2_2BOOSTVOLr(x)         (((x) & WMC_L2_2BOOSTVOL) >> 4)
    #define WMC_L2_2BOOSTVOLw(x)         (((x) << 4) & WMC_L2_2BOOSTVOL)
    /* 000=disabled, 001=-12dB, 010=-9dB...111=+6dB */
#define WMC_AUXL2BOOSTVOL                (7 << 0)
    #define WMC_AUXL2BOOSTVOLr(x)        ((x) & WMC_AUXL2BOOSTVOL)
    #define WMC_AUXL2BOOSTVOLw(x)        ((x) & WMC_AUXL2BOOSTVOL)

/* WMC_RIGHT_ADC_BOOST_CTRL (0x30) */
#define WMC_PGABOOSTR                    (1 << 8)
    /* 000=disabled, 001=-12dB, 010=-9dB...111=+6dB */
#define WMC_R2_2BOOSTVOL                (7 << 4)
    #define WMC_R2_2BOOSTVOLr(x)         (((x) & WMC_R2_2BOOSTVOL) >> 4)
    #define WMC_R2_2BOOSTVOLw(x)         (((x) << 4) & WMC_R2_2BOOSTVOL)
    /* 000=disabled, 001=-12dB, 010=-9dB...111=+6dB */
#define WMC_AUXR2BOOSTVOL                (7 << 0)
    #define WMC_AUXR2BOOSTVOLr(x)        ((x) & WMC_AUXR2BOOSTVOL)
    #define WMC_AUXR2BOOSTVOLw(x)        ((x) & WMC_AUXR2BOOSTVOL)

/* WMC_OUTPUT_CTRL (0x31) */
#define WMC_DACL2RMIX                    (1 << 6)
#define WMC_DACR2LMIX                    (1 << 5)
#define WMC_OUT4BOOST                    (1 << 4)
#define WMC_OUT3BOOST                    (1 << 3)
#define WMC_SPKBOOST                     (1 << 2)
#define WMC_TSDEN                        (1 << 1)
#define WMC_VROI                         (1 << 0)

/* WMC_LEFT_MIXER_CTRL (0x32) */
    /* 000=-15dB, 001=-12dB...101=0dB, 110=+3dB, 111=+6dB */
#define WMC_AUXLMIXVOL                   (7 << 6)
    #define WMC_AUXLMIXVOLr(x)           (((x) & WMC_AUXLMIXVOL) >> 6)
    #define WMC_AUXLMIXVOLw(x)           (((x) << 6) & WMC_AUXLMIXVOL)
#define WMC_AUXL2LMIX                    (1 << 5)
    /* 000=-15dB, 001=-12dB...101=0dB, 110=+3dB, 111=+6dB */
#define WMC_BYPLMIXVOL                   (7 << 2)
    #define WMC_BYPLMIXVOLr(x)           (((x) & WMC_BYPLMIXVOL) >> 2)
    #define WMC_BYPLMIXVOLw(x)           (((x) << 2) & WMC_BYPLMIXVOL)
#define WMC_BYPL2LMIX                    (1 << 1)
#define WMC_DACL2LMIX                    (1 << 0)

/* WMC_RIGHT_MIXER_CTRL (0x33) */
    /* 000=-15dB, 001=-12dB...101=0dB, 110=+3dB, 111=+6dB */
#define WMC_AUXRMIXVOL                   (7 << 6)
    #define WMC_AUXRMIXVOLr(x)           (((x) & WMC_AUXRMIXVOL) >> 6)
    #define WMC_AUXRMIXVOLw(x)           (((x) << 6) & WMC_AUXRMIXVOL)
#define WMC_AUXR2RMIX                    (1 << 5)
    /* 000=-15dB, 001=-12dB...101=0dB, 110=+3dB, 111=+6dB */
#define WMC_BYPRMIXVOL                   (7 << 2)
    #define WMC_BYPRMIXVOLr(x)           (((x) & WMC_BYPRMIXVOL) >> 2)
    #define WMC_BYPRMIXVOLw(x)           (((x) << 2) & WMC_BYPRMIXVOL)
#define WMC_BYPR2RMIX                    (1 << 1)
#define WMC_DACR2RMIX                    (1 << 0)

/* WMC_LOUT1_HP_VOLUME_CTRL (0x34) */
/* WMC_ROUT1_HP_VOLUME_CTRL (0x35) */
/* WMC_LOUT2_SPK_VOLUME_CTRL (0x36) */
/* WMC_ROUT2_SPK_VOLUME_CTRL (0x37) */
    /* 000000=-57dB...111001=0dB...111111=+6dB */
    /* Uses WMC_AVOL* macros */

/* WMC_OUT3_MIXER_CTRL (0x38) */
#define WMC_OUT42OUT3                    (1 << 3)
#define WMC_BYPL2OUT3                    (1 << 2)
#define WMC_LMIX2OUT3                    (1 << 1)
#define WMC_LDAC2OUT3                    (1 << 0)

/* WMC_OUT4_MONO_MIXER_CTRL (0x39) */
#define WMC_HALFSIG                      (1 << 5)
#define WMC_LMIX2OUT4                    (1 << 4)
#define WMC_LDAC2OUT4                    (1 << 3)
#define WMC_BYPR2OUT4                    (1 << 2)
#define WMC_RMIX2OUT4                    (1 << 1)
#define WMC_RDAC2OUT4                    (1 << 0)

#endif /* _WM8978_H */
