/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: usb-fw-pp502x.c 21932 2009-07-17 22:07:06Z roolku $
 *
 * Copyright (C) 2009 by ?????
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
#include "usb.h"

void usb_init_device(void)
{
}

void usb_enable(bool on)
{
    (void)on;
}

void usb_attach(void)
{

}

static bool usb_pin_state(void)
{
    return false;
}

/* detect host or charger (INSERTED or EXTRACTED) */
int usb_detect(void)
{
    return usb_pin_state() ? USB_INSERTED : USB_EXTRACTED;
}
