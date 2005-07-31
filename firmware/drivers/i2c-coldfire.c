/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Andy Young
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "cpu.h"
#include "kernel.h"
#include "debug.h"
#include "system.h"
#include "i2c-coldfire.h"

#define I2C_DEVICE_1    ((volatile unsigned char *)&MADR)
#define I2C_DEVICE_2    ((volatile unsigned char *)&MADR2)

/* Local functions definitions */

static int i2c_write_byte(int device, unsigned char data);
static int i2c_gen_start(int device);
static void i2c_gen_stop(int device);
static volatile unsigned char *i2c_get_addr(int device);

/* Public functions */

void i2c_init(void)
{
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
    /* Audio Codec */
    MADR = 0x6c; /* iRiver firmware uses this addr */
    MBDR = 0;    /* iRiver firmware does this */
    MBCR = IEN;  /* Enable interface */

#if 0
    /* FM Tuner */
    MADR2 = 0x6c;
    MBDR2 = 0;
    MBCR2 = IEN;
#endif

#endif
}

void i2c_close(void)
{
    MBCR  = 0;
    MBCR2 = 0;
}

/**
 * Writes bytes to the selected device.
 *
 * Use device=1 for bus 1 at 0x40000280
 * Use device=2 for bus 2 at 0x80000440
 * 
 * Returns number of bytes successfully send or -1 if START failed
 */
int i2c_write(int device, unsigned char *buf, int count)
{
    int i;
    int rc;

    rc = i2c_gen_start(device);
    if (rc < 0)
    {
        DEBUGF("i2c: gen_start failed (d=%d)", device);
        return rc*10 - 1;
    }

    for (i=0; i<count; i++)
    {
        rc = i2c_write_byte(device, buf[i]);
        if (rc < 0)
        {
            DEBUGF("i2c: write failed at (d=%d,i=%d)", device, i);
            return rc*10 - 2;
        }
    }

    i2c_gen_stop(device);

    return count;
}

/* Write a byte to the interface, returns 0 on success, -1 otherwise. */
int i2c_write_byte(int device, unsigned char data)
{
    volatile unsigned char *regs = i2c_get_addr(device);

    long count = 0;

    regs[O_MBDR] = data;                /* Write data byte */   

    /* Wait for bus busy */
    while (!(regs[O_MBSR] & IBB) && count < MAX_LOOP)
    {
        yield();
        count++;
    }

    if (count >= MAX_LOOP)
        return -1;

    count = 0;

    /* Wait for interrupt flag */
    while (!(regs[O_MBSR] & IFF) && count < MAX_LOOP)
    {
        yield();
        count++;
    }

    if (count >= MAX_LOOP)
        return -2;

    regs[O_MBSR] &= ~IFF;       /* Clear interrupt flag  */

    if (!(regs[O_MBSR] & ICF))  /* Check that transfer is complete */
        return -3;

    if (regs[O_MBSR] & RXAK)    /* Check that the byte has been ACKed */
        return -4;

    return 0;
}


/* Returns 0 on success, -1 on failure */
int i2c_gen_start(int device)
{
    volatile unsigned char *regs = i2c_get_addr(device);
    long count = 0;

    /* Wait for bus to become free */
    while ((regs[O_MBSR] & IBB) && (count < MAX_LOOP))
    {
        yield();
        count++;
    }

    if (count >= MAX_LOOP)
        return -1;

    regs[O_MBCR] |= MSTA | MTX;         /* Generate START */

    return 0;
}  

void i2c_gen_stop(int device)
{
    volatile unsigned char *regs = i2c_get_addr(device);
    regs[O_MBCR] &= ~MSTA;          /* Clear MSTA to generate STOP */
}


volatile unsigned char *i2c_get_addr(int device)
{
    if (device == 1)
        return I2C_DEVICE_1;

    return I2C_DEVICE_2;
}
