/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Michael Sparmann
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
#include "usb.h"
#include "usb_drv.h"

#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "panic.h"

#include "usb-s3c6400x.h"


#define DPTXFSIZ1   (*((uint32_t volatile*)(OTGBASE + 0x104)))
#define DPTXFSIZ2   (*((uint32_t volatile*)(OTGBASE + 0x108)))
#define DPTXFSIZ3   (*((uint32_t volatile*)(OTGBASE + 0x10C)))
#define DPTXFSIZ4   (*((uint32_t volatile*)(OTGBASE + 0x110)))
#define DPTXFSIZ5   (*((uint32_t volatile*)(OTGBASE + 0x114)))
#define DPTXFSIZ6   (*((uint32_t volatile*)(OTGBASE + 0x118)))
#define DPTXFSIZ7   (*((uint32_t volatile*)(OTGBASE + 0x11C)))
#define DPTXFSIZ8   (*((uint32_t volatile*)(OTGBASE + 0x120)))
#define DPTXFSIZ9   (*((uint32_t volatile*)(OTGBASE + 0x124)))
#define DPTXFSIZ10  (*((uint32_t volatile*)(OTGBASE + 0x128)))
#define DPTXFSIZ11  (*((uint32_t volatile*)(OTGBASE + 0x12C)))
#define DPTXFSIZ12  (*((uint32_t volatile*)(OTGBASE + 0x130)))
#define DPTXFSIZ13  (*((uint32_t volatile*)(OTGBASE + 0x134)))
#define DPTXFSIZ14  (*((uint32_t volatile*)(OTGBASE + 0x138)))
#define DPTXFSIZ15  (*((uint32_t volatile*)(OTGBASE + 0x13C)))

