/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Aidan MacDonald
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
#ifndef __AIC310X_H__
#define __AIC310X_H__

#include "audiohw.h"
#include "mutex.h"
#include <stdint.h>
#include <stddef.h>

/* I2C address (7-bit address without r/w bit) */
#define AIC310X_I2C_ADDR                    0x18

/* Use bit 7 as page indicator */
#define AIC310X_REG_PAGE_MASK               0x80
#define AIC310X_REG_ADDR_MASK               0x7F

/* Get register address / page number */
#define AIC310X_REG_PAGE(x)                 ((x) & AIC310X_REG_PAGE_MASK ? 1 : 0)
#define AIC310X_REG_ADDR(x)                 ((x) & AIC310X_REG_ADDR_MASK)

/* Create page register address */
#define AIC310X_PAGE0_REG(x)                (x)
#define AIC310X_PAGE1_REG(x)                ((x) | AIC310X_REG_PAGE_MASK)

/* Page 0 registers */
#define AIC310X_PAGE_SELECT                 AIC310X_PAGE0_REG(0)
#define AIC310X_SW_RESET                    AIC310X_PAGE0_REG(1)
#define AIC310X_SAMPLE_RATE_SELECT          AIC310X_PAGE0_REG(2)
#define AIC310X_PLL_A                       AIC310X_PAGE0_REG(3)
#define AIC310X_PLL_B                       AIC310X_PAGE0_REG(4)
#define AIC310X_PLL_C                       AIC310X_PAGE0_REG(5)
#define AIC310X_PLL_D                       AIC310X_PAGE0_REG(6)
#define AIC310X_DATA_PATH                   AIC310X_PAGE0_REG(7)
#define AIC310X_INTERFACE_CONTROL_A         AIC310X_PAGE0_REG(8)
#define AIC310X_INTERFACE_CONTROL_B         AIC310X_PAGE0_REG(9)
#define AIC310X_INTERFACE_CONTROL_C         AIC310X_PAGE0_REG(10)
#define AIC310X_OVERFLOW_FLAG               AIC310X_PAGE0_REG(11)
#define AIC310X_DIGITAL_FILTER_CONTROL      AIC310X_PAGE0_REG(12)
#define AIC310X_HEADSET_DETECT_A            AIC310X_PAGE0_REG(13)
#define AIC310X_HEADSET_DETECT_B            AIC310X_PAGE0_REG(14)
#define AIC310X_LEFT_PGA_GAIN               AIC310X_PAGE0_REG(15)
#define AIC310X_RIGHT_PGA_GAIN              AIC310X_PAGE0_REG(16)
#define AIC310X_MIC2_LEFT_ADC_CTRL          AIC310X_PAGE0_REG(17)
#define AIC310X_MIC2_RIGHT_ADC_CTRL         AIC310X_PAGE0_REG(18)
#define AIC310X_MIC1LP_LEFT_ADC_CTRL        AIC310X_PAGE0_REG(19)
#define AIC310X_MIC1RP_LEFT_ADC_CTRL        AIC310X_PAGE0_REG(21)
#define AIC310X_MIC1RP_RIGHT_ADC_CTRL       AIC310X_PAGE0_REG(22)
#define AIC310X_MIC1LP_RIGHT_ADC_CTRL       AIC310X_PAGE0_REG(24)
#define AIC310X_MICBIAS_CTRL                AIC310X_PAGE0_REG(25)
#define AIC310X_LEFT_AGC_CTRL_A             AIC310X_PAGE0_REG(26)
#define AIC310X_LEFT_AGC_CTRL_B             AIC310X_PAGE0_REG(27)
#define AIC310X_LEFT_AGC_CTRL_C             AIC310X_PAGE0_REG(28)
#define AIC310X_RIGHT_AGC_CTRL_A            AIC310X_PAGE0_REG(29)
#define AIC310X_RIGHT_AGC_CTRL_B            AIC310X_PAGE0_REG(30)
#define AIC310X_RIGHT_AGC_CTRL_C            AIC310X_PAGE0_REG(31)
#define AIC310X_LEFT_AGC_GAIN               AIC310X_PAGE0_REG(32)
#define AIC310X_RIGHT_AGC_GAIN              AIC310X_PAGE0_REG(33)
#define AIC310X_LEFT_AGC_NOISE_GATE         AIC310X_PAGE0_REG(34)
#define AIC310X_RIGHT_AGC_NOISE_GATE        AIC310X_PAGE0_REG(35)
#define AIC310X_ADC_FLAG                    AIC310X_PAGE0_REG(36)
#define AIC310X_DAC_POWER_CTRL              AIC310X_PAGE0_REG(37)
#define AIC310X_HPOUT_DRIVER_CTRL           AIC310X_PAGE0_REG(38)
#define AIC310X_HPOUT_STAGE_CTRL            AIC310X_PAGE0_REG(40)
#define AIC310X_DAC_SWITCHING_CTRL          AIC310X_PAGE0_REG(41)
#define AIC310X_DRIVER_POP_REDUCTION        AIC310X_PAGE0_REG(42)
#define AIC310X_LEFT_DAC_DIGITAL_VOLUME     AIC310X_PAGE0_REG(43)
#define AIC310X_RIGHT_DAC_DIGITAL_VOLUME    AIC310X_PAGE0_REG(44)
#define AIC310X_PGA_L_TO_HPLOUT_VOLUME      AIC310X_PAGE0_REG(46)
#define AIC310X_DAC_L1_TO_HPLOUT_VOLUME     AIC310X_PAGE0_REG(47)
#define AIC310X_PGA_R_TO_HPLOUT_VOLUME      AIC310X_PAGE0_REG(49)
#define AIC310X_DAC_R1_TO_HPLOUT_VOLUME     AIC310X_PAGE0_REG(50)
#define AIC310X_HPLOUT_LEVEL_CONTROL        AIC310X_PAGE0_REG(51)
#define AIC310X_PGA_L_TO_HPLCOM_VOLUME      AIC310X_PAGE0_REG(53)
#define AIC310X_DAC_L1_TO_HPLCOM_VOLUME     AIC310X_PAGE0_REG(54)
#define AIC310X_PGA_R_TO_HPLCOM_VOLUME      AIC310X_PAGE0_REG(56)
#define AIC310X_DAC_R1_TO_HPLCOM_VOLUME     AIC310X_PAGE0_REG(57)
#define AIC310X_HPLCOM_LEVEL_CONTROL        AIC310X_PAGE0_REG(58)
#define AIC310X_PGA_L_TO_HPROUT_VOLUME      AIC310X_PAGE0_REG(60)
#define AIC310X_DAC_L1_TO_HPROUT_VOLUME     AIC310X_PAGE0_REG(61)
#define AIC310X_PGA_R_TO_HPROUT_VOLUME      AIC310X_PAGE0_REG(63)
#define AIC310X_DAC_R1_TO_HPROUT_VOLUME     AIC310X_PAGE0_REG(64)
#define AIC310X_HPROUT_LEVEL_CONTROL        AIC310X_PAGE0_REG(65)
#define AIC310X_PGA_L_TO_HPRCOM_VOLUME      AIC310X_PAGE0_REG(67)
#define AIC310X_DAC_L1_TO_HPRCOM_VOLUME     AIC310X_PAGE0_REG(68)
#define AIC310X_PGA_R_TO_HPRCOM_VOLUME      AIC310X_PAGE0_REG(70)
#define AIC310X_DAC_R1_TO_HPRCOM_VOLUME     AIC310X_PAGE0_REG(71)
#define AIC310X_HPRCOM_LEVEL_CONTROL        AIC310X_PAGE0_REG(72)
#define AIC310X_PGA_L_TO_LEFT_LO_VOLUME     AIC310X_PAGE0_REG(81)
#define AIC310X_DAC_L1_TO_LEFT_LO_VOLUME    AIC310X_PAGE0_REG(82)
#define AIC310X_PGA_R_TO_LEFT_LO_VOLUME     AIC310X_PAGE0_REG(84)
#define AIC310X_DAC_R1_TO_LEFT_LO_VOLUME    AIC310X_PAGE0_REG(85)
#define AIC310X_LEFT_LO_LEVEL_CONTROL       AIC310X_PAGE0_REG(86)
#define AIC310X_PGA_L_TO_RIGHT_LO_VOLUME    AIC310X_PAGE0_REG(88)
#define AIC310X_DAC_L1_TO_RIGHT_LO_VOLUME   AIC310X_PAGE0_REG(89)
#define AIC310X_PGA_R_TO_RIGHT_LO_VOLUME    AIC310X_PAGE0_REG(91)
#define AIC310X_DAC_R1_TO_RIGHT_LO_VOLUME   AIC310X_PAGE0_REG(92)
#define AIC310X_RIGHT_LO_LEVEL_CONTROL      AIC310X_PAGE0_REG(93)
#define AIC310X_MODULE_PWR_STATUS           AIC310X_PAGE0_REG(94)
#define AIC310X_SHORT_CIRCUIT_STATUS        AIC310X_PAGE0_REG(95)
#define AIC310X_STICKY_INTERRUPT_FLAGS      AIC310X_PAGE0_REG(96)
#define AIC310X_REAL_TIME_INTERRUPT_FLAGS   AIC310X_PAGE0_REG(97)
#define AIC310X_CLOCK                       AIC310X_PAGE0_REG(101)
#define AIC310X_CLOCK_GEN_CTRL              AIC310X_PAGE0_REG(102)
#define AIC310X_LEFT_AGC_ATTACK_TIME        AIC310X_PAGE0_REG(103)
#define AIC310X_LEFT_AGC_DECAY_TIME         AIC310X_PAGE0_REG(104)
#define AIC310X_RIGHT_AGC_ATTACK_TIME       AIC310X_PAGE0_REG(105)
#define AIC310X_RIGHT_AGC_DECAY_TIME        AIC310X_PAGE0_REG(106)
#define AIC310X_ADC_PATH_AND_I2C_BUS        AIC310X_PAGE0_REG(107)
#define AIC310X_BYPASS_SEL                  AIC310X_PAGE0_REG(108)
#define AIC310X_DAC_CURRENT_ADJUST          AIC310X_PAGE0_REG(109)

