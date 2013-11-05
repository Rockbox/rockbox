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
#include "pcm1792.h"
#include "config.h"
#include "audio.h"
#include "audiohw.h"

/* pcm1792 registers initial values */
#define REG18_INIT_VALUE (PCM1792_ATLD_OFF|PCM1792_FMT_16_I2S| \
                          PCM1792_DMF_DISABLE|PCM1792_DME_OFF)

#define REG19_INIT_VALUE (PCM1792_REV_ON|PCM1792_ATS_DIV1| \
                          PCM1792_OPE_ON|PCM1792_DFMS_MONO| \
                          PCM1792_FLT_SHARP|PCM1792_INZD_OFF)

#define REG20_INIT_VALUE (PCM1792_SRST_NORMAL|PCM1792_DSD_OFF|\
                          PCM1792_DFTH_ENABLE|PCM1792_STEREO|\
                          PCM1792_OS_64)

#define REG21_INIT_VALUE (PCM1792_DZ_DISABLE|PCM1792_PCMZ_ON)


static void pcm1792_write_reg(const int reg, const unsigned int value)
{
    int i;

    pcm1792_set_ml_dir(0);
    pcm1792_set_md(1);
    pcm1792_set_mc(0);
    pcm1792_set_ml(0);

    for (i = (1<<15); i; i >>= 1) {
        udelay(40);
        pcm1792_set_mc(0);

        if ((reg|value) & i) {
            pcm1792_set_md(1);
        } else {
            pcm1792_set_md(0);
        }

        udelay(40);
        pcm1792_set_mc(1);
    }

    udelay(40);
    pcm1792_set_ml(1);
    pcm1792_set_mc(0);
    udelay(130);
    pcm1792_set_md(1);
}

static int vol_tenthdb2hw(const int tdb)
{
    if (tdb < PCM1792_VOLUME_MIN) {
        return 0;
    } else if (tdb > PCM1792_VOLUME_MAX) {
        return 0xff;
    } else {
        return (tdb/5+0xff);
    }
}


void audiohw_init(void)
{
    pcm1792_write_reg(PCM1792_REG(18), REG18_INIT_VALUE);
    pcm1792_write_reg(PCM1792_REG(19), REG19_INIT_VALUE);
    pcm1792_write_reg(PCM1792_REG(20), REG20_INIT_VALUE);
    pcm1792_write_reg(PCM1792_REG(21), REG21_INIT_VALUE);

    /* Left & Right volumes */
    pcm1792_write_reg(PCM1792_REG(16), 0xff);
    pcm1792_write_reg(PCM1792_REG(17), 0xff);
}

void audiohw_mute(void)
{
    pcm1792_write_reg(PCM1792_REG(18), REG18_INIT_VALUE|PCM1792_MUTE_ON);
}

void audiohw_unmute(void)
{
    pcm1792_write_reg(PCM1792_REG(18), REG18_INIT_VALUE);
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
    pcm1792_write_reg(PCM1792_REG(16), vol_tenthdb2hw(vol_l));
    pcm1792_write_reg(PCM1792_REG(17), vol_tenthdb2hw(vol_r));
}

void audiohw_set_filter_roll_off(int value)
{
    if (value == 0) {
        pcm1792_write_reg(PCM1792_REG(19),
                          REG19_INIT_VALUE
                          |PCM1792_FLT_SHARP);
    } else {
        pcm1792_write_reg(PCM1792_REG(19),
                          REG19_INIT_VALUE
                          |PCM1792_FLT_SLOW);
    }
}