#define DIEPINT(x)  (*((uint32_t volatile*)(OTGBASE + 0x908 + 0x20 * (x))))
#define DIEPTSIZ(x) (*((uint32_t volatile*)(OTGBASE + 0x910 + 0x20 * (x))))
#define DIEPDMA(x)  (*((const void* volatile*)(OTGBASE + 0x914 + 0x20 * (x))))
#define DIEPCTL0    (*((uint32_t volatile*)(OTGBASE + 0x900)))
#define DIEPINT0    (*((uint32_t volatile*)(OTGBASE + 0x908)))
#define DIEPTSIZ0   (*((uint32_t volatile*)(OTGBASE + 0x910)))
#define DIEPDMA0    (*((const void* volatile*)(OTGBASE + 0x914)))
#define DIEPCTL1    (*((uint32_t volatile*)(OTGBASE + 0x920)))
#define DIEPINT1    (*((uint32_t volatile*)(OTGBASE + 0x928)))
#define DIEPTSIZ1   (*((uint32_t volatile*)(OTGBASE + 0x930)))
#define DIEPDMA1    (*((const void* volatile*)(OTGBASE + 0x934)))
#define DIEPCTL2    (*((uint32_t volatile*)(OTGBASE + 0x940)))
#define DIEPINT2    (*((uint32_t volatile*)(OTGBASE + 0x948)))
#define DIEPTSIZ2   (*((uint32_t volatile*)(OTGBASE + 0x950)))
#define DIEPDMA2    (*((const void* volatile*)(OTGBASE + 0x954)))
#define DIEPCTL3    (*((uint32_t volatile*)(OTGBASE + 0x960)))
#define DIEPINT3    (*((uint32_t volatile*)(OTGBASE + 0x968)))
#define DIEPTSIZ3   (*((uint32_t volatile*)(OTGBASE + 0x970)))
#define DIEPDMA3    (*((const void* volatile*)(OTGBASE + 0x974)))
#define DIEPCTL4    (*((uint32_t volatile*)(OTGBASE + 0x980)))
#define DIEPINT4    (*((uint32_t volatile*)(OTGBASE + 0x988)))
#define DIEPTSIZ4   (*((uint32_t volatile*)(OTGBASE + 0x990)))
#define DIEPDMA4    (*((const void* volatile*)(OTGBASE + 0x994)))
#define DIEPCTL5    (*((uint32_t volatile*)(OTGBASE + 0x9A0)))
#define DIEPINT5    (*((uint32_t volatile*)(OTGBASE + 0x9A8)))
#define DIEPTSIZ5   (*((uint32_t volatile*)(OTGBASE + 0x9B0)))
#define DIEPDMA5    (*((const void* volatile*)(OTGBASE + 0x9B4)))
#define DIEPCTL6    (*((uint32_t volatile*)(OTGBASE + 0x9C0)))
#define DIEPINT6    (*((uint32_t volatile*)(OTGBASE + 0x9C8)))
#define DIEPTSIZ6   (*((uint32_t volatile*)(OTGBASE + 0x9D0)))
#define DIEPDMA6    (*((const void* volatile*)(OTGBASE + 0x9D4)))
#define DIEPCTL7    (*((uint32_t volatile*)(OTGBASE + 0x9E0)))
#define DIEPINT7    (*((uint32_t volatile*)(OTGBASE + 0x9E8)))
#define DIEPTSIZ7   (*((uint32_t volatile*)(OTGBASE + 0x9F0)))
#define DIEPDMA7    (*((const void* volatile*)(OTGBASE + 0x9F4)))
#define DIEPCTL8    (*((uint32_t volatile*)(OTGBASE + 0xA00)))
#define DIEPINT8    (*((uint32_t volatile*)(OTGBASE + 0xA08)))
#define DIEPTSIZ8   (*((uint32_t volatile*)(OTGBASE + 0xA10)))
#define DIEPDMA8    (*((const void* volatile*)(OTGBASE + 0xA14)))
#define DIEPCTL9    (*((uint32_t volatile*)(OTGBASE + 0xA20)))
#define DIEPINT9    (*((uint32_t volatile*)(OTGBASE + 0xA28)))
#define DIEPTSIZ9   (*((uint32_t volatile*)(OTGBASE + 0xA30)))
#define DIEPDMA9    (*((const void* volatile*)(OTGBASE + 0xA34)))
#define DIEPCTL10   (*((uint32_t volatile*)(OTGBASE + 0xA40)))
#define DIEPINT10   (*((uint32_t volatile*)(OTGBASE + 0xA48)))
#define DIEPTSIZ10  (*((uint32_t volatile*)(OTGBASE + 0xA50)))
#define DIEPDMA10   (*((const void* volatile*)(OTGBASE + 0xA54)))
#define DIEPCTL11   (*((uint32_t volatile*)(OTGBASE + 0xA60)))
#define DIEPINT11   (*((uint32_t volatile*)(OTGBASE + 0xA68)))
#define DIEPTSIZ11  (*((uint32_t volatile*)(OTGBASE + 0xA70)))
#define DIEPDMA11   (*((const void* volatile*)(OTGBASE + 0xA74)))
#define DIEPCTL12   (*((uint32_t volatile*)(OTGBASE + 0xA80)))
#define DIEPINT12   (*((uint32_t volatile*)(OTGBASE + 0xA88)))
#define DIEPTSIZ12  (*((uint32_t volatile*)(OTGBASE + 0xA90)))
#define DIEPDMA12   (*((const void* volatile*)(OTGBASE + 0xA94)))
#define DIEPCTL13   (*((uint32_t volatile*)(OTGBASE + 0xAA0)))
#define DIEPINT13   (*((uint32_t volatile*)(OTGBASE + 0xAA8)))
#define DIEPTSIZ13  (*((uint32_t volatile*)(OTGBASE + 0xAB0)))
#define DIEPDMA13   (*((const void* volatile*)(OTGBASE + 0xAB4)))
#define DIEPCTL14   (*((uint32_t volatile*)(OTGBASE + 0xAC0)))
#define DIEPINT14   (*((uint32_t volatile*)(OTGBASE + 0xAC8)))
#define DIEPTSIZ14  (*((uint32_t volatile*)(OTGBASE + 0xAD0)))
#define DIEPDMA14   (*((const void* volatile*)(OTGBASE + 0xAD4)))
#define DIEPCTL15   (*((uint32_t volatile*)(OTGBASE + 0xAE0)))
#define DIEPINT15   (*((uint32_t volatile*)(OTGBASE + 0xAE8)))
#define DIEPTSIZ15  (*((uint32_t volatile*)(OTGBASE + 0xAF0)))
#define DIEPDMA15   (*((const void* volatile*)(OTGBASE + 0xAF4)))
#define DOEPDMA(x)  (*((void* volatile*)(OTGBASE + 0xB14 + 0x20 * (x))))
#define DOEPCTL0    (*((uint32_t volatile*)(OTGBASE + 0xB00)))
#define DOEPINT0    (*((uint32_t volatile*)(OTGBASE + 0xB08)))
#define DOEPTSIZ0   (*((uint32_t volatile*)(OTGBASE + 0xB10)))
#define DOEPDMA0    (*((void* volatile*)(OTGBASE + 0xB14)))
#define DOEPCTL1    (*((uint32_t volatile*)(OTGBASE + 0xB20)))
#define DOEPINT1    (*((uint32_t volatile*)(OTGBASE + 0xB28)))
#define DOEPTSIZ1   (*((uint32_t volatile*)(OTGBASE + 0xB30)))
#define DOEPDMA1    (*((void* volatile*)(OTGBASE + 0xB34)))
#define DOEPCTL2    (*((uint32_t volatile*)(OTGBASE + 0xB40)))
#define DOEPINT2    (*((uint32_t volatile*)(OTGBASE + 0xB48)))
#define DOEPTSIZ2   (*((uint32_t volatile*)(OTGBASE + 0xB50)))
#define DOEPDMA2    (*((void* volatile*)(OTGBASE + 0xB54)))
#define DOEPCTL3    (*((uint32_t volatile*)(OTGBASE + 0xB60)))
#define DOEPINT3    (*((uint32_t volatile*)(OTGBASE + 0xB68)))
#define DOEPTSIZ3   (*((uint32_t volatile*)(OTGBASE + 0xB70)))
#define DOEPDMA3    (*((void* volatile*)(OTGBASE + 0xB74)))
#define DOEPCTL4    (*((uint32_t volatile*)(OTGBASE + 0xB80)))
#define DOEPINT4    (*((uint32_t volatile*)(OTGBASE + 0xB88)))
#define DOEPTSIZ4   (*((uint32_t volatile*)(OTGBASE + 0xB90)))
#define DOEPDMA4    (*((void* volatile*)(OTGBASE + 0xB94)))
#define DOEPCTL5    (*((uint32_t volatile*)(OTGBASE + 0xBA0)))
#define DOEPINT5    (*((uint32_t volatile*)(OTGBASE + 0xBA8)))
#define DOEPTSIZ5   (*((uint32_t volatile*)(OTGBASE + 0xBB0)))
#define DOEPDMA5    (*((void* volatile*)(OTGBASE + 0xBB4)))
#define DOEPCTL6    (*((uint32_t volatile*)(OTGBASE + 0xBC0)))
#define DOEPINT6    (*((uint32_t volatile*)(OTGBASE + 0xBC8)))
#define DOEPTSIZ6   (*((uint32_t volatile*)(OTGBASE + 0xBD0)))
#define DOEPDMA6    (*((void* volatile*)(OTGBASE + 0xBD4)))
#define DOEPCTL7    (*((uint32_t volatile*)(OTGBASE + 0xBE0)))
#define DOEPINT7    (*((uint32_t volatile*)(OTGBASE + 0xBE8)))
#define DOEPTSIZ7   (*((uint32_t volatile*)(OTGBASE + 0xBF0)))
#define DOEPDMA7    (*((void* volatile*)(OTGBASE + 0xBF4)))
#define DOEPCTL8    (*((uint32_t volatile*)(OTGBASE + 0xC00)))
#define DOEPINT8    (*((uint32_t volatile*)(OTGBASE + 0xC08)))
#define DOEPTSIZ8   (*((uint32_t volatile*)(OTGBASE + 0xC10)))
#define DOEPDMA8    (*((void* volatile*)(OTGBASE + 0xC14)))
#define DOEPCTL9    (*((uint32_t volatile*)(OTGBASE + 0xC20)))
#define DOEPINT9    (*((uint32_t volatile*)(OTGBASE + 0xC28)))
#define DOEPTSIZ9   (*((uint32_t volatile*)(OTGBASE + 0xC30)))
#define DOEPDMA9    (*((void* volatile*)(OTGBASE + 0xC34)))
#define DOEPCTL10   (*((uint32_t volatile*)(OTGBASE + 0xC40)))
#define DOEPINT10   (*((uint32_t volatile*)(OTGBASE + 0xC48)))
#define DOEPTSIZ10  (*((uint32_t volatile*)(OTGBASE + 0xC50)))
#define DOEPDMA10   (*((void* volatile*)(OTGBASE + 0xC54)))
#define DOEPCTL11   (*((uint32_t volatile*)(OTGBASE + 0xC60)))
#define DOEPINT11   (*((uint32_t volatile*)(OTGBASE + 0xC68)))
#define DOEPTSIZ11  (*((uint32_t volatile*)(OTGBASE + 0xC70)))
#define DOEPDMA11   (*((void* volatile*)(OTGBASE + 0xC74)))
#define DOEPCTL12   (*((uint32_t volatile*)(OTGBASE + 0xC80)))
#define DOEPINT12   (*((uint32_t volatile*)(OTGBASE + 0xC88)))
#define DOEPTSIZ12  (*((uint32_t volatile*)(OTGBASE + 0xC90)))
#define DOEPDMA12   (*((void* volatile*)(OTGBASE + 0xC94)))
#define DOEPCTL13   (*((uint32_t volatile*)(OTGBASE + 0xCA0)))
#define DOEPINT13   (*((uint32_t volatile*)(OTGBASE + 0xCA8)))
#define DOEPTSIZ13  (*((uint32_t volatile*)(OTGBASE + 0xCB0)))
#define DOEPDMA13   (*((void* volatile*)(OTGBASE + 0xCB4)))
#define DOEPCTL14   (*((uint32_t volatile*)(OTGBASE + 0xCC0)))
#define DOEPINT14   (*((uint32_t volatile*)(OTGBASE + 0xCC8)))
#define DOEPTSIZ14  (*((uint32_t volatile*)(OTGBASE + 0xCD0)))
#define DOEPDMA14   (*((void* volatile*)(OTGBASE + 0xCD4)))
#define DOEPCTL15   (*((uint32_t volatile*)(OTGBASE + 0xCE0)))
#define DOEPINT15   (*((uint32_t volatile*)(OTGBASE + 0xCE8)))
#define DOEPTSIZ15  (*((uint32_t volatile*)(OTGBASE + 0xCF0)))
#define DOEPDMA15   (*((void* volatile*)(OTGBASE + 0xCF4)))




