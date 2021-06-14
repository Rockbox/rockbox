/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2011 by Tomasz Moń
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
#include "config.h"
#include "logf.h"
#include "system.h"
#include "string.h"
#include "audio.h"
#if CONFIG_I2C == I2C_DM320
#include "i2c-dm320.h"
#endif
#include "audiohw.h"

/* (7-bit) address is 0x18, the LSB is read/write flag */
#define AIC3X_ADDR (0x18 << 1)

static char volume_left = 0, volume_right = 0;

/* convert tenth of dB volume to master volume register value */
static int vol_tenthdb2hw(int db)
{
    /* 0 to -63.0dB in 1dB steps, aic3x can goto -63.5 in 0.5dB steps */
    if (db <= -640)
    {
        return 0x7E;
    }
    else if (db >= 0)
    {
        return 0x00;
    }
    else
    {
        return (-((db)/5));
    }
}

static void aic3x_write_reg(unsigned reg, unsigned value)
{
    unsigned char data[2];

    data[0] = reg;
    data[1] = value;

#if CONFIG_I2C == I2C_DM320
    if (i2c_write(AIC3X_ADDR, data, 2) != 0)
#else
    #warning Implement aic3x_write_reg()
#endif
    {
        logf("AIC3X error reg=0x%x", reg);
        return;
    }
}

static unsigned char aic3x_read_reg(unsigned reg)
{
    unsigned char data;

#if CONFIG_I2C == I2C_DM320
    if (i2c_read_bytes(AIC3X_ADDR, reg, &data, 1))
#else
    #warning Implement aic3x_read_reg()
#endif
    {
        logf("AIC3X read error reg=0x%0x", reg);
        data = 0;
    }

    return data;
}

static void aic3x_change_reg(unsigned reg, unsigned char or_mask,
                             unsigned char and_mask)
{
    unsigned char data;

    data = aic3x_read_reg(reg);

    data &= and_mask;
    data |= or_mask;

    aic3x_write_reg(reg, data);
}
                                    
static void aic3x_apply_volume(void)
{
    unsigned char data[3];

#if 0 /* handle page switching once we use first page at all */
    aic3x_write_reg(0, 0); /* switch to page 0 */
#endif

    data[0] = AIC3X_LEFT_VOL;
    data[1] = volume_left;
    data[2] = volume_right;

    /* use autoincrement write */
#if CONFIG_I2C == I2C_DM320
    if (i2c_write(AIC3X_ADDR, data, 3) != 0)
#else
    #warning Implement aic3x_apply_volume()
#endif
    {
        logf("AIC3X error in apply volume");
        return;
    }
}


static void audiohw_mute(bool mute)
{
    if (mute)
    {
        /* DAC_L1 routed to HPLOUT, mute */
        aic3x_write_reg(AIC3X_DAC_L1_VOL, 0xF6);
        /* DAC_R1 routed to HPROUT, mute */
        aic3x_write_reg(AIC3X_DAC_R1_VOL, 0xF6);
        /* DAC_L1 routed to MONO_LOP/M, mute */
        aic3x_write_reg(AIC3X_DAC_L1_MONO_LOP_M_VOL, 0xF6);
        /* DAC_R1 routed to MONO_LOP/M, mute */ 
        aic3x_write_reg(AIC3X_DAC_R1_MONO_LOP_M_VOL, 0xF6);

        volume_left |= 0x80;
        volume_right |= 0x80;
    }
    else
    {
        /* DAC_L1 routed to HPLOUT, volume analog gain 0xC (-6.0dB) */
        aic3x_write_reg(AIC3X_DAC_L1_VOL, 0x8C);
        /* DAC_R1 routed to HPROUT, volume analog gain 0xC (-6.0 dB) */
        aic3x_write_reg(AIC3X_DAC_R1_VOL, 0x8C);
        /* DAC_L1 routed to MONO_LOP/M, gain 0x2 (-1.0dB) */
        aic3x_write_reg(AIC3X_DAC_L1_MONO_LOP_M_VOL, 0x92);
        /* DAC_R1 routed to MONO_LOP/M, gain 0x2 (-1.0dB) */ 
        aic3x_write_reg(AIC3X_DAC_R1_MONO_LOP_M_VOL, 0x92);

        volume_left &= 0x7F;
        volume_right &= 0x7F;
    }

    aic3x_apply_volume();
}

