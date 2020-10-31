/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2020
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
#include "config.h"
#include "bluetooth-target.h"
//#include <stdbool.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
//#include "system.h"
//#include "kernel.h"
/*
/usr/bin # hcitool -i hci0 scan
Scanning ...

	00:00:AB:CD:59:B3	n/a
	00:22:94:5F:B6:E2	Juno

* */

int bluetooth_init(void)
{
    /*hciconfig hci0 up*/
    return 0;
}

void bluetooth_scan(struct bluetooth_device *devices, int *ndevices)
{
    if (!devices || !ndevices)
        return;

    memset(devices, 0, (size_t) sizeof(struct bluetooth_device) * (*ndevices));
/* pass an arry of bluetooth_device struct, passed to bluez as --numrsp=ndevices */
/* driver will have to enforce name limits while parsing data */
/* hcitool scan --numrsp=5, hcitool inq --numrsp=5;hcitool name <addr> */

#if 1 /*testing*/
    for (int i=0;i < (*ndevices);i++)
    {
        strcpy((&devices[i])->address, "00:00:00:00:00:00");
        snprintf((&devices[i])->name, BT_NAME_BYTES, "UNKNOWN %d", i);
        if (i == 3)
            devices[i].flags |= BT_DEVICE_CONNECTED;
    }
    (*ndevices)--;
#endif
}

int bluetooth_connect(struct bluetooth_device *device)
{
    if (!device)
        return -1;

    /* connect perhaps flags can be used to signal auth with pin? */
    return 0;
}
/*BLUETOOTH DRIVER FUNCTIONS*/
