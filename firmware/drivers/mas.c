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

int mas_default_read(unsigned short *buf)
{
    unsigned char *dest = (unsigned char *)buf;
    int ret = 0;
    
    i2c_start();
    i2c_outb(MAS_DEV_WRITE);
    if (i2c_getack()) {
        i2c_outb(MAS_DATA_READ);
        if (i2c_getack()) {
            i2c_start();
            i2c_outb(MAS_DEV_READ);
            if (i2c_getack()) {
                    dest[0] = i2c_inb(0);
                    dest[1] = i2c_inb(1);
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

int mas_run(unsigned short address)
{
    int i;
    unsigned char buf[16];

    i=0;
    buf[i++] = MAS_DATA_WRITE;
    buf[i++] = address << 8;
    buf[i++] = address & 0xff;

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
    buf[i++] = bank?MAS_CMD_READ_D1_MEM:MAS_CMD_READ_D0_MEM;
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
    buf[i++] = bank?MAS_CMD_WRITE_D1_MEM:MAS_CMD_WRITE_D0_MEM;
    buf[i++] = 0x00;
    buf[i++] = (len & 0xff00) >> 8;
    buf[i++] = len & 0xff;
    buf[i++] = (addr & 0xff00) >> 8;
    buf[i++] = addr & 0xff;

    j = 0;
    while(len--) {
#ifdef ARCHOS_RECORDER
        buf[i++] = 0;
        buf[i++] = ptr[j+1];
        buf[i++] = ptr[j+2];
        buf[i++] = ptr[j+3];
#else
        buf[i++] = ptr[j+2];
        buf[i++] = ptr[j+3];
        buf[i++] = 0;
        buf[i++] = ptr[j+1];
#endif
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
    unsigned long value;

    i=0;
    buf[i++] = MAS_DATA_WRITE;
    buf[i++] = MAS_CMD_READ_REG | (reg >> 4);
    buf[i++] = (reg & 0x0f) << 4;

    /* send read command */
    if (i2c_write(MAS_DEV_WRITE,buf,i))
    {
        return -1;
    }

    if(mas_devread(&value, 1))
    {
        return -2;
    }

    return value;
}

int mas_writereg(int reg, unsigned int val)
{
    int i;
    unsigned char buf[16];

    i=0;
    buf[i++] = MAS_DATA_WRITE;
    buf[i++] = MAS_CMD_WRITE_REG | (reg >> 4);
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
    /* Remember, the MAS values are only 20 bits, so we set
       the upper 12 bits to 0 */
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
#ifdef ARCHOS_RECORDER
                    i2c_inb(0); /* Dummy read */
                    ptr[i*4+0] = 0;
                    ptr[i*4+1] = i2c_inb(0) & 0x0f;
                    ptr[i*4+2] = i2c_inb(0);
                    if(len)
                        ptr[i*4+3] = i2c_inb(0);
                    else
                        ptr[i*4+3] = i2c_inb(1); /* NAK the last byte */
#else
                    ptr[i*4+2] = i2c_inb(0);
                    ptr[i*4+3] = i2c_inb(0);
                    ptr[i*4+0] = i2c_inb(0);
                    if(len)
                        ptr[i*4+1] = i2c_inb(0);
                    else
                        ptr[i*4+1] = i2c_inb(1); /* NAK the last byte */
#endif
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

#ifdef ARCHOS_RECORDER
int mas_direct_config_read(unsigned char reg)
{
    unsigned char tmp[2];
    int ret = 0;
    
    i2c_start();
    i2c_outb(MAS_DEV_WRITE);
    if (i2c_getack()) {
        i2c_outb(reg);
        if (i2c_getack()) {
            i2c_start();
            i2c_outb(MAS_DEV_READ);
            if (i2c_getack()) {
                tmp[0] = i2c_inb(0);
                tmp[1] = i2c_inb(1); /* NAK the last byte */
                ret = (tmp[0] << 8) | tmp[1];
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

int mas_direct_config_write(unsigned char reg, unsigned int val)
{
    unsigned char buf[3];

    buf[0] = reg;
    buf[1] = (val >> 8) & 0xff;
    buf[2] = val & 0xff;

    /* send write command */
    if (i2c_write(MAS_DEV_WRITE,buf,3))
    {
        return -1;
    }
    return 0;
}

int mas_codec_writereg(int reg, unsigned int val)
{
    int i;
    unsigned char buf[16];

    i=0;
    buf[i++] = MAS_CODEC_WRITE;
    buf[i++] = (reg >> 8) & 0xff;
    buf[i++] = reg & 0xff;
    buf[i++] = (val >> 8) & 0xff;
    buf[i++] = val & 0xff;

    /* send write command */
    if (i2c_write(MAS_DEV_WRITE,buf,i))
    {
        return -1;
    }
    return 0;
}

int mas_codec_readreg(int reg)
{
    int i;
    int ret = 0;
    unsigned char buf[16];
    unsigned char tmp[2];

    i=0;
    buf[i++] = MAS_CODEC_WRITE;
    buf[i++] = (reg >> 8) & 0xff;
    buf[i++] = reg & 0xff;

    /* send read command */
    if (i2c_write(MAS_DEV_WRITE,buf,i))
    {
        return -1;
    }

    i2c_start();
    i2c_outb(MAS_DEV_WRITE);
    if (i2c_getack()) {
        i2c_outb(MAS_CODEC_READ);
        if (i2c_getack()) {
            i2c_start();
            i2c_outb(MAS_DEV_READ);
            if (i2c_getack()) {
                tmp[0] = i2c_inb(0);
                tmp[1] = i2c_inb(1); /* NAK the last byte */
                ret = (tmp[0] << 8) | tmp[1];
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
#endif
