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
#include "atj213x.h"

#define USB_FULL_SPEED 0
#define USB_HIGH_SPEED 1

volatile bool setup_data_valid = false;
volatile int udc_speed = USB_FULL_SPEED;

static void usb_copy_from(void *ptr, volatile void *reg, size_t sz)
{
    uint32_t *p = ptr;
    volatile uint32_t *rp = reg;
    /* do not overflow the destination buffer ! */
    while(sz >= 4)
    {
        *p++ = *rp++;
        sz -= 4;
    }

    if(sz == 0)
        return;

    /* reminder */
    uint32_t cache = *rp;
    uint8_t *p8 = (void *)p;
    while(sz-- > 0)
    {
        *p8++ = cache;
        cache >>= 8;
    }
}

static void usb_copy_to(volatile void *reg, void *ptr, size_t sz)
{
    uint32_t *p = ptr;
    volatile uint32_t *rp = reg;
    sz = (sz + 3) / 4;
    /* read may overflow the source buffer but
     * it will not overwrite anything
     */
    while(sz-- > 0)
        *rp++ = *p++;
}

void INT_UDC(void)
{
    /* get possible sources */
    unsigned int usbirq = OTG_USBIRQ;
    unsigned int otgirq = OTG_OTGIRQ;
#if 0
    unsigned int usbeirq = OTG_USBEIRQ;
    unsigned int epinirq = OTG_IN04IRQ;
    unsigned int epoutirq = OTG_OUT04IRQ;
#endif

    /* HS, Reset, Setup */
    if (usbirq)
    {

        if (usbirq & (1<<5))
        {
            /* HS irq */
            udc_speed = USB_HIGH_SPEED;
        }
        else if (usbirq & (1<<4))
        {
            /* Reset */
            udc_speed = USB_FULL_SPEED;

            /* clear all pending irqs */
            OTG_OUT04IRQ = 0xff;
            OTG_IN04IRQ = 0xff;
        }
        else if (usbirq & (1<<0))
        {
            /* Setup data valid */
            setup_data_valid = true;
        }

        /* clear irq flags */
        OTG_USBIRQ = usbirq;
    }

#if 0
    if (epoutirq)
    {
        OTG_OUT04IRQ = epoutirq;
    }

    if (epinirq)
    {
        OTG_IN04IRQ = epinirq;
    }
#endif

    if (otgirq)
    {
        OTG_OTGIRQ = otgirq;
    }

    OTG_USBEIRQ = 0x50;
}

void usb_drv_init(void)
{
    OTG_USBCS |= 0x40;                    /* soft disconnect */

    OTG_ENDPRST = 0x10;                   /* reset all ep fifos */
    OTG_ENDPRST = 0x70;
    OTG_ENDPRST = 0x00;
    OTG_ENDPRST = 0x60;

    OTG_USBIRQ = 0xff;                    /* clear all pending interrupts */
    OTG_OTGIRQ = 0xff;
    OTG_IN04IRQ = 0xff;
    OTG_OUT04IRQ = 0xff;
    OTG_USBEIRQ = 0x50;                   /* UDC ? with 0x40 there is irq storm */

    OTG_USBIEN = (1<<5) | (1<<4) | (1<<0); /* HS, Reset, Setup_data */
    OTG_OTGIEN = 0;

    /* disable interrupts from ep0 */
    OTG_IN04IEN = 0;
    OTG_OUT04IEN = 0;

    /* unmask UDC interrupt in interrupt controller */
    INTC_MSK = (1<<4);

    target_mdelay(100);

    OTG_USBCS &= ~0x40;                  /* soft connect */
}

int usb_drv_recv_setup(struct usb_ctrlrequest *req)
{
    while (!setup_data_valid)
        ;

    usb_copy_from(req, &OTG_SETUPDAT, sizeof(struct usb_ctrlrequest));
    setup_data_valid = false;
    return 0;
}

int usb_drv_port_speed(void)
{
    return (int)udc_speed;
}

/* Set the address (usually it's in a register).
 * There is a problem here: some controller want the address to be set between
 * control out and ack and some want to wait for the end of the transaction.
 * In the first case, you need to write some code special code when getting
 * setup packets and ignore this function (have a look at other drives)
 */
void usb_drv_set_address(int address)
{
    (void)address;
    /* UDC sets this automaticaly */
}

/* TODO: Maybe adapt to irq scheme */
int usb_drv_send(int endpoint, void *ptr, int length)
{
    (void)endpoint;

    int xfer_size, cnt = length;

    while (cnt)
    {
        xfer_size = MIN(cnt, 64);

        /* copy data to ep0in buffer */
        usb_copy_to(&OTG_EP0INDAT, ptr, xfer_size);

        /* this marks data as ready to send */
        OTG_IN0BC = xfer_size;

        /* wait for the transfer end */
        while(OTG_EP0CS & 0x04)
            ;

        cnt -= xfer_size;
        ptr += xfer_size;
    }

    /* ZLP stage */
    if((length % 64) == 0)
        OTG_EP0CS = 2;

    return 0;
}

/* TODO:  Maybe adapt to irq scheme */
int usb_drv_recv(int endpoint, void* ptr, int length)
{
   (void)endpoint;
   int xfer_size, cnt = 0;

    while (cnt < length)
    {
        /* Arm receiving buffer by writing
         * any value to OUT0BC. This sets
         * OUT_BUSY bit in EP0CS until the data
         * are correctly received and ACK'd
         */
        OTG_OUT0BC = 0;

        while (OTG_EP0CS & 0x08)
            ;

        xfer_size = OTG_OUT0BC;

        usb_copy_from(ptr, &OTG_EP0OUTDAT, xfer_size);
        cnt += xfer_size;
        ptr += xfer_size;

        if (xfer_size < 64)
            break;
    }

    /* ZLP stage */
    if (length == 0)
        OTG_EP0CS = 2;

    return cnt;
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
    (void)endpoint;
    (void)in;

    /* only EP0 in hwstub */
    if (stall)
        OTG_EP0CS |= 1;
    else
        OTG_EP0CS &= ~1;
}

void usb_drv_exit(void)
{
}
