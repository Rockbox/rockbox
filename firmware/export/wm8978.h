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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _WM8978_H
#define _WM8978_H

#define WM8978_I2C_ADDR 0x34

/* Registers */
#define WM8978_SOFTWARE_RESET               0x00
#define WM8978_POWER_MANAGEMENT1            0x01
#define WM8978_POWER_MANAGEMENT2            0x02
#define WM8978_POWER_MANAGEMENT3            0x03
#define WM8978_AUDIO_INTERFACE              0x04
#define WM8978_COMPANDING_CTRL              0x05
#define WM8978_CLOCK_GEN_CTRL               0x06
#define WM8978_ADDITIONAL_CTRL              0x07
#define WM8978_GPIO                         0x08
#define WM8978_JACK_DETECT_CONTROL1         0x09
#define WM8978_DAC_CONTROL                  0x0a
#define WM8978_LEFT_DAC_DIGITAL_VOL         0x0b
#define WM8978_RIGHT_DAC_DIGITAL_VOL        0x0c
#define WM8978_JACK_DETECT_CONTROL2         0x0d
#define WM8978_ADC_CONTROL                  0x0e
#define WM8978_LEFT_ADC_DIGITAL_VOL         0x0f
#define WM8978_RIGHT_ADC_DITIGAL_VOL        0x10
#define WM8978_EQ1_LOW_SHELF                0x12
#define WM8978_EQ2_PEAK1                    0x13
#define WM8978_EQ3_PEAK2                    0x14
#define WM8978_EQ4_PEAK3                    0x15
#define WM8978_EQ5_HIGH_SHELF               0x16
#define WM8978_DAC_LIMITER1                 0x18
#define WM8978_DAC_LIMITER2                 0x19
#define WM8978_NOTCH_FILTER1                0x1b
#define WM8978_NOTCH_FILTER2                0x1c
#define WM8978_NOTCH_FILTER3                0x1d
#define WM8978_NOTCH_FILTER4                0x1e
#define WM8978_ALC_CONTROL1                 0x20
#define WM8978_ALC_CONTROL2                 0x21
#define WM8978_ALC_CONTROL3                 0x22
#define WM8978_NOISE_GATE                   0x23
#define WM8978_PLL_N                        0x24
#define WM8978_PLL_K1                       0x25
#define WM8978_PLL_K2                       0x26
#define WM8978_PLL_K3                       0x27
#define WM8978_3D_CONTROL                   0x29
#define WM8978_BEEP_CONTROL                 0x2b
#define WM8978_INPUT_CTRL                   0x2c
#define WM8978_LEFT_INP_PGA_GAIN_CTRL       0x2d
#define WM8978_RIGHT_INP_PGA_GAIN_CTRL      0x2e
#define WM8978_LEFT_ADC_BOOST_CTRL          0x2f
#define WM8978_RIGHT_ADC_BOOST_CTRL         0x30
#define WM8978_OUTPUT_CTRL                  0x31
#define WM8978_LEFT_MIXER_CTRL              0x32
#define WM8978_RIGHT_MIXER_CTRL             0x33
#define WM8978_LOUT1_HP_VOLUME_CTRL         0x34
#define WM8978_ROUT1_HP_VOLUME_CTRL         0x35
#define WM8978_LOUT2_SPK_VOLUME_CTRL        0x36
#define WM8978_ROUT2_SPK_VOLUME_CTRL        0x37
#define WM8978_OUT3_MIXER_CTRL              0x38
#define WM8978_OUT4_MONO_MIXER_CTRL         0x39

/* Register bitmasks */

/* WM8978_SOFTWARE_RESET (0x00) */
#define WM8978_RESET
    /* Write any value */

/* WM8978_POWER_MANAGEMENT1 (0x01) */
#define WM8978_BUFDCOMPEN                   (1 << 8)
#define WM8978_OUT4MIXEN                    (1 << 7)
#define WM8978_OUT3MIXEN                    (1 << 6)
#define WM8978_PLLEN                        (1 << 5)
#define WM8978_MICBEN                       (1 << 4)
#define WM8978_BIASEN                       (1 << 3)
#define WM8978_BUFIOEN                      (1 << 2)
#define WM8978_VMIDSEL                      (3 << 0)
    #define WM8978_VMIDSEL_OFF              (0 << 0)
    #define WM8978_VMIDSEL_75K              (1 << 0)
    #define WM8978_VMIDSEL_300K             (2 << 0)
    #define WM8978_VMIDSEL_5K               (3 << 0)

