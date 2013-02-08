/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

#include "ascodec.h"

int afe_dev = -1;

/* ioctl parameter struct */
struct codec_req_struct {
/* This works for every kind of afe.ko module requests */
    unsigned char reg; /* Main register address */
    unsigned char subreg; /* Set this only if you are reading/writing a PMU register*/
    unsigned char value; /* To be read if reading a register; to be set if writing to a register */
} __attribute__((packed));


/* Write to a normal register */
#define IOCTL_REG_WRITE         0x40034101
/* Write to a PMU register */
#define IOCTL_SUBREG_WRITE      0x40034103
/* Read from a normal register */
#define IOCTL_REG_READ          0x80034102
/* Read from a PMU register */
#define IOCTL_SUBREG_READ       0x80034103


void ascodec_init(void)
{
    afe_dev = open("/dev/afe", O_RDWR);
}

void ascodec_close(void)
{
    if (afe_dev >= 0) {
        close(afe_dev);
    }
}

/* Read functions returns -1 if fail, otherwise the register's value if success */
/* Write functions return >= 0 if success, otherwise -1 if fail */

int ascodec_write(unsigned int reg, unsigned int value)
{
    struct codec_req_struct r = { .reg = reg, .value = value };
    return ioctl(afe_dev, IOCTL_REG_WRITE, &r);
}

int ascodec_read(unsigned int reg)
{
    struct codec_req_struct r = { .reg = reg };
    int retval = ioctl(afe_dev, IOCTL_REG_READ, &r);
    if (retval >= 0)
        return r.value;
    else
        return retval;
}

void ascodec_write_pmu(unsigned int index, unsigned int subreg,
                                     unsigned int value)
{
    struct codec_req_struct r = {.reg = index, .subreg = subreg, .value = value};
    ioctl(afe_dev, IOCTL_SUBREG_WRITE, &r);
}

int ascodec_read_pmu(unsigned int index, unsigned int subreg)
{
    struct codec_req_struct r = { .reg = index, .subreg = subreg, };
    int retval = ioctl(afe_dev, IOCTL_SUBREG_READ, &r);
    if (retval >= 0)
        return r.value;
    else
        return retval;
}

int ascodec_readbytes(unsigned int index, unsigned int len, unsigned char *data)
{
    int i, val, ret = 0;

    for (i = 0; i < (int)len; i++)
    {
        val = ascodec_read(i + index);
        if (val >= 0) data[i] = val;
        else ret = -1;
    }

    return (ret ?: (int)len);
}

/*
 * NOTE:
 * After the conversion to interrupts, ascodec_(lock|unlock) are only used by
 * adc-as3514.c to protect against other threads corrupting the result by using
 * the ADC at the same time. this adc_read() doesn't yield but blocks, so
 * lock/unlock is not needed
 * 
 * Additionally, concurrent ascodec_?(read|write) calls are instead protected
 * by the R0's Kernel I2C driver for ascodec (mutexed), so it's automatically
 * safe
 */

void ascodec_lock(void)
{
}

void ascodec_unlock(void)
{
}

bool ascodec_chg_status(void)
{
    return ascodec_read(AS3514_IRQ_ENRD0) & CHG_STATUS;
}   

bool ascodec_endofch(void)
{
    return ascodec_read(AS3514_IRQ_ENRD0) & CHG_ENDOFCH;
}

void ascodec_monitor_endofch(void)
{
    ascodec_write(AS3514_IRQ_ENRD0, IRQ_ENDOFCH);
}


void ascodec_write_charger(int value)
{
    ascodec_write_pmu(AS3543_CHARGER, 1, value);
}

int ascodec_read_charger(void)
{
    return ascodec_read_pmu(AS3543_CHARGER, 1);
}

void ascodec_wait_adc_finished(void)
{
}
