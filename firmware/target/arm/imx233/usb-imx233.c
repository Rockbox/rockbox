/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "cpu.h"
#include "string.h"
#include "usb.h"
#include "usb_drv.h"
#include "usb_core.h"
#include "usb-target.h"
#include "system.h"
#include "system-target.h"

int usb_status = USB_EXTRACTED;

void usb_drv_usb_detect_event()
{
    usb_status_event(USB_INSERTED);
}

void usb_attach(void)
{
    usb_drv_attach();
}

void usb_drv_int_enable(bool enable)
{
    imx233_enable_interrupt(INT_SRC_USB_CTRL, enable);
}

void INT_USB_CTRL(void)
{
    printf("usb int");
    usb_drv_int();
}

void usb_init_device(void)
{
    usb_drv_startup();
}

int usb_detect(void)
{
    return usb_status;
}

bool usb_plugged(void)
{
    return true;
}

void usb_enable(bool on)
{
    if(on)
        usb_core_init();
    else
        usb_core_exit();
}
