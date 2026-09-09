/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

#ifndef __CS43131_H__
#define __CS43131_H__

#include <stdbool.h>
#include <stdint.h>

#define CS43131_I2C_ADDR        0x30

#define CS43131_REG_SYS_CLK_CTL 0x010006
#define CS43131_REG_PCM_PATH_CTL1 0x090003
#define CS43131_REG_INT_STATUS1 0x0f0000
#define CS43131_REG_INT_MASK1   0x0f0010
#define CS43131_REG_PWDN_CTL    0x020000
#define CS43131_PWDN_DOWN       0xbe
#define CS43131_PWDN_RELEASE    0xae

#define CS43131_REG_SP_SRATE    0x01000b
#define CS43131_REG_SP_BITSIZE   0x01000c

#define CS43131_REG_PATH_SEL    0x070004
#define CS43131_PATH_PCM        0x02

#define CS43131_REG_PCM_VOL_A   0x090002
#define CS43131_REG_PCM_VOL_B   0x090001
#define CS43131_REG_DSD_VOL_A   0x070001
#define CS43131_REG_DSD_VOL_B   0x070000

/* Volume codes are attenuation: 0x00 is 0 dB, 0xff is -127.5 dB. */
#define CS43131_VOL_SAFE_DEFAULT 0x50

#define CS43131_VOLUME_MIN      (-1275)
#define CS43131_VOLUME_MAX      0
#define CS43131_VOLUME_STEP     5

#define AUDIOHW_CAPS            0

AUDIOHW_SETTING(VOLUME, "dB", 1, CS43131_VOLUME_STEP,
                CS43131_VOLUME_MIN, CS43131_VOLUME_MAX, -400)

extern void cs43131_init(int i2c_bus);

/* Remains muted until configured; returns false and powers off on failure. */
extern bool cs43131_open(void);

extern void cs43131_close(void);

extern void cs43131_mute(bool mute);

extern void cs43131_set_volume(int vol_l, int vol_r);

/* Call while closed, with the target supplying the matching MCLK family. */
extern void cs43131_set_frequency(int sampr);

/* Desired startup configuration, not hardware readback. */
extern int cs43131_read_shadow(uint32_t reg);

extern void cs43131_bus_status(unsigned* writes, unsigned* naks, int* first_err);

extern void cs43131_set_power_pin(int level);
extern void cs43131_set_reset_pin(int level);

#endif /* __CS43131_H__ */
