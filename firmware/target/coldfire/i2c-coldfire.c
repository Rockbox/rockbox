/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
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
#include "logf.h"
#include "system.h"
#include "i2c-coldfire.h"


/* --- Local functions - declarations --- */

static int i2c_start(volatile unsigned char *iface);
static int i2c_wait_for_slave(volatile unsigned char *iface);
static int i2c_outb(volatile unsigned char *iface, unsigned char byte);
static inline void i2c_stop(volatile unsigned char *iface);


/* --- Public functions - implementation --- */

void i2c_init(void)
{
#ifdef IRIVER_H100_SERIES
    /* The FM chip has no pullup for SCL, so we have to bit-bang the
       I2C for that one. */
    or_l(0x00800000, &GPIO1_OUT);
    or_l(0x00000008, &GPIO_OUT);
    or_l(0x00800000, &GPIO1_ENABLE);
    or_l(0x00000008, &GPIO_ENABLE);
    or_l(0x00800000, &GPIO1_FUNCTION);
    or_l(0x00000008, &GPIO_FUNCTION);
#elif defined(IRIVER_H300_SERIES)
    /* The FM chip has no pullup for SCL, so we have to bit-bang the
       I2C for that one. */
    or_l(0x03000000, &GPIO1_OUT);
    or_l(0x03000000, &GPIO1_ENABLE);
    or_l(0x03000000, &GPIO1_FUNCTION);
#endif
    
    /* I2C Clock divisor = 160 => 124.1556 MHz / 2 / 160 = 388.08 kHz */
    MFDR  = 0x0d;

#if defined(IAUDIO_M3)
    MBCR  = IEN;  /* Enable interface 1 */
    /* secondary channel is handled in the interrupt driven ADC driver */
#elif defined(IAUDIO_X5) || defined(IAUDIO_M5)
    MBCR  = IEN;  /* Enable interface 1 */

    MFDR2 = 0x0d;
    MBCR2 = IEN;  /* Enable interface 2 */
#elif defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
    MBDR = 0;    /* iRiver firmware does this */
    MBCR = IEN;  /* Enable interface */
#endif
}

void i2c_close(void)
{
    MBCR  = 0;
    MBCR2 = 0;
}

/* End I2C session on the given interface. */
static inline void i2c_stop(volatile unsigned char *iface)
{
    iface[O_MBCR] &= ~MSTA;
}

/*
 * Writes bytes to a I2C device.
 *
 * Returns number of bytes successfully sent or a negative value on error.
 */
int i2c_write(volatile unsigned char *iface, unsigned char addr, 
              const unsigned char *buf, int count)
{
    int i, rc;

    if (count <= 0)
        return 0;
   
    rc = i2c_start(iface);
    if (rc < 0)
        return rc;
    
    rc = i2c_outb(iface, addr & 0xfe);
    if (rc < 0)
        return rc;
    
    for (i = 0; i < count; i++)
    {
        rc = i2c_outb(iface, *buf++);
        if (rc < 0)
            return rc;
    }
    i2c_stop(iface);
    
    return count;
}

/*
 * Reads bytes from a I2C device.
 *
 * Returns number of bytes successfully received or a negative value on error.
 */
int i2c_read(volatile unsigned char *iface, unsigned char addr, 
             unsigned char *buf, int count)
{
    int i, rc;

    if (count <= 0)
        return 0;
   
    rc = i2c_start(iface);
    if (rc < 0)
        return rc;

    rc = i2c_outb(iface, addr | 1);
    if (rc < 0)
        return rc;

    /* Switch to Rx mode */
    iface[O_MBCR] &= ~MTX;

    /* Turn on ACK generation if reading multiple bytes */
    if (count > 1)
        iface[O_MBCR] &= ~TXAK;
 
    /* Dummy read */
    rc = (int) iface[O_MBDR];
  
    for (i = count; i > 0; i--)
    {
        rc = i2c_wait_for_slave(iface);
        if (rc < 0)
            return rc;

        if (i == 2)
            /* Don't ACK the last byte to be read from the slave */
            iface[O_MBCR] |= TXAK;
        else if (i == 1)
            /* Generate STOP before reading last byte received */
            i2c_stop(iface);

        *buf++ = iface[O_MBDR];
    }
    return count;
}

/* --- Local functions - implementation --- */

/* Begin I2C session on the given interface.
 *
 * Returns 0 on success, negative value on error.
 */
int i2c_start(volatile unsigned char *iface)
{
    /* Wait for bus to become free */
    int j = MAX_LOOP;
    while (--j && (iface[O_MBSR] & IBB))
        ;
    if (!j)
    {
        logf("i2c: bus is busy (iface=%08lX)", (uintptr_t)iface);
        return -1;
    }
 
    /* Generate START and prepare for write */
    iface[O_MBCR] |= (MSTA | TXAK | MTX);
  
    return 0; 
}

/* Wait for slave to act on given I2C interface.
 *
 * Returns 0 on success, negative value on error.
 */
int i2c_wait_for_slave(volatile unsigned char *iface)
{
    int j = MAX_LOOP;
    while (--j && ! (iface[O_MBSR] & IFF))
        ;
    if (!j)
    {
        logf("i2c: IFF not set (iface=%08lX)", (uintptr_t)iface);
        i2c_stop(iface); 
        return -2;
    }
  
    /* Clear interrupt flag */
    iface[O_MBSR] &= ~IFF;
    
    return 0;
}

/* Write the given byte to the given I2C interface.
 *
 * Returns 0 on success, negative value on error.
 */
int i2c_outb(volatile unsigned char *iface, unsigned char byte)
{
    int rc;

    iface[O_MBDR] = byte;

    rc = i2c_wait_for_slave(iface);
    if (rc < 0)
        return rc;
    
    /* Check that transfer is complete */
    if ( !(iface[O_MBSR] & ICF))
    {
        logf("i2c: transfer error (iface=%08lX)", (uintptr_t)iface);
        i2c_stop(iface); 
        return -3;
    }
    
    /* Check that the byte has been ACKed */
    if (iface[O_MBSR] & RXAK)
    {
        logf("i2c: no ACK (iface=%08lX)", (uintptr_t)iface);
        i2c_stop(iface); 
        return -4;
    }
    
    return 0;
}
