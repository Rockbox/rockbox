/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 Amaury Pouly
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

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "kernel.h"
#include "rbendian.h"

#include "nwz_tuner.h"
#include "si4700.h"
#include "power.h"

static int radio_fd = -1;

bool tuner_power(bool status)
{
    if(status != tuner_powered())
    {
        if(status)
        {
            radio_fd = open(NWZ_TUNER_DEV, 0);
            if(radio_fd != -1)
            {
                /* Sony is braindead, opening the radio device mutes all audio (even from the DAC)
                 * until SET_FREQ is called with a valid frequency, choose 90MHz because it is valid
                 * in all bands */
                struct nwz_tuner_ioctl_t req;
                memset(&req, 0, sizeof(req)); // just to avoid garbage in
                req.freq = 90000;
                int ret = ioctl(radio_fd, NWZ_TUNER_SET_FREQ, &req);
                if(ret != 0)
                    perror("tuner set_freq error");
            }
            else
                perror("tuner open error");
        }
        else
        {
            close(radio_fd);
            radio_fd = -1;
        }
    }
    return tuner_powered();
}

bool tuner_powered(void)
{
    return radio_fd != -1;
}

/* one can adjust the following depending on the tuner, for now it is calibrated for the si470x
 * which is crazy, because it starts reading at register 0xA and writing at 0x2. We need to
 * "emulate" this behavior... Note that Si470x transmits in big-endian */
#define READ_START_REG  0xA
#define WRITE_START_REG 0x2
#define REG_COUNT       16
#define REG_SIZE        2
#define REG_TYPE        uint16_t
#define REG_SWAP        swap16

int fmradio_i2c_write(unsigned char address, unsigned char* buf, int count)
{
    (void)address;
    struct nwz_tuner_ioctl_t req;
    memset(&req, 0, sizeof(req)); // just to avoid garbage in
    for(int i = 0; i < count / REG_SIZE; i++)
    {
        req.put.regno = (WRITE_START_REG + i) % REG_COUNT;
        req.put.val = REG_SWAP(*(REG_TYPE *)&buf[i * REG_SIZE]);
        int ret = ioctl(radio_fd, NWZ_TUNER_PUT_REG, &req);
        if(ret != 0)
        {
            perror("tuner put_reg error");
            return ret;
        }
    }
    return 0;
}

int fmradio_i2c_read(unsigned char address, unsigned char* buf, int count)
{
    (void)address;
    struct nwz_tuner_ioctl_t req;
    memset(&req, 0, sizeof(req)); // just to avoid garbage in
    int ret = ioctl(radio_fd, NWZ_TUNER_GET_REG_ALL, &req);
    if(ret != 0)
    {
        perror("radio get_reg_all error");
        return ret;
    }
    for(int i = 0; i < count / REG_SIZE; i++)
    {
        /* on version 1, some registers are not reported by Sony's driver */
        int sony_reg = (READ_START_REG + i) % REG_COUNT;
        if(sony_reg < NWZ_TUNER_REG_COUNT)
            *(REG_TYPE *)&buf[i * REG_SIZE] = REG_SWAP(req.regs[sony_reg]);
        else
            *(REG_TYPE *)&buf[i * REG_SIZE] = 0xfeca; /* recognizable pattern for debug menu */
    }
    return 0;
}