/* WM8978_POWER_MANAGEMENT2 (0x02) */
#define WM8978_ROUT1EN                      (1 << 8)
#define WM8978_LOUT1EN                      (1 << 7)
#define WM8978_SLEEP                        (1 << 6)
#define WM8978_BOOSTENR                     (1 << 5)
#define WM8978_BOOSTENL                     (1 << 4)
#define WM8978_INPPGAENR                    (1 << 3)
#define WM8978_INPPGAENL                    (1 << 2)
#define WM8978_ADCENR                       (1 << 1)
#define WM8978_ADCENL                       (1 << 0)

/* WM8978_POWER_MANAGEMENT3 (0x03) */
#define WM8978_OUT4EN                       (1 << 8)
#define WM8978_OUT3EN                       (1 << 7)
#define WM8978_LOUT2EN                      (1 << 6)
#define WM8978_ROUT2EN                      (1 << 5)
#define WM8978_RMIXEN                       (1 << 3)
#define WM8978_LMIXEN                       (1 << 2)
#define WM8978_DACENR                       (1 << 1)
#define WM8978_DACENL                       (1 << 0)

/* WM8978_AUDIO_INTERFACE (0x04) */
#define WM8978_BCP                          (1 << 8)
#define WM8978_LRP                          (1 << 7)
#define WM8978_WL                           (3 << 5)
    #define WM8978_WL_16                    (0 << 5)
    #define WM8978_WL_20                    (1 << 5)
    #define WM8978_WL_24                    (2 << 5)
    #define WM8978_WL_32                    (3 << 5)
#define WM8978_FMT                          (3 << 3)
    #define WM8978_FMT_RJUST                (0 << 3)
    #define WM8978_FMT_LJUST                (1 << 3)
    #define WM8978_FMT_I2S                  (2 << 3)
    #define WM8978_FMT_DSP_PCM              (3 << 3)
#define WM8978_DACLRSWAP                    (1 << 2)
#define WM8978_ADCLRSWAP                    (1 << 1)
#define WM8978_MONO                         (1 << 0)

/* WM8978_COMPANDING_CTRL (0x05) */
#define WM8978_WL8                          (1 << 5)
#define WM8978_DAC_COMP                     (3 << 3)
    #define WM8978_DAC_COMP_OFF             (0 << 3)
    #define WM8978_DAC_COMP_U_LAW           (2 << 3)
    #define WM8978_DAC_COMP_A_LAW           (3 << 3)
#define WM8978_ADC_COMP                     (3 << 1)
    #define WM8978_DAC_COMP_OFF             (0 << 1)
    #define WM8978_ADC_COMP_U_LAW           (2 << 1)
    #define WM8978_ADC_COMP_A_LAW           (3 << 1)
#define WM8978_LOOPBACK                     (1 << 0)

/* WM8978_CLOCK_GEN_CTRL (0x06) */
#define WM8978_CLKSEL                       (1 << 8)
#define WM8978_MCLKDIV                      (7 << 5)
    #define WM8978_MCLKDIV_1                (0 << 5)
    #define WM8978_MCLKDIV_1_5              (1 << 5)
    #define WM8978_MCLKDIV_2                (2 << 5)
    #define WM8978_MCLKDIV_3                (3 << 5)
    #define WM8978_MCLKDIV_4                (4 << 5)
    #define WM8978_MCLKDIV_6                (5 << 5)
    #define WM8978_MCLKDIV_8                (6 << 5)
    #define WM8978_MCLKDIV_12               (7 << 5)
#define WM8978_BCLKDIV                      (7 << 2)
    #define WM8978_MCLKDIV_1                (0 << 2)
    #define WM8978_MCLKDIV_2                (1 << 2)
    #define WM8978_MCLKDIV_4                (2 << 2)
    #define WM8978_MCLKDIV_8                (3 << 2)
    #define WM8978_MCLKDIV_16               (4 << 2)
    #define WM8978_MCLKDIV_32               (5 << 2)
#define WM8978_MS                           (1 << 0)

/* WM8978_ADDITIONAL_CTRL (0x07) */
#define WM8978_SR                           (7 << 1)
#define WM8978_SLOWCLKEN                    (1 << 0)

/* WM8978_GPIO (0x08) */
#define WM8978_OPCLKDIV                     (3 << 4)
    #define WM8978_OPCLKDIV_1               (0 << 4)
    #define WM8978_OPCLKDIV_2               (1 << 4)
    #define WM8978_OPCLKDIV_3               (2 << 4)
    #define WM8978_OPCLKDIV_4               (3 << 4)
