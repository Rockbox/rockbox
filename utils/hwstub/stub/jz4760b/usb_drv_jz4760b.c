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

static void udc_reset(void)
{
    REG_USB_FADDR = 0;
    /* Reset EP0 */
    REG_USB_INDEX = 0;
    REG_USB_CSR0 = USB_CSR0_FLUSHFIFO | USB_CSR0_SVDOUTPKTRDY | USB_CSR0_SVDSETUPEND; /* clear setupend and rxpktrdy */
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

static void *read_fifo0(void *dst, unsigned size)
{
    unsigned char *p = dst;
    while(size-- > 0)
        *p++ = *(volatile uint8_t *)USB_FIFO_EP(0);
    return p;
}

static void *write_fifo0(void *src, unsigned size)
{
    unsigned char *p = src;
    while(size-- > 0)
        *(volatile uint8_t *)USB_FIFO_EP(0) = *p++;
    return p;
}

int usb_drv_recv_setup(struct usb_ctrlrequest *req)
{
    while(1)
    {
        unsigned intr = REG_USB_INTRUSB;
        unsigned intrin = REG_USB_INTRIN;
        /* handle reset */
        if(intr & USB_INTR_RESET)
        {
            udc_reset();
            continue;
        }
        /* ignore anything but EP0 irq */
        if(!(intrin & 1))
            continue;
        /* select EP0 */
        REG_USB_INDEX = 0;
        /* load csr to examine the cause of the interrupt */
        unsigned csr0 = REG_USB_CSR0;
        /* wait setup: we expect to receive a packet */
        if(csr0 & USB_CSR0_OUTPKTRDY)
        {
            unsigned cnt = REG_USB_COUNT0;
            /* anything other than 8-byte is wrong */
            if(cnt == 8)
            {
                read_fifo0(req, 8);
                /* clear packet ready flag */
                REG_USB_CSR0 = csr0 | USB_CSR0_SVDOUTPKTRDY;
                return 0;
            }
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
    REG_USB_FADDR = address;
}

int usb_drv_send(int endpoint, void *ptr, int length)
{
    (void) endpoint;
    /* select EP0 */
    REG_USB_INDEX = 0;
    do
    {
        unsigned csr = REG_USB_CSR0;
        /* write data if any */
        if(length > 0)
        {
            int cnt = MIN(length, 64);
            ptr = write_fifo0(ptr, cnt);
            length -= cnt;
            csr |= USB_CSR0_INPKTRDY;
        }
        /* last packet ? */
        if(length == 0)
            csr |= USB_CSR0_DATAEND;
        /* write csr */
        REG_USB_CSR0 = csr;
        /* wait for packet to be transmitted */
        if(length > 0)
            while(REG_USB_CSR0 & USB_CSR0_INPKTRDY) {}
    }while(length > 0);
    /* wait until acknowledgement */
    while(REG_USB_CSR0 & USB_CSR0_DATAEND) {}
    return 0;
}

int usb_drv_recv(int endpoint, void* ptr, int length)
{
    (void) endpoint;
    int old_len = length;
    /* select EP0 */
    REG_USB_INDEX = 0;
    /* ZLP: ignore receive, the core does it automatically on DataEnd */
    if(length == 0)
        return 0;
    /* receive data
     * NOTE at the end of the transfer, we do not set DataEnd. Rather the code
     * will call usb_drv_send() for a ZLP which will set DataEnd */
    while(length > 0)
    {
        /* wait for data */
        unsigned csr0 = REG_USB_CSR0;
        if(!(csr0 & USB_CSR0_OUTPKTRDY))
            continue;
        int cnt = REG_USB_COUNT0;
        /* clamp just in case */
        cnt = MIN(cnt, length);
        /* read fifo */
        ptr = read_fifo0(ptr, cnt);
        length -= cnt;
        /* clear packet ready flag */
        REG_USB_CSR0 = csr0 | USB_CSR0_SVDOUTPKTRDY;
    }
    return old_len;
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
    /* select EP0 */
    REG_USB_INDEX = 0;
    /* set stall */
    REG_USB_CSR0 |= USB_CSR0_SENDSTALL;
}

void usb_drv_exit(void)
{
}
