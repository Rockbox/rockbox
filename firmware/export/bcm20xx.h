/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Bluetooth hardware abstraction layer
 *
 * Copyright (C) 2015 Lorenzo Miori
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
#ifndef __BCM20XX_H__
#define __BCM20XX_H__

#include "config.h"

#define HCI_VENDOR_SET_BDADDR       0x00
#define HCI_VENDOR_SET_BAUDRATE     0x18

int bcm20xx_init(void) INIT_ATTR;
int bcm20xx_close(void);
int bcm20xx_reset(void);
int bcm20xx_set_bdaddr(char* bdaddr[6]);
int bcm20xx_get_bdaddr(char* bdaddr[6]);
static void bcm20xx_speed(int speed);
//static void bcm20xx_patchram_upload(void);
//static void bcm20xx_pcm(int speed);


#endif /* __BCM20XX_H__ */
