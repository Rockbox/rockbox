/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * PP5020 I2C driver
 *
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in November 2005
 *
 * Original file: linux/arch/armnommu/mach-ipod/hardware.c
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

#include "cpu.h"
#include "kernel.h"
#include "logf.h"
#include "system.h"
#include "i2c-pp5020.h"

/* Local functions definitions */

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

static int ipod_i2c_wait_not_busy(void)
{
    unsigned long timeout;
    timeout = current_tick + POLL_TIMEOUT;
    while (TIME_BEFORE(current_tick, timeout)) {
         if (!(inb(IPOD_I2C_STATUS) & IPOD_I2C_BUSY)) {
            return 0;
         }
         yield();
    }

    return -1;
}


static int ipod_i2c_read_byte(unsigned int addr, unsigned int *data)
{
    if (ipod_i2c_wait_not_busy() < 0)
    {
        return -1;
    }

    {
        int old_irq_level = set_irq_level(HIGHEST_IRQ_LEVEL);

        /* clear top 15 bits, left shift 1, or in 0x1 for a read */
        outb(((addr << 17) >> 16) | 0x1, IPOD_I2C_ADDR);

        outb(inb(IPOD_I2C_CTRL) | 0x20, IPOD_I2C_CTRL);

        outb(inb(IPOD_I2C_CTRL) | IPOD_I2C_SEND, IPOD_I2C_CTRL);

        set_irq_level(old_irq_level);

        if (data)
        {
            if (ipod_i2c_wait_not_busy() < 0)
            {
                return -1;
            }
            old_irq_level = set_irq_level(HIGHEST_IRQ_LEVEL);
            *data = inb(IPOD_I2C_DATA0);
            set_irq_level(old_irq_level);
        }
    }

    return 0;
}

static int ipod_i2c_send_bytes(unsigned int addr, unsigned int len, unsigned char *data)
{
    int data_addr;
    unsigned int i;

    if (len < 1 || len > 4)
    {
        return -1;
    }

    if (ipod_i2c_wait_not_busy() < 0)
    {
        return -2;
    }

    {
        int old_irq_level = set_irq_level(HIGHEST_IRQ_LEVEL);

        /* clear top 15 bits, left shift 1 */
        outb((addr << 17) >> 16, IPOD_I2C_ADDR);

        outb(inb(IPOD_I2C_CTRL) & ~0x20, IPOD_I2C_CTRL);

        data_addr = IPOD_I2C_DATA0;
        for ( i = 0; i < len; i++ )
        {
            outb(*data++, data_addr);
            data_addr += 4;
        }

        outb((inb(IPOD_I2C_CTRL) & ~0x26) | ((len-1) << 1), IPOD_I2C_CTRL);

        outb(inb(IPOD_I2C_CTRL) | IPOD_I2C_SEND, IPOD_I2C_CTRL);

        set_irq_level(old_irq_level);
    }

    return 0x0;
}

static int ipod_i2c_send_byte(unsigned int addr, int data0)
{
    unsigned char data[1];

    data[0] = data0;

    return ipod_i2c_send_bytes(addr, 1, data);
}

/* Public functions */
static struct mutex i2c_mutex;

int i2c_readbyte(unsigned int dev_addr, int addr)
{
    int retval;
    int data;

    mutex_lock(&i2c_mutex);
    ipod_i2c_send_byte(dev_addr, addr);
    ipod_i2c_read_byte(dev_addr, &data);
    mutex_unlock(&i2c_mutex);

    return data;
}

int ipod_i2c_send(unsigned int addr, int data0, int data1)
{
        int retval;
        unsigned char data[2];

        data[0] = data0;
        data[1] = data1;

        mutex_lock(&i2c_mutex);
        retval = ipod_i2c_send_bytes(addr, 2, data);
        mutex_unlock(&i2c_mutex);

        return retval;
}

void i2c_init(void)
{
   /* From ipodlinux */

#if defined(APPLE_IPODMINI)
   /* GPIO port C disable port 0x10 */
   GPIOC_ENABLE &= ~0x10;

   /* GPIO port C disable port 0x20 */
   GPIOC_ENABLE &= ~0x20;
#endif

   outl(inl(0x6000600c) | 0x1000, 0x6000600c);     /* enable 12 */
   outl(inl(0x60006004) | 0x1000, 0x60006004);     /* start reset 12 */
   outl(inl(0x60006004) & ~0x1000, 0x60006004);    /* end reset 12 */

   outl(0x0, 0x600060a4);
   outl(0x80 | (0 << 8), 0x600060a4);

   mutex_init(&i2c_mutex);
   
   i2c_readbyte(0x8, 0);
}
