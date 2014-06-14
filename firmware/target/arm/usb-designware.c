/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009-2014 by Michael Sparmann
 * Copyright © 2010 Amaury Pouly
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

#include "usb-designware-regs.h"
#include "usb-designware.h"


#include "usb_ch9.h"
#include "usb_core.h"
#include <inttypes.h>
#include "power.h"

//#define LOGF_ENABLE
#include "logf.h"

#include "tick.h"

#ifndef USB_DW_AHB_BURST_LEN
#define USB_DW_AHB_BURST_LEN 5
#endif
#ifndef USB_DW_AHB_THRESHOLD
#define USB_DW_AHB_THRESHOLD 8
#endif
#ifndef USB_DW_TURNAROUND
#define USB_DW_TURNAROUND 3
#endif

#define WHILE_WITH_TMO(cond, us) timeout = us; do { if (!--timeout) { panicf("%s(): %s", __func__, #cond); } udelay(1); } while (cond)

static union usb_ep0_buffer ep0_buffer USB_DEVBSS_ATTR;

struct usb_dw_ep_state //__attribute__((packed,aligned(4))) usb_dw_ep_state
{
    struct semaphore complete;
    uint32_t size;
    int8_t status;
    uint8_t active;
    uint8_t done;
    uint8_t busy;
};

static struct
{
    struct
    {
        uint32_t* rxaddr;
        struct usb_dw_ep_state state;
    } out;
    struct
    {
        const uint32_t* txaddr;
        struct usb_dw_ep_state state;
    } in;
} usb_dw_ep_state[16];

static void usb_dw_flush_out_endpoint(int ep)
{
    int timeout;

    if (!ep) return;
    if (DESIGNWARE_DOEPCTL(ep) & EPENA)// (usb_dw_config.core->outep_regs[ep].doepctl.b.epena)
    {
        // We are waiting for an OUT packet on this endpoint, which might arrive any moment.
        // Assert a global output NAK to avoid race conditions while shutting down the endpoint.
        usb_dw_target_disable_irq();
        DESIGNWARE_DCTL |= SGONAK;                   //usb_dw_config.core->dregs.dctl.b.sgoutnak = 1;

        //while (!(DESIGNWARE_GINTSTS & GOUTNAKEFF)); //(!(usb_dw_config.core->gregs.gintsts.b.goutnakeff));
        WHILE_WITH_TMO(!(DESIGNWARE_GINTSTS & GOUTNAKEFF), 1000);

        DESIGNWARE_DOEPCTL(ep) |= (EPDIS|SNAK);      // usb_dw_config.core->outep_regs[ep].doepctl.b.snak = 1;
                                                     // usb_dw_config.core->outep_regs[ep].doepctl.b.epdis = 1;
        //while (!(DESIGNWARE_DOEPINT(ep) & EPDISD)); // (!(usb_dw_config.core->outep_regs[ep].doepint.b.epdisabled));
        WHILE_WITH_TMO(!(DESIGNWARE_DOEPINT(ep) & EPDISD), 1000);

        DESIGNWARE_DCTL |= CGONAK;                  //usb_dw_config.core->dregs.dctl.b.cgoutnak = 1;
        usb_dw_target_enable_irq();
    }
    //usb_dw_config.core->outep_regs[ep].doepctl.b.usbactep = 0;  // This is broken in Rockbox.
    // Reset the transfer size register. Not strictly necessary, but can't hurt.
    
    DESIGNWARE_DOEPTSIZ(ep) = 0;                    //usb_dw_config.core->outep_regs[ep].doeptsiz.d32 = 0;
}

static void usb_dw_flush_in_endpoint(int ep)
{
    int timeout;
    if (!ep) return;
    if (DESIGNWARE_DIEPCTL(ep) & EPENA)               //(usb_dw_config.core->inep_regs[ep].diepctl.b.epena)
    {
        // We are shutting down an endpoint that might still have IN packets in the FIFO.
        // Disable the endpoint, wait for things to settle, and flush the relevant FIFO.
        usb_dw_target_disable_irq();
        DESIGNWARE_DIEPCTL(ep) |= SNAK;              //usb_dw_config.core->inep_regs[ep].diepctl.b.snak = 1;

        //while (!(DESIGNWARE_DIEPINT(ep) & INEPNE)); //(!(usb_dw_config.core->inep_regs[ep].diepint.b.inepnakeff));
        WHILE_WITH_TMO(!(DESIGNWARE_DIEPINT(ep) & INEPNE), 1000);

        DESIGNWARE_DIEPCTL(ep) |= EPDIS;            //usb_dw_config.core->inep_regs[ep].diepctl.b.epdis = 1;

        //while (!(DESIGNWARE_DIEPINT(ep) & EPDISD)); //(!(usb_dw_config.core->inep_regs[ep].diepint.b.epdisabled));
        WHILE_WITH_TMO(!(DESIGNWARE_DIEPINT(ep) & EPDISD), 1000);

        //usb_dw_config.core->inep_regs[ep].diepctl.b.usbactep = 0;  // This is broken in Rockbox.
        usb_dw_target_enable_irq();

        // Wait for any DMA activity to stop, to make sure nobody will touch the FIFO.
        //while (!(DESIGNWARE_GRSTCTL & AHBIDL));    //(!usb_dw_config.core->gregs.grstctl.b.ahbidle);
        WHILE_WITH_TMO(!(DESIGNWARE_GRSTCTL & AHBIDL), 1000);
        // Flush it all the way down!
                                                   //union usb_dw_grstctl grstctl = { .b = { .txfnum = usb_dw_config.core->inep_regs[ep].diepctl.b.txfnum, .txfflsh = 1 } };
                                                   //usb_dw_config.core->gregs.grstctl = grstctl;
        DESIGNWARE_GRSTCTL = GRSTCTL_TXFNUM((DESIGNWARE_DIEPCTL(ep)>>22) & 0x07) | TXFFLSH;

        //while (DESIGNWARE_GRSTCTL & TXFFLSH);     //(usb_dw_config.core->gregs.grstctl.b.txfflsh);
        WHILE_WITH_TMO(DESIGNWARE_GRSTCTL & TXFFLSH, 1000);
    }
    //else if (ep) usb_dw_config.core->inep_regs[ep].diepctl.b.usbactep = 0;  // This is broken in Rockbox.
    // Reset the transfer size register. Not strictly necessary, but can't hurt.
    DESIGNWARE_DIEPTSIZ(ep) = 0;                      //usb_dw_config.core->inep_regs[ep].dieptsiz.d32 = 0;
}

