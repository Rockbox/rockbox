/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "logf.h"
#include "debug.h"
#include "string.h"
#include "generic_i2c.h"

static void i2c_sda_output(void)
{
    GPECON |= (1 << 30);
}

static void i2c_sda_input(void)
{
    GPECON &= ~(3 << 30);
}

static void i2c_sda_lo(void)
{
    GPEDAT &= ~(1 << 15);
}

static void i2c_sda_hi(void)
{
    GPEDAT |= (1 << 15);
}

static int i2c_sda(void)
{
    return GPEDAT & (1 << 15);
}

static void i2c_scl_output(void)
{
    GPECON |= (1 << 28);
}

static void i2c_scl_input(void)
{
    GPECON &= ~(3 << 28);
}

static void i2c_scl_lo(void)
{
    GPEDAT &= ~(1 << 14);
}

static int i2c_scl(void)
{
    return GPEDAT & (1 << 14);
}

static void i2c_scl_hi(void)
{
    i2c_scl_input();
    while(!i2c_scl());
    GPEDAT |= (1 << 14);
    i2c_scl_output();
}



static void i2c_delay(void)
{
     unsigned _x;

    /* The i2c can clock at 500KHz: 2uS period -> 1uS half period */
    /* about 30 cycles overhead + X * 7 */
    /* 300MHz: 1000nS @3.36nS/cyc = 297cyc: X = 38*/
    /* 100MHz: 1000nS @10nS/cyc = 100cyc : X = 10 */
    for (_x = 38; _x; _x--)
    {
        /* burn CPU cycles */
        /* gcc makes it an inc loop - check with objdump for asm timing */
    }
}



struct i2c_interface s3c2440_i2c = {
    0x34, /* Address */

    /* Bit-banged interface definitions */
    i2c_scl_hi,  /* Drive SCL high, might sleep on clk stretch */
    i2c_scl_lo,  /* Drive SCL low */
    i2c_sda_hi,  /* Drive SDA high */
    i2c_sda_lo,  /* Drive SDA low */
    i2c_sda_input,  /* Set SDA as input */
    i2c_sda_output, /* Set SDA as output */
    i2c_scl_input,  /* Set SCL as input */
    i2c_scl_output, /* Set SCL as output */
    i2c_scl,      /* Read SCL, returns 0 or nonzero */
    i2c_sda,      /* Read SDA, returns 0 or nonzero */

    i2c_delay,  /* START SDA hold time (tHD:SDA) */
    i2c_delay,  /* SDA hold time (tHD:DAT) */
    i2c_delay,  /* SDA setup time (tSU:DAT) */
    i2c_delay,  /* STOP setup time (tSU:STO) */
    i2c_delay,  /* Rep. START setup time (tSU:STA) */
    i2c_delay,  /* SCL high period (tHIGH) */
};

void i2c_init(void)
{
    /* Set GPE15 (SDA) and GPE14 (SCL) to 1 */
    GPECON = (GPECON & ~(0xF<<28)) | 5<<28;
    i2c_add_node(&s3c2440_i2c);
}

void i2c_send(int bus_address, int reg_address, const unsigned char buf)
{
    i2c_write_data(bus_address, reg_address, &buf, 1);
}
