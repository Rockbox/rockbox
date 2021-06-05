/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2011-2021 by Tomasz Moń
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
#include "usb_core.h"

static int usb_detect_callback(struct timeout *tmo)
{
    (void)tmo;
    usb_status_event(usb_detect());
    return 0;
}

void GIO9(void) __attribute__ ((section(".icode")));
void GIO9(void)
{
    static struct timeout usb_oneshot;

    /* Clear interrupt */
    IO_INTC_IRQ1 = (1 << 14);

    timeout_register(&usb_oneshot, usb_detect_callback, HZ, 0);
}

int usb_detect(void)
{
    if (IO_GIO_BITSET0 & (1 << 9))
    {
        return USB_EXTRACTED;
    }
    else
    {
        return USB_INSERTED;
    }
}

void usb_init_device(void)
{
    /* set VLYNQ port functions */
    IO_GIO_FSEL1 = 0xAAAA;
    IO_GIO_FSEL2 = (IO_GIO_FSEL2 & 0xFFF0) | 0xA;

    /* set GIO7 as output (TNETV reset) */
    /* set GIO9 as input (USB inserted indicator) */
    IO_GIO_INV0 &= ~((1 << 7) | (1 << 9));
    IO_GIO_DIR0 = (IO_GIO_DIR0 & ~(1 << 7)) | (1 << 9);

    /* Enable interrupt on GIO9 change (any edge) */
    IO_GIO_IRQPORT |= (1 << 9);
    IO_GIO_IRQEDGE |= (1 << 9);

    /* set GIO33 as output (TNETV clock) */
    IO_GIO_DIR2 &= ~(1 << 1);
    IO_GIO_INV2 &= ~(1 << 1);
    /* use M48XI clock on GIO33 */
    IO_CLK_OSEL = (IO_CLK_OSEL & 0xF0F) | 0x50;

    /* Powerdown internal USB */
    IO_CLK_LPCTL1 = 0x11;
    /* Disable internal USB clock */
    IO_CLK_MOD2 &= ~(1 << 6);

    /* Enable USB insert detection interrupt */
    IO_INTC_EINT1 |= (1 << 14);
}

void usb_enable(bool on)
{
    if (on)
    {
        usb_core_init();
    }
    else
    {
        usb_core_exit();
    }
}

void usb_attach(void)
{
}