static void usb_dw_flush_ints(void)
{
    uint32_t gintsts, doepint, diepint;
    int i;
    for (i = 0; i < 16; i++)
    {
        //usb_dw_config.core->outep_regs[i].doepint = usb_dw_config.core->outep_regs[i].doepint;
        doepint = DESIGNWARE_DOEPINT(i);
        DESIGNWARE_DOEPINT(i) = doepint;
        
        //usb_dw_config.core->inep_regs[i].diepint = usb_dw_config.core->inep_regs[i].diepint;
        diepint = DESIGNWARE_DIEPINT(i);
        DESIGNWARE_DIEPINT(i) = diepint;
    }
    //usb_dw_config.core->gregs.gintsts = usb_dw_config.core->gregs.gintsts;
    gintsts = DESIGNWARE_GINTSTS;
    DESIGNWARE_GINTSTS = gintsts;
}

static void usb_dw_try_push(int ep)
{
    uint32_t diepctl = DESIGNWARE_DIEPCTL(ep); //union usb_dw_depctl depctl = usb_dw_config.core->inep_regs[ep].diepctl;
    if (!(diepctl & EPENA) || !(diepctl & USBAEP) || (diepctl & STALL) || (diepctl & NAKSTS)) return; //if (!depctl.b.epena || !depctl.b.usbactep || depctl.b.stall || depctl.b.naksts) return;
    
    uint32_t bytesleft = DESIGNWARE_DIEPTSIZ(ep) & 0x7ffff; //int bytesleft = usb_dw_config.core->inep_regs[ep].dieptsiz.b.xfersize;
    if (!bytesleft) return;   
    uint32_t maxpacket = ep ? (DESIGNWARE_DIEPCTL(ep) & 0x3ff) : 64; //int maxpacket = ep ? usb_dw_config.core->inep_regs[ep].diepctl.b.mps : 64;
    uint32_t fifospace = DESIGNWARE_GNPTXSTS; //union usb_dw_hnptxsts fifospace = usb_dw_config.core->gregs.hnptxsts;
    uint32_t words = (MIN(maxpacket, bytesleft) + 3) >> 2;
    if (((fifospace >> 16) & 0xff) && (((fifospace & 0xffff) << 2) >= words)) //if (fifospace.b.nptxqspcavail && fifospace.b.nptxfspcavail << 2 >= words)
        while (words--) DESIGNWARE_DFIFO(ep) = *usb_dw_ep_state[ep].in.txaddr++; //usb_dw_config.core->dfifo[ep][0] = *usb_dw_ep_state[ep].in.txaddr++;
    if (!words && bytesleft <= maxpacket) return;
    DESIGNWARE_GINTMSK |= NPTXFE; //usb_dw_config.core->gregs.gintmsk.b.nptxfempty = true;
}

int usb_dw_get_stall(struct usb_endpoint_number ep)
{
    if (ep.direction == USB_ENDPOINT_DIRECTION_IN)
        return !!(DESIGNWARE_DIEPCTL(ep.number) & STALL); // !!usb_dw_config.core->inep_regs[ep.number].diepctl.b.stall;
    return !!(DESIGNWARE_DOEPCTL(ep.number) & STALL);     // !!usb_dw_config.core->outep_regs[ep.number].doepctl.b.stall;
}

void usb_dw_set_stall(struct usb_endpoint_number ep, int stall)
{
    if (ep.direction == USB_ENDPOINT_DIRECTION_IN)
    {
        //usb_dw_config.core->inep_regs[ep.number].diepctl.b.stall = !!stall;
        if (stall)
        {
            DESIGNWARE_DIEPCTL(ep.number) |= STALL;
        }
        else
        {
            DESIGNWARE_DIEPCTL(ep.number) &= ~STALL;
            DESIGNWARE_DIEPCTL(ep.number) |= SD0PID; // if (!stall) usb_dw_config.core->inep_regs[ep.number].diepctl.b.setd0pid = true;
        }
    }
    else
    {
        //usb_dw_config.core->outep_regs[ep.number].doepctl.b.stall = !!stall;
        if (stall)
        {
            DESIGNWARE_DOEPCTL(ep.number) |= STALL;
        }
        else
        {
            DESIGNWARE_DOEPCTL(ep.number) &= ~STALL;
            DESIGNWARE_DOEPCTL(ep.number) |= SD0PID; //if (!stall) usb_dw_config.core->outep_regs[ep.number].doepctl.b.setd0pid = true;
        }
    }
}

void usb_dw_unconfigure_ep(struct usb_endpoint_number ep)
{
    if (ep.direction == USB_ENDPOINT_DIRECTION_IN)
    {
        // Kill any outstanding IN transfers
        usb_dw_flush_in_endpoint(ep.number);
        // Mask interrupts for this endpoint
        DESIGNWARE_DAINTMSK &= ~(1<<ep.number); //usb_dw_config.core->dregs.daintmsk.ep.in &= ~(1 << ep.number);
    }
    else
    {
        // Kill any outstanding OUT transfers
        usb_dw_flush_out_endpoint(ep.number);
        // Mask interrupts for this endpoint
        DESIGNWARE_DAINTMSK &= ~((1<<ep.number)<<16); //usb_dw_config.core->dregs.daintmsk.ep.out &= ~(1 << ep.number);
    }
}

