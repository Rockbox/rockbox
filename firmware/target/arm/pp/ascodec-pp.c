/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for AS3514 audio codec
 *
 * Copyright (c) 2007 Daniel Ankers
 * Copyright (c) 2007 Christian Gmeiner
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
#include "cpu.h"
#include "system.h"

#include "audiohw.h"
#include "i2s.h"
#include "i2c-pp.h"
#include "ascodec.h"

/*
 * Initialise the PP I2C and I2S.
 */
void audiohw_init(void)
{
    /* normal outputs for CDI and I2S pin groups */
    DEV_INIT2 &= ~0x300;

    /*mini2?*/
    DEV_INIT1 &=~0x3000000;
    /*mini2?*/

    /* I2S device reset */
    DEV_RS |= DEV_I2S;
    DEV_RS &=~DEV_I2S;

    /* I2S device enable */
    DEV_EN |= DEV_I2S;

    /* enable external dev clock clocks */
    DEV_EN |= DEV_EXTCLOCKS;

    /* external dev clock to 24MHz */
    outl(inl(0x70000018) & ~0xc, 0x70000018);

#ifdef SANSA_E200
    /* Prevent pops on startup */
    GPIOG_ENABLE |= 0x08;
    GPIO_SET_BITWISE(GPIOG_OUTPUT_VAL, 0x08);
    GPIOG_OUTPUT_EN |= 0x08;
#endif

    i2s_reset();

    audiohw_preinit();
}

void ascodec_write(unsigned int reg, unsigned int value)
{
    pp_i2c_send(AS3514_I2C_ADDR, reg, value);
}

int ascodec_read(unsigned int reg)
{
    return i2c_readbyte(AS3514_I2C_ADDR, reg);
}

void ascodec_readbytes(unsigned int addr, unsigned int len, unsigned char *data)
{
    i2c_readbytes(AS3514_I2C_ADDR, addr, len, data);
}

void ascodec_lock(void)
{
    i2c_lock();
}

void ascodec_unlock(void) 
{
    i2c_unlock();
}

bool ascodec_chg_status(void)
{
    return ascodec_read(AS3514_IRQ_ENRD0) & CHG_STATUS;
}

bool ascodec_endofch(void)
{
    return ascodec_read(AS3514_IRQ_ENRD0) & CHG_ENDOFCH;
}

void ascodec_monitor_endofch(void)
{
    ascodec_write(AS3514_IRQ_ENRD0, IRQ_ENDOFCH);
}

void ascodec_write_charger(int value)
{
    ascodec_write(AS3514_CHARGER, value);
}

int ascodec_read_charger(void)
{
    return ascodec_read(AS3514_CHARGER);
}

void ascodec_wait_adc_finished(void)
{
    /*
     * FIXME: not implemented
     *
     * If irqs are not available on the target platform,
     * this should be most likely implemented by polling
     * AS3514_IRQ_ENRD2 in the same way powermgmt-ascodec.c
     * is polling IRQ_ENDOFCH.
     */
}