#define WM8978_GPIO1POL                     (1 << 3)
#define WM8978_GPIO1SEL                     (7 << 0)
    #define WM8978_GPIO1SEL_TEMP_OK         (2 << 0)
    #define WM8978_GPIO1SEL_AMUTE_ACTIVE    (3 << 0)
    #define WM8978_GPIO1SEL_PLL_CLK_OP      (4 << 0)
    #define WM8978_GPIO1SEL_PLL_LOCK        (5 << 0)
    #define WM8978_GPIO1SEL_LOGIC_1         (6 << 0)
    #define WM8978_GPIO1SEL_LOGIC_0         (7 << 0)

/* WM8978_JACK_DETECT_CONTROL1 (0x09) */
#define WM8978_JD_VMID                      (3 << 7)
    #define WM8978_JD_VMID_EN_0             (1 << 7)
    #define WM8978_JD_VMID_EN_1             (2 << 7)
#define WM8978_JD_EN                        (1 << 6)
#define WM8978_JD_SEL                       (3 << 4)
    #define WM8978_JD_SEL_GPIO1             (0 << 4)
    #define WM8978_JD_SEL_GPIO2             (1 << 4)
    #define WM8978_JD_SEL_GPIO3             (2 << 4)

/* WM8978_DAC_CONTROL (0x0a) */
#define WM8978_SOFT_MUTE                    (1 << 6)
#define WM8978_DACOSR_128                   (1 << 3)
#define WM8978_AMUTE                        (1 << 2)
#define WM8978_DACPOLR                      (1 << 1)
#define WM8978_DACPOLL                      (1 << 0)

/* WM8978_LEFT_DAC_DIGITAL_VOL (0x0b) */
#define WM8978_DACVUL                       (1 << 8)
    /* 00000000=mute, 00000001=-127dB...(0.5dB steps)...11111111=0dB */
#define WM8978_DACVOLL                      (0xff << 0)
    #define WM8978_DACVOLLr(x)              ((x) & WM8978_DACVOLL)
    #define WM8978_DACVOLLw(x)              ((x) & WM8978_DACVOLL)

/* WM8978_RIGHT_DAC_DIGITAL_VOL (0x0c) */
#define WM8978_DACVUR                       (1 << 8)
    /* 00000000=mute, 00000001=-127dB...(0.5dB steps)...11111111=0dB */
#define WM8978_DACVOLR                      (0xff << 0)
    #define WM8978_DACVOLRr(x)              ((x) & WM8978_DACVOLR)
    #define WM8978_DACVOLRw(x)              ((x) & WM8978_DACVOLR)

/* WM8978_JACK_DETECT_CONTROL2 (0x0d) */
#define WM8978_JD_EN1                       (0xf << 4)
    #define WM8978_OUT1_EN1                 (1 << 4)
    #define WM8978_OUT2_EN1                 (2 << 4)
    #define WM8978_OUT3_EN1                 (4 << 4)
    #define WM8978_OUT4_EN1                 (8 << 4)
#define WM8978_JD_EN0                       (0xf << 0)
    #define WM8978_OUT1_EN0                 (1 << 0)
    #define WM8978_OUT2_EN0                 (2 << 0)
    #define WM8978_OUT3_EN0                 (4 << 0)
    #define WM8978_OUT4_EN0                 (8 << 0)

/* WM8978_ADC_CONTROL (0x0e) */
#define WM8978_HPFEN                        (1 << 8)
#define WM8978_HPFAPP                       (1 << 7)
#define WM8978_HPFCUT                       (7 << 4)
#define WM8978_ADCOSR                       (1 << 3)
#define WM8978_ADCRPOL                      (1 << 1)
#define WM8978_ADCLPOL                      (1 << 0)

/* WM8978_LEFT_ADC_DIGITAL_VOL (0x0f) */
/* WM8978_RIGHT_ADC_DITIGAL_VOL (0x10) */
#define WM8978_ADCVU                        (1 << 8)
    /* 0.5dB steps: Mute:0x00, -127dB:0x01...0dB:0xff */
#define WM8978_ADCVOL                       (0xff << 0)
    #define WM8978_ADCVOLr(x)               ((x) & 0xff)
    #define WM8978_ADCVOLw(x)               ((x) & 0xff)

