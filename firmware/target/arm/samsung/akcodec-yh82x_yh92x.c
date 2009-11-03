/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2009 Mark Arigo
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
#include "audiohw.h"
#include "i2c-pp.h"
#include "i2s.h"
#include "akcodec.h"

/*#define LOGF_ENABLE*/
#include "logf.h"

#define I2C_AUDIO_ADDRESS 0x10

/*
 * Initialise the PP I2C and I2S.
 */
void audiohw_init(void)
{
    unsigned long tmp;

    logf("audiohw_init");

    /* I2S device enable */
    DEV_EN |= (DEV_I2S | DEV_EXTCLOCKS);

    /* I2S device reset */
    DEV_RS |=  DEV_I2S;
    asm volatile ("nop\n");
    DEV_RS &= ~DEV_I2S;

    DEV_INIT1 &= ~0x3000000;

    tmp = DEV_INIT1;
    DEV_INIT1 = tmp;

    DEV_INIT2 &= ~0x100;

    /* reset the I2S controller into known state */
    i2s_reset();

    /* this gpio pin maybe powers the codec chip */
    GPIOB_ENABLE     |=  0x01;
    GPIOB_OUTPUT_EN  |=  0x01;
    GPIOB_OUTPUT_VAL |=  0x01;

    GPIOL_ENABLE     |=  0x20;
    GPIOL_OUTPUT_VAL &= ~0x20;
    GPIOL_OUTPUT_EN  |=  0x20;

#ifdef SAMSUNG_YH920
    GPO32_ENABLE     |=  0x00000002;
    GPO32_VAL        &= ~0x00000002;
#endif

    GPO32_VAL        |=  0x00000020;
    GPO32_ENABLE     |=  0x00000020;

    GPIOF_ENABLE     |=  0x80;
    GPIOF_OUTPUT_VAL |=  0x80;
    GPIOF_OUTPUT_EN  |=  0x80;

    audiohw_preinit();

    GPIOL_ENABLE     |=  0x20;
    GPIOL_OUTPUT_VAL |=  0x20;
    GPIOL_OUTPUT_EN  |=  0x20;
}

void akcodec_close(void)
{
    GPIOF_ENABLE     |=  0x80;
    GPIOF_OUTPUT_VAL &= ~0x80;
    GPIOF_OUTPUT_EN  |=  0x80;

    GPIOB_ENABLE     |=  0x01;
    GPIOB_OUTPUT_EN  |=  0x01;
    GPIOB_OUTPUT_VAL |=  0x01;
}

void akcodec_write(int reg, int data)
{
    pp_i2c_send(I2C_AUDIO_ADDRESS, reg, data);
}

int akcodec_read(int reg)
{
    return i2c_readbyte(I2C_AUDIO_ADDRESS, reg);
}
