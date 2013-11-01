/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 Andrew Ryabinin
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

#include "system.h"
#include "df1704.h"
#include "config.h"
#include "audio.h"
#include "audiohw.h"

static void df1704_write_reg(const int reg, const unsigned int value)
{
    int i;

    df1704_set_ml_dir(0);
    df1704_set_ml(1);
    df1704_set_md(1);
    df1704_set_mc(1);

    for (i = (1<<15); i; i >>= 1) {
        udelay(40);
        df1704_set_mc(0);

        if ((reg|value) & i) {
            df1704_set_md(1);
        } else {
            df1704_set_md(0);
        }

        udelay(40);
        df1704_set_mc(1);
    }

    df1704_set_ml(0);
    udelay(130);
    df1704_set_ml(1);
    udelay(130);
    df1704_set_ml_dir(1);
}

static int vol_tenthdb2hw(const int tdb)
{
    if (tdb < DF1704_VOLUME_MIN) {
        return 0;
    } else if (tdb > DF1704_VOLUME_MAX) {
        return 0xff;
    } else {
        return (tdb/5+0xff);
    }
}


void audiohw_init(void)
{
    df1704_write_reg(DF1704_MODE(2),
                     DF1704_OW_24     |
                     DF1704_IW_16_I2S |
                     DF1704_DEM_OFF   |
                     DF1704_MUTE_OFF);
    df1704_write_reg(DF1704_MODE(3),
                     DF1704_CKO_HALF |
                     DF1704_SRO_SHARP|
                     DF1704_LRP_H    |
                     DF1704_I2S_ON);
}

void audiohw_mute(void)
{
    df1704_write_reg(DF1704_MODE(2),
                     DF1704_OW_24     |
                     DF1704_IW_16_I2S |
                     DF1704_DEM_OFF   |
                     DF1704_MUTE_ON);
}

void audiohw_unmute(void)
{
    df1704_write_reg(DF1704_MODE(2),
                     DF1704_OW_24     |
                     DF1704_IW_16_I2S |
                     DF1704_DEM_OFF   |
                     DF1704_MUTE_OFF);
}

void audiohw_preinit(void)
{
}

void audiohw_set_frequency(int fsel)
{
    (void)fsel;
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    df1704_write_reg(DF1704_MODE(0), DF1704_LDL_ON|vol_tenthdb2hw(vol_l));
    df1704_write_reg(DF1704_MODE(1), DF1704_LDR_ON|vol_tenthdb2hw(vol_r));
}

void audiohw_set_filter_roll_off(int value)
{
    int mode3_val = DF1704_CKO_HALF |
                    DF1704_LRP_H    |
                    DF1704_I2S_ON;

    if (value == 0) {
        mode3_val |= DF1704_SRO_SHARP;
    } else {
        mode3_val |= DF1704_SRO_SLOW;
    }
    df1704_write_reg(DF1704_MODE(3), mode3_val);
}