#define DIEPCTL(x)  (*((uint32_t volatile*)(OTGBASE + 0x900 + 0x20 * (x))))
/* Device Logical OUT Endpoint-Specific Registers */
#define DOEPCTL(x)  (*((uint32_t volatile*)(OTGBASE + 0xB00 + 0x20 * (x))))
/** Device IN Endpoint (ep) Transfer Size Register */
#define DIEPTSIZ(x) (*((uint32_t volatile*)(OTGBASE + 0x910 + 0x20 * (x))))
/** Device OUT Endpoint (ep) Transfer Size Register */
#define DOEPTSIZ(x) (*((uint32_t volatile*)(OTGBASE + 0xB10 + 0x20 * (x))))
#define DIEPINT(x)  (*((uint32_t volatile*)(OTGBASE + 0x908 + 0x20 * (x))))
#define DOEPINT(x)  (*((uint32_t volatile*)(OTGBASE + 0xB08 + 0x20 * (x))))
//#define DIEPDMA(x)  (*((const void* volatile*)(OTGBASE + 0x914 + 0x20 * (x)))
//#define DOEPDMA(x)  (*((const void* volatile*)(OTGBASE + 0xB14 + 0x20 * (x)))



#ifdef HAVE_USBSTACK
#include "usb_ch9.h"
#include "usb_core.h"
#include <inttypes.h>
#include "power.h"

