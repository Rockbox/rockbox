/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
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

#define LOGF_ENABLE
#include "audiohw.h"
#include "cs43131.h"
#include "generic_i2c.h"
#include "system.h"
#include "kernel.h"
#include "logf.h"

/* Register sequence from the R1 vendor driver; fields from CS43131 DS1155F2. */
struct cs43131_reg {
    uint32_t reg;
    uint8_t  val;
};

static struct cs43131_reg cs43131_regs[] = {
    { CS43131_REG_SYS_CLK_CTL, 0x04 },
    { 0x020052, 0x06 },
    { CS43131_REG_SP_SRATE, 0x01 },
    { CS43131_REG_SP_BITSIZE, 0x00 },
    { 0x05000a, 0x07 },
    { 0x05000b, 0x0f },
    { 0x06000a, 0x07 },
    { 0x06000b, 0x0f },
    { CS43131_REG_DSD_VOL_B, 0x00 },
    { CS43131_REG_DSD_VOL_A, 0x00 },
    { CS43131_REG_PATH_SEL, 0x02 },
    { 0x070006, 0x40 },
    { 0x090000, 0x02 },
    { CS43131_REG_PCM_VOL_B, 0x00 },
    { CS43131_REG_PCM_VOL_A, 0x00 },
    { CS43131_REG_PCM_PATH_CTL1, 0x93 },
    { 0x090004, 0x00 },
    { 0x0b0000, 0x06 },
    { 0x080000, 0x31 },
    { 0x070002, 0xa0 },
    /* Pop-free power-up settings, DS1155F2 section 5.7.1. */
    { 0x010010, 0x99 },
    { 0x080032, 0x20 },
    { CS43131_REG_PWDN_CTL, 0xbe },
    { CS43131_REG_PWDN_CTL, 0xae },
    { 0x09000a, 0xf0 },
    { 0x09000b, 0x0c },
};

static const struct {
    uint32_t rate;
    uint8_t  code;
} cs43131_srate_tbl[] = {
    {  32000, 0 },
    {  44100, 1 },
    {  48000, 2 },
    {  88200, 3 },
    {  96000, 4 },
    { 176400, 5 },
    { 192000, 6 },
    { 352800, 7 },
    { 384000, 8 },
};

static int cs_bus = -1;
static bool cs_open;
static bool cs_muted = true;
static bool cs_mute_requested = true;
static uint8_t cs_volreg_l = CS43131_VOL_SAFE_DEFAULT;
static uint8_t cs_volreg_r = CS43131_VOL_SAFE_DEFAULT;

static unsigned cs_writes;
static unsigned cs_naks;
static int cs_first_err;

static int cs43131_write_hw(uint32_t reg, uint8_t val)
{
    uint8_t buf[5];

    if(cs_bus < 0)
        return -1;

    buf[0] = (reg >> 16) & 0xff;
    buf[1] = (reg >> 8) & 0xff;
    buf[2] = reg & 0xff;
    buf[3] = 0x00;
    buf[4] = val;


    int rc = i2c_write_data(cs_bus, CS43131_I2C_ADDR << 1, -1, buf, sizeof(buf));

    cs_writes++;
    if(rc < 0) {
        cs_naks++;
        if(cs_first_err == 0)
            cs_first_err = rc;
    }

    return rc;
}

void cs43131_bus_status(unsigned* writes, unsigned* naks, int* first_err)
{
    if(writes)    *writes    = cs_writes;
    if(naks)      *naks      = cs_naks;
    if(first_err) *first_err = cs_first_err;
}

static int cs43131_read_hw(uint32_t reg)
{
    uint8_t buf[4] = {reg >> 16, reg >> 8, reg, 0};
    int rc = i2c_write_data(cs_bus, CS43131_I2C_ADDR << 1, -1, buf, 4);
    if(rc < 0)
        return rc;
    rc = i2c_read_data(cs_bus, CS43131_I2C_ADDR << 1, -1, buf, 1);
    return rc < 0 ? rc : buf[0];
}

static void cs43131_shadow_poke(uint32_t reg, uint8_t val)
{
    for(unsigned i = 0; i < ARRAYLEN(cs43131_regs); ++i)
        if(cs43131_regs[i].reg == reg)
            cs43131_regs[i].val = val;
}

static void cs43131_power_off(void)
{
    cs43131_set_reset_pin(0);
    cs43131_set_power_pin(0);
    cs_open = false;
    cs_muted = true;
}

static uint8_t cs43131_vol_to_reg(int vol)
{
    vol = MAX(CS43131_VOLUME_MIN, MIN(vol, CS43131_VOLUME_MAX));
    return -vol / CS43131_VOLUME_STEP;
}

