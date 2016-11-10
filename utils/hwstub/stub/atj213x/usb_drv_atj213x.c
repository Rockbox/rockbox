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

struct endpoint_t
{
    void *buf;
    int length;
    bool zlp;
    bool finished;
};

static volatile struct endpoint_t ep0in;
static volatile struct endpoint_t ep0out;

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

static void reset_all_fifos(void)
{
    /* reset all ep fifos */

    /* IN fifos */
    OTG_ENDPRST = 0x10;
    OTG_ENDPRST = 0x70;

    /* OUT fifos */
    OTG_ENDPRST = 0x00;
    OTG_ENDPRST = 0x60;
}

static void cancel_all_transfers(void)
{
    ep0out.buf = NULL;
    ep0out.length = 0;
    ep0out.zlp = false;
    ep0out.finished =  true;

    ep0in.buf = NULL;
    ep0in.length = 0;
    ep0in.zlp = false;
    ep0in.finished = true;
}

void usb_drv_init(void)
{
    /* soft disconnect */
    OTG_USBCS |= 0x40;

    cancel_all_transfers();
    reset_all_fifos();

    /* clear all pending interrupts */
    OTG_USBIRQ = 0xff;
    OTG_OTGIRQ = 0xff;
    OTG_IN04IRQ = 0xff;
    OTG_OUT04IRQ = 0xff;

    /* bit6 - USB wakeup
     * bit4 - connect/disconnect
     *
     * with 0x40 here there is irq storm
     */
    OTG_USBEIRQ = 0x50;

    /* HS, Reset, Setup_data */
    OTG_USBIEN = (1<<5) | (1<<4) | (1<<0);

    /* No OTG interrupts ? */
    OTG_OTGIEN = 0;

    /* enable interrupts from ep0 */
    OTG_IN04IEN = 1;
    OTG_OUT04IEN = 1;

    /* unmask UDC interrupt in interrupt controller */
    INTC_MSK = (1<<4);

    target_mdelay(100);

    /* soft connect */
    OTG_USBCS &= ~0x40;
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

static void ep0_write(void)
{
    int xfer_size = MIN(ep0in.length, 64);

    /* copy data to UDC buffer */
    usb_copy_to(&OTG_EP0INDAT, ep0in.buf, xfer_size);
    ep0in.buf += xfer_size;
    ep0in.length -= xfer_size;

    /* this marks data as ready to send */
    OTG_IN0BC = xfer_size;
}

int usb_drv_send(int endpoint, void *ptr, int length)
{
    (void)endpoint;

    if (length)
    {
        ep0in.length = length;
        ep0in.buf = ptr;
        ep0in.zlp = (length % 64 == 0) ? true : false;
        ep0in.finished = false;

        ep0_write();

        while(!ep0in.finished)
            ;
    }
    else
    {
        /* clear NAK bit to ACK host */
        OTG_EP0CS = 2;
    }

    return 0;
}

static int ep0_read(void)
{
    int xfer_size = OTG_OUT0BC;
    usb_copy_from(ep0out.buf, &OTG_EP0OUTDAT, xfer_size);
    ep0out.buf += xfer_size;
    ep0out.length -= xfer_size;

    return xfer_size;
}

int usb_drv_recv(int endpoint, void* ptr, int length)
{
   (void)endpoint;

   ep0out.length = length;

   if (length > 0)
   {
       ep0out.buf = ptr;
       ep0out.finished = false;

       /* Arm receiving buffer by writing
        * any value to OUT0BC. This sets
        * OUT_BUSY bit in EP0CS until the data
        * are correctly received and ACK'd
        */
       OTG_OUT0BC = 0;

       while (!ep0out.finished)
           ;
   }
   else
   {
       /* clear NAK bit to ACK host */
       OTG_EP0CS = 2;
   }

   return (length - ep0out.length);
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

void INT_UDC(void)
{
    /* get possible sources */
    unsigned int usbirq = OTG_USBIRQ;
    unsigned int otgirq = OTG_OTGIRQ;
    unsigned int epinirq = OTG_IN04IRQ;
    unsigned int epoutirq = OTG_OUT04IRQ;

    /* HS, Reset, Setup */
    if (usbirq)
    {

        if (usbirq & (1<<5)) /* HS irq */
        {
            udc_speed = USB_HIGH_SPEED;
        }
        else if (usbirq & (1<<4)) /* Reset irq */
        {
            cancel_all_transfers();
            reset_all_fifos();

            /* clear all pending EP irqs */
            OTG_OUT04IRQ = 0xff;
            OTG_IN04IRQ = 0xff;

            udc_speed = USB_FULL_SPEED;
            setup_data_valid = false;
        }
        else if (usbirq & (1<<0)) /* Setup data valid */
        {
            setup_data_valid = true;
        }

        /* clear irq flags */
        OTG_USBIRQ = usbirq;
    }

    if (epoutirq)
    {
        if (ep0_read() == 64)
        {
            /* rearm receive buffer */ 
            OTG_OUT0BC = 0;
        }
        else
        {
            /* short packet means end of transfer */   
            ep0out.finished = true;
        }

        /* ack interrupt */
        OTG_OUT04IRQ = epoutirq;
    }

    if (epinirq)
    {
        if (ep0in.length)
        {
            ep0_write();
        }
        else
        {
            if (ep0in.zlp)
            {
                /* clear NAK bit to ACK hosts ZLP */
                OTG_EP0CS = 2;
            }

            ep0in.finished = true;
        }

        /* ack interrupt */
        OTG_IN04IRQ = epinirq;
    }

    if (otgirq)
    {
        OTG_OTGIRQ = otgirq;
    }

    OTG_USBEIRQ = 0x50;
}
