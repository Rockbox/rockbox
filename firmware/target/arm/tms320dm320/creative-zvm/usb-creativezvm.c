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

#include "config.h"
#include "system.h"
#include "kernel.h"
#include "usb.h"
#include "usb-target.h"
#include "usb_drv.h"
#include "usb_core.h"
#include "isp1583.h"

#define printf

bool usb_drv_connected(void)
{
    return button_usb_connected();
}

int usb_detect(void)
{
    if(button_usb_connected())
        return USB_INSERTED;
    else
        return USB_EXTRACTED;
}

void usb_init_device(void)
{
    return;
}

void usb_enable(bool on)
{
    if(on)
        usb_core_init();
    else
         usb_core_exit();
}

void usb_attach(void)
{
    usb_enable(true);
}

void IRAM_ATTR GIO7(void)
{
#ifdef DEBUG
    //printf("GIO7 interrupt... [%d]", current_tick);
#endif
	usb_drv_int();
    
	IO_INTC_IRQ1 = INTR_IRQ1_EXT7;
}
