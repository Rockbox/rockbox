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

#include "audiohw.h"
#include "sound.h"
#include "panic.h"
#include "pcm_sampr.h"
#include "pcm_sw_volume.h"
#include "system.h"
#include "i2c-async.h"

/* sample rates supported by the hardware */
#define CAPS (SAMPR_CAP_192 | SAMPR_CAP_176 | \
              SAMPR_CAP_96  | SAMPR_CAP_88  | SAMPR_CAP_64 | \
              SAMPR_CAP_48  | SAMPR_CAP_44  | SAMPR_CAP_32 | \
              SAMPR_CAP_24  | SAMPR_CAP_22  | SAMPR_CAP_16 | \
              SAMPR_CAP_12  | SAMPR_CAP_11  | SAMPR_CAP_8)

/* future proofing */
#if (HW_SAMPR_CAPS & ~CAPS) != 0
# error "incorrect HW_SAMPR_CAPS"
#endif

#ifndef HAVE_SW_VOLUME_CONTROL
# error "AK4376 requires HAVE_SW_VOLUME_CONTROL!"
#endif

/* NOTE: At present, only the FiiO M3K uses this driver so the handling of
 * the clock / audio interface is limited to I2S slave, 16-bit samples, with
 * DAC master clock provided directly on the MCLK input pin, fitting the
 * clock setup of the M3K.
 *
 * Feel free to expand upon this if another target ever needs this driver.
 */

/* Converts HW_FREQ_XX constants to register values */
static const uint8_t ak4376_fsel_to_hw[] = {
    HW_HAVE_192_(AK4376_FS_192,)
    HW_HAVE_176_(AK4376_FS_176,)
    HW_HAVE_96_(AK4376_FS_96,)
    HW_HAVE_88_(AK4376_FS_88,)
    HW_HAVE_64_(AK4376_FS_64,)
    HW_HAVE_48_(AK4376_FS_48,)
    HW_HAVE_44_(AK4376_FS_44,)
    HW_HAVE_32_(AK4376_FS_32,)
    HW_HAVE_24_(AK4376_FS_24,)
    HW_HAVE_22_(AK4376_FS_22,)
    HW_HAVE_16_(AK4376_FS_16,)
    HW_HAVE_12_(AK4376_FS_12,)
    HW_HAVE_11_(AK4376_FS_11,)
    HW_HAVE_8_(AK4376_FS_8,)
};

static int ak4376_regs[AK4376_NUM_REGS];

void ak4376_open(void)
{
    /* Initialize DAC state */
    for(int i = 0; i < AK4376_NUM_REGS; ++i)
        ak4376_regs[i] = -1;

    /* Initial reset after power-on */
    ak4376_set_pdn_pin(0);
    mdelay(1);
    ak4376_set_pdn_pin(1);
    mdelay(1);

    static const uint8_t init_config[] = {
        /* Ensure HPRHZ, HPLHZ are 0 */
        AK4376_REG_OUTPUT_MODE,  0x00,
        /* Mute all volume controls */
        AK4376_REG_MIXER,        AK4376_MIX_LCH | (AK4376_MIX_RCH << 4),
        AK4376_REG_LCH_VOLUME,   0x80,
        AK4376_REG_RCH_VOLUME,   0x00,
        AK4376_REG_AMP_VOLUME,   0x00,
        /* Clock source = MCLK, divider = 1 */
        AK4376_REG_DAC_CLK_SRC,  0x00,
        AK4376_REG_DAC_CLK_DIV,  0x00,
        /* I2S slave mode, 16-bit samples */
        AK4376_REG_AUDIO_IF_FMT, 0x03,
        /* Recommended by datasheet */
        AK4376_REG_ADJUST1,      0x20,
        AK4376_REG_ADJUST2,      0x05,
        /* Power controls */
        AK4376_REG_PWR2,         0x33,
        AK4376_REG_PWR3,         0x01,
        AK4376_REG_PWR4,         0x03,
    };

    /* Write initial configuration prior to power-up */
    for(size_t i = 0; i < ARRAYLEN(init_config); i += 2)
        ak4376_write(init_config[i], init_config[i+1]);
}

void ak4376_close(void)
{
    /* Shut off power */
    ak4376_write(AK4376_REG_PWR3, 0x00);
    ak4376_write(AK4376_REG_PWR4, 0x00);
    ak4376_write(AK4376_REG_PWR2, 0x00);

    /* PDN pin low */
    ak4376_set_pdn_pin(0);
}

void ak4376_write(int reg, int value)
{
    /* Ensure value is sensible and differs from the last set value */
    if((value & 0xff) == value && ak4376_regs[reg] != value) {
        int r = i2c_reg_write1(AK4376_BUS, AK4376_ADDR, reg, value);
        if(r == I2C_STATUS_OK)
            ak4376_regs[reg] = value;
        else
            ak4376_regs[reg] = -1;
    }
}

int ak4376_read(int reg)
{
    /* Only read from I2C if we don't already know the value */
    if(ak4376_regs[reg] < 0)
        ak4376_regs[reg] = i2c_reg_read1(AK4376_BUS, AK4376_ADDR, reg);

    return ak4376_regs[reg];
}