/* WM8978_EQ1_LOW_SHELF (0x12) */
#define WM8978_EQ3DMODE                     (1 << 8)
#define WM8978_EQ1C                         (3 << 5) /* Cutoff */
    #define WM8978_EQ1C_80HZ                (0 << 5) /* 80Hz */
    #define WM8978_EQ1C_105HZ               (1 << 5) /* 105Hz */
    #define WM8978_EQ1C_135HZ               (2 << 5) /* 135Hz */
    #define WM8978_EQ1C_175HZ               (3 << 5) /* 175Hz */
    /* 00000=+12dB, 00001=+11dB...(-1dB steps)...11000=-12dB, 11001-11111=reserved */
#define WM8978_EQ1G                         (0x1f << 0)
    #define WM8978_EQ1Gr(x)                 ((x) & WM8978_EQ1G)
    #define WM8978_EQ1Gw(x)                 ((x) & WM8978_EQ1G)

/* WM8978_EQ2_PEAK1 (0x13) */
#define WM8978_EQ2BW                        (1 << 8)
#define WM8978_EQ2C                         (3 << 5) /* Center */
    #define WM8978_EQ2C_230HZ               (0 << 5) /* 230Hz */
    #define WM8978_EQ2C_300HZ               (1 << 5) /* 300Hz */
    #define WM8978_EQ2C_385HZ               (2 << 5) /* 385Hz */
    #define WM8978_EQ2C_500HZ               (3 << 5) /* 500Hz */
    /* 00000=+12dB, 00001=+11dB...(-1dB steps)...11000=-12dB,
       11001-11111=reserved */
#define WM8978_EQ2G                         (0x1f << 0)
    #define WM8978_EQ2Gr(x)                 ((x) & WM8978_EQ2G)
    #define WM8978_EQ2Gw(x)                 ((x) & WM8978_EQ2G)

/* WM8978_EQ3_PEAK2 (0x14) */
#define WM8978_EQ3BW                        (1 << 8)
#define WM8978_EQ3C                         (3 << 5) /* Center */
    #define WM8978_EQ3C_650HZ               (0 << 5) /* 650Hz */
    #define WM8978_EQ3C_850HZ               (1 << 5) /* 850Hz */
    #define WM8978_EQ3C_1_1KHZ              (2 << 5) /* 1.1kHz */
    #define WM8978_EQ3C_1_4KHZ              (3 << 5) /* 1.4kHz */
    /* 00000=+12dB, 00001=+11dB...(-1dB steps)...11000=-12dB,
       11001-11111=reserved */
#define WM8978_EQ3G                         (0x1f << 0)
    #define WM8978_EQ3Gr(x)                 ((x) & WM8978_EQ3G)
    #define WM8978_EQ3Gw(x)                 ((x) & WM8978_EQ3G)

/* WM8978_EQ4_PEAK3 (0x15) */
#define WM8978_EQ4BW                        (1 << 8)
#define WM8978_EQ4C                         (3 << 5) /* Center */
    #define WM8978_EQ4C_1_8KHZ              (0 << 5) /* 1.8kHz */
    #define WM8978_EQ4C_2_4KHZ              (1 << 5) /* 2.4kHz */
    #define WM8978_EQ4C_3_2KHZ              (2 << 5) /* 3.2kHz */
    #define WM8978_EQ4C_4_1KHZ              (3 << 5) /* 4.1kHz */
    /* 00000=+12dB, 00001=+11dB...(-1dB steps)...11000=-12dB,
       11001-11111=reserved */
#define WM8978_EQ4G                         (0x1f << 0)
    #define WM8978_EQ4Gr(x)                 ((x) & WM8978_EQ4G)
    #define WM8978_EQ4Gw(x)                 ((x) & WM8978_EQ4G)

/* WM8978_EQ5_HIGH_SHELF (0x16) */
#define WM8978_EQ5C                         (3 << 5) /* Cutoff */
    #define WM8978_EQ5C_5_3KHZ              (0 << 5) /* 5.3kHz */
    #define WM8978_EQ5C_6_9KHZ              (1 << 5) /* 6.9kHz */
    #define WM8978_EQ5C_9KHZ                (2 << 5) /* 9.0kHz */
    #define WM8978_EQ5C_11_7KHZ             (3 << 5) /* 11.7kHz */
    /* 00000=+12dB, 00001=+11dB...(-1dB steps)...11000=-12dB,
       11001-11111=reserved */
#define WM8978_EQ5G                         (0x1f << 0)
    #define WM8978_EQ5Gr(x)                 ((x) & WM8978_EQ5G)
    #define WM8978_EQ5Gw(x)                 ((x) & WM8978_EQ5G)

/* WM8978_DAC_LIMITER1 (0x18) */
#define WM8978_LIMEN                        (1 << 8)
    /* 0000=750uS, 0001=1.5mS...(x2 each step)...1010-1111=768mS */
