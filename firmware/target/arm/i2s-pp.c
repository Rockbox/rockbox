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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "system.h"

/* TODO: Add in PP5002 defs */
#if CONFIG_CPU == PP5002
void i2s_reset(void)
{
    /* I2S device reset */
    DEV_RS |= 0x80;
    DEV_RS &= ~0x80;

    /* I2S controller enable */
    IISCONFIG |= 1;

    /* BIT.FORMAT [11:10] = I2S (default) */
    /* BIT.SIZE [9:8] = 24bit */
    /* FIFO.FORMAT = 24 bit LSB */

    /* reset DAC and ADC fifo */
    IISFIFO_CFG |= 0x30000;
}
#else /* PP502X */

/* Data format on the I2S bus */
#define FORMAT_MASK (0x3 << 10)
#define FORMAT_I2S  (0x00 << 10)
/* Other formats not yet known */

/* Data size on I2S bus */
#define SIZE_MASK   (0x3 << 8)
#define SIZE_16BIT  (0x00 << 10)
/* Other sizes not yet known */

/* Data size/format on I2S FIFO */
#define FIFO_FORMAT_MASK (0x7 << 4)
#define FIFO_FORMAT_32LSB (0x03 << 4)
/* Other formats not yet known */

/* Are we I2S Master or slave? */
#define I2S_MASTER (1<<25)

#define I2S_RESET (0x1 << 31)

/*
 * Reset the I2S BIT.FORMAT I2S, 16bit, FIFO.FORMAT 32bit
 */
void i2s_reset(void)
{
    /* I2S soft reset */
    IISCONFIG |= I2S_RESET;
    IISCONFIG &= ~I2S_RESET;

    /* BIT.FORMAT */
    IISCONFIG = ((IISCONFIG & ~FORMAT_MASK) | FORMAT_I2S);

    /* BIT.SIZE */
    IISCONFIG = ((IISCONFIG & ~SIZE_MASK) | SIZE_16BIT);

    /* FIFO.FORMAT */
    /* If BIT.SIZE < FIFO.FORMAT low bits will be 0 */
    IISCONFIG = ((IISCONFIG & ~FIFO_FORMAT_MASK) | FIFO_FORMAT_32LSB);

    /* RX_ATN_LVL=1 == when 12 slots full */
    /* TX_ATN_LVL=1 == when 12 slots empty */
    IISFIFO_CFG |= 0x33;

    /* Rx.CLR = 1, TX.CLR = 1 */
    IISFIFO_CFG |= 0x1100;
}
#endif
