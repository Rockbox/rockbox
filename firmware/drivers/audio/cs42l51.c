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

#include "audiohw.h"
#include "system.h"
#include "logf.h"
#include "i2c-async.h"
#include "i2c-target.h"

/* Driver for the Cirrus Logic CS42L51 DAC path. ADC/PGA/mic paths are
 * left powered down.
 *
 * The init sequence follows what the Agptek Rocker OF kernel does:
 * - pulse PDN in POWER_CTL1
 * - DAC interface I2S, slave
 * - DAC data from the signal processing engine (needed for AOUTx volume)
 * - HP_GAIN = 110b
 * - PCMA/PCMB mixer volume 0 dB, unmuted
 */

int cs42l51_read(int reg)
{
    return i2c_reg_read1(CS42L51_BUS, CS42L51_ADDR, reg);
}

int cs42l51_write(int reg, int value)
{
    return i2c_reg_write1(CS42L51_BUS, CS42L51_ADDR, reg, value);
}

static int cs42l51_modify(int reg, int clr, int set)
{
    return i2c_reg_modify1(CS42L51_BUS, CS42L51_ADDR, reg, clr, set, NULL);
}

int cs42l51_open(void)
{
    int id = cs42l51_read(CS42L51_REG_CHIP_ID);
    if(id < 0 || (id & CS42L51_CHIP_ID_MASK) != CS42L51_CHIP_ID) {
        logf("cs42l51: bad chip id %d", id);
        return -1;
    }

    /* Hold the device in power down while configuring it */
    cs42l51_write(CS42L51_REG_POWER_CTL1, CS42L51_POWER_CTL1_PDN);

    /* Mics and mic bias off, single speed mode until set_frequency() */
    cs42l51_write(CS42L51_REG_MIC_POWER_CTL, 0x2e);

    /* Slave, DAC interface I2S */
    cs42l51_modify(CS42L51_REG_INTF_CTL,
                   CS42L51_INTF_CTL_MASTER | CS42L51_DAC_DIF_MASK,
                   CS42L51_DAC_DIF_I2S);

    /* DAC fed from signal processing engine, no soft ramp / zero cross */
    cs42l51_modify(CS42L51_REG_DAC_CTL,
                   CS42L51_DATA_SEL_MASK | CS42L51_DAC_SZC_MASK,
                   CS42L51_DATA_SEL_DSP | CS42L51_DAC_AMUTE);

    /* Headphone gain as in the OF; start muted */
    cs42l51_modify(CS42L51_REG_DAC_OUT_CTL, CS42L51_HP_GAIN_MASK,
                   CS42L51_HP_GAIN_OF |
                   CS42L51_DACA_MUTE | CS42L51_DACB_MUTE);

    /* PCM mixer: 0 dB and unmuted (the OF writes 0x00 to both) */
    cs42l51_write(CS42L51_REG_PCMA_VOL, 0x00);
    cs42l51_write(CS42L51_REG_PCMB_VOL, 0x00);

    /* Power up, ADC/PGA stay powered down.
     * TODO: check which PDN_xxx bits the OF leaves set. */
    cs42l51_write(CS42L51_REG_POWER_CTL1, 0x1e);
    mdelay(10);
    return 0;
}

void cs42l51_close(void)
{
    cs42l51_set_mute(true);
    cs42l51_write(CS42L51_REG_POWER_CTL1, 0x7f);
}

void cs42l51_set_frequency(unsigned long sampr)
{
    int speed;
    if(sampr <= 12500)
        speed = CS42L51_SPEED_QSM;
    else if(sampr <= 25000)
        speed = CS42L51_SPEED_HSM;
    else if(sampr <= 50000)
        speed = CS42L51_SPEED_SSM;
    else
        speed = CS42L51_SPEED_DSM;

    cs42l51_modify(CS42L51_REG_MIC_POWER_CTL, CS42L51_SPEED_MASK, speed);
}

static int vol_to_hw(int vol)
{
    /* 0.5 dB per LSB, two's complement */
    if(vol < CS42L51_VOLUME_MIN)
        vol = CS42L51_VOLUME_MIN;
    if(vol > CS42L51_VOLUME_MAX)
        vol = CS42L51_VOLUME_MAX;

    return (vol / 5) & 0xff;
}

void cs42l51_set_volume(int vol_l, int vol_r)
{
    cs42l51_write(CS42L51_REG_AOUTA_VOL, vol_to_hw(vol_l));
    cs42l51_write(CS42L51_REG_AOUTB_VOL, vol_to_hw(vol_r));

    int mute = 0;
    if(vol_l <= CS42L51_VOLUME_MUTE)
        mute |= CS42L51_DACA_MUTE;
    if(vol_r <= CS42L51_VOLUME_MUTE)
        mute |= CS42L51_DACB_MUTE;

    cs42l51_modify(CS42L51_REG_DAC_OUT_CTL,
                   CS42L51_DACA_MUTE | CS42L51_DACB_MUTE, mute);
}

void cs42l51_set_mute(bool mute)
{
    cs42l51_modify(CS42L51_REG_DAC_OUT_CTL,
                   CS42L51_DACA_MUTE | CS42L51_DACB_MUTE,
                   mute ? (CS42L51_DACA_MUTE | CS42L51_DACB_MUTE) : 0);
}
