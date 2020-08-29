/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2016 by Roman Stolyarov
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

#ifndef _CS4398_H
#define _CS4398_H

#define CS4398_VOLUME_MIN	-1270
#define CS4398_VOLUME_MAX	0

#define AUDIOHW_CAPS (FILTER_ROLL_OFF_CAP | LINEOUT_CAP)
AUDIOHW_SETTING(VOLUME, "dB", 0, 1, CS4398_VOLUME_MIN/10, CS4398_VOLUME_MAX/10, -25)
AUDIOHW_SETTING(FILTER_ROLL_OFF, "", 0, 1, 0, 1, 0)

void cs4398_write_reg(uint8_t reg, uint8_t val);
uint8_t cs4398_read_reg(uint8_t reg);

#define CS4398_I2C_ADDR		0x4f /* 1001111 */

#define CS4398_REG_CHIPID       0x01
#define CS4398_REG_MODECTL      0x02
#define CS4398_REG_VOLMIX       0x03
#define CS4398_REG_MUTE         0x04
#define CS4398_REG_VOL_A        0x05
#define CS4398_REG_VOL_B        0x06
#define CS4398_REG_RAMPFILT     0x07
#define CS4398_REG_MISC         0x08
#define CS4398_REG_MISC2        0x09

/* REG_CHIPID */
#define CS4398_REV_MASK		0x07
#define CS4398_PART_MASK	0xf8
#define CS4398_PART_CS4398	0x70
/* REG_MODECTL */
#define CS4398_FM_MASK		0x03
#define CS4398_FM_SINGLE	0x00
#define CS4398_FM_DOUBLE	0x01
#define CS4398_FM_QUAD		0x02
#define CS4398_FM_DSD		0x03
#define CS4398_DEM_MASK		0x0c
#define CS4398_DEM_NONE		0x00
#define CS4398_DEM_44100	0x04
#define CS4398_DEM_48000	0x08
#define CS4398_DEM_32000	0x0c
#define CS4398_DIF_MASK		0x70
#define CS4398_DIF_LJUST	0x00
#define CS4398_DIF_I2S		0x10
#define CS4398_DIF_RJUST_16	0x20
#define CS4398_DIF_RJUST_24	0x30
#define CS4398_DIF_RJUST_20	0x40
#define CS4398_DIF_RJUST_18	0x50
#define CS4398_DSD_SRC		0x80
/* REG_VOLMIX */
#define CS4398_ATAPI_MASK	0x1f
#define CS4398_ATAPI_B_MUTE	0x00
#define CS4398_ATAPI_B_R	0x01
#define CS4398_ATAPI_B_L	0x02
#define CS4398_ATAPI_B_LR	0x03
#define CS4398_ATAPI_A_MUTE	0x00
#define CS4398_ATAPI_A_R	0x04
#define CS4398_ATAPI_A_L	0x08
#define CS4398_ATAPI_A_LR	0x0c
#define CS4398_ATAPI_MIX_LR_VOL	0x10
#define CS4398_INVERT_B		0x20
#define CS4398_INVERT_A		0x40
#define CS4398_VOL_B_EQ_A	0x80
/* REG_MUTE */
#define CS4398_MUTEP_MASK	0x03
#define CS4398_MUTEP_AUTO	0x00
#define CS4398_MUTEP_LOW	0x02
#define CS4398_MUTEP_HIGH	0x03
#define CS4398_MUTE_B		0x08
#define CS4398_MUTE_A		0x10
#define CS4398_MUTEC_A_EQ_B	0x20
#define CS4398_DAMUTE		0x40
#define CS4398_PAMUTE		0x80
/* REG_VOL_A */
#define CS4398_VOL_A_MASK	0xff
/* REG_VOL_B */
#define CS4398_VOL_B_MASK	0xff
/* REG_RAMPFILT */
#define CS4398_DIR_DSD		0x01
#define CS4398_FILT_SEL		0x04
#define CS4398_RMP_DN		0x10
#define CS4398_RMP_UP		0x20
#define CS4398_ZERO_CROSS	0x40
#define CS4398_SOFT_RAMP	0x80
/* REG_MISC */
#define CS4398_MCLKDIV3		0x08
#define CS4398_MCLKDIV2		0x10
#define CS4398_FREEZE		0x20
#define CS4398_CPEN		0x40
#define CS4398_PDN		0x80
/* REG_MISC2 */
#define CS4398_DSD_PM_EN	0x01
#define CS4398_DSD_PM_MODE	0x02
#define CS4398_INVALID_DSD	0x04
#define CS4398_STATIC_DSD	0x08

#endif