struct ep_type
{
    bool active;
    bool busy;
    bool done;
    int rc;
    int size;
    struct semaphore complete;
} ;

static struct ep_type endpoints[USB_NUM_ENDPOINTS];

/* USB control requests may be up to 64 bytes in size.
   Even though we never use anything more than the 8 header bytes,
   we are required to accept request packets of up to 64 bytes size.
   Provide buffer space for these additional payload bytes so that
   e.g. write descriptor requests (which are rejected by us, but the
   payload is transferred anyway) do not cause memory corruption.
   Fixes FS#12310. -- Michael Sparmann (theseven) */
static struct
{
    struct usb_ctrlrequest header; /* 8 bytes */
    unsigned char payload[64 - sizeof(struct usb_ctrlrequest)];
} ctrlreq USB_DEVBSS_ATTR;

int usb_drv_port_speed(void)
{
    return (DSTS & 2) == 0 ? 1 : 0;
}

static void reset_endpoints(int reinit)
{
    unsigned int i;
    for (i = 0; i < sizeof(endpoints)/sizeof(struct ep_type); i++)
    {
        if (reinit) endpoints[i].active = false;
        endpoints[i].busy = false;
        endpoints[i].rc = -1;
        endpoints[i].done = true;
        semaphore_release(&endpoints[i].complete);
    }
    DIEPCTL0 = 0x8800;  /* EP0 IN ACTIVE NEXT=1 */
    DOEPCTL0 = 0x8000;  /* EP0 OUT ACTIVE */
    DOEPTSIZ0 = 0x20080040;  /* EP0 OUT Transfer Size:
                                64 Bytes, 1 Packet, 1 Setup Packet */
    DOEPDMA0 = &ctrlreq;
    DOEPCTL0 |= 0x84000000;  /* EP0 OUT ENABLE CLEARNAK */
    if (reinit)
    {
        /* The size is getting set to zero, because we don't know
           whether we are Full Speed or High Speed at this stage */
        /* EP1 IN INACTIVE DATA0 SIZE=0 NEXT=3 */
        DIEPCTL1 = 0x10001800;
        /* EP2 OUT INACTIVE DATA0 SIZE=0 */
        DOEPCTL2 = 0x10000000;
        /* EP3 IN INACTIVE DATA0 SIZE=0 NEXT=0 */
        DIEPCTL3 = 0x10000000;
        /* EP4 OUT INACTIVE DATA0 SIZE=0 */
        DOEPCTL4 = 0x10000000;
    }
    else
    {
        /* INACTIVE DATA0 */
        DIEPCTL1 = (DIEPCTL1 & ~0x00008000) | 0x10000000;
        DOEPCTL2 = (DOEPCTL2 & ~0x00008000) | 0x10000000;
        DIEPCTL3 = (DIEPCTL3 & ~0x00008000) | 0x10000000;
        DOEPCTL4 = (DOEPCTL4 & ~0x00008000) | 0x10000000;
    }
    DAINTMSK = 0xFFFFFFFF;  /* Enable interrupts on all EPs */
}

