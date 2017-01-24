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
    /* in case usb is running, soft disconnect */
    REG_USB_POWER &= ~USB_POWER_SOFTCONN;
    /* A delay seems necessary to avoid causing havoc. The USB spec says disconnect
     * detection time (T_DDIS) is around 2us but in practice many hubs might
     * require more. */
    target_mdelay(1);
    /* disable usb */
    REG_CPM_CLKGR0 |= CLKGR0_OTG;
    /* power up usb: assume EXCLK=12Mhz */
    REG_CPM_CPCCR &= ~CPCCR_ECS; /* use EXCLK as source (and not EXCLK/2) */
    REG_CPM_USBCDR = 0; /* use EXCLK as source, no divisor */
    REG_CPM_CPCCR |= CPCCR_CE; /* change source now */
    /* wait for stable clock */
    target_udelay(3);
    /* enable usb */
    REG_CPM_CLKGR0 &= ~CLKGR0_OTG;
    /* tweak various parameters */
    REG_CPM_USBVBFIL = 0x80;
    REG_CPM_USBRDT = 0x96;
    REG_CPM_USBRDT |= (1 << 25);
    REG_CPM_USBPCR &= ~0x3f;
    REG_CPM_USBPCR |= 0x35;
    REG_CPM_USBPCR &= ~USBPCR_USB_MODE;
    REG_CPM_USBPCR |= USBPCR_VBUSVLDEXT;
    /* reset otg phy */
    REG_CPM_USBPCR |= USBPCR_POR;
    target_udelay(30);
    REG_CPM_USBPCR &= ~USBPCR_POR;
    target_udelay(300);
    /* enable otg phy */
    REG_CPM_OPCR |= OPCR_OTGPHY_ENABLE;
    /* wait for stable phy */
    target_udelay(300);
    /* reset */
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

/* NOTE: this core is a bit weird, it handles the status stage automatically
 * as soon as DataEnd is written to CSR. The problem is that DataEnd needs
 * to be written as part of a read (INPKTRDY) or write (SVDOUTPKTRDY) request
 * but not on its own.
 * Thus the design is follows: after receiving the setup packet, we DO NOT
 * acknowledge it with SVDOUTPKTRDY. Instead it will be acknowledged
 * either as part of STALL or recv/send. If there is an OUT data stage, we use
 * a similar trick: we do not acknowledge the last packet and leave a pending
 * SVDOUTPKTRDY to be done as part of a final STALL or ZLP. */

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
                /* DO NOT acknowledge the packet, leave this to recv/send/stall */
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
    /* clear packet ready for the PREVIOUS packet: warning, there is a trap here!
     * if we are clearing without sending anything (length=0) then we must
     * set DataEnd at the same time. Otherwise, we must set it by itself and then
     * send data */
    if(length > 0)
    {
        /* clear packet ready for the PREVIOUS packet (SETUP) */
        REG_USB_CSR0 |= USB_CSR0_SVDOUTPKTRDY;
        /* send data */
        do
        {
            unsigned csr = REG_USB_CSR0;
            /* write data  */
            int cnt = MIN(length, 64);
            ptr = write_fifo0(ptr, cnt);
            length -= cnt;
            csr |= USB_CSR0_INPKTRDY;
            /* last packet ? */
            if(length == 0)
                csr |= USB_CSR0_DATAEND;
            /* write csr */
            REG_USB_CSR0 = csr;
            /* wait for packet to be transmitted */
            while(REG_USB_CSR0 & USB_CSR0_INPKTRDY) {}
        }while(length > 0);
    }
    else
    {
        /* clear packet ready for the PREVIOUS packet (SETUP or DATA) and finish */
        REG_USB_CSR0 |= USB_CSR0_SVDOUTPKTRDY | USB_CSR0_DATAEND;
    }
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
     * NOTE when we are called here, there is a pending SVDOUTPKTRDY to
     * be done (see note above usb_drv_recv_setup), and when we will finish,
     * we will also leave a pending SVDOUTPKTRDY to be done in stall or send */
    while(length > 0)
    {
        /* clear packet ready for the PREVIOUS packet */
        REG_USB_CSR0 |= USB_CSR0_SVDOUTPKTRDY;
        /* wait for data */
        while(!(REG_USB_CSR0 & USB_CSR0_OUTPKTRDY)) {}
        int cnt = REG_USB_COUNT0;
        /* clamp just in case */
        cnt = MIN(cnt, length);
        /* read fifo */
        ptr = read_fifo0(ptr, cnt);
        length -= cnt;
    }
    /* there is still a pending SVDOUTPKTRDY here */
    return old_len;
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
    (void) endpoint;
    (void) in;
    if(!stall)
        return; /* EP0 unstall automatically */
    /* select EP0 */
    REG_USB_INDEX = 0;
    /* set stall */
    REG_USB_CSR0 |= USB_CSR0_SVDOUTPKTRDY | USB_CSR0_SENDSTALL;
}

void usb_drv_exit(void)
{
    /* in case usb is running, soft disconnect */
    REG_USB_POWER &= ~USB_POWER_SOFTCONN;
    /* A delay seems necessary to avoid causing havoc. The USB spec says disconnect
     * detection time (T_DDIS) is around 2us but in practice many hubs might
     * require more. */
    target_mdelay(1);
    /* disable usb */
    REG_CPM_CLKGR0 |= CLKGR0_OTG;
}