static void cs43131_seed_volume_shadow(void)
{
    cs43131_shadow_poke(CS43131_REG_PCM_VOL_A, cs_volreg_l);
    cs43131_shadow_poke(CS43131_REG_PCM_VOL_B, cs_volreg_r);
    cs43131_shadow_poke(CS43131_REG_DSD_VOL_A, cs_volreg_l);
    cs43131_shadow_poke(CS43131_REG_DSD_VOL_B, cs_volreg_r);
}

void cs43131_set_volume(int vol_l, int vol_r)
{
    cs_volreg_l = cs43131_vol_to_reg(vol_l);
    cs_volreg_r = cs43131_vol_to_reg(vol_r);
    cs43131_seed_volume_shadow();

    if(!cs_open)
        return;

    if(cs43131_write_hw(CS43131_REG_PCM_VOL_A, cs_volreg_l) < 0 ||
       cs43131_write_hw(CS43131_REG_DSD_VOL_A, cs_volreg_l) < 0 ||
       cs43131_write_hw(CS43131_REG_PCM_VOL_B, cs_volreg_r) < 0 ||
       cs43131_write_hw(CS43131_REG_DSD_VOL_B, cs_volreg_r) < 0)
        cs43131_power_off();
}

/* The target closes the DAC before changing its external MCLK. */
void cs43131_set_frequency(int sampr)
{
    uint8_t code = 1;
    for(unsigned i = 0; i < ARRAYLEN(cs43131_srate_tbl); ++i)
        if(cs43131_srate_tbl[i].rate == (uint32_t)sampr)
            code = cs43131_srate_tbl[i].code;

    cs43131_shadow_poke(CS43131_REG_SP_SRATE, code);
    cs43131_shadow_poke(CS43131_REG_SYS_CLK_CTL,
                        sampr % 44100 == 0 ? 0x04 : 0x00);
}

void cs43131_init(int i2c_bus)
{
    cs_bus = i2c_bus;
    cs_mute_requested = true;
    cs43131_seed_volume_shadow();
    cs43131_power_off();
}

bool cs43131_open(void)
{
    if(cs_open)
        return true;

    cs43131_set_power_pin(1);
    mdelay(10);
    cs43131_set_reset_pin(1);
    mdelay(10);

    /* Program volume and rate before releasing headphone power-down. */
    cs43131_seed_volume_shadow();
    for(unsigned i = 0; i < ARRAYLEN(cs43131_regs); ++i) {
        if(cs43131_write_hw(cs43131_regs[i].reg, cs43131_regs[i].val) < 0)
            goto error;
    }

    mdelay(12); /* Headphone power-up settling time, DS1155F2 section 5.7.1. */
    if(cs43131_write_hw(0x080032, 0x00) < 0 ||
       cs43131_write_hw(0x010010, 0x00) < 0)
        goto error;

    cs_open = true;
    cs_muted = true;
    cs43131_mute(cs_mute_requested);
    if(cs_muted != cs_mute_requested)
        goto error;
    return true;

error:
    cs43131_power_off();
    logf("cs43131: init failed");
    return false;
}

void cs43131_close(void)
{
    if(cs_open) {
        cs43131_write_hw(CS43131_REG_PCM_PATH_CTL1, 0x93);
        /* PDN_DONE is latched only when its interrupt is unmasked. */
        if(cs43131_write_hw(CS43131_REG_INT_MASK1, 0xfe) >= 0 &&
           cs43131_read_hw(CS43131_REG_INT_STATUS1) >= 0 &&
           cs43131_write_hw(CS43131_REG_PWDN_CTL, CS43131_PWDN_DOWN) >= 0) {
            for(int i = 0; i < 100; ++i) {
                int status = cs43131_read_hw(CS43131_REG_INT_STATUS1);
                if(status < 0 || (status & 1))
                    break;
                mdelay(1);
            }
        }
    }

    /* Reset also makes it safe to remove MCLK after a failed I2C transfer. */
    cs43131_power_off();
}

void cs43131_mute(bool mute)
{
    cs_mute_requested = mute;
    if(!cs_open || cs_muted == mute)
        return;

    if(cs43131_write_hw(CS43131_REG_PCM_PATH_CTL1, mute ? 0x93 : 0x90) >= 0)
        cs_muted = mute;
}

int cs43131_read_shadow(uint32_t reg)
{
    for(unsigned i = 0; i < ARRAYLEN(cs43131_regs); ++i)
        if(cs43131_regs[i].reg == reg)
            return cs43131_regs[i].val;
    return -1;
}