/* public functions */

/**
 * Init our tlv with default values
 */
void audiohw_init(void)
{
    logf("AIC3X init");

    /* Do software reset (self-clearing) */
    aic3x_write_reg(AIC3X_SOFT_RESET, 0x80);

    /* driver power-on time 200 ms, ramp-up step time 4 ms */
    aic3x_write_reg(AIC3X_POP_REDUCT, 0x7C);

    /* Output common-move voltage 1.35V, disable LINE2[LR] bypass */
    /* Output soft-stepping = one step per fs */
    aic3x_write_reg(AIC3X_POWER_OUT, 0x00);

    /* Audio data interface */
    /* GPIO1 used for audio serial data bus ADC word clock */
    aic3x_write_reg(AIC3X_GPIO1_CTRL, 0x10);
    /* BCLK and WCLK are outputs (master mode) */
    aic3x_write_reg(AIC3X_DATA_REG_A, 0xC0);
    /* right-justified mode */
    aic3x_write_reg(AIC3X_DATA_REG_B, 0x80);
    /* data offset = 0 clocks */
    aic3x_write_reg(AIC3X_DATA_REG_C, 0);

    /* Left DAC plays left channel, Right DAC plays right channel */ 
    aic3x_write_reg(AIC3X_DATAPATH, 0xA);

    /* power left and right DAC, HPLCOM constant VCM output */
    aic3x_write_reg(AIC3X_DAC_POWER, 0xD0);
    /* HPRCOM as constant VCM output. Enable short-circuit protection
       (limit current) */
    aic3x_write_reg(AIC3X_HIGH_POWER, 0xC);

    /* DAC_L1 routed to HPLOUT */
    aic3x_write_reg(AIC3X_DAC_L1_VOL, 0x80);
    /* DAC_R1 routed to HPROUT */
    aic3x_write_reg(AIC3X_DAC_R1_VOL, 0x80);

    /* DAC_L1 routed to MONO_LOP/M */
    aic3x_write_reg(AIC3X_DAC_L1_MONO_LOP_M_VOL, 0x80);
    /* DAC_R1 routed to MONO_LOP/M */
    aic3x_write_reg(AIC3X_DAC_R1_MONO_LOP_M_VOL, 0x80);

    /* DAC_L1 routed to LEFT_LOP/M */
    aic3x_write_reg(AIC3X_DAC_L1_LEFT_LOP_M_VOL, 0x80);
    /* DAC_R1 routed to RIGHT_LOP/M */
    aic3x_write_reg(AIC3X_DAC_R1_RIGHT_LOP_M_VOL, 0x80);

    /* LEFT_LOP/M output level 0dB, not muted */
    aic3x_write_reg(AIC3X_LEFT_LOP_M_LVL, 0x8);
    /* RIGHT_LOP/M output level 0dB, not muted */
    aic3x_write_reg(AIC3X_RIGHT_LOP_M_LVL, 0x8);

    /* Enable PLL. Set Q=16, P=1 */
    aic3x_write_reg(AIC3X_PLL_REG_A, 0x81);
    /* PLL J = 53 */
    aic3x_write_reg(AIC3X_PLL_REG_B, 0xD4);
    /* PLL D = 5211 */
    aic3x_write_reg(AIC3X_PLL_REG_C, 0x51); 
    aic3x_write_reg(AIC3X_PLL_REG_D, 0x6C);
    /* PLL R = 1 */
    aic3x_write_reg(AIC3X_OVERFLOW, 0x01);

    /* ADC fs = fs(ref)/5.5; DAC fs = fs(ref) */
    aic3x_write_reg(AIC3X_SMPL_RATE, 0x90);

    /* HPLOUT output level 0dB, muted, high impedance */
    aic3x_write_reg(AIC3X_HPLOUT_LVL, 0x04);
    /* HPROUT output level 0dB, muted, high impedance */
    aic3x_write_reg(AIC3X_HPROUT_LVL, 0x04);

    /* HPLCOM is high impedance when powered down, not fully powered up */
    aic3x_write_reg(AIC3X_HPLCOM_LVL, 0x04);
}

