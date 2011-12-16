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

static bool usb_is_connected = false;

static int usb_detect_callback(struct timeout *tmo)
{
    (void)tmo;

    if (IO_GIO_BITSET0 & (1 << 9))
    {
        /* Set GIO33 as normal output, drive it low */
        IO_GIO_FSEL3 &= ~(0x0003);
        IO_GIO_BITCLR2 = (1 << 1);

        /* Disable M48XI crystal resonator */
        IO_CLK_LPCTL1 |= 0x01;

        /* Drive reset low */
        IO_GIO_BITCLR0 = (1 << 7);

        /* Disable VLYNQ clock */
        IO_CLK_MOD2 &= ~(1 << 13);

        usb_is_connected = false;
    }
    else
    {
        /* Enable M48XI crystal resonator */
        IO_CLK_LPCTL1 &= ~(0x01);

        /* Set GIO33 as CLKOUT1B */
        IO_GIO_FSEL3 |= 0x0003;

        /* Drive reset high */
        IO_GIO_BITSET0 = (1 << 7);

        /* Enable VLYNQ clock */
        IO_CLK_MOD2 |= (1 << 13);

        usb_is_connected = true;
    }
 
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

bool usb_drv_connected(void)
{
    return false;
}

int usb_detect(void)
{
    if (usb_is_connected == true)
    {
        return USB_INSERTED;
    }
    else
    {
        return USB_EXTRACTED;
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

    /* Check if USB is connected */
    usb_detect_callback(NULL);
}

void usb_enable(bool on)
{
    (void)on;
}

void usb_attach(void)
{
}