int usb_drv_request_endpoint(int type, int dir)
{
    size_t ep;
    int ret = -1;

    if (dir == USB_DIR_IN) ep = 1;
    else ep = 2;

    while (ep < USB_NUM_ENDPOINTS)
    {
        if (!endpoints[ep].active)
        {
            endpoints[ep].active = true;
            ret = ep | dir;
            uint32_t newbits = (type << 18) | 0x10000000;
            if (dir) DIEPCTL(ep) = (DIEPCTL(ep) & ~0x000C0000) | newbits;
            else DOEPCTL(ep) = (DOEPCTL(ep) & ~0x000C0000) | newbits;
            break;
        }
        ep += 2;
    }

    return ret;
}

void usb_drv_release_endpoint(int ep)
{
    ep = ep & 0x7f;

    if (ep < 1 || ep > USB_NUM_ENDPOINTS) return;

    endpoints[ep].active = false;
}

static void usb_reset(void)
{
    DCTL = 0x802;  /* Soft Disconnect */

    OPHYPWR = 0;  /* PHY: Power up */
    udelay(10);
    OPHYUNK1 = 1;
    OPHYUNK2 = 0xE3F;
    ORSTCON = 1;  /* PHY: Assert Software Reset */
    udelay(10);
    ORSTCON = 0;  /* PHY: Deassert Software Reset */
    OPHYUNK3 = 0x600;
    OPHYCLK = SYNOPSYSOTG_CLOCK;
    udelay(400);

    GRSTCTL = 1;  /* OTG: Assert Software Reset */
    while (GRSTCTL & 1);  /* Wait for OTG to ack reset */
    while (!(GRSTCTL & 0x80000000));  /* Wait for OTG AHB master idle */

    GRXFSIZ = 0x00000200;  /* RX FIFO: 512 bytes */
    GNPTXFSIZ = 0x02000200;  /* Non-periodic TX FIFO: 512 bytes */
    GAHBCFG = SYNOPSYSOTG_AHBCFG;
    GUSBCFG = 0x1408;  /* OTG: 16bit PHY and some reserved bits */

    DCFG = 4;  /* Address 0 */
    DCTL = 0x800;  /* Soft Reconnect */
    DIEPMSK = 0x0D;  /* IN EP interrupt mask */
    DOEPMSK = 0x0D;  /* IN EP interrupt mask */
    DAINTMSK = 0xFFFFFFFF;  /* Enable interrupts on all endpoints */
    GINTMSK = 0xC3000;  /* Interrupt mask: IN event, OUT event, bus reset */

    reset_endpoints(1);
}