static int round_step_up(int x, int step)
{
    int rem = x % step;
    if(rem > 0)
        rem -= step;
    return x - rem;
}

static void calc_volumes(int vol, int* dig, int* sw)
{
    /* Apply digital volume */
    *dig = round_step_up(vol, AK4376_DIG_VOLUME_STEP);
    *dig = MIN(*dig, AK4376_DIG_VOLUME_MAX);
    *dig = MAX(*dig, AK4376_DIG_VOLUME_MIN);
    vol -= *dig;

    /* Seems that this is the allowable range for software volume */
    *sw = MIN(vol, 60);
    *sw = MAX(*sw, -730);
    vol -= *sw;
}

static int dig_vol_to_hw(int vol)
{
    if(vol < AK4376_DIG_VOLUME_MIN) return 0;
    if(vol > AK4376_DIG_VOLUME_MAX) return 31;
    return (vol - AK4376_DIG_VOLUME_MIN) / AK4376_DIG_VOLUME_STEP + 1;
}

static int amp_vol_to_hw(int vol)
{
    if(vol < AK4376_AMP_VOLUME_MIN) return 0;
    if(vol > AK4376_AMP_VOLUME_MAX) return 14;
    return (vol - AK4376_AMP_VOLUME_MIN) / AK4376_AMP_VOLUME_STEP + 1;
}

void ak4376_set_volume(int vol_l, int vol_r)
{
    int amp;
    int dig_l, sw_l;
    int dig_r, sw_r;

    if(vol_l <= AK4376_MIN_VOLUME && vol_r <= AK4376_MIN_VOLUME) {
        /* Special case for full mute */
        amp = AK4376_AMP_VOLUME_MUTE;
        dig_l = dig_r = AK4376_DIG_VOLUME_MUTE;
        sw_l = sw_r = PCM_MUTE_LEVEL;
    } else {
        /* Amp is a mono control -- calculate based on the loudest channel.
         * The quieter channel then gets reduced more by digital controls. */
        amp = round_step_up(MAX(vol_l, vol_r), AK4376_AMP_VOLUME_STEP);
        amp = MIN(amp, AK4376_AMP_VOLUME_MAX);
        amp = MAX(amp, AK4376_AMP_VOLUME_MIN);

        /* Other controls are stereo */
        calc_volumes(vol_l - amp, &dig_l, &sw_l);
        calc_volumes(vol_r - amp, &dig_r, &sw_r);
    }

    ak4376_write(AK4376_REG_LCH_VOLUME, dig_vol_to_hw(dig_l) | (1 << 7));
    ak4376_write(AK4376_REG_RCH_VOLUME, dig_vol_to_hw(dig_r));
    ak4376_write(AK4376_REG_AMP_VOLUME, amp_vol_to_hw(amp));
    pcm_set_master_volume(sw_l, sw_r);
}

void ak4376_set_filter_roll_off(int val)
{
    int reg = ak4376_read(AK4376_REG_FILTER);
    reg &= ~0xc0;
    reg |= (val & 3) << 6;
    ak4376_write(AK4376_REG_FILTER, reg);
}

void ak4376_set_freqmode(int fsel, int mult, int power_mode)
{
    /* Calculate clock mode for frequency. Multipliers of 32/64 are only
     * for rates >= 256 KHz which are not supported by Rockbox, so they
     * are commented out -- but they're in the correct place. */
    int clock_mode = ak4376_fsel_to_hw[fsel];
    switch(mult) {
    /* case 32: */
    case 256:
        break;
    /* case 64: */
    case 512:
        clock_mode |= 0x20;
        break;
    case 1024:
        clock_mode |= 0x40;
        break;
    case 128:
        clock_mode |= 0x60;
        break;
    default:
        panicf("ak4376: bad master clock multiple %d", mult);
        return;
    }

    /* Handle the DSMLP bit in the MODE_CTRL register */
    int mode_ctrl = 0x00;
    if(power_mode == SOUND_LOW_POWER || hw_freq_sampr[fsel] <= SAMPR_12)
        mode_ctrl |= 0x40;

    /* Handle the LPMODE bit */
    int pwr3 = power_mode == SOUND_LOW_POWER ? 0x11 : 0x01;

    /* The datasheet says the HP amp must be powered down before changing
     * the operating mode of the DAC or HP amp. I'm assuming this means
     * the amp must be shut down when changing DSMLP or LPMODE. */
    int cur_mode_ctrl = ak4376_read(AK4376_REG_MODE_CTRL);
    int cur_pwr3 = ak4376_read(AK4376_REG_PWR3);
    bool disable_amp = mode_ctrl != cur_mode_ctrl || pwr3 != cur_pwr3;

    /* Program the new settings */
    if(disable_amp)
        ak4376_write(AK4376_REG_PWR4, 0x00);

    ak4376_write(AK4376_REG_CLOCK_MODE, clock_mode);
    ak4376_write(AK4376_REG_MODE_CTRL, mode_ctrl);
    ak4376_write(AK4376_REG_PWR3, pwr3);

    if(disable_amp) {
        ak4376_write(AK4376_REG_PWR4, 0x03);
        mdelay(26);
    }
}