void usb_dw_configure_ep(struct usb_endpoint_number ep,
                         enum usb_endpoint_type type, int maxpacket)
{
    // Write the new configuration and unmask interrupts for the endpoint.
    // Reset data toggle to DATA0, as required by the USB specification.
    int txfifo = usb_dw_config.shared_txfifo ? 0 : ep.number;
    int nextep = (DESIGNWARE_DIEPCTL(ep.number) >> 10) & 0x0f; //usb_dw_config.core->inep_regs[ep.number].diepctl.b.nextep;
    //union usb_dw_depctl depctl = { .b = { .usbactep = 1, .eptype = type, .mps = maxpacket,
    //                                      .txfnum = txfifo, .setd0pid = 1, .nextep = nextep } };
    if (ep.direction == USB_ENDPOINT_DIRECTION_IN)
    {
		DESIGNWARE_DIEPCTL(ep.number) = SD0PID|DTXFNUM(txfifo)|EPTYP(type)|USBAEP|NEXTEP(nextep)|maxpacket; //usb_dw_config.core->inep_regs[ep.number].diepctl = depctl;
        DESIGNWARE_DAINTMSK |= (1<<ep.number); //usb_dw_config.core->dregs.daintmsk.ep.in |= (1 << ep.number);
    }
    else
    {
		DESIGNWARE_DOEPCTL(ep.number) = SD0PID|DTXFNUM(txfifo)|EPTYP(type)|USBAEP|NEXTEP(nextep)|maxpacket; //usb_dw_config.core->outep_regs[ep.number].doepctl = depctl;
        DESIGNWARE_DAINTMSK |= (1<<ep.number)<<16; //usb_dw_config.core->dregs.daintmsk.ep.out |= (1 << ep.number);
    }
}

void usb_dw_ep0_start_rx(int non_setup, int size)
{
    struct usb_dw_ep_state* endpoint = &usb_dw_ep_state[0].out.state;
    endpoint->busy = true;
    endpoint->size = size;
    endpoint->status = -1;

    // Set up data destination
    if (usb_dw_config.use_dma)
        DESIGNWARE_DOEPDMA(0) = (uint32_t)ep0_buffer.raw;//usb_dw_config.core->outep_regs[0].doepdma = ep0_buffer.raw;
    else
        usb_dw_ep_state[0].out.rxaddr = (uint32_t*)ep0_buffer.raw;
        
    //union usb_dw_dep0xfrsiz deptsiz = { .b = { .supcnt = 3, .pktcnt = !!non_setup, .xfersize = size } };
    DESIGNWARE_DOEPTSIZ(0) = STUPCNT(3)|PKTCNT(!!non_setup)|size; //usb_dw_config.core->outep_regs[0].doeptsiz.d32 = deptsiz.d32;

    // Flush CPU cache if necessary
    if (usb_dw_config.use_dma) discard_dcache_range(ep0_buffer.raw, sizeof(ep0_buffer));

    // Enable the endpoint
    uint32_t doepctl = DESIGNWARE_DOEPCTL(0); //union usb_dw_depctl depctl = usb_dw_config.core->outep_regs[0].doepctl;
    if (non_setup)
        doepctl |= CNAK;
    else
        doepctl &= ~CNAK;
    //depctl.b.epena = 1;
    //depctl.b.cnak = non_setup;
    DESIGNWARE_DOEPCTL(0) = EPENA | doepctl; //usb_dw_config.core->outep_regs[0].doepctl = depctl;
}

void usb_dw_ep0_start_tx(const void* buf, int len)
{
    struct usb_dw_ep_state* endpoint = &usb_dw_ep_state[0].in.state;
    endpoint->busy = true;
    endpoint->size = len;
    endpoint->status = -1;

    if (len)
    {
        // Set up data source
        usb_dw_ep_state[0].in.txaddr = buf;
        if (usb_dw_config.use_dma) DESIGNWARE_DIEPDMA(0) = (uint32_t)buf; //usb_dw_config.core->inep_regs[0].diepdma = buf;
        //union usb_dw_dep0xfrsiz deptsiz = { .b = { .pktcnt = !!len, .xfersize = MIN(len, 64) } };
        //usb_dw_config.core->inep_regs[0].dieptsiz.d32 = deptsiz.d32;
        DESIGNWARE_DIEPTSIZ(0) = PKTCNT(!!len) | MIN(len, 64);
    }
    else
    {
        // Set up the IN pipe for a zero-length packet
        //union usb_dw_dep0xfrsiz deptsiz = { .b = { .pktcnt = 1 } };
        //usb_dw_config.core->inep_regs[0].dieptsiz.d32 = deptsiz.d32;
        DESIGNWARE_DIEPTSIZ(0) = PKTCNT(1);
    }

    // Flush CPU cache if necessary
    if (usb_dw_config.use_dma) commit_dcache_range(buf, len);

    // Enable the endpoint
    //union usb_dw_depctl depctl = usb_dw_config.core->inep_regs[0].diepctl;
    //depctl.b.epena = 1;
    //depctl.b.cnak = 1;
    //usb_dw_config.core->inep_regs[0].diepctl = depctl;
    DESIGNWARE_DIEPCTL(0) |= EPENA | CNAK;

    // Start pushing data into the FIFO (must be done after enabling the endpoint)
    if (len && !usb_dw_config.use_dma)
    {
        if (usb_dw_config.shared_txfifo)
            usb_dw_try_push(0);
        else
            DESIGNWARE_DIEPEMPMSK |= 1; //usb_dw_config.core->dregs.diepempmsk.ep.in |= 1;
    }
}

