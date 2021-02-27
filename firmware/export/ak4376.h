/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __AK4376_H__
#define __AK4376_H__

#define AK4376_REG_PWR1         0x00
#define AK4376_REG_PWR2         0x01
#define AK4376_REG_PWR3         0x02
#define AK4376_REG_PWR4         0x03
#define AK4376_REG_OUTPUT_MODE  0x04
#define AK4376_REG_CLOCK_MODE   0x05
#define AK4376_REG_FILTER       0x06
#define AK4376_REG_MIXER        0x07
#define AK4376_REG_LCH_VOLUME   0x0b
#define AK4376_REG_RCH_VOLUME   0x0c
#define AK4376_REG_HP_VOLUME    0x0d
#define AK4376_REG_PLL_CLK_SRC  0x0e
#define AK4376_REG_PLL_REF_DIV1 0x0f
#define AK4376_REG_PLL_REF_DIV2 0x10
#define AK4376_REG_PLL_FB_DIV1  0x11
#define AK4376_REG_PLL_FB_DIV2  0x12
#define AK4376_REG_DAC_CLK_SRC  0x13
#define AK4376_REG_DAC_CLK_DIV  0x14
#define AK4376_REG_AUDIO_IF_FMT 0x15
#define AK4376_REG_CHIP_ID      0x21
#define AK4376_REG_MODE_CTRL    0x24
#define AK4376_REG_ADJUST1      0x26
#define AK4376_REG_ADJUST2      0x2a
#define AK4376_NUM_REGS         0x2b

/* LCH    = add LCH signal to output
 * RCH    = add RCH signal to output
 * HALF   = multiply output by 1/2
 * INVERT = invert the output after everything else
 */
#define AK4376_MIX_MUTE   0
#define AK4376_MIX_LCH    1
#define AK4376_MIX_RCH    2
#define AK4376_MIX_HALF   4
#define AK4376_MIX_INVERT 8

/* Min/max digital volumes in units of dB/10 */
#define AK4376_DIG_VOLUME_MIN (-120)
#define AK4376_DIG_VOLUME_MAX 30
#define AK4376_DIG_VOLUME_STEP 5
#define AK4376_DIG_VOLUME_MUTE (AK4376_DIG_VOLUME_MIN - 1)

/* Min/max headphone amp volumes in units of dB/10 */
#define AK4376_AMP_VOLUME_MIN (-200)
#define AK4376_AMP_VOLUME_MAX 60
#define AK4376_AMP_VOLUME_STEP 20
#define AK4376_AMP_VOLUME_MUTE (AK4376_AMP_VOLUME_MIN - 1)

/* Digital filters */
#define AK4376_FILTER_SHARP       0
#define AK4376_FILTER_SLOW        1
#define AK4376_FILTER_SHORT_SHARP 2
#define AK4376_FILTER_SHORT_SLOW  3

/* Frequency selection */
#define AK4376_FS_8   0
#define AK4376_FS_11  1
#define AK4376_FS_12  2
#define AK4376_FS_16  4
#define AK4376_FS_22  5
#define AK4376_FS_24  6
#define AK4376_FS_32  8
#define AK4376_FS_44  9
#define AK4376_FS_48  10
#define AK4376_FS_64  12
#define AK4376_FS_88  13
#define AK4376_FS_96  14
#define AK4376_FS_176 17
#define AK4376_FS_192 18

/* Cached read and write register */
extern void ak4376_clear_reg_cache(void);
extern void ak4376_write(int reg, int value);
extern int  ak4376_read(int reg);

/* Setters */
extern void ak4376_set_dig_volume(int vol_l, int vol_r);
extern void ak4376_set_amp_volume(int vol);
extern void ak4376_set_mix(int lch, int rch);
extern void ak4376_set_filter_roll_off(int val);

/* Read and write registers unconditionally.
 * (Target specific)
 */
extern void ak4376_reg_write(int reg, int value);
extern int  ak4376_reg_read(int reg);

/* Power pin control (target specific) */
extern void ak4376_power_on(void);
extern void ak4376_power_off(void);

#endif /* __AK4376_H__ */
