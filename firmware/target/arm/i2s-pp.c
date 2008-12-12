/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Portalplayer specific code for I2S
 *
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in December 2005
 *
 * Original file: linux/arch/armnommu/mach-ipod/audio.c
 *
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
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
#include "cpu.h"
#include "i2s.h"

#if CONFIG_CPU == PP5002
void i2s_reset(void)
{
    /* I2S device reset */
    DEV_RS |= DEV_I2S;
    DEV_RS &= ~DEV_I2S;

    /* I2S controller enable */
    IISCONFIG |= IIS_ENABLE;

    /* reset DAC and ADC fifo */
    IISFIFO_CFG |= IIS_RXCLR | IIS_TXCLR;
}
#else /* PP502X */

/*
 * Reset the I2S BIT.FORMAT I2S, 16bit, FIFO.FORMAT 32bit
 */
void i2s_reset(void)
{
    /* I2S soft reset */
    IISCONFIG |= IIS_RESET;
    IISCONFIG &= ~IIS_RESET;

    /* BIT.FORMAT */
    IISCONFIG = ((IISCONFIG & ~IIS_FORMAT_MASK) | IIS_FORMAT_IIS);
    /* BIT.SIZE */
    IISCONFIG = ((IISCONFIG & ~IIS_SIZE_MASK) | IIS_SIZE_16BIT);

    /* FIFO.FORMAT */
    /* If BIT.SIZE < FIFO.FORMAT low bits will be 0 */
#ifdef HAVE_AS3514
    /* AS3514 can only operate as I2S Slave */
    IISCONFIG |= IIS_MASTER;
    /* Set I2S to 44.1kHz */
    IISCLK = (IISCLK & ~0x1ff) | 33;
    IISDIV = 7;
#endif /* HAVE_AS3514 */

    IISCONFIG = ((IISCONFIG & ~IIS_FIFO_FORMAT_MASK) | IIS_FIFO_FORMAT_LE16_2);

    /* RX_ATN_LVL = when 12 slots full */
    /* TX_ATN_LVL = when 12 slots empty */
    IISFIFO_CFG |= IIS_RX_FULL_LVL_12 | IIS_TX_EMPTY_LVL_12;

    /* Rx.CLR = 1, TX.CLR = 1 */
    IISFIFO_CFG |= IIS_RXCLR | IIS_TXCLR;
}

#endif /* CONFIG_CPU == */
