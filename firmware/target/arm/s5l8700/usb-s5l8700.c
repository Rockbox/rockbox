/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Bertrik Sikken
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
#include "s5l87xx.h"
#include "usb.h"

void usb_init_device(void)
{
    /* TODO implement */
    
    /* enable USB2.0 function controller to allow VBUS monitoring */
    PWRCON &= ~(1 << 15);
}

void usb_enable(bool on)
{
    (void)on;
    /* TODO implement */
}

int usb_detect(void)
{
    return (USB_TR & (1 << 15)) ? USB_INSERTED : USB_EXTRACTED;
}