/* IRQ handler */
void INT_USB_FUNC(void)
{
    int i;
    uint32_t ints = GINTSTS;
    uint32_t epints;
    if (ints & 0x1000)  /* bus reset */
    {
        DCFG = 4;  /* Address 0 */
        reset_endpoints(1);
        usb_core_bus_reset();
    }

    if (ints & 0x2000)  /* enumeration done, we now know the speed */
    {
        /* Set up the maximum packet sizes accordingly */
        uint32_t maxpacket = usb_drv_port_speed() ? 512 : 64;
        DIEPCTL1 = (DIEPCTL1 & ~0x000003FF) | maxpacket;
        DOEPCTL2 = (DOEPCTL2 & ~0x000003FF) | maxpacket;
        DIEPCTL3 = (DIEPCTL3 & ~0x000003FF) | maxpacket;
        DOEPCTL4 = (DOEPCTL4 & ~0x000003FF) | maxpacket;
    }

    if (ints & 0x40000)  /* IN EP event */
        for (i = 0; i < 4; i += i + 1)  // 0, 1, 3
            if ((epints = DIEPINT(i)))
            {
                if (epints & 1)  /* Transfer completed */
                {
                    commit_discard_dcache();
                    int bytes = endpoints[i].size - (DIEPTSIZ(i) & 0x3FFFF);
                    if (endpoints[i].busy)
                    {
                        endpoints[i].busy = false;
                        endpoints[i].rc = 0;
                        endpoints[i].done = true;
                        usb_core_transfer_complete(i, USB_DIR_IN, 0, bytes);
                        semaphore_release(&endpoints[i].complete);
                    }
                }
                if (epints & 4)  /* AHB error */
                    panicf("USB: AHB error on IN EP%d", i);
                if (epints & 8)  /* Timeout */
                {
                    if (endpoints[i].busy)
                    {
                        endpoints[i].busy = false;
                        endpoints[i].rc = 1;
                        endpoints[i].done = true;
                        semaphore_release(&endpoints[i].complete);
                    }
                }
                DIEPINT(i) = epints;
            }

    if (ints & 0x80000)  /* OUT EP event */
        for (i = 0; i < USB_NUM_ENDPOINTS; i += 2)
            if ((epints = DOEPINT(i)))
            {
                if (epints & 1)  /* Transfer completed */
                {
                    commit_discard_dcache();
                    int bytes = endpoints[i].size - (DOEPTSIZ(i) & 0x3FFFF);
                    if (endpoints[i].busy)
                    {
                        endpoints[i].busy = false;
                        endpoints[i].rc = 0;
                        endpoints[i].done = true;
                        usb_core_transfer_complete(i, USB_DIR_OUT, 0, bytes);
                        semaphore_release(&endpoints[i].complete);
                    }
                }
                if (epints & 4)  /* AHB error */
                    panicf("USB: AHB error on OUT EP%d", i);
                if (epints & 8)  /* SETUP phase done */
                {
                    commit_discard_dcache();
                    if (i == 0)
                    {
                        if (ctrlreq.header.bRequest == 5)
                        {
                            /* Already set the new address here,
                               before passing the packet to the core.
                               See below (usb_drv_set_address) for details. */
                            DCFG = (DCFG & ~0x7F0) | (ctrlreq.header.wValue << 4);
                        }
                        usb_core_control_request(&ctrlreq.header);
                    }
                    else panicf("USB: SETUP done on OUT EP%d!?", i);
                }
                /* Make sure EP0 OUT is set up to accept the next request */
                if (!i)
                {
                    DOEPTSIZ0 = 0x20080040;
                    DOEPDMA0 = &ctrlreq;
                    DOEPCTL0 |= 0x84000000;
                }
                DOEPINT(i) = epints;
            }

    GINTSTS = ints;
}