/* Page 1 registers */
#define AIC310X_LEFT_EFF_FILTER             AIC310X_PAGE1_REG(1)
#define AIC310X_LEFT_DEEMPH_FILTER          AIC310X_PAGE1_REG(21)
#define AIC310X_RIGHT_EFF_FILTER            AIC310X_PAGE1_REG(27)
#define AIC310X_RIGHT_DEEMPH_FILTER         AIC310X_PAGE1_REG(47)
#define AIC310X_3D_ATTEN_COEFF              AIC310X_PAGE1_REG(53)
#define AIC310X_LEFT_ADC_HIGHPASS_FILTER    AIC310X_PAGE1_REG(65)
#define AIC310X_RIGHT_ADC_HIGHPASS_FILTER   AIC310X_PAGE1_REG(71)

struct aic310x
{
    /* Mutex to protect bus access */
    struct mutex lock;

    /* Cached page select register */
    uint8_t current_page;

    /* I2C bus operations */
    int (*read_multiple) (uint8_t reg, uint8_t *data, size_t count);
    int (*write_multiple) (uint8_t reg, const uint8_t *data, size_t count);
};

void aic310x_init(struct aic310x *codec);

/* Read/write multiple I2C registers */
int aic310x_read_multiple(struct aic310x *codec,
                          uint8_t reg, uint8_t *data, size_t count);
int aic310x_write_multiple(struct aic310x *codec,
                           uint8_t reg, const uint8_t *data, size_t count);

/* Single I2C register read/write and modify operations */
int aic310x_read(struct aic310x *codec, uint8_t reg, uint8_t *value);
int aic310x_write(struct aic310x *codec, uint8_t reg, uint8_t value);
int aic310x_modify(struct aic310x *codec, uint8_t reg, uint8_t mask, uint8_t newvalue);

/* Lock/unlock the codec for performing multi-register operations */
static inline void aic310x_lock(struct aic310x *codec)
{
    mutex_lock(&codec->lock);
}

static inline void aic310x_unlock(struct aic310x *codec)
{
    mutex_unlock(&codec->lock);
}

#endif /* __AIC310X_H__ */