#define WM8978_LIMDCY                       (0xf << 4)
    #define WM8978_LIMDCYr(x)               (((x) & WM8978_LIMDCY) >> 4)
    #define WM8978_LIMDCYw(x)               (((x) << 4) & WM8978_LIMDCY)
    /* 0000=94uS, 0001=188uS...(x2 each step)...1011-1111=192mS */
#define WM8978_LIMATK                       (0xf << 0)
    #define WM8978_LIMATKr(x)               ((x) & WM8978_LIMATK)
    #define WM8978_LIMATKw(x)               ((x) & WM8978_LIMATK)

/* WM8978_DAC_LIMITER2 (0x19) */
#define WM8978_LIMLVL                       (7 << 4)
    /* 000=-1dB, 001=-2dB...(-1dB steps)...101-111:-6dB */
    #define WM8978_LIMLVLr(x)               (((x) & WM8978_LIMLVL) >> 4)
    #define WM8978_LIMLVLw(x)               (((x) << 4) & WM8978_LIMLVL)
#define WM8978_LIMBOOST                     (0xf << 0)
    /* 0000=0dB, 0001=+1dB...1100=+12dB, 1101-1111=reserved */
    #define WM8978_LIMBOOSTr(x)             (((x) & WM8978_LIMBOOST)    
    #define WM8978_LIMBOOSTw(x)             (((x) & WM8978_LIMBOOST)    

/* WM8978_NOTCH_FILTER1 (0x1b) */
#define WM8978_NFU1                         (1 << 8)
#define WM8978_NFEN                         (1 << 7)
#define WM8978_NFA0_13_7                    (0x7f << 0)
    #define WM8978_NFA0_13_7r(x)            ((x) & WM8978_NFA0_13_7)
    #define WM8978_NFA0_13_7w(x)            ((x) & WM8978_NFA0_13_7)

/* WM8978_NOTCH_FILTER2 (0x1c) */
#define WM8978_NFU2                         (1 << 8)
#define WM8978_NFA0_6_0                     (0x7f << 0)
    #define WM8978_NFA0_6_0r(x)             ((x) & WM8978_NFA0_6_0)
    #define WM8978_NFA0_6_0w(x)             ((x) & WM8978_NFA0_6_0)

/* WM8978_NOTCH_FILTER3 (0x1d) */
#define WM8978_NFU3                         (1 << 8)
#define WM8978_NFA1_13_7                    (0x7f << 0)
    #define WM8978_NFA1_13_7r(x)            ((x) & WM8978_NFA1_13_7)
    #define WM8978_NFA1_13_7w(x)            ((x) & WM8978_NFA1_13_7)

/* WM8978_NOTCH_FILTER4 (0x1e) */
#define WM8978_NFU4                         (1 << 8)
#define WM8978_NFA1_6_0                     (0x7f << 0)
    #define WM8978_NFA1_6_0r(x)             ((x) & WM8978_NFA1_6_0)
    #define WM8978_NFA1_6_0w(x)             ((x) & WM8978_NFA1_6_0)

/* WM8978_ALC_CONTROL1 (0x20) */
#define WM8978_ALCSEL                       (3 << 7)
    #define WM8978_ALCSEL_OFF               (0 << 7)
    #define WM8978_ALCSEL_RIGHT_ONLY        (1 << 7)
    #define WM8978_ALCSEL_LEFT_ONLY         (2 << 7)
    #define WM8978_ALCSEL_BOTH_ON           (3 << 7)
    /* 000=-6.75dB, 001=-0.75dB...(6dB steps)...111=+35.25dB */
#define WM8978_ALCMAXGAIN                   (7 << 3)
    #define WM8978_ALCMAXGAINr(x)           (((x) & WM8978_ALCMAXGAIN) >> 3)
    #define WM8978_ALCMAXGAINw(x)           (((x) << 3) & WM8978_ALCMAXGAIN)
    /* 000:-12dB...(6dB steps)...111:+30dB */
#define WM8978_ALCMINGAIN                   (7 << 0)
    #define WM8978_ALCMINGAINr(x)           ((x) & WM8978_ALCMINGAIN)
    #define WM8978_ALCMINGAINw(x)           ((x) & WM8978_ALCMINGAIN)

/* WM8978_ALC_CONTROL2 (0x21) */
    /* 0000=0ms, 0001=2.67ms, 0010=5.33ms...
       (2x with every step)...43.691s */