static void usb_dw_ep0_init(void)
{
    // Make sure both EP0 pipes are active.
    // (The hardware should take care of that, but who knows...)
    //union usb_dw_depctl depctl = { .b = { .usbactep = 1, .nextep = usb_dw_config.core->inep_regs[0].diepctl.b.nextep } };
    uint32_t depctl = USBAEP | ((DESIGNWARE_DIEPCTL(0) >> 10) & 0x0f);
    DESIGNWARE_DOEPCTL(0) = depctl; // usb_dw_config.core->outep_regs[0].doepctl = depctl;
    DESIGNWARE_DIEPCTL(0) = depctl; // usb_dw_config.core->inep_regs[0].diepctl = depctl;
    
    usb_dw_ep0_start_rx(false, 0);
}

void usb_dw_start_rx(struct usb_endpoint_number ep, void* buf, int size)
{
    if (!ep.number)
    {
        usb_dw_ep0_start_rx(true, size);
        return;
    }
    
    struct usb_dw_ep_state* endpoint = &usb_dw_ep_state[ep.number].out.state;
    endpoint->busy = true;
    endpoint->size = size;
    endpoint->status = -1;

    // Find the appropriate set of endpoint registers
    //volatile struct usb_dw_outepregs* regs = &usb_dw_config.core->outep_regs[ep.number];
    // Calculate number of packets (if size == 0 an empty packet will be sent)
    int maxpacket = (DESIGNWARE_DOEPCTL(ep.number) & 0x3ff); //regs->doepctl.b.mps;
    int packets = (size + maxpacket - 1) / maxpacket;
    if (!packets) packets = 1;

    // Set up data destination
    if (usb_dw_config.use_dma)
        DESIGNWARE_DOEPDMA(ep.number) = (uint32_t)buf; // regs->doepdma = buf;
    else
        usb_dw_ep_state[ep.number].out.rxaddr = (uint32_t*)buf;
        
    //union usb_dw_depxfrsiz deptsiz = { .b = { .pktcnt = packets, .xfersize = size } };
    DESIGNWARE_DOEPTSIZ(ep.number) = PKTCNT(packets) | size; //regs->doeptsiz = deptsiz;

    // Flush CPU cache if necessary
    if (usb_dw_config.use_dma) discard_dcache_range(buf, size);

    // Enable the endpoint
    //union usb_dw_depctl depctl = regs->doepctl;
    //depctl.b.epena = 1;
    //depctl.b.cnak = 1;
    DESIGNWARE_DOEPCTL(ep.number) |= EPENA | CNAK; //regs->doepctl = depctl;
}

void usb_dw_start_tx(struct usb_endpoint_number ep, const void* buf, int size)
{
    if (!ep.number)
    {
        usb_dw_ep0_start_tx(buf, size);
        return;
    }
    
    struct usb_dw_ep_state* endpoint = &usb_dw_ep_state[ep.number].in.state;
    endpoint->busy = true;
    endpoint->size = size;
    endpoint->status = -1;

    // Find the appropriate set of endpoint registers
    //volatile struct usb_dw_inepregs* regs = &usb_dw_config.core->inep_regs[ep.number];
    // Calculate number of packets (if size == 0 an empty packet will be sent)
    int maxpacket = DESIGNWARE_DIEPCTL(ep.number) & 0x3ff; //regs->diepctl.b.mps;
    int packets = (size + maxpacket - 1) / maxpacket;
    if (!packets) packets = 1;

    // Set up data source
    if (usb_dw_config.use_dma)
        DESIGNWARE_DIEPDMA(ep.number) = (uint32_t)buf; //regs->diepdma = buf;
    else
        usb_dw_ep_state[ep.number].in.txaddr = (uint32_t*)buf;
        
    //union usb_dw_depxfrsiz deptsiz = { .b = { .pktcnt = packets, .xfersize = size } };
    DESIGNWARE_DIEPTSIZ(ep.number) = PKTCNT(packets) | size; //regs->dieptsiz = deptsiz;

    // Flush CPU cache if necessary
    if (usb_dw_config.use_dma) commit_dcache_range(buf, size);

    // Enable the endpoint
    //union usb_dw_depctl depctl = regs->diepctl;
    //depctl.b.epena = 1;
    //depctl.b.cnak = 1;
    DESIGNWARE_DIEPCTL(ep.number) |= EPENA | CNAK; //regs->diepctl = depctl;

    // Start pushing data into the FIFO (must be done after enabling the endpoint)
    if (size && !usb_dw_config.use_dma)
    {
        if (usb_dw_config.shared_txfifo)
            usb_dw_try_push(ep.number);
        else
            DESIGNWARE_DIEPEMPMSK |= (1<<ep.number); //usb_dw_config.core->dregs.diepempmsk.ep.in |= (1 << ep.number);
    }
}

void usb_dw_set_address(uint8_t address)
{
    DESIGNWARE_DCFG = (DESIGNWARE_DCFG & ~(0x7f0)) | DAD(address); //usb_dw_config.core->dregs.dcfg.b.devaddr = address;
}

