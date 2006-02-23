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
#include "hwcompat.h"
#include "logf.h"
#include "debug.h"
#include "string.h"
#include "generic_i2c.h"

void pcf50606_sda_output(void)
{
    or_l( 0x00001000, &GPIO1_ENABLE);
}

void pcf50606_sda_input(void)
{
    and_l(~0x00001000, &GPIO1_ENABLE);
}

void pcf50606_sda_lo(void)
{
    and_l(~0x00001000, &GPIO1_OUT);
}

void pcf50606_sda_hi(void)
{
    or_l( 0x00001000, &GPIO1_OUT);
}

int pcf50606_sda(void)
{
    return 0x00001000 & GPIO1_READ;
}

void pcf50606_scl_output(void)
{
    or_l( 0x00000400, &GPIO_ENABLE);
}

void pcf50606_scl_input(void)
{
    and_l(~0x00000400, &GPIO_ENABLE);
}

int pcf50606_scl(void)
{
    return 0x00000400 & GPIO_READ;
}

void pcf50606_scl_lo(void)
{
    and_l(~0x00000400, &GPIO_OUT);
}

void pcf50606_scl_hi(void)
{
    pcf50606_scl_input();
    while(!pcf50606_scl())
    {
    }
    or_l(0x0400, &GPIO_OUT);
    pcf50606_scl_output();
}

void pcf50606_delay(void)
{
    do { int _x; for(_x=0;_x<32;_x++);} while(0);
}

struct i2c_interface pcf50606_i2c = {
    0x10, /* Address */

    /* Bit-banged interface definitions */
    pcf50606_scl_hi,  /* Drive SCL high, might sleep on clk stretch */
    pcf50606_scl_lo,  /* Drive SCL low */
    pcf50606_sda_hi,  /* Drive SDA high */
    pcf50606_sda_lo,  /* Drive SDA low */
    pcf50606_sda_input,  /* Set SDA as input */
    pcf50606_sda_output, /* Set SDA as output */
    pcf50606_scl_input,  /* Set SCL as input */
    pcf50606_scl_output, /* Set SCL as output */
    pcf50606_scl,      /* Read SCL, returns 0 or nonzero */
    pcf50606_sda,      /* Read SDA, returns 0 or nonzero */

    pcf50606_delay,  /* START SDA hold time (tHD:SDA) */
    pcf50606_delay,  /* SDA hold time (tHD:DAT) */
    pcf50606_delay,  /* SDA setup time (tSU:DAT) */
    pcf50606_delay,  /* STOP setup time (tSU:STO) */
    pcf50606_delay,  /* Rep. START setup time (tSU:STA) */
    pcf50606_delay,  /* SCL high period (tHIGH) */
};

int pcf50606_read_multiple(int address, unsigned char* buf, int count)
{
    return i2c_read_data(0x10, address, buf, count);
}

int pcf50606_read(int address)
{
    int ret;
    unsigned char c;

    ret = pcf50606_read_multiple(address, &c, 1);
    if(ret >= 0)
        return c;
    else
        return ret;
}

int pcf50606_write_multiple(int address, const unsigned char* buf, int count)
{
    return i2c_write_data(0x10, address, buf, count);
}

int pcf50606_write(int address, unsigned char val)
{
    return pcf50606_write_multiple(address, &val, 1);
}

/* These voltages were determined by measuring the output of the PCF50606
   on a running X5, and verified by disassembling the original firmware */
static void set_voltages(void)
{
    static const unsigned char buf[5] =
        {
            0xf4,   /* IOREGC = 2.9V, ON in all states */
            0xf0,   /* D1REGC = 2.5V, ON in all states */
            0xf6,   /* D2REGC = 3.1V, ON in all states */
            0xf4,   /* D3REGC = 2.9V, ON in all states */
        };

    pcf50606_write_multiple(0x23, buf, 4);
}

void pcf50606_init(void)
{
    /* Bit banged I2C */
    or_l(0x00001000, &GPIO1_OUT);
    or_l(0x00000400, &GPIO_OUT);
    or_l(0x00001000, &GPIO1_ENABLE);
    or_l(0x00000400, &GPIO_ENABLE);
    or_l(0x00001000, &GPIO1_FUNCTION);
    or_l(0x00000400, &GPIO_FUNCTION);

    i2c_add_node(&pcf50606_i2c);

    set_voltages();

    pcf50606_write(0x35, 0xf1); /* Backlight PWM = 7kHz 8/16 */
    pcf50606_write(0x38, 0x30); /* Backlight ON */
}
