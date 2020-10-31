/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 by Marcin Bukat
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
#ifndef _BLUETOOTH_TARGET_H_
#define _BLUETOOTH_TARGET_H_

/*BLUETOOTH DRIVER FUNCTIONS -- bluetooth.h?*/
#define BT_DEVICE_MAX_DEVICES 6 /*(64 x 6) 384 bytes*/
#define BT_ADDR_BYTES 18 /*'xx:xx:xx:xx:xx:xx\0'*/
#define BT_NAME_BYTES 32 /*device name*/
#define BT_PIN_BYTES 5 /*0000\0*/
#define BT_RSSI_BYTES 5 /*0000\0*/

/* BT Flags */
#define BT_DEVICE_PIN_REQUIRED 0x01
#define BT_DEVICE_PIN_SUPPLIED 0x02
#define BT_DEVICE_AUTO_CONNECT 0x04
#define BT_DEVICE_CONNECTED    0x08
#define BT_DEVICE_TRUSTED      0x10
// #define BT_ERRORS?

struct bluetooth_device
{
    char address[BT_ADDR_BYTES];
    char name[BT_NAME_BYTES];
    char pin[BT_PIN_BYTES];
    //char rssi[BT_RSSI_BYTES];
    int flags;
};

int  bluetooth_init(void);
void bluetooth_scan(struct bluetooth_device *devices, int *ndevices);
int  bluetooth_connect(struct bluetooth_device *device);

#endif /* _BLUETOOTH_TARGET_H_ */

