/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Dave Chapman
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
#include "i2c-pp5020.h"

#define I2C_DEVICE_1    ((volatile unsigned char *)0)
#define I2C_DEVICE_2    ((volatile unsigned char *)0)

/* Local functions definitions */

#if 0
static int i2c_write_byte(int device, unsigned char data);
static int i2c_gen_start(int device);
static void i2c_gen_stop(int device);
static volatile unsigned char *i2c_get_addr(int device);
#endif

#define IPOD_I2C_BASE   0x7000c000
#define IPOD_I2C_CTRL   (IPOD_I2C_BASE+0x00)
#define IPOD_I2C_ADDR   (IPOD_I2C_BASE+0x04)
#define IPOD_I2C_DATA0  (IPOD_I2C_BASE+0x0c)
#define IPOD_I2C_DATA1  (IPOD_I2C_BASE+0x10)
#define IPOD_I2C_DATA2  (IPOD_I2C_BASE+0x14)
#define IPOD_I2C_DATA3  (IPOD_I2C_BASE+0x18)
#define IPOD_I2C_STATUS (IPOD_I2C_BASE+0x1c)

/* IPOD_I2C_CTRL bit definitions */
#define IPOD_I2C_SEND   0x80

/* IPOD_I2C_STATUS bit definitions */
#define IPOD_I2C_BUSY   (1<<6)

#define POLL_TIMEOUT (HZ)

static int
ipod_i2c_wait_not_busy(void)
{
#if 0
    unsigned long timeout;
    timeout = jiffies + POLL_TIMEOUT;
    while (time_before(jiffies, timeout)) {
         if (!(inb(IPOD_I2C_STATUS) & IPOD_I2C_BUSY)) {
            return 0;
         }
         yield();
    }

    return -ETIMEDOUT;
#endif
    return 0;
}


/* Public functions */

void i2c_init(void)
{
  /* From ipodlinux */

   outl(inl(0x6000600c) | 0x1000, 0x6000600c);     /* enable 12 */
   outl(inl(0x60006004) | 0x1000, 0x60006004);     /* start reset 12 */
   outl(inl(0x60006004) & ~0x1000, 0x60006004);    /* end reset 12 */

   outl(0x0, 0x600060a4);
   outl(0x80 | (0 << 8), 0x600060a4);

   //i2c_readbyte(0x8, 0);
}

void i2c_close(void)
{

}

/**
 * Writes bytes to the selected device.
 *
 * Returns number of bytes successfully send or -1 if START failed
 */
int i2c_write(int device, unsigned char *buf, int count)
{
  /* From ipodlinux */
        int data_addr;
        int i;

        if (count < 1 || count > 4) {
                return -2;
        }

        if (ipod_i2c_wait_not_busy() < 0) {
           return -1;
        }

        // clear top 15 bits, left shift 1
        outb((device << 17) >> 16, IPOD_I2C_ADDR);

        outb(inb(IPOD_I2C_CTRL) & ~0x20, IPOD_I2C_CTRL);

        data_addr = IPOD_I2C_DATA0;
        for ( i = 0; i < count; i++ ) {
                outb(*buf++, data_addr);

                data_addr += 4;
        }

        outb((inb(IPOD_I2C_CTRL) & ~0x26) | ((count-1) << 1), IPOD_I2C_CTRL);

        outb(inb(IPOD_I2C_CTRL) | IPOD_I2C_SEND, IPOD_I2C_CTRL);

        return 0x0;

    return count;
}

#if 0
/* Write a byte to the interface, returns 0 on success, -1 otherwise. */
static int i2c_write_byte(int device, unsigned char data)
{
    if (ipod_i2c_wait_not_busy() < 0) {
        return -2;
    }

    // clear top 15 bits, left shift 1
    outb((device << 17) >> 16, IPOD_I2C_ADDR);

    outb(inb(IPOD_I2C_CTRL) & ~0x20, IPOD_I2C_CTRL);

    outb(data, IPOD_I2C_DATA0);

    outb((inb(IPOD_I2C_CTRL) & ~0x26), IPOD_I2C_CTRL);

    outb(inb(IPOD_I2C_CTRL) | IPOD_I2C_SEND, IPOD_I2C_CTRL);

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

#endif

volatile unsigned char *i2c_get_addr(int device)
{
    if (device == 1)
        return I2C_DEVICE_1;

    return I2C_DEVICE_2;
}
