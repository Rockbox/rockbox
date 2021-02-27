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

#define AUDIOHW_CAPS (FILTER_ROLL_OFF_CAP|POWER_MODE_CAP)
#define AUDIOHW_HAVE_SHORT2_ROLL_OFF

#define AK4376_MIN_VOLUME (-890)
#define AK4376_MAX_VOLUME 150

AUDIOHW_SETTING(VOLUME, "dB", 1, 5, AK4376_MIN_VOLUME, AK4376_MAX_VOLUME, -200)
AUDIOHW_SETTING(FILTER_ROLL_OFF, "", 0, 1, 0, 3, 0)
AUDIOHW_SETTING(POWER_MODE, "", 0, 1, 0, 1, 0)

/* Register addresses */
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
#define AK4376_REG_AMP_VOLUME   0x0d
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

/* Mixer controls, simply OR them together.
 * LCH    = add LCH signal to output
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

/* Functions to power on / off the DAC which should be called from
 * the target's audiohw_init() / audiohw_close() implementation.
 */
extern void ak4376_init(void);
extern void ak4376_close(void);

/* Register read/write. Cached to avoid redundant reads/writes. */
extern void ak4376_write(int reg, int value);
extern int  ak4376_read(int reg);

/* Target-specific function to set the PDN pin level. */
extern void ak4376_set_pdn_pin(int level);

/* Target-specific function to control the external master clock frequency.
 * This is called by the ak4376's audiohw implementation when switching to
 * or from a frequency that is configured to use this clock source.
 *
 * - hw_freq is the new sample rate -- one of the HW_FREQ_XX constants.
 * - enabled is true if clock should be output, false if not.
 *
 * The return value is the master clock rate as a multiple of the sampling
 * frequency. The allowed multiples depend on the sampling frequency, shown
 * in the table below.
 *
 * +-----------+------------------------+
 * | frequency | master clock rate      |
 * +-----------+------------------------+
 * |   8 -  24 | 256fs / 512fs / 1024fs |
 * |  32 -  48 | 256fs / 512fs          |
 * |  64 -  96 | 256fs                  |
 * | 128 - 192 | 128fs                  |
 * +-----------+------------------------+
 *
 * For example, at 48 KHz you could return either 256 or 512 depending on
 * the rate you decided to actually use.
 *
 * You need to return a valid master multiplier for supported frequencies
 * even when enabled = false, since the driver needs to know the multiplier
 * _before_ enabling the clock.
 *
 * For unsupported frequencies you don't need to return a valid master
 * multiplier, because the DAC doesn't need the return value in such cases.
 */
extern int ak4376_set_mclk_freq(int hw_freq, bool enabled);

#endif /* __AK4376_H__ */