static void usb_handle_setup_received(struct usb_endpoint_number ep)
{
    usb_dw_ep_state[ep.number].out.state.status = -1;
    usb_dw_ep_state[ep.number].out.state.busy = false;
    usb_dw_ep_state[ep.number].out.state.done = false;
    semaphore_release(&usb_dw_ep_state[ep.number].out.state.complete);
    usb_dw_ep_state[ep.number].in.state.status = -1;
    usb_dw_ep_state[ep.number].in.state.busy = false;
    usb_dw_ep_state[ep.number].in.state.done = false;
    semaphore_release(&usb_dw_ep_state[ep.number].in.state.complete);
    if (ep.number) return;

    if (ep0_buffer.setup.bmRequestType.recipient == USB_SETUP_BMREQUESTTYPE_RECIPIENT_DEVICE
     && ep0_buffer.setup.bmRequestType.type == USB_SETUP_BMREQUESTTYPE_TYPE_STANDARD
     && ep0_buffer.setup.bRequest.req == USB_SETUP_BREQUEST_SET_ADDRESS)
        usb_dw_set_address(ep0_buffer.setup.wValue);

    usb_core_control_request((struct usb_ctrlrequest*)ep0_buffer.raw);
    usb_dw_ep0_start_rx(false, 0);
}

static void usb_handle_timeout(struct usb_endpoint_number ep)
{
    struct usb_dw_ep_state* endpoint;
    if (ep.direction == USB_ENDPOINT_DIRECTION_OUT)
        endpoint = &usb_dw_ep_state[ep.number].out.state;
    else
        endpoint = &usb_dw_ep_state[ep.number].in.state;
        
    endpoint->busy = false;
    endpoint->status = 1;
    endpoint->done = true;
    semaphore_release(&endpoint->complete);
}

static void usb_handle_xfer_complete(struct usb_endpoint_number ep, int bytesleft)
{
    struct usb_dw_ep_state* endpoint;
    if (ep.direction == USB_ENDPOINT_DIRECTION_OUT)
        endpoint = &usb_dw_ep_state[ep.number].out.state;
    else
        endpoint = &usb_dw_ep_state[ep.number].in.state;

    if (endpoint->busy)
    {
        if (!ep.number && ep.direction == USB_ENDPOINT_DIRECTION_IN && endpoint->size > 64)
        {
            usb_dw_start_tx(ep, usb_dw_ep_state[0].in.txaddr + 64, endpoint->size - 64);
            return;
        }
        endpoint->busy = false;
        endpoint->status = 0;
        int transfered = endpoint->size - bytesleft;
        if (ep.direction == USB_ENDPOINT_DIRECTION_IN)
            endpoint->size = bytesleft;
            
        usb_core_transfer_complete(ep.number, ep.direction == USB_ENDPOINT_DIRECTION_OUT ? USB_DIR_OUT : USB_DIR_IN, 0, transfered);
        endpoint->done = true;
        semaphore_release(&endpoint->complete);
    }
    
    if (ep.direction == USB_ENDPOINT_DIRECTION_OUT)
        usb_dw_ep0_start_rx(false, 0);
}

void usb_drv_cancel_all_transfers()
{
    int flags = disable_irq_save();

    int i;
    for (i = 0; i < 16; i++)
    {
        if (USB_ENDPOINTS & (1 << i))
        {
            usb_dw_ep_state[i].out.state.status = -1;
            usb_dw_ep_state[i].out.state.busy = false;
            usb_dw_ep_state[i].out.state.done = false;
            semaphore_release(&usb_dw_ep_state[i].out.state.complete);
            usb_dw_flush_out_endpoint(i);
        }
        if (USB_ENDPOINTS & (1 << (16 + i)))
        {
            usb_dw_ep_state[i].in.state.status = -1;
            usb_dw_ep_state[i].in.state.busy = false;
            usb_dw_ep_state[i].in.state.done = false;
            semaphore_release(&usb_dw_ep_state[i].in.state.complete);
            usb_dw_flush_in_endpoint(i);
        }
    }

    restore_irq(flags);
}

