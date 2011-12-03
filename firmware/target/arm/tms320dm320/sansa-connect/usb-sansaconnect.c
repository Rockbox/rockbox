/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2011 by Tomasz Moń
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
#ifdef HAVE_USBSTACK
#include "usb_drv.h"
#include "usb_core.h"
#endif

bool usb_drv_connected(void)
{
    return false;
}

int usb_detect(void)
{
    return USB_EXTRACTED;
}

void usb_init_device(void)
{
    /* set TNETV USB nreset high */
    IO_GIO_DIR0 &= ~(1 << 7);
    IO_GIO_BITSET0 = (1 << 7);

    /* set VLYNQ port functions */
    IO_GIO_FSEL1 = 0xAAAA;
    IO_GIO_FSEL2 = (IO_GIO_FSEL2 & 0xFFF0) | 0xA;

    return;
}

void usb_enable(bool on)
{
    (void)on;
}

void usb_attach(void)
{
}
