/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
 *
 * DM320 I²C driver
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "system.h"
#include "thread.h"
#include "i2c-dm320.h"

#define I2C_SCS_COND_START  0x0001
#define I2C_SCS_COND_STOP   0x0002
#define I2C_SCS_XMIT        0x0004

#define I2C_TX_ACK          (1 << 8)

static struct mutex i2c_mtx;

static inline void i2c_begin(void)
{
    mutex_lock(&i2c_mtx);
}

static inline void i2c_end(void)
{
    mutex_unlock(&i2c_mtx);
}

static inline bool i2c_getack(void)
{
    return (IO_I2C_RXDATA >> 8) & 1;
}

static inline void i2c_ack(void)
{
    IO_I2C_TXDATA |= I2C_TX_ACK;
}

#define WAIT_FOR_I2C    if(IO_I2C_SCS & 0x4){ \
                            while(IO_I2C_SCS & 0x4) { \
                                asm volatile("nop"); \
                            } \
                        } \

static inline void i2c_start(void)
{
    IO_I2C_SCS |= I2C_SCS_XMIT;
    return;
}

int i2c_write(unsigned short address, const unsigned char *buf, int count)
{
    int i;
    int ret=0;
    i2c_begin();
    IO_I2C_TXDATA = ( (address << 1) & 0xFF ) | (address>0x7F ? 0 : 1 ) | I2C_TX_ACK;
    IO_I2C_SCS &= ~0x3; //clear conditions
    IO_I2C_SCS |= I2C_SCS_COND_START; // write 'start condition'
    i2c_start();
    WAIT_FOR_I2C;
    /* experimental */
    if(address>0x7F){ // check if it is 10-bit instead of 7-bit
        IO_I2C_TXDATA = ( (address >> 7) & 0xFF) | I2C_TX_ACK;
        IO_I2C_SCS &= ~0x3; //normal transfer
        i2c_start();
        WAIT_FOR_I2C;
        IO_I2C_TXDATA = ( (address << 1) & 0xFF) | 1 | I2C_TX_ACK;
        IO_I2C_SCS &= ~0x3; //clear conditions
        IO_I2C_SCS |= I2C_SCS_COND_START; //write 'start condition'
        i2c_start();
        WAIT_FOR_I2C;
    }

    for(i=0; i<count; i++){
        IO_I2C_TXDATA = buf[i] | I2C_TX_ACK;
        IO_I2C_SCS &= ~0x3; //normal transfer
        i2c_start();
        WAIT_FOR_I2C;
        if(!i2c_getack())
            ret = -1;
    }
    IO_I2C_SCS &= ~0x3; //clear conditions
    IO_I2C_SCS |= I2C_SCS_COND_STOP; //write 'stop condition'
    i2c_start();
    WAIT_FOR_I2C;
    i2c_end();
    return ret;
}

int i2c_read(unsigned short address, unsigned char* buf, int count)
{
    int i;
    int ack=0;
    i2c_begin();
    IO_I2C_TXDATA = ( (address << 1) & 0xFF ) | (address>0x7F ? 0 : 1 ) | I2C_TX_ACK;
    IO_I2C_SCS &= ~0x3; //clear conditions
    IO_I2C_SCS |= I2C_SCS_COND_START; // write 'start condition'
    i2c_start();
    WAIT_FOR_I2C;
    /* experimental */
    if(address>0x7F){ // check if it is 10-bit instead of 7-bit
        IO_I2C_TXDATA = ( (address >> 7) & 0xFF ) | I2C_TX_ACK;
        IO_I2C_SCS &= ~0x3; //normal transfer
        i2c_start();
        WAIT_FOR_I2C;
        IO_I2C_TXDATA = ( (address << 1) & 0xFF ) | 1 | I2C_TX_ACK;
        IO_I2C_SCS &= ~0x3; //clear conditions
        IO_I2C_SCS |= I2C_SCS_COND_START; //write 'start condition'
        i2c_start();
        WAIT_FOR_I2C;
    }

    for(i=0; i<count; i++){
        unsigned short temp;
        IO_I2C_TXDATA = 0xFF | ( (count-1)==i ? I2C_TX_ACK : 0);
        IO_I2C_SCS &= ~0x3; //normal transfer
        i2c_start();
        WAIT_FOR_I2C;
        temp = IO_I2C_RXDATA;
        buf[i] = temp & 0xFF;
        ack = (temp & 0x100) >> 8;
    }
    IO_I2C_SCS &= ~0x3; //clear conditions
    IO_I2C_SCS |= I2C_SCS_COND_STOP; //write 'stop condition'
    i2c_start();
    WAIT_FOR_I2C;
    i2c_end();
    return ack;
}

void i2c_init(void)
{
#if 0 //TODO: mimic OF I2C clock settings; currently this is done by the bootloader
    IO_CLK_MOD2 &= ~CLK_MOD2_I2C; // turn I²C clock off (just to be sure)
    IO_CLK_LPCTL1 &= ~1; // set Powerdown mode to off
    IO_CLK_SEL0 &= ~0x800; // set I²C clock to PLLA
    IO_CLK_DIV4 &= ~0x1F; // I²C clock division = 1
    IO_CLK_MOD2 |= CLK_MOD2_I2C; // enable I²C clock
#endif
    IO_I2C_SCS &= ~0x8; //set clock to 100 kHz
    IO_INTC_EINT2 &= ~INTR_EINT2_I2C; // disable I²C interrupt
}