void usb_dw_irq(void)
{
    //union usb_dw_gintsts gintsts = usb_dw_config.core->gregs.gintsts;
    uint32_t gintsts = DESIGNWARE_GINTSTS;
    uint32_t epints;

    if (gintsts & USBRST) //(gintsts.b.usbreset)
    {
        DESIGNWARE_DCFG &= ~(0x7f<<4); //usb_dw_config.core->dregs.dcfg.b.devaddr = 0;
        usb_drv_cancel_all_transfers();
        usb_core_bus_reset();
    }

    if (gintsts & ENUMDNE)//(gintsts.b.enumdone)
    {
        //usb_core_bus_reset();
        usb_dw_ep0_init();
    }

    if ((gintsts & RXFLVL) && !usb_dw_config.use_dma)//(gintsts.b.rxstsqlvl && !usb_dw_config.use_dma)
    {
        // Device to memory part of the "software DMA" implementation, used to receive data if use_dma == 0.
        // Handle one packet at a time, the IRQ will re-trigger if there's something left.
        //union usb_dw_grxfsts rxsts = usb_dw_config.core->gregs.grxstsp;
        uint32_t rxsts = DESIGNWARE_GRXSTSP;
        int ep = rxsts & 0x0f; //rxsts.b.chnum;
        int words = (((rxsts >> 4) & 0x7ff) + 3) >> 2;//(rxsts.b.bcnt + 3) >> 2;
        while (words--) *usb_dw_ep_state[ep].out.rxaddr++ = DESIGNWARE_DFIFO(0); //usb_dw_config.core->dfifo[0][0];
    }

    if ((gintsts & NPTXFE) && (DESIGNWARE_GINTMSK & NPTXFE))//(gintsts.b.nptxfempty && usb_dw_config.core->gregs.gintmsk.b.nptxfempty)
    {
        // Old style, "shared TX FIFO" memory to device part of the "software DMA" implementation,
        // used to send data if use_dma == 0 and the device doesn't support one non-periodic TX FIFO per endpoint.

        // First disable the IRQ, it will be re-enabled later if there is anything left to be done.
        DESIGNWARE_GINTMSK &= ~NPTXFE; //usb_dw_config.core->gregs.gintmsk.b.nptxfempty = false;

        // Check all endpoints for anything to be transmitted
        int ep;
        for (ep = 0; ep < 16; ep++) usb_dw_try_push(ep);
    }

    if (gintsts & IEPINT)//(gintsts.b.inepintr)
    {
        //union usb_dw_daint daint = usb_dw_config.core->dregs.daint;
        uint32_t daint = DESIGNWARE_DAINT;
        int ep;
        for (ep = 0; ep < 16; ep++)
            if (daint & (1 << ep))
            {
                //union usb_dw_diepintn epints = usb_dw_config.core->inep_regs[ep].diepint;
                epints = DESIGNWARE_DIEPINT(ep);
                 
                if (epints & ITTXFE)//(epints.b.emptyintr) //CHECK
                {
                    // Memory to device part of the "software DMA" implementation, used to transmit data if use_dma == 0.
                    //union usb_dw_depxfrsiz deptsiz = usb_dw_config.core->inep_regs[ep].dieptsiz;
                    uint32_t deptsiz = DESIGNWARE_DIEPTSIZ(ep);
                    if (!(deptsiz & 0x7ffff)) //(!deptsiz.b.xfersize)
                        DESIGNWARE_DIEPEMPMSK &= ~(1<<ep); //usb_dw_config.core->dregs.diepempmsk.ep.in &= ~(1 << ep);
                    else
                    {
                        // Push data into the TX FIFO until we don't have anything left or the FIFO would overflow.
                        int left = deptsiz & 0x7ffff; //(deptsiz.b.xfersize + 3) >> 2;
                        while (left)
                        {
                            int words = DESIGNWARE_DTXFSTS(ep) & 0xffff; //usb_dw_config.core->inep_regs[ep].dtxfsts.b.txfspcavail;
                            if (words > left)
                                words = left;
                                
                            if (!words)
                                break;
                                
                            left -= words;
                            while (words--)
                                DESIGNWARE_DFIFO(ep) = *usb_dw_ep_state[ep].in.txaddr++; //usb_dw_config.core->dfifo[ep][0] = *usb_dw_ep_state[ep].in.txaddr++;
                        }
                    }
                }
                struct usb_endpoint_number epnum = { .direction = USB_ENDPOINT_DIRECTION_IN, .number = ep };
                uint32_t bytesleft = DESIGNWARE_DIEPTSIZ(ep) & 0x7ffff;//usb_dw_config.core->inep_regs[ep].dieptsiz.b.xfersize;
                if (epints & TOC)
                    usb_handle_timeout(epnum); //(epints.b.timeout) usb_handle_timeout(epnum, bytesleft);
                    
                if (epints & XFRC)
                    usb_handle_xfer_complete(epnum, bytesleft); //(epints.b.xfercompl) usb_handle_xfer_complete(epnum, bytesleft);
                    
                DESIGNWARE_DIEPINT(ep) = epints; //usb_dw_config.core->inep_regs[ep].diepint = epints;
            }
    }

    if (gintsts & OEPINT)//(gintsts.b.outepintr)
    {
        //union usb_dw_daint daint = usb_dw_config.core->dregs.daint;
        uint32_t daint = DESIGNWARE_DAINT;
        int ep;
        for (ep = 0; ep < 16; ep++)
            if (daint & ((1<<ep)<<16))//(daint.ep.out & (1 << ep))
            {
                //union usb_dw_doepintn epints = usb_dw_config.core->outep_regs[ep].doepint;
                uint32_t epints = DESIGNWARE_DOEPINT(ep);
                struct usb_endpoint_number epnum = { .direction = USB_ENDPOINT_DIRECTION_OUT, .number = ep };
                if (epints & STUP) //(epints.b.setup)
                {
                    if (usb_dw_config.use_dma) discard_dcache_range((const void*)&(DESIGNWARE_DIEPDMA(ep)), 8);//usb_dw_config.core->inep_regs[ep].diepdma, 8);
                    usb_dw_flush_in_endpoint(ep);
                    usb_handle_setup_received(epnum);
                }
                else if (epints & XFRC)//(epints.b.xfercompl)
                {
                    int bytesleft = DESIGNWARE_DIEPTSIZ(ep) & 0x7ffff; //usb_dw_config.core->inep_regs[ep].dieptsiz.b.xfersize;
                    usb_handle_xfer_complete(epnum, bytesleft);
                }
                DESIGNWARE_DOEPINT(ep) = epints; // usb_dw_config.core->outep_regs[ep].doepint = epints;
            }
    }

    DESIGNWARE_GINTSTS = gintsts; //usb_dw_config.core->gregs.gintsts = gintsts;
}

