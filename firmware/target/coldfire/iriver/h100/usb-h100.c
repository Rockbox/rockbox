/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
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
#include <stdbool.h>
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "usb.h"

void usb_init_device(void)
{
    or_l(0x00000080, &GPIO1_FUNCTION); /* GPIO39 is the USB detect input */
    and_l(~0x01000040, &GPIO_OUT);     /* GPIO24 is the Cypress chip power */
    or_l(0x01000040, &GPIO_ENABLE);
    or_l(0x01000040, &GPIO_FUNCTION);
}

int usb_detect(void)
{
    return (GPIO1_READ & 0x80) ? USB_INSERTED : USB_EXTRACTED;
}

void usb_enable(bool on)
{
    if(on)
    {
        /* Power on the Cypress chip */
        or_l(0x01000040, &GPIO_OUT);
        sleep(2);
    }
    else
    {
        /* Power off the Cypress chip */
        and_l(~0x01000040, &GPIO_OUT);
    }
}