#define WM8978_ALCHLD                       (0xf << 4)
    #define WM8978_ALCHLDr(x)               (((x) & WM8978_ALCHLD) >> 4)
    #define WM8978_ALCHLDw(x)               (((x) << 4) & WM8978_ALCHLD)
    /* 1111:-1.5dBFS, 1110:-1.5dBFS, 1101:-3dBFS, 1100:-4.5dBFS...
       (-1.5dB steps)...0001:-21dBFS, 0000:-22.5dBFS */
#define WM8978_ALCLVL                       (0xf << 0)
    #define WM8978_ALCLVLr(x)               ((x) & WM8978_ALCLVL)
    #define WM8978_ALCLVLw(x)               ((x) & WM8978_ALCLVL)

/* WM8978_ALC_CONTROL3 (0x22) */
#define WM8978_ALCMODE                      (1 << 8)
#define WM8978_ALCDCY                       (0xf << 4)
#define WM8978_ALCATK                       (0xf << 0)

/* WM8978_NOISE_GATE (0x23) */
#define WM8978_NGEN                         (1 << 3)
    /* 000=-39dB, 001=-45dB, 010=-51dB...(6dB steps)...111=-81dB */
#define WM8978_NGTH                         (7 << 0)
    #define WM8978_NGTHr(x)                 ((x) & WM8978_NGTH)
    #define WM8978_NGTHw(x)                 ((x) & WM8978_NGTH)

/* WM8978_PLL_N (0x24) */
#define WM8978_PLL_PRESCALE                 (1 << 4)
#define WM8978_PLLN                         (0xf << 0)
    #define WM8978_PLLNr(x)                 ((x) & WM8978_PLLN)
    #define WM8978_PLLNw(x)                 ((x) & WM8978_PLLN)

/* WM8978_PLL_K1 (0x25) */
#define WM8978_PLLK_23_18                   (0x3f << 0)
    #define WM8978_PLLK_23_18r(x)           ((x) & WM8978_PLLK_23_18)
    #define WM8978_PLLK_23_18w(x)           ((x) & WM8978_PLLK_23_18)

/* WM8978_PLL_K2 (0x26) */
#define WM8978_PLLK_17_9                    (0x1ff << 0)
    #define WM8978_PLLK_17_9r(x)            ((x) & WM8978_PLLK_17_9)
    #define WM8978_PLLK_17_9w(x)            ((x) & WM8978_PLLK_17_9)

/* WM8978_PLL_K3 (0x27) */
#define WM8978_PLLK_8_0                     (0x1ff << 0)
    #define WM8978_PLLK_8_0r(x)             ((x) & WM8978_PLLK_8_0)
    #define WM8978_PLLK_8_0w(x)             ((x) & WM8978_PLLK_8_0)

/* WM8978_3D_CONTROL (0x29) */
    /* 0000: 0%, 0001: 6.67%...1110: 93.3%, 1111: 100% */
#define WM8978_DEPTH3D                      (0xf << 0)
    #define WM8978_DEPTH3Dw(x)              ((x) & WM8978_DEPTH3D)
    #define WM8978_DEPTH3Dr(x)              ((x) & WM8978_DEPTH3D)

/* WM8978_BEEP_CONTROL (0x2b) */
#define WM8978_MUTERPGA2INV                 (1 << 5)
#define WM8978_INVROUT2                     (1 << 4)
    /* 000=-15dB, 001=-12dB...111=+6dB */
#define WM8978_BEEPVOL                      (7 << 1)
    #define WM8978_BEEPVOLr(x)              (((x) & WM8978_BEEPVOL) >> 1)
    #define WM8978_BEEPVOLw(x)              (((x) << 1) & WM8978_BEEPVOL)
#define WM8978_BEEPEN                       (1 << 0)

/* WM8978_INPUT_CTRL (0x2c) */
#define WM8978_MBVSEL                       (1 << 8)
#define WM8978_R2_2INPPGA                   (1 << 6)
#define WM8978_RIN2INPPGA                   (1 << 5)
#define WM8978_RIP2INPPGA                   (1 << 4)
#define WM8978_L2_2INPPGA                   (1 << 2)
#define WM8978_LIN2INPPGA                   (1 << 1)
#define WM8978_LIP2INPPGA                   (1 << 0)

/* WM8978_LEFT_INP_PGA_GAIN_CTRL (0x2d) */
#define WM8978_INPPGAUPDATEL                (1 << 8)
#define WM8978_NPPGAZCL                     (1 << 7)
#define WM8978_INPPGAMUTEL                  (1 << 6)
    /* 000000=-12dB, 000001=-11.25dB...010000=0dB, 111111=+35.25dB */
