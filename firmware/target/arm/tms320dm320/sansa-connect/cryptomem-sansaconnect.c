/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id: $
*
* Copyright (C) 2021 by Tomasz Moń
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

#include <ctype.h>
#include "cryptomem-sansaconnect.h"
#include "i2c-dm320.h"

/* Command values */
#define WRITE_USER_ZONE  0xB0
#define READ_USER_ZONE   0xB2
#define SYSTEM_WRITE     0xB4
#define SYSTEM_READ      0xB6
#define VERIFY_CRYPTO    0xB8
#define VERIFY_PASSWORD  0xBA

/* SYSTEM_WRITE ADDR 1 values */
#define WRITE_CONFIG     0x00
#define WRITE_FUSES      0x01
#define SEND_CHECKSUM    0x02
#define SET_USER_ZONE    0x03

int cryptomem_read_deviceid(char deviceid[32])
{
    int ret;
    unsigned int i;
    unsigned char cmd_data[3];

    /* It is assumed that other I2C communication has already taken place
     * (e.g. power_init()) before this function is called and thus we don't
     * have to send atleast 5 dummy clock cycles here.
     */

    cmd_data[0] = SET_USER_ZONE;
    cmd_data[1] = 0;
    cmd_data[2] = 0;
    ret = i2c_write(SYSTEM_WRITE, cmd_data, sizeof(cmd_data));
    if (ret < 0)
    {
        return ret;
    }

    cmd_data[0] = 0;
    cmd_data[1] = 0;
    cmd_data[2] = 32;
    ret = i2c_write_read_bytes(READ_USER_ZONE, cmd_data, sizeof(cmd_data),
                               deviceid, 32);
    if (ret < 0)
    {
        return ret;
    }

    /* Verify that deviceid contains only printable ASCII characters */
    for (i = 0; i < 32; i++)
    {
        if (!isprint(deviceid[i]))
        {
            return -1;
        }
    }

    return 0;
}