void audiohw_postinit(void)
{
    audiohw_mute(false);

    /* HPLOUT output level 0dB, not muted, fully powered up */
    aic3x_write_reg(AIC3X_HPLOUT_LVL, 0x09);
    /* HPROUT output level 0dB, not muted, fully powered up */
    aic3x_write_reg(AIC3X_HPROUT_LVL, 0x09);

    /* MONO_LOP output level 6dB, not muted */
    aic3x_write_reg(AIC3X_MONO_LOP_M_LVL, 0x69);

    /* PGA_R is not routed to MONO_LOP/M, analog gain -52.7dB */
    aic3x_write_reg(AIC3X_PGA_R_MONO_LOP_M_VOL, 0x69);
}

void audiohw_set_frequency(int fsel)
{
    (void)fsel;
    /* TODO */
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    vol_l = vol_tenthdb2hw(vol_l);
    vol_r = vol_tenthdb2hw(vol_r);

    if ((volume_left & 0x7F) == (vol_l & 0x7F) &&
        (volume_right & 0x7F) == (vol_r & 0x7F))
    {
        /* Volume already set to this value */
        return;
    }

    volume_left &= 0x80; /* preserve mute bit */
    volume_left |= (vol_l & 0x7F); /* set gain */

    volume_right &= 0x80; /* preserve mute bit */
    volume_right |= (vol_r & 0x7F); /* set gain */

    aic3x_apply_volume();
}

/* Nice shutdown of AIC3X codec */
void audiohw_close(void)
{
    /* HPLOUT, HPROUT, HPLCOM not fully powered up */
    aic3x_change_reg(AIC3X_HPLOUT_LVL, 0x00, 0xFE);
    aic3x_change_reg(AIC3X_HPROUT_LVL, 0x00, 0xFE);
    aic3x_change_reg(AIC3X_HPLCOM_LVL, 0x00, 0xFC);

    /* MONO_LOP/M, LEFT_LOP/M, RIGHT_LOP/M muted, not fully powered up */
    aic3x_change_reg(AIC3X_MONO_LOP_M_LVL, 0x00, 0xF6);
    aic3x_change_reg(AIC3X_LEFT_LOP_M_LVL, 0x00, 0xF6);
    aic3x_change_reg(AIC3X_RIGHT_LOP_M_LVL, 0x00, 0xF6);

    /* Power down left and right DAC */
    aic3x_change_reg(AIC3X_DAC_POWER, 0x00, 0x30);

    /* Disable PLL */
    aic3x_change_reg(AIC3X_PLL_REG_A, 0x00, 0x7F);
}

void aic3x_switch_output(bool stereo)
{
    if (stereo)
    {
        /* MONO_LOP/M not fully powered up */
        aic3x_change_reg(AIC3X_MONO_LOP_M_LVL, 0x00, 0xFE);
        /* HPLOUT fully powered up */
        aic3x_change_reg(AIC3X_HPLOUT_LVL, 0x01, 0xFF);
        /* HPROUT fully powered up */
        aic3x_change_reg(AIC3X_HPROUT_LVL, 0x01, 0xFF);
        /* HPLCOM fully powered up */
        aic3x_change_reg(AIC3X_HPLCOM_LVL, 0x01, 0xFF);
    }
    else
    {
        /* MONO_LOP/M fully powered up */
        aic3x_change_reg(AIC3X_MONO_LOP_M_LVL, 0x01, 0xFF);
        /* HPLOUT not fully powered up */
        aic3x_change_reg(AIC3X_HPLOUT_LVL, 0x00, 0xFE);
        /* HPROUT not fully powered up */
        aic3x_change_reg(AIC3X_HPROUT_LVL, 0x00, 0xFE);
        /* HPLCOM not fully powered up */
        aic3x_change_reg(AIC3X_HPLCOM_LVL, 0x00, 0xFE);
    }
}

