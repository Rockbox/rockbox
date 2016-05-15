/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Marcin Bukat
                         Amaury Pouly
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

#include "usb_drv.h"
#include "config.h"
#include "memory.h"
#include "target.h"
#include "jz4760b.h"

enum
{
    WAIT_SETUP,
    IN_PHASE,
    OUT_PHASE,
    WAIT_END
} ep0_state;

static void udc_reset(void)
{
    REG_USB_FADDR = 0;
    /* Reset EP0 */
    REG_USB_INDEX = 0;
    REG_USB_CSR0 = USB_CSR0_FLUSHFIFO | USB_CSR0_SVDOUTPKTRDY | USB_CSR0_SVDSETUPEND; /* clear setupend and rxpktrdy */
    ep0_state = WAIT_SETUP;
    REG_USB_POWER = USB_POWER_SOFTCONN | USB_POWER_HSENAB | USB_POWER_SUSPENDM;
}

void usb_drv_init(void)
{
    REG_USB_POWER &= ~USB_POWER_SOFTCONN;
    /* A delay seems necessary to avoid causing havoc. The USB spec says disconnect
     * detection time (T_DDIS) is around 2us but in practice many hubs might
     * require more. */
    target_mdelay(1);
    udc_reset();
}

int usb_drv_recv_setup(struct usb_ctrlrequest *req)
{
    while(1)
    {
        unsigned intr = REG_USB_INTRUSB;
        if(intr & USB_INTR_RESET)
        {
            udc_reset();
            continue;
        }
    }
    return 0;
}

int usb_drv_port_speed(void)
{
    return (REG_USB_POWER & USB_POWER_HSMODE) ? 1 : 0;
}

void usb_drv_set_address(int address)
{
    (void)address;
    /* UDC sets this automaticaly */
}

int usb_drv_send(int endpoint, void *ptr, int length)
{
    return 0;
}

int usb_drv_recv(int endpoint, void* ptr, int length)
{
    return 0;
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
}

void usb_drv_exit(void)
{
}
