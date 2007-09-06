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
#include "cpu.h"

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

/* All I2S formats send MSB first */

/* Data format on the I2S bus */
#define FORMAT_MASK  (0x3 << 10)
#define FORMAT_I2S   (0x0 << 10) /* Standard I2S - leading dummy bit */
#define FORMAT_1     (0x1 << 10)
#define FORMAT_LJUST (0x2 << 10) /* Left justified - no dummy bit */
#define FORMAT_3     (0x3 << 10)
/* Other formats not yet known  */

/* Data size on I2S bus */
#define SIZE_MASK   (0x3 << 8)
#define SIZE_16BIT  (0x0 << 8)
/* Other sizes not yet known */

/* Data size/format on I2S FIFO */
#define FIFO_FORMAT_MASK    (0x7 << 4)
#define FIFO_FORMAT_0       (0x0 << 4)
/* Big-endian formats - data sent to the FIFO must be big endian.
 * I forgot which is which size but did test them. */
#define FIFO_FORMAT_1       (0x1 << 4)
#define FIFO_FORMAT_2       (0x2 << 4)
 /* 32bit-MSB-little endian */
#define FIFO_FORMAT_LE32 (0x3 << 4)
 /* 16bit-MSB-little endian */
#define FIFO_FORMAT_LE16 (0x4 << 4)

/* FIFO formats 0x5 and above seem equivalent to 0x4 ?? */

/**
 * PP502x
 *
 * IISCONFIG bits:
 * |   31   |   30   |   29   |   28   |   27   |   26   |   25   |   24   |
 * | RESET  |        |TXFIFOEN|RXFIFOEN|        |  ????  |   MS   |  ????  |
 * |   23   |   22   |   21   |   20   |   19   |   18   |   17   |   16   |
 * |        |        |        |        |        |        |        |        |
 * |   15   |   14   |   13   |   12   |   11   |   10   |    9   |    8   |
 * |        |        |        |        | Bus Format[1:0] |     Size[1:0]   |
 * |    7   |    6   |    5   |    4   |    3   |    2   |    1   |    0   |
 * |        |     Size Format[2:0]     |  ????  |  ????  | IRQTX  | IRQRX  |
 *
 * IISFIFO_CFG bits:
 * |   31   |   30   |   29   |   28   |   27   |   26   |   25   |   24   |
 * |        |                         Free[6:0]                            |
 * |   23   |   22   |   21   |   20   |   19   |   18   |   17   |   16   |
 * |        |        |        |        |        |        |        |        |
 * |   15   |   14   |   13   |   12   |   11   |   10   |    9   |    8   |
 * |        |        |        | RXCLR  |        |        |        | TXCLR  |
 * |    7   |    6   |    5   |    4   |    3   |    2   |    1   |    0   |
 * |        |        |   RX_ATN_LEVEL  |        |        |   TX_ATN_LEVEL  |
 */

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
#ifdef HAVE_AS3514
    /* AS3514 can only operate as I2S Slave */
    IISCONFIG |= I2S_MASTER;
    /* Set I2S to 44.1kHz */
    outl((inl(0x70002808) & ~(0x1ff)) | 33, 0x70002808);
    outl(7, 0x60006080);

    IISCONFIG = ((IISCONFIG & ~FIFO_FORMAT_MASK) | FIFO_FORMAT_LE16);
#else
    IISCONFIG = ((IISCONFIG & ~FIFO_FORMAT_MASK) | FIFO_FORMAT_LE32);
#endif

    /* RX_ATN_LVL=1 == when 12 slots full */
    /* TX_ATN_LVL=1 == when 12 slots empty */
    IISFIFO_CFG |= 0x33;

    /* Rx.CLR = 1, TX.CLR = 1 */
    IISFIFO_CFG |= 0x1100;
}

#if defined(SANSA_E200) || defined(SANSA_C200)
void i2s_scale_attn_level(long frequency)
{
    unsigned int iisfifo_cfg = IISFIFO_CFG & ~0xff;

    /* TODO: set this more appropriately for frequency */
    if (frequency <= CPUFREQ_DEFAULT)
    {
        /* when 4 slots full */
        /* when 4 slots empty */
        iisfifo_cfg |= 0x11;
    }
    else if (frequency < CPUFREQ_MAX)
    {
        /* when 8 slots full */
        /* when 8 slots empty */
        iisfifo_cfg |= 0x22;
    }
    else
    {
        /* when 12 slots full */
        /* when 12 slots empty */
        iisfifo_cfg |= 0x33;
    }

    IISFIFO_CFG = iisfifo_cfg;
}
#endif /* SANSA_E200 */

#endif