void usb_dw_init(void)
{
    int i;
    int timeout;

    // Disable IRQ during setup
    usb_dw_target_disable_irq();

    // Enable OTG clocks
    usb_dw_target_enable_clocks();

    // Enable PHY clocks
    //union usb_dw_pcgcctl pcgcctl = { .b = { .stoppclk = 0, .gatehclk = 0 } };
    //usb_dw_config.core->pcgcctl = pcgcctl;
    DESIGNWARE_PCGCCTL = 0; // ???

    // Configure PHY type (must be done before reset)
    //union usb_dw_gccfg gccfg = { .b = { .disablevbussensing = 1, .pwdn = 0 } };
    //usb_dw_config.core->gregs.gccfg = gccfg;
    DESIGNWARE_GCCFG = NOVBUSSENS;
    
    //union usb_dw_gusbcfg gusbcfg = { .b = { .force_dev = 1, .usbtrdtim = USB_DW_TURNAROUND } };
    uint32_t gusbcfg = FDMOD|TRDT(USB_DW_TURNAROUND);
    if (usb_dw_config.phy_16bit)
        gusbcfg |= PHYIF16; //gusbcfg.b.phyif = 1;
    else if (usb_dw_config.phy_ulpi)
        gusbcfg |= ULPI_UTMI_SEL; //gusbcfg.b.ulpi_utmi_sel = 1;
    else
        gusbcfg |= PHSEL;//gusbcfg.b.physel  = 1;
        
    DESIGNWARE_GUSBCFG = gusbcfg;//usb_dw_config.core->gregs.gusbcfg = gusbcfg;

    // Reset the whole USB core
    //union usb_dw_grstctl grstctl = { .b = { .csftrst = 1 } };
    udelay(100);
    //while (!(DESIGNWARE_GRSTCTL & AHBIDL)); //(!usb_dw_config.core->gregs.grstctl.b.ahbidle);
    WHILE_WITH_TMO(!(DESIGNWARE_GRSTCTL & AHBIDL), 1000);

    DESIGNWARE_GRSTCTL = CSRST; // usb_dw_config.core->gregs.grstctl = grstctl;

    //while (DESIGNWARE_GRSTCTL & CSRST); //(usb_dw_config.core->gregs.grstctl.b.csftrst);
    WHILE_WITH_TMO(DESIGNWARE_GRSTCTL & CSRST, 1000);

    //while (!(DESIGNWARE_GRSTCTL & AHBIDL)); //(!usb_dw_config.core->gregs.grstctl.b.ahbidle);
    WHILE_WITH_TMO(!(DESIGNWARE_GRSTCTL & AHBIDL), 1000);

    // Soft disconnect
    //union usb_dw_dctl dctl = { .b = { .sftdiscon = 1 } };
    DESIGNWARE_DCTL = SDIS;//usb_dw_config.core->dregs.dctl = dctl;

    // Configure the core
    //union usb_dw_gahbcfg gahbcfg = { .b = { .dmaenable = usb_dw_config.use_dma, .hburstlen = USB_DW_AHB_BURST_LEN, .glblintrmsk = 1 } };
    uint32_t gahbcfg = HBSTLEN(USB_DW_AHB_BURST_LEN)|GINT;
    if (usb_dw_config.use_dma)
        gahbcfg |= DMAEN;
        
    if (usb_dw_config.disable_double_buffering)
    {
        //gahbcfg.b.nptxfemplvl_txfemplvl = 1;
        //gahbcfg.b.ptxfemplvl = 1;
        gahbcfg |= PTXFELVL|TXFELVL;
    }
    DESIGNWARE_GAHBCFG = gahbcfg; //usb_dw_config.core->gregs.gahbcfg = gahbcfg;
    DESIGNWARE_GUSBCFG = gusbcfg; //usb_dw_config.core->gregs.gusbcfg = gusbcfg;
    //gccfg.b.pwdn = 1;
    DESIGNWARE_GCCFG |= PWRDWN; //usb_dw_config.core->gregs.gccfg = gccfg;
    //union usb_dw_dcfg dcfg = { .b = { .nzstsouthshk = 1 } };
    //usb_dw_config.core->dregs.dcfg = dcfg;
    DESIGNWARE_DCFG = NZLSOHSK;

    // Configure the FIFOs
    if (usb_dw_config.use_dma)
    {
        //union usb_dw_dthrctl dthrctl = { .b = { .arb_park_en = 1, .rx_thr_en = 1, .iso_thr_en = 0, .non_iso_thr_en = 0,
        //                                        .rx_thr_len = USB_DW_AHB_THRESHOLD } };
        //usb_dw_config.core->dregs.dthrctl = dthrctl;
        DESIGNWARE_DTHRCTL = ARPEN|RXTHRLEN(USB_DW_AHB_THRESHOLD)|RXTHREN;
    }
    int addr = usb_dw_config.fifosize;
    for (i = 0; i < 16; i++)
    {
        //usb_dw_config.core->inep_regs[i].diepctl.b.nextep = (i + 1) & 0xf;
        DESIGNWARE_DIEPCTL(i) = (DESIGNWARE_DIEPCTL(i) & ~(0xf<<11)) | NEXTEP((i+1) & 0xf);
        int size = usb_dw_config.txfifosize[i];
        addr -= size;
        if (size)
        {
            //union usb_dw_txfsiz fsiz = { .b = { .startaddr = addr, .depth = size } };
            if (!i)
                DESIGNWARE_GNPTXFSIZ_TX0FSIZ = (size<<16)|addr; //usb_dw_config.core->gregs.dieptxf0_hnptxfsiz = fsiz;
            else
                DESIGNWARE_DIEPTXF(i-1) = (size<<16)|addr; //usb_dw_config.core->gregs.dieptxf[i - 1] = fsiz;
        }
    }
    //union usb_dw_rxfsiz fsiz = { .b = { .depth = addr } };
    DESIGNWARE_GRXFSIZ = addr; //usb_dw_config.core->gregs.grxfsiz = fsiz;
    
    // Set up interrupts
    //union usb_dw_doepintn doepmsk =  { .b = { .xfercompl = 1, .setup = 1 } };
    DESIGNWARE_DOEPMSK = STUP|XFRC;//usb_dw_config.core->dregs.doepmsk = doepmsk;
    //union usb_dw_diepintn diepmsk =  { .b = { .xfercompl = 1, .timeout = 1 } };
    DESIGNWARE_DIEPMSK = TOC|XFRC; //usb_dw_config.core->dregs.diepmsk = diepmsk;
    DESIGNWARE_DIEPEMPMSK = 0; //usb_dw_config.core->dregs.diepempmsk.d32 = 0;
    //union usb_dw_daint daintmsk = { .ep = { .in = 0b0000000000000001, .out = 0b0000000000000001 } };
    DESIGNWARE_DAINTMSK = (1<<16)|1; //usb_dw_config.core->dregs.daintmsk = daintmsk;
    //union usb_dw_gintmsk gintmsk =  { .b = { .usbreset = 1, .enumdone = 1, .outepintr = 1, .inepintr = 1 } };
    uint32_t gintmsk = USBRST|ENUMDNE|IEPINT|OEPINT;
    if (!usb_dw_config.use_dma)
        gintmsk |= RXFLVL; //gintmsk.b.rxstsqlvl = 1;
        
    DESIGNWARE_GINTMSK = gintmsk;//usb_dw_config.core->gregs.gintmsk = gintmsk;
    usb_dw_flush_ints();
    usb_dw_target_clear_irq();
    usb_dw_target_enable_irq();

    // Soft reconnect
    //dctl.b.sftdiscon = 0;
    DESIGNWARE_DCTL &= ~SDIS; //usb_dw_config.core->dregs.dctl = dctl;
}