void usb_drv_set_address(int address)
{
    (void)address;
    /* Ignored intentionally, because the controller requires us to set the
       new address before sending the response for some reason. So we'll
       already set it when the control request arrives, before passing that
       into the USB core, which will then call this dummy function. */
}

static void ep_send(int ep, const void *ptr, int length)
{
    endpoints[ep].busy = true;
    endpoints[ep].size = length;
    DIEPCTL(ep) |= 0x8000;  /* EPx OUT ACTIVE */
    int blocksize = usb_drv_port_speed() ? 512 : 64;
    int packets = (length + blocksize - 1) / blocksize;
    if (!length)
    {
        DIEPTSIZ(ep) = 1 << 19;  /* one empty packet */
        DIEPDMA(ep) = NULL;
    }
    else
    {
        DIEPTSIZ(ep) = length | (packets << 19);
        DIEPDMA(ep) = ptr;
    }
    commit_dcache();
    DIEPCTL(ep) |= 0x84000000;  /* EPx OUT ENABLE CLEARNAK */
}

static void ep_recv(int ep, void *ptr, int length)
{
    endpoints[ep].busy = true;
    endpoints[ep].size = length;
    DOEPCTL(ep) &= ~0x20000;  /* EPx UNSTALL */
    DOEPCTL(ep) |= 0x8000;  /* EPx OUT ACTIVE */
    int blocksize = usb_drv_port_speed() ? 512 : 64;
    int packets = (length + blocksize - 1) / blocksize;
    if (!length)
    {
        DOEPTSIZ(ep) = 1 << 19;  /* one empty packet */
        DOEPDMA(ep) = NULL;
    }
    else
    {
        DOEPTSIZ(ep) = length | (packets << 19);
        DOEPDMA(ep) = ptr;
    }
    commit_dcache();
    DOEPCTL(ep) |= 0x84000000;  /* EPx OUT ENABLE CLEARNAK */
}

int usb_drv_send(int endpoint, void *ptr, int length)
{
    endpoint &= 0x7f;
    endpoints[endpoint].done = false;
    ep_send(endpoint, ptr, length);
    while (!endpoints[endpoint].done && endpoints[endpoint].busy)
        semaphore_wait(&endpoints[endpoint].complete, TIMEOUT_BLOCK);
    return endpoints[endpoint].rc;
}