#define WM8978_INPPGAVOLL                   (0x3f << 0)
    #define WM8978_INPPGAVOLLr(x)           ((x) & WM8978_INPPGAVOLL)
    #define WM8978_INPPGAVOLLw(x)           ((x) & WM8978_INPPGAVOLL)

/* WM8978_RIGHT_INP_PGA_GAIN_CTRL (0x2e) */
#define WM8978_INPPGAUPDATER                (1 << 8)
#define WM8978_NPPGAZCR                     (1 << 7)
#define WM8978_INPPGAMUTER                  (1 << 6)
    /* 000000=-12dB, 000001=-11.25dB...010000=0dB, 111111=+35.25dB */
#define WM8978_INPPGAVOLR                   (0x3f << 0)
    #define WM8978_INPPGAVOLRr(x)           ((x) & WM8978_INPPGAVOLR)
    #define WM8978_INPPGAVOLRw(x)           ((x) & WM8978_INPPGAVOLR)

/* WM8978_LEFT_ADC_BOOST_CTRL (0x2f) */
#define WM8978_PGABOOSTL                    (1 << 8)
    /* 000=disabled, 001=-12dB, 010=-9dB...111=+6dB */
#define WM8978_L2_2BOOSTVOL                 (7 << 4)
    #define WM8978_L2_2BOOSTVOLr(x)         ((x) & WM8978_L2_2_BOOSTVOL) >> 4)
    #define WM8978_L2_2BOOSTVOLw(x)         ((x) << 4) & WM8978_L2_2_BOOSTVOL)
    /* 000=disabled, 001=-12dB, 010=-9dB...111=+6dB */
#define WM8978_AUXL2BOOSTVOL                (7 << 0)
    #define WM8978_AUXL2BOOSTVOLr(x)        ((x) & WM8978_AUXL2BOOSTVOL)
    #define WM8978_AUXL2BOOSTVOLw(x)        ((x) & WM8978_AUXL2BOOSTVOL)

/* WM8978_RIGHT_ADC_BOOST_CTRL (0x30) */
#define WM8978_PGABOOSTR                    (1 << 8)
    /* 000=disabled, 001=-12dB, 010=-9dB...111=+6dB */
#define WM8978_R2_2_BOOSTVOL                (7 << 4)
    #define WM8978_R2_2BOOSTVOLr(x)         ((x) & WM8978_R2_2_BOOSTVOL) >> 4)
    #define WM8978_R2_2BOOSTVOLw(x)         ((x) << 4) & WM8978_R2_2_BOOSTVOL)
    /* 000=disabled, 001=-12dB, 010=-9dB...111=+6dB */
#define WM8978_AUXR2BOOSTVOL                (7 << 0)
    #define WM8978_AUXR2BOOSTVOLr(x)        ((x) & WM8978_AUXR2BOOSTVOL)
    #define WM8978_AUXR2BOOSTVOLw(x)        ((x) & WM8978_AUXR2BOOSTVOL)

/* WM8978_OUTPUT_CTRL (0x31) */
#define WM8978_DACL2RMIX                    (1 << 6)
#define WM8978_DACR2LMIX                    (1 << 5)
#define WM8978_OUT4BOOST                    (1 << 4)
#define WM8978_OUT3BOOST                    (1 << 3)
#define WM8978_SPKBOOST                     (1 << 2)
#define WM8978_TSDEN                        (1 << 1)
#define WM8978_VROI                         (1 << 0)

/* WM8978_LEFT_MIXER_CTRL (0x32) */
    /* 000=-15dB, 001=-12dB...101=0dB, 110=+3dB, 111=+6dB */
#define WM8978_AUXLMIXVOL                   (7 << 6)
    #define WM8978_AUXLMIXVOLr(x)           ((x) & WM8978_AUXLMIXVOL) >> 6)
    #define WM8978_AUXLMIXVOLw(x)           ((x) << 6) & WM8978_AUXLMIXVOL)
#define WM8978_AUXL2LMIX                    (1 << 5)
    /* 000=-15dB, 001=-12dB...101=0dB, 110=+3dB, 111=+6dB */
#define WM8978_BYPLMIXVOL                   (7 << 2)
    #define WM8978_BYPLMIXVOLr(x)           ((x) & WM8978_BYPLMIXVOL) >> 2)
    #define WM8978_BYPLMIXVOLw(x)           ((x) << 2) & WM8978_BYPLMIXVOL)
#define WM8978_BYPL2LMIX                    (1 << 1)
#define WM8978_DACL2LMIX                    (1 << 0)