void usb_dw_exit(void)
{
    // Soft disconnect
    //union usb_dw_dctl dctl = { .b = { .sftdiscon = 1 } };
    //usb_dw_config.core->dregs.dctl = dctl;
    DESIGNWARE_DCTL = SDIS;

    // Disable IRQs
    usb_dw_target_disable_irq();

    // Disable clocks
    usb_dw_target_disable_clocks();
}

int usb_dw_get_max_transfer_size(struct usb_endpoint_number ep)
{
    (void)ep;
    return 512;
}

bool usb_drv_stalled(int endpoint, bool in)
{
    struct usb_endpoint_number ep; // = { .byte = 0 };
    ep.direction = in ? USB_ENDPOINT_DIRECTION_IN : USB_ENDPOINT_DIRECTION_OUT;
    ep.number = endpoint;
    return usb_dw_get_stall(ep);
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
    struct usb_endpoint_number ep; // = { .byte = 0 };
    ep.direction = in ? USB_ENDPOINT_DIRECTION_IN : USB_ENDPOINT_DIRECTION_OUT;
    ep.number = endpoint;
    usb_dw_set_stall(ep, stall);
}

void usb_drv_set_address(int address)
{
    (void)address;
    /* Ignored intentionally, because the controller requires us to set the
       new address before sending the response for some reason. So we'll
       already set it when the control request arrives, before passing that
       into the USB core, which will then call this dummy function. */
}

int usb_drv_send_nonblocking(int endpoint, void *ptr, int length)
{
    struct usb_endpoint_number ep; // = { .byte = 0 };
    ep.direction = USB_ENDPOINT_DIRECTION_IN;
    ep.number = endpoint & 0xf;
    usb_dw_start_tx(ep, ptr, length);
    return 0;
}

int usb_drv_recv(int endpoint, void* ptr, int length)
{
    struct usb_endpoint_number ep; // = { .byte = 0 };
    ep.direction = USB_ENDPOINT_DIRECTION_OUT;
    ep.number = endpoint & 0xf;
    usb_dw_start_rx(ep, ptr, length);
    return 0;
}

int usb_drv_port_speed(void)
{
    return ((DESIGNWARE_DSTS & 0x6) == 0); // usb_dw_config.core->dregs.dsts.b.enumspd == 0;
}

void usb_drv_set_test_mode(int mode)
{
    (void)mode;
    /* Ignore this for now */
}

void usb_attach(void)
{
    //usb_enable(true);
}

void usb_drv_init(void)
{
    usb_dw_init();
}

void usb_drv_exit(void)
{
    usb_dw_exit();
}

void INT_USB_FUNC(void)
{
    usb_dw_irq();
}

int usb_drv_request_endpoint(int type, int dir)
{
    struct usb_endpoint_number ep; // = { .byte = 0 };
    int i;
    if (dir == USB_DIR_OUT)
    {
        for (i = 1; i < 16; i++)
            if (USB_ENDPOINTS & (1 << i))
                if (!usb_dw_ep_state[i].out.state.active)
                {
                    usb_dw_ep_state[i].out.state.active = true;
                    ep.direction = USB_ENDPOINT_DIRECTION_OUT;
                    ep.number = i;
                    usb_dw_configure_ep(ep, type, usb_drv_port_speed() ? 512 : 64);
                    return ep.number; //direction = 0;
                }
    }
    else
    {
        for (i = 1; i < 16; i++)
            if (USB_ENDPOINTS & (1 << (16 + i)))
                if (!usb_dw_ep_state[i].in.state.active)
                {
                    usb_dw_ep_state[i].in.state.active = true;
                    ep.direction = USB_ENDPOINT_DIRECTION_IN;
                    ep.number = i;
                    usb_dw_configure_ep(ep, type, usb_drv_port_speed() ? 512 : 64);
                    return ep.number | 0x80;
                }
    }
    return -1;
}

void usb_drv_release_endpoint(int epnum)
{
    struct usb_endpoint_number ep = { .number = epnum & 0x7f, .direction = !!(epnum & 0x80) };
    if (!ep.number)
        return;
        
    usb_dw_unconfigure_ep(ep);
    
    if (ep.direction == USB_ENDPOINT_DIRECTION_IN)
        usb_dw_ep_state[ep.number].in.state.active = false;
    else
        usb_dw_ep_state[ep.number].out.state.active = false;
}

int usb_drv_send(int endpoint, void *ptr, int len)
{
    struct usb_endpoint_number ep;
    ep.direction = USB_ENDPOINT_DIRECTION_IN;
    ep.number = endpoint & 0xf;
    usb_dw_ep_state[ep.number].in.state.done = false;
    usb_dw_start_tx(ep, ptr, len);
    while (usb_dw_ep_state[ep.number].in.state.busy && !usb_dw_ep_state[ep.number].in.state.done)
        semaphore_wait(&usb_dw_ep_state[ep.number].in.state.complete, TIMEOUT_BLOCK);
        
    return usb_dw_ep_state[ep.number].in.state.status;
}

