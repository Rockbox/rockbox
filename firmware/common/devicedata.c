/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2024 by William Wilgus
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

#include "devicedata.h"
#include "crc32.h"
#include <stddef.h>
#include <string.h>
#include "debug.h"

#ifndef BOOTLOADER
void verify_device_data(void) INIT_ATTR;
void verify_device_data(void)
{
    DEBUGF("%s", __func__);
    /* verify payload with checksum */
    uint32_t crc = crc_32(device_data.payload, device_data.length, 0xffffffff);
    if (crc == device_data.crc)
        return; /* return if data is valid */

    /* Write the default if data is invalid */
    memset(device_data.payload, 0xff, DEVICE_DATA_PAYLOAD_SIZE); /* Invalid data */
    device_data.length = DEVICE_DATA_PAYLOAD_SIZE;
    device_data.crc = crc_32(device_data.payload, device_data.length, 0xffffffff);

}

/******************************************************************************/
#endif /* ndef BOOTLOADER ******************************************************/
/******************************************************************************/

#if defined(HAVE_DEVICEDATA)
void  __attribute__((weak)) fill_devicedata(struct device_data_t *data)
{
    memset(data->payload, 0xff, data->length);
}
#endif

/* Write bootdata into location in FIRMWARE marked by magic header
 * Assumes buffer is already loaded with the firmware image
 * We just need to find the location and write data into the
 * payload region along with the crc for later verification and use.
 * Returns payload len on success,
 * On error returns false
 */
bool write_devicedata(unsigned char* buf, int len)
{
    int search_len = MIN(len, DEVICE_DATA_SEARCH_SIZE) - sizeof(struct device_data_t);

    /* search for decvice data header prior to search_len */
    for(int i = 0; i < search_len; i++)
    {
        struct device_data_t *data = (struct device_data_t *)&buf[i];
        if (data->magic[0] != DEVICE_DATA_MAGIC0 ||
            data->magic[1] != DEVICE_DATA_MAGIC1)
            continue;

        /* Ignore it if the length extends past the end of the buffer. */
        int data_len = offsetof(struct device_data_t, payload) + data->length;
        if (i + data_len > len)
            continue;

        fill_devicedata(data);

        /* Calculate payload CRC */
        data->crc = crc_32(data->payload, data->length, 0xffffffff);
        return true;
    }

    return false;
}

