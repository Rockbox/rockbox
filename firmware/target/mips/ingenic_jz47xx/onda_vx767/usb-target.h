/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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

#ifndef __USB_TARGET_H
#define __USB_TARGET_H

#include "config.h"

#define GPIO_UDC_DETE      (32 * 3 + 28)
#define IRQ_GPIO_UDC_DETE  (IRQ_GPIO_0 + GPIO_UDC_DETE)

#define USB_INIT_GPIO()              \
{                                    \
    REG_GPIO_PXFUNS(3) = 0x10000000; \
    REG_GPIO_PXSELS(3) = 0x10000000; \
    REG_GPIO_PXPES(3)  = 0x10000000; \
    __gpio_as_input(GPIO_UDC_DETE);  \
}

#define USB_DRV_CONNECTED()    (__gpio_get_pin(GPIO_UDC_DETE) == 1)

int usb_detect(void);
void usb_init_device(void);
bool usb_drv_connected(void);

#endif
