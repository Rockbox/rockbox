/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "i2c.h"
#include "debug.h"
#include "mas.h"

int mas_run(int prognum)
{
    int i;
    unsigned char buf[16];

    i=0;
    buf[i++] = MAS_DATA_WRITE;
    buf[i++] = 0x00;
    buf[i++] = prognum;

    /* send run command */
    if (i2c_write(MAS_DEV_WRITE,buf,i))
    {
        return -1;
    }

    return 0;
}

/* note: 'len' is number of 32-bit words, not number of bytes! */
int mas_readmem(int bank, int addr, unsigned long* dest, int len)
{
    int i;
    unsigned char buf[16];

    i=0;
    buf[i++] = MAS_DATA_WRITE;
    buf[i++] = bank?0xf0:0xe0;
    buf[i++] = 0x00;
    buf[i++] = (len & 0xff00) >> 8;
    buf[i++] = len & 0xff;
    buf[i++] = (addr & 0xff00) >> 8;
    buf[i++] = addr & 0xff;

    /* send read command */
    if (i2c_write(MAS_DEV_WRITE,buf,i))
    {
        return -1;
    }

    return mas_devread(dest, len);
}

/* note: 'len' is number of 32-bit words, not number of bytes! */
int mas_writemem(int bank, int addr, unsigned long* src, int len)
{
    int i, j;
    unsigned char buf[60];
    unsigned char* ptr = (unsigned char*)src;

    i=0;
    buf[i++] = MAS_DATA_WRITE;
    buf[i++] = bank;
    buf[i++] = 0x00;
    buf[i++] = (len & 0xff00) >> 8;
    buf[i++] = len & 0xff;
    buf[i++] = (addr & 0xff00) >> 8;
    buf[i++] = addr & 0xff;

    j = 0;
    while(len--) {
        buf[i++] = ptr[j*4+1];
        buf[i++] = ptr[j*4+0];
        buf[i++] = 0;
        buf[i++] = ptr[j*4+2];
        j += 4;
    }
    
    /* send write command */
    if (i2c_write(MAS_DEV_WRITE,buf,i))
    {
        return -1;
    }

    return 0;
}

int mas_readreg(int reg)
{
    int i;
    unsigned char buf[16];

    i=0;
    buf[i++] = MAS_DATA_WRITE;
    buf[i++] = 0xd0 | reg >> 4;
    buf[i++] = (reg & 0x0f) << 4;

    /* send read command */
    if (i2c_write(MAS_DEV_WRITE,buf,i))
    {
        return -1;
    }

    if(mas_devread((unsigned long *)buf, 1))
    {
        return -2;
    }

    return buf[0] | buf[1] << 8 | buf[3] << 16;
}

int mas_writereg(int reg, unsigned short val)
{
    int i;
    unsigned char buf[16];

    i=0;
    buf[i++] = MAS_DATA_WRITE;
    buf[i++] = 0x90 | reg >> 4;
    buf[i++] = ((reg & 0x0f) << 4) | (val & 0x0f);
    buf[i++] = (val >> 12) & 0xff;
    buf[i++] = (val >> 4) & 0xff;

    /* send write command */
    if (i2c_write(MAS_DEV_WRITE,buf,i))
    {
        return -1;
    }
    return 0;
}

/* note: 'len' is number of 32-bit words, not number of bytes! */
int mas_devread(unsigned long *dest, int len)
{
    unsigned char* ptr = (unsigned char*)dest;
    int ret = 0;
    int i;
    
    /* handle read-back */
    i2c_start();
    i2c_outb(MAS_DEV_WRITE);
    if (i2c_getack()) {
        i2c_outb(MAS_DATA_READ);
        if (i2c_getack()) {
            i2c_start();
            i2c_outb(MAS_DEV_READ);
            if (i2c_getack()) {
                for (i=0;len;i++) {
                    len--;
                    ptr[i*4+1] = i2c_inb(0);
                    ptr[i*4+0] = i2c_inb(0);
                    ptr[i*4+3] = i2c_inb(0);
                    if(len)
                        ptr[i*4+2] = i2c_inb(0);
                    else
                        ptr[i*4+2] = i2c_inb(1); /* NAK the last byte */
                }
            }
            else
                ret = -3;
        }
        else
            ret = -2;
    }
    else
        ret = -1;
    
    i2c_stop();

    return ret;
}