/* WM8978_RIGHT_MIXER_CTRL (0x33) */
    /* 000=-15dB, 001=-12dB...101=0dB, 110=+3dB, 111=+6dB */
#define WM8978_AUXRMIXVOL                   (7 << 6)
    #define WM8978_AUXRMIXVOLr(x)           ((x) & WM8978_AUXRMIXVOL) >> 6)
    #define WM8978_AUXRMIXVOLw(x)           ((x) << 6) & WM8978_AUXRMIXVOL)
#define WM8978_AUXR2RMIX                    (1 << 5)
    /* 000=-15dB, 001=-12dB...101=0dB, 110=+3dB, 111=+6dB */
#define WM8978_BYPRMIXVOL                   (7 << 2)
    #define WM8978_BYPRMIXVOLr(x)           ((x) & WM8978_BYPRMIXVOL) >> 2)
    #define WM8978_BYPRMIXVOLw(x)           ((x) << 2) & WM8978_BYPRMIXVOL)
#define WM8978_BYPR2RMIX                    (1 << 1)
#define WM8978_DACR2RMIX                    (1 << 0)

/* WM8978_LOUT1_HP_VOLUME_CTRL (0x34) */
#define WM8978_LHPVU                        (1 << 8)
#define WM8978_LOUT1ZC                      (1 << 7)
#define WM8978_LOUT1MUTE                    (1 << 6)
    /* 000000=-57dB...111001=0dB...111111=+6dB */
#define WM8978_LOUT1VOL                     (0x3f << 0)
    #define WM8978_LOUT1VOLr(x)             ((x) & WM8978_LOUT1VOL)
    #define WM8978_LOUT1VOLw(x)             ((x) & WM8978_LOUT1VOL)

/* WM8978_ROUT1_HP_VOLUME_CTRL (0x35) */
#define WM8978_RHPVU                        (1 << 8)
#define WM8978_ROUT1ZC                      (1 << 7)
#define WM8978_ROUT1MUTE                    (1 << 6)
    /* 000000=-57dB...111001=0dB...111111=+6dB */
#define WM8978_ROUT1VOL                     (0x3f << 0)
    #define WM8978_ROUT1VOLr(x)             ((x) & WM8978_ROUT1VOL)
    #define WM8978_ROUT1VOLw(x)             ((x) & WM8978_ROUT1VOL)

/* WM8978_LOUT2_SPK_VOLUME_CTRL (0x36) */
#define WM8978_LSPKVU                       (1 << 8)
#define WM8978_LOUT2ZC                      (1 << 7)
#define WM8978_LOUT2MUTE                    (1 << 6)
    /* 000000=-57dB...111001=0dB...111111=+6dB */
#define WM8978_LOUT2VOL                     (0x3f << 0)
    #define WM8978_LOUT2VOLr(x)             ((x) & WM8978_LOUT2VOL)
    #define WM8978_LOUT2VOLw(x)             ((x) & WM8978_LOUT2VOL)

/* WM8978_ROUT2_SPK_VOLUME_CTRL (0x37) */
#define WM8978_RSPKVU                       (1 << 8)
#define WM8978_ROUT2ZC                      (1 << 7)
#define WM8978_ROUT2MUTE                    (1 << 6)
    /* 000000=-57dB...111001=0dB...111111=+6dB */
#define WM8978_ROUT2VOL                     (0x3f << 0)
    #define WM8978_ROUT2VOLr(x)             ((x) & WM8978_ROUT2VOL)
    #define WM8978_ROUT2VOLw(x)             ((x) & WM8978_ROUT2VOL)

/* WM8978_OUT3_MIXER_CTRL (0x38) */
#define WM8978_OUT3MUTE                     (1 << 6)
#define WM8978_OUT42OUT3                    (1 << 3)
#define WM8978_BYPL2OUT3                    (1 << 2)
#define WM8978_LMIX2OUT3                    (1 << 1)
#define WM8978_LDAC2OUT3                    (1 << 0)

/* WM8978_OUT4_MONO_MIXER_CTRL (0x39) */
#define WM8978_OUT4MUTE                     (1 << 6)
#define WM8978_HALFSIG                      (1 << 5)
#define WM8978_LMIX2OUT4                    (1 << 4)
#define WM8978_LDAC2OUT4                    (1 << 3)
#define WM8978_BYPR2OUT4                    (1 << 2)
#define WM8978_RMIX2OUT4                    (1 << 1)
#define WM8978_RDAC2OUT4                    (1 << 0)

#endif /* _WM8978_H */