int usb_drv_send_nonblocking(int endpoint, void *ptr, int length)
{
    ep_send(endpoint & 0x7f, ptr, length);
    return 0;
}

int usb_drv_recv(int endpoint, void* ptr, int length)
{
    ep_recv(endpoint & 0x7f, ptr, length);
    return 0;
}

void usb_drv_cancel_all_transfers(void)
{
    int flags = disable_irq_save();
    reset_endpoints(0);
    restore_irq(flags);
}

void usb_drv_set_test_mode(int mode)
{
    (void)mode;
}

bool usb_drv_stalled(int endpoint, bool in)
{
    if (in) return DIEPCTL(endpoint) & 0x00200000 ? true : false;
    else return DOEPCTL(endpoint) & 0x00200000 ? true : false;
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
    if (in)
    {
        if (stall) DIEPCTL(endpoint) |= 0x00200000;
        else DIEPCTL(endpoint) &= ~0x00200000;
    }
    else
    {
        if (stall) DOEPCTL(endpoint) |= 0x00200000;
        else DOEPCTL(endpoint) &= ~0x00200000;
    }
}

void usb_drv_init(void)
{
    /* Enable USB clock */
#if CONFIG_CPU==S5L8701
    PWRCON &= ~0x4000;
    PWRCONEXT &= ~0x800;
    INTMSK |= INTMSK_USB_OTG;
#elif CONFIG_CPU==S5L8702
    PWRCON(0) &= ~0x4;
    PWRCON(1) &= ~0x8;
    VIC0INTENABLE |= 1 << 19;
#endif
    PCGCCTL = 0;

    /* reset the beast */
    usb_reset();
}

void usb_drv_exit(void)
{
    DCTL = 0x802;  /* Soft Disconnect */

    OPHYPWR = 0xF;  /* PHY: Power down */
    udelay(10);
    ORSTCON = 7;  /* Put the PHY into reset (needed to get current down) */
    udelay(10);
    PCGCCTL = 1;  /* Shut down PHY clock */
    
#if CONFIG_CPU==S5L8701
    PWRCON |= 0x4000;
    PWRCONEXT |= 0x800;
#elif CONFIG_CPU==S5L8702
    PWRCON(0) |= 0x4;
    PWRCON(1) |= 0x8;
#endif
}

void usb_init_device(void)
{
    unsigned int i;
    for (i = 0; i < sizeof(endpoints)/sizeof(struct ep_type); i++)
        semaphore_init(&endpoints[i].complete, 1, 0);

    /* Power up the core clocks to allow writing
       to some registers needed to power it down */
    PCGCCTL = 0;
#if CONFIG_CPU==S5L8701
    PWRCON &= ~0x4000;
    PWRCONEXT &= ~0x800;
    INTMSK |= INTMSK_USB_OTG;
#elif CONFIG_CPU==S5L8702
    PWRCON(0) &= ~0x4;
    PWRCON(1) &= ~0x8;
    VIC0INTENABLE |= 1 << 19;
#endif

    usb_drv_exit();
}

void usb_enable(bool on)
{
    if (on) usb_core_init();
    else usb_core_exit();
}

void usb_attach(void)
{
    usb_enable(true);
}

int usb_detect(void)
{
    if (charger_inserted())
        return USB_INSERTED;
    return USB_EXTRACTED;
}

#else
void usb_init_device(void)
{
    DCTL = 0x802;  /* Soft Disconnect */

    ORSTCON = 1;  /* Put the PHY into reset (needed to get current down) */
    PCGCCTL = 1;  /* Shut down PHY clock */
    OPHYPWR = 0xF;  /* PHY: Power down */
    
#if CONFIG_CPU==S5L8701
    PWRCON |= 0x4000;
    PWRCONEXT |= 0x800;
#elif CONFIG_CPU==S5L8702
    PWRCON(0) |= 0x4;
    PWRCON(1) |= 0x8;
#endif
}

void usb_enable(bool on)
{
    (void)on;
}

/* Always return false for now */
int usb_detect(void)
{
    return USB_EXTRACTED;
}
#endif
