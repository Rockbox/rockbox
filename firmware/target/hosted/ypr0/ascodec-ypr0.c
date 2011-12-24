/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: ascodec-target.h 26116 2010-05-17 20:53:25Z funman $
 *
 * Module wrapper for AS3543 audio codec, using /dev/afe (afe.ko) of Samsung YP-R0
 *
 * Copyright (c) 2011 Lorenzo Miori
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "fcntl.h"
#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "sys/ioctl.h"
#include "stdlib.h"

#include "ascodec-target.h"

int afe_dev = -1;

/* Write to a normal register */
#define IOCTL_REG_WRITE 0x40034101
/* Write to a PMU register */
#define IOCTL_SUBREG_WRITE 0x40034103
/* Read from a normal register */
#define IOCTL_REG_READ 0x80034102
/* Read from a PMU register */
#define IOCTL_SUBREG_READ 0x80034103

static struct mutex as_mtx;

int ascodec_init(void) {

    afe_dev = open("/dev/afe", O_RDWR);

    mutex_init(&as_mtx);

    return afe_dev;

}

void ascodec_close(void) {

    if (afe_dev >= 0) {
        close(afe_dev);
    }

}

/* Read functions returns -1 if fail, otherwise the register's value if success */
/* Write functions return >= 0 if success, otherwise -1 if fail */

int ascodec_write(unsigned int reg, unsigned int value)
{
    struct codec_req_struct y;
    struct codec_req_struct *p;
    p = &y;
    p->reg = reg;
    p->value = value;
    return ioctl(afe_dev, IOCTL_REG_WRITE, p);
}

int ascodec_read(unsigned int reg)
{
    int retval = -1;
    struct codec_req_struct y;
    struct codec_req_struct *p;
    p = &y;
    p->reg = reg;
    retval = ioctl(afe_dev, IOCTL_REG_READ, p);
    if (retval >= 0)
        return p->value;
    else
        return retval;
}

void ascodec_write_pmu(unsigned int index, unsigned int subreg,
                                     unsigned int value)
{
    struct codec_req_struct y;
    struct codec_req_struct *p;
    p = &y;
    p->reg = index;
    p->subreg = subreg;
    p->value = value;
    ioctl(afe_dev, IOCTL_SUBREG_WRITE, p);
}

int ascodec_read_pmu(unsigned int index, unsigned int subreg)
{
    int retval = -1;
    struct codec_req_struct y;
    struct codec_req_struct *p;
    p = &y;
    p->reg = index;
    p->subreg = subreg;
    retval = ioctl(afe_dev, IOCTL_SUBREG_READ, p);
    if (retval >= 0)
        return p->value;
    else
        return retval;
}

/* Helpers to set/clear bits */
void ascodec_set(unsigned int reg, unsigned int bits)
{
    ascodec_write(reg, ascodec_read(reg) | bits);
}

void ascodec_clear(unsigned int reg, unsigned int bits)
{
    ascodec_write(reg, ascodec_read(reg) & ~bits);
}

void ascodec_write_masked(unsigned int reg, unsigned int bits,
                                unsigned int mask)
{
    ascodec_write(reg, (ascodec_read(reg) & ~mask) | (bits & mask));
}

/*FIXME: doesn't work */
int ascodec_readbytes(unsigned int index, unsigned int len, unsigned char *data)
{
    unsigned int i;

    for (i=index; i<len; i++) {
        data[i] = ascodec_read(i);
        printf("Register %i: value=%i\n",index,data[i]);
    }

    printf("TOTAL: %i\n", i);

    return i;
}

/*
 * NOTE:
 * After the conversion to interrupts, ascodec_(lock|unlock) are only used by
 * adc-as3514.c to protect against other threads corrupting the result by using
 * the ADC at the same time.
 * 
 * Concurrent ascodec_(async_)?(read|write) calls are instead protected
 * by the R0's Kernel I2C driver for ascodec (mutexed), so it's automatically safe
 */

void ascodec_lock(void)
{
    mutex_lock(&as_mtx);
}

void ascodec_unlock(void)
{
    mutex_unlock(&as_mtx);
}

/* Read 10-bit channel data */
unsigned short adc_read(int channel)
{
    unsigned short data = 0;

    if ((unsigned)channel >= NUM_ADC_CHANNELS)
        return 0;

    ascodec_lock();

    /* Select channel */
    ascodec_write(AS3514_ADC_0, (channel << 4));
    unsigned char buf[2];

        /*
         * The AS3514 ADC will trigger an interrupt when the conversion
         * is finished, if the corresponding enable bit in IRQ_ENRD2
         * is set.
         * Previously the code did not wait and this apparently did
         * not pose any problems, but this should be more correct.
         * Without the wait the data read back may be completely or
         * partially (first one of the two bytes) stale.
         */
    /*FIXME: not implemented*/
    ascodec_wait_adc_finished();

        /* Read data */
    ascodec_readbytes(AS3514_ADC_0, 2, buf);
    data = (((buf[0] & 0x3) << 8) | buf[1]);

    ascodec_unlock();
    return data;
}

void adc_init(void)
{
}
