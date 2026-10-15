/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Marcin Bukat
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

#ifndef __CS42L51_H__
#define __CS42L51_H__

/* AOUTx volume: 0.5 dB steps, +12 dB .. -102 dB. We don't use the
 * positive range to avoid clipping. Values are in tenths of a dB. */
#define CS42L51_VOLUME_MIN  (-1020)
#define CS42L51_VOLUME_MAX  0
#define CS42L51_VOLUME_MUTE (CS42L51_VOLUME_MIN - 1)

AUDIOHW_SETTING(VOLUME, "dB", 1, 5, CS42L51_VOLUME_MIN, CS42L51_VOLUME_MAX, -200)

/* Registers */
#define CS42L51_REG_CHIP_ID         0x01
#define CS42L51_REG_POWER_CTL1      0x02
#define CS42L51_REG_MIC_POWER_CTL   0x03
#define CS42L51_REG_INTF_CTL        0x04
#define CS42L51_REG_DAC_OUT_CTL     0x08
#define CS42L51_REG_DAC_CTL         0x09
#define CS42L51_REG_PCMA_VOL        0x10
#define CS42L51_REG_PCMB_VOL        0x11
#define CS42L51_REG_AOUTA_VOL       0x16
#define CS42L51_REG_AOUTB_VOL       0x17
#define CS42L51_REG_LIMITER         0x1a

/* CHIP_ID */
#define CS42L51_CHIP_ID_MASK        0xf8
#define CS42L51_CHIP_ID             0xd8

/* POWER_CTL1 */
#define CS42L51_POWER_CTL1_PDN      0x01

/* MIC_POWER_CTL: speed mode */
#define CS42L51_SPEED_MASK          0x60
#define CS42L51_SPEED_DSM           0x00 /* 50 - 100 kHz */
#define CS42L51_SPEED_SSM           0x20 /* 25 -  50 kHz */
#define CS42L51_SPEED_HSM           0x40 /* 12.5 - 25 kHz */
#define CS42L51_SPEED_QSM           0x60 /*  4 - 12.5 kHz */

/* INTF_CTL */
#define CS42L51_INTF_CTL_MASTER     0x40
#define CS42L51_DAC_DIF_MASK        0x38
#define CS42L51_DAC_DIF_I2S         0x08

/* DAC_OUT_CTL */
#define CS42L51_HP_GAIN_MASK        0xe0
#define CS42L51_HP_GAIN_OF          0xc0 /* HP_GAIN = 110b, as set by the OF */
#define CS42L51_DACB_MUTE           0x02
#define CS42L51_DACA_MUTE           0x01

/* PCMx_VOL */
#define CS42L51_PCM_MUTE            0x80

/* DAC_CTL */
#define CS42L51_DATA_SEL_MASK       0xc0
#define CS42L51_DATA_SEL_DSP        0x40
#define CS42L51_DAC_AMUTE           0x04
#define CS42L51_DAC_SZC_MASK        0x03

/* Low level driver API. Bus and address come from i2c-target.h
 * (CS42L51_BUS, CS42L51_ADDR). Reset pin is handled by the target. */
extern int cs42l51_open(void);
extern void cs42l51_close(void);
extern void cs42l51_set_frequency(unsigned long sampr);
extern void cs42l51_set_volume(int vol_l, int vol_r);
extern void cs42l51_set_mute(bool mute);

extern int cs42l51_read(int reg);
extern int cs42l51_write(int reg, int value);

#endif /* __CS42L51_H__ */
