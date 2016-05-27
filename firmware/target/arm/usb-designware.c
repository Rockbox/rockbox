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
#include <inttypes.h>
#include <string.h>

#include "config.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "panic.h"
#include "power.h"
#include "usb.h"
#include "usb_drv.h"
#include "usb_ch9.h"
#include "usb_core.h"

#include "usb-designware-regs.h"
#include "usb-designware.h"


/*#define LOGF_ENABLE*/
#include "logf.h"


/* The ARM940T uses a subset of the ARMv4 functions, not
 * supporting clean/invalidate cache entries using MVA.
 */
#if CONFIG_CPU == S5L8700 || CONFIG_CPU == S5L8701
#define DISCARD_DCACHE_RANGE(b,s)   commit_discard_dcache()
#define COMMIT_DCACHE_RANGE(b,s)    commit_dcache()
#else
#define DISCARD_DCACHE_RANGE(b,s)   discard_dcache_range(b,s)
#define COMMIT_DCACHE_RANGE(b,s)    commit_dcache_range(b,s)
#endif

#ifndef USB_DW_TOUTCAL
#define USB_DW_TOUTCAL 0
#endif

#define GET_DTXFNUM(ep) ((DESIGNWARE_DIEPCTL(ep)>>22) & 0xf)

#ifdef USB_DW_SHARED_FIFO
#define IS_PERIODIC(ep) (!!GET_DTXFNUM(ep))
#endif
// #define IS_PERIODIC(ep) (((DESIGNWARE_DIEPCTL(ep)>>18) & 0x3) == USB_ENDPOINT_TYPE_INTERRUPT)

static union usb_ep0_buffer ep0_buffer USB_DEVBSS_ATTR;

struct usb_dw_ep_state //__attribute__((packed,aligned(4))) usb_dw_ep_state
{
    struct semaphore complete;
    uint32_t* req_addr;
    uint32_t req_size;
    uint32_t sizeleft;
    uint32_t size;
    int8_t status;
    uint8_t active;
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
} usb_dw_ep[USB_NUM_ENDPOINTS];

/* For SHARED_TXFIFO mode this is the number of periodic Tx FIFOs (usually 1),
   otherwise it is the number of dedicated Tx FIFOs (not counting NPTX FIFO
   that is always dedicated for IN0). */
static int n_ptxfifos;
static uint16_t ptxfifo_usage;

static uint32_t hw_maxbytes;
static uint32_t hw_maxpackets;

#ifdef USB_DW_SHARED_FIFO
static uint8_t hw_nptxqdepth;
static uint32_t epmis_msk;
#endif


static int usb_dw_get_maxpktsize(struct usb_endpoint_number ep)
{
    return ep.number ? ((ep.direction == USB_ENDPOINT_DIRECTION_IN)
                        ? DESIGNWARE_DIEPCTL(ep.number)
                        : DESIGNWARE_DOEPCTL(ep.number)) & 0x3ff : 64;
}

static int usb_dw_get_maxxfersize(struct usb_endpoint_number ep)
{
    return ep.number ? ALIGN_DOWN_P2(MIN(hw_maxbytes,
            hw_maxpackets * usb_dw_get_maxpktsize(ep)), CACHEALIGN_BITS) : 64;
}

#if defined(USB_DW_ARCH_SLAVE) || defined(USB_DW_SHARED_FIFO)
static int usb_dw_get_maxpktsize_in(int epnumber)
{
    struct usb_endpoint_number ep = {
        .number = epnumber, .direction = USB_ENDPOINT_DIRECTION_IN };
    return usb_dw_get_maxpktsize(ep);
}
#endif

/* Calculate number of packets (if size == 0 an empty packet will be sent) */
static int usb_dw_calc_packets(uint32_t size, uint32_t maxpktsize)
{
    int packets = (size + maxpktsize - 1) / maxpktsize;
    if (!packets) packets = 1;
    return packets;
}

static int usb_dw_get_stall(struct usb_endpoint_number ep)
{
    if (ep.direction == USB_ENDPOINT_DIRECTION_IN)
        return !!(DESIGNWARE_DIEPCTL(ep.number) & STALL);
    return !!(DESIGNWARE_DOEPCTL(ep.number) & STALL);
}

static void usb_dw_set_stall(struct usb_endpoint_number ep, int stall)
{
    if (ep.direction == USB_ENDPOINT_DIRECTION_IN)
    {
        if (stall)
        {
            DESIGNWARE_DIEPCTL(ep.number) |= STALL;
        }
        else
        {
            DESIGNWARE_DIEPCTL(ep.number) &= ~STALL;
            DESIGNWARE_DIEPCTL(ep.number) |= SD0PID;
        }
    }
    else
    {
        if (stall)
        {
            DESIGNWARE_DOEPCTL(ep.number) |= STALL;
        }
        else
        {
            DESIGNWARE_DOEPCTL(ep.number) &= ~STALL;
            DESIGNWARE_DOEPCTL(ep.number) |= SD0PID;
        }
    }
}

static void usb_dw_set_address(uint8_t address)
{
    DESIGNWARE_DCFG = (DESIGNWARE_DCFG & ~(0x7f0)) | DAD(address);
}

static void usb_dw_flush_txfifo(int fnum)
{
    #ifndef USB_DW_ARCH_SLAVE
    /* Wait for any DMA activity to stop, to make sure nobody will touch the FIFO. */
    while (!(DESIGNWARE_GRSTCTL & AHBIDL));
    #endif
    /* Flush FIFO */
    DESIGNWARE_GRSTCTL = GRSTCTL_TXFNUM(fnum) | TXFFLSH;
    while (DESIGNWARE_GRSTCTL & TXFFLSH);

    udelay(3);  /* Wait 3 PHY cycles */
}

#ifdef USB_DW_SHARED_FIFO
static unsigned usb_dw_bytes_in_txfifo(int ep, uint32_t *sentbytes)
{
    uint32_t size = usb_dw_ep[ep].in.state.size;
    if (sentbytes) *sentbytes = size;
    uint32_t dieptsiz = DESIGNWARE_DIEPTSIZ(ep);
    uint32_t packetsleft = (dieptsiz >> 19) & 0x3ff;
    if (!packetsleft) return 0;
    int maxpktsize = usb_dw_get_maxpktsize_in(ep);
    int packets = usb_dw_calc_packets(size, maxpktsize);
    uint32_t bytesleft = dieptsiz & 0x7ffff;
    uint32_t bytespushed = size - bytesleft;
    uint32_t bytespulled = (packets - packetsleft) * maxpktsize;

    if (sentbytes) *sentbytes = bytespulled;
    return bytespushed - bytespulled;
}
#endif /* USB_DW_SHARED_FIFO */

#ifdef USB_DW_ARCH_SLAVE
/* read one packet/token from Rx FIFO */
static void usb_dw_handle_rxfifo(void)
{
    uint32_t rxsts = DESIGNWARE_GRXSTSP;
    int pktsts = (rxsts >> 17) & 0xf;

    switch (pktsts)
    {
        case PKTSTS_OUTRX:
        case PKTSTS_SETUPRX:
        {
            int ep = rxsts & 0xf;
            int words = (((rxsts >> 4) & 0x7ff) + 3) >> 2;
            if (usb_dw_ep[ep].out.state.busy)
            {
                while (words--)
                    *usb_dw_ep[ep].out.rxaddr++ = DESIGNWARE_DFIFO(0);
            }
            else
            {
                /* Discard data */
                while (words--)
                    (void) DESIGNWARE_DFIFO(0);
            }
            break;
        }
        case PKTSTS_OUTDONE:
        case PKTSTS_SETUPDONE:
        case PKTSTS_GLOBALOUTNAK:
        default:
            break;
    }
}

#ifdef USB_DW_SHARED_FIFO
static void usb_dw_try_push(int ep)
{
    if (!usb_dw_ep[ep].in.state.busy)
        return;

    if (epmis_msk & (1<<ep))
        return;

    uint32_t wordsleft = ((DESIGNWARE_DIEPTSIZ(ep) & 0x7ffff) + 3) >> 2;
    if (!wordsleft) return;

    /* Get fifo space for NPTXFIFO or PTXFIFO */
    uint32_t fifospace;
    int dtxfnum = GET_DTXFNUM(ep);

    if (dtxfnum)
    {
        uint32_t fifosize = DESIGNWARE_DIEPTXF(dtxfnum-1) >> 16;
        fifospace = fifosize - ((usb_dw_bytes_in_txfifo(ep, NULL) + 3) >> 2);
    }
    else
    {
        uint32_t gnptxsts = DESIGNWARE_GNPTXSTS;
        fifospace = ((gnptxsts >> 16) & 0xff) ? (gnptxsts & 0xffff) : 0;
    }

    uint32_t maxpktsize = usb_dw_get_maxpktsize_in(ep);
    uint32_t words = MIN((maxpktsize + 3) >> 2, wordsleft);

    if (fifospace >= words)
    {
        wordsleft -= words;
        while (words--)
            DESIGNWARE_DFIFO(ep) = *usb_dw_ep[ep].in.txaddr++;
    }

    if (wordsleft)
        DESIGNWARE_GINTMSK |= (dtxfnum ? PTXFE : NPTXFE);
}

#else /* !USB_DW_SHARED_FIFO */
static void usb_dw_handle_dtxfifo(int ep)
{
    if (usb_dw_ep[ep].in.state.busy)
    {
        uint32_t wordsleft = ((DESIGNWARE_DIEPTSIZ(ep) & 0x7ffff) + 3) >> 2;

        while (wordsleft)
        {
            uint32_t words = wordsleft;
            uint32_t fifospace = DESIGNWARE_DTXFSTS(ep) & 0xffff;

            if (fifospace < words)
            {
                /* we push whole packets to read consistent info on DIEPTSIZ
                   (i.e. when FIFO size is not maxpktsize multiplo). */
                int maxpktwords = usb_dw_get_maxpktsize_in(ep) >> 2;
                words = (fifospace / maxpktwords) * maxpktwords;
            }

            if (!words)
                break;

            wordsleft -= words;
            while (words--)
                DESIGNWARE_DFIFO(ep) = *usb_dw_ep[ep].in.txaddr++;
        }

        if (!wordsleft)
            DESIGNWARE_DIEPEMPMSK &= ~(1 << GET_DTXFNUM(ep));
    }
}
#endif /* !USB_DW_SHARED_FIFO */
#endif /* USB_DW_ARCH_SLAVE */

static void usb_dw_disable_ep(struct usb_endpoint_number ep)
{
    int tmo = 50;
    if (ep.direction == USB_ENDPOINT_DIRECTION_IN)
    {
        DESIGNWARE_DIEPCTL(ep.number) |= EPDIS;
        while (DESIGNWARE_DIEPCTL(ep.number) & EPDIS)
        {
            if (!tmo--) panicf("%s() IN%d failed!", __func__, ep.number);
            else udelay(1);
        }
    }
    else
    {
        if (!ep.number) return;  /* The application cannot disable OUT0 */

        DESIGNWARE_DOEPCTL(ep.number) |= EPDIS;
        while (DESIGNWARE_DOEPCTL(ep.number) & EPDIS)
        {
            if (!tmo--) panicf("%s() OUT%d failed!", __func__, ep.number);
            else udelay(1);
        }
    }
}

static void usb_dw_gonak_effective(bool enable)
{
    if (enable)
    {
        if (!(DESIGNWARE_DCTL & GONSTS))
            DESIGNWARE_DCTL |= SGONAK;

        /* Wait for global IN NAK effective */
        int tmo = 50;
        while (~DESIGNWARE_GINTSTS & GOUTNAKEFF)
        {
            if (!tmo--) panicf("%s(): failed!", __func__);
            #ifdef USB_DW_ARCH_SLAVE
            /* Pull Rx queue until GLOBALOUTNAK token is received. */
            if (DESIGNWARE_GINTSTS & RXFLVL)
                usb_dw_handle_rxfifo();
            #endif
            else udelay(1);
        }
    }
    else
    {
        if (DESIGNWARE_DCTL & GONSTS)
            DESIGNWARE_DCTL |= CGONAK;
    }
}

#ifdef USB_DW_SHARED_FIFO
static void usb_dw_ginak_effective(bool enable)
{
    if (enable)
    {
        if (!(DESIGNWARE_DCTL & GINSTS))
            DESIGNWARE_DCTL |= SGINAK;

        /* Wait for global IN NAK effective */
        int tmo = 50;
        while (~DESIGNWARE_GINTSTS & GINAKEFF)
            if (!tmo--) panicf("%s(): failed!", __func__);
            else udelay(1);

        #ifndef USB_DW_ARCH_SLAVE
        /* Wait for any DMA activity to stop, to make sure nobody will touch the FIFO. */
        while (!(DESIGNWARE_GRSTCTL & AHBIDL));
        #endif
    }
    else
    {
        if (DESIGNWARE_DCTL & GINSTS)
            DESIGNWARE_DCTL |= CGINAK;
    }
}

static void usb_dw_nptx_unqueue(int ep)
{
    uint32_t reenable_msk = 0;

    usb_dw_ginak_effective(true);

    /* Disable EPs */
    for (int i = 0; i < USB_NUM_ENDPOINTS; i++)
    {
        if (USB_ENDPOINTS & (1 << i))
        {
            /* Skip periodic EPs */
            if (IS_PERIODIC(i))
                continue;

            if (~DESIGNWARE_DIEPCTL(i) & EPENA)
                continue;

            /* Disable */
            DESIGNWARE_DIEPCTL(i) |= EPDIS|SNAK;

            /* Adjust */
            uint32_t packetsleft = (DESIGNWARE_DIEPTSIZ(i) >> 19) & 0x3ff;
            if (!packetsleft) continue;

            uint32_t sentbytes;
            uint32_t bytesinfifo = usb_dw_bytes_in_txfifo(i, &sentbytes);
            uint32_t size = usb_dw_ep[i].in.state.size;

#ifdef USB_DW_ARCH_SLAVE
            usb_dw_ep[i].in.txaddr -= (bytesinfifo + 3) >> 2;
#else
            (void) bytesinfifo;
            DESIGNWARE_DIEPDMA(i) = (uint32_t)usb_dw_ep[i].in.txaddr + sentbytes;
#endif
            DESIGNWARE_DIEPTSIZ(i) = PKTCNT(packetsleft) | (size - sentbytes);

            /* do not re-enable the EP we are going to unqueue */
            if (i == ep)
                continue;

            /* Mark EP to be re-enabled later */
            reenable_msk |= (1 << i);
        }
    }

    /* Flush NPTXFIFO */
    usb_dw_flush_txfifo(0);

    /* Re-enable EPs */
    for (int i = 0; i < USB_NUM_ENDPOINTS; i++)
        if (reenable_msk & (1 << i))
            DESIGNWARE_DIEPCTL(i) |= EPENA|CNAK;

#ifdef USB_DW_ARCH_SLAVE
    if (reenable_msk)
        DESIGNWARE_GINTMSK |= NPTXFE;
#endif

    usb_dw_ginak_effective(false);
}
#endif /* USB_DW_SHARED_FIFO */

static void usb_dw_flush_out_endpoint(int ep)
{
    /* for ARCH_SLAVE, setting !busy will discard already received
       packets on this endpoint. */
    usb_dw_ep[ep].out.state.busy = false;
    usb_dw_ep[ep].out.state.status = -1;
    semaphore_release(&usb_dw_ep[ep].out.state.complete);

    DESIGNWARE_DOEPCTL(ep) |= SNAK;

    if (DESIGNWARE_DOEPCTL(ep) & EPENA)
    {
        /* We are waiting for an OUT packet on this endpoint, which might
           arrive any moment. Assert a global output NAK to avoid race
           conditions while shutting down the endpoint. */
        usb_dw_gonak_effective(true);

        struct usb_endpoint_number epnum = {
            .number = ep, .direction = USB_ENDPOINT_DIRECTION_OUT };
        usb_dw_disable_ep(epnum);

        usb_dw_gonak_effective(false);
    }

    /* Clear all channel interrupts to avoid to process
       pending STUP or XFRC on already flushed packets */
    uint32_t doepint = DESIGNWARE_DOEPINT(ep);
    DESIGNWARE_DOEPINT(ep) = doepint;
}

static void usb_dw_flush_in_endpoint(int ep)
{
    usb_dw_ep[ep].in.state.busy = false;
    usb_dw_ep[ep].in.state.status = -1;
    semaphore_release(&usb_dw_ep[ep].in.state.complete);

    DESIGNWARE_DIEPCTL(ep) |= SNAK;

    if (DESIGNWARE_DIEPCTL(ep) & EPENA)
    {
        /* We are shutting down an endpoint that might still have IN packets in the FIFO.
         * Disable the endpoint, wait for things to settle, and flush the relevant FIFO.
         */
        int dtxfnum = GET_DTXFNUM(ep);

#ifdef USB_DW_SHARED_FIFO
        if (!dtxfnum)
        {
            usb_dw_nptx_unqueue(ep);
        }
        else
#endif
        {
            /* Disable the EP we are going to flush */
            struct usb_endpoint_number epnum = {
                .number = ep, .direction = USB_ENDPOINT_DIRECTION_IN };
            usb_dw_disable_ep(epnum);

            /* Flush it all the way down! */
            usb_dw_flush_txfifo(dtxfnum);

#if !defined(USB_DW_SHARED_FIFO) && defined(USB_DW_ARCH_SLAVE)
            DESIGNWARE_DIEPEMPMSK &= ~(1 << dtxfnum);
#endif
        }

    }

#ifdef USB_DW_SHARED_FIFO
    epmis_msk &= ~(1 << ep);
    if (!epmis_msk)
        DESIGNWARE_DIEPMSK &= ~ITTXFE;
#endif

    /* Clear all channel interrupts to avoid to process
       pending XFRC for the flushed EP */
    uint32_t diepint = DESIGNWARE_DIEPINT(ep);
    DESIGNWARE_DIEPINT(ep) = diepint;
}

static void usb_dw_reset_endpoints(void)
{
    int ep;

    for (ep = 0; ep < USB_NUM_ENDPOINTS; ep++)
    {
        /* out */
        usb_dw_ep[ep].out.state.active = !ep;
        usb_dw_ep[ep].out.state.busy = false;
        usb_dw_ep[ep].out.state.status = -1;
        semaphore_release(&usb_dw_ep[ep].out.state.complete);
        /* in */
        usb_dw_ep[ep].in.state.active = !ep;
        usb_dw_ep[ep].in.state.busy = false;
        usb_dw_ep[ep].in.state.status = -1;
        semaphore_release(&usb_dw_ep[ep].in.state.complete);
    }

// #define EPROTO_WORKAROUND
#ifdef EPROTO_WORKAROUND
    #if defined(USB_DW_ARCH_SLAVE)
    while (DESIGNWARE_GINTSTS & RXFLVL)
        usb_dw_handle_rxfifo();
    #else
    /* Wait for any DMA activity to stop, to make sure nobody will touch the FIFO. */
    while (!(DESIGNWARE_GRSTCTL & AHBIDL));
    #endif
    /* Flush Rx FIFO */
    DESIGNWARE_GRSTCTL = RXFFLSH;
    while (DESIGNWARE_GRSTCTL & RXFFLSH);
    //udelay(3);  /* Wait 3 PHY cycles */
#endif

    /* initialize EPs, includes disabling USBAEP on all EPs except
       EP0 (USB HW core keeps EP0 active on all configurations). */
    for (ep = 0; ep < USB_NUM_ENDPOINTS; ep++)
    {
        if (USB_ENDPOINTS & (1 << (ep + 16)))
        {
            usb_dw_flush_out_endpoint(ep);
            DESIGNWARE_DOEPCTL(ep) = 0;
        }
        if (USB_ENDPOINTS & (1 << ep))
        {
            usb_dw_flush_in_endpoint(ep);
#if !defined(USB_DW_ARCH_SLAVE) && defined(USB_DW_SHARED_FIFO)
            int next;
            for (next = ep + 1; next < USB_NUM_ENDPOINTS; next++)
                if (USB_ENDPOINTS & (1 << next))
                    break;
            next %= USB_NUM_ENDPOINTS;
            DESIGNWARE_DIEPCTL(ep) = NEXTEP(next);
#else
            DESIGNWARE_DIEPCTL(ep) = 0;
#endif
        }

    }

    ptxfifo_usage = 0;
}

static void usb_dw_unconfigure_ep(struct usb_endpoint_number ep)
{
    if (ep.direction == USB_ENDPOINT_DIRECTION_IN)
    {
        ptxfifo_usage &= ~(1 << GET_DTXFNUM(ep.number));
        usb_dw_flush_in_endpoint(ep.number);
#if defined(USB_DW_SHARED_FIFO) && !defined(USB_DW_ARCH_SLAVE)
        DESIGNWARE_DIEPCTL(ep.number) &= NEXTEP(0xf);
#else
        DESIGNWARE_DIEPCTL(ep.number) = 0;
#endif
    }
    else
    {
        usb_dw_flush_out_endpoint(ep.number);
        DESIGNWARE_DOEPCTL(ep.number) = 0;
    }
}

static int usb_dw_configure_ep(struct usb_endpoint_number ep,
                         enum usb_endpoint_type type, int maxpktsize)
{
    uint32_t epctl = SD0PID|EPTYP(type)|USBAEP|maxpktsize;

    if (ep.direction == USB_ENDPOINT_DIRECTION_IN)
    {
        /*
         * if the hardware has dedicated fifos, we must give each IN EP
         * a unique tx-fifo even if it is non-periodic.
         */
#ifdef USB_DW_SHARED_FIFO
        if (type == USB_ENDPOINT_TYPE_INTERRUPT)
#endif
        {
            int fnum;
            for (fnum = 1; fnum <= n_ptxfifos; fnum++)
                if (~ptxfifo_usage & (1 << fnum))
                    break;
            if (fnum > n_ptxfifos)
                return 0; /* no available fifos */
            ptxfifo_usage |= (1 << fnum);
            epctl |= DTXFNUM(fnum);
        }
#if defined(USB_DW_SHARED_FIFO) && !defined(USB_DW_ARCH_SLAVE)
        epctl |= DESIGNWARE_DIEPCTL(ep.number) & NEXTEP(0xf);
#endif
        DESIGNWARE_DIEPCTL(ep.number) = epctl;
    }
    else
    {
        DESIGNWARE_DOEPCTL(ep.number) = epctl;
    }

    return 1; /* ok */
}

static void usb_dw_start_rx(struct usb_endpoint_number ep, void* buf, int size)
{
    struct usb_dw_ep_state* endpoint = &usb_dw_ep[ep.number].out.state;

    if ((uint32_t)buf & (CACHEALIGN_SIZE-1))
        logf("%s: OUT%d %lx unaligned!", __func__, ep.number, (uint32_t)buf);

    endpoint->busy = true;
    endpoint->status = -1;
    endpoint->sizeleft = size;
    size = MIN(size, usb_dw_get_maxxfersize(ep));
    endpoint->size = size;

    int packets = usb_dw_calc_packets(size, usb_dw_get_maxpktsize(ep));

    /* Set up data destination */
    usb_dw_ep[ep.number].out.rxaddr = (uint32_t*)buf;
#ifndef USB_DW_ARCH_SLAVE
    DESIGNWARE_DOEPDMA(ep.number) = (uint32_t)buf;
    DISCARD_DCACHE_RANGE(buf, size);
#endif

    DESIGNWARE_DOEPTSIZ(ep.number) = STUPCNT(!ep.number) | PKTCNT(packets) | size;

    /* Enable the endpoint */
    DESIGNWARE_DOEPCTL(ep.number) |= EPENA | (!ep.number ? SNAK:CNAK);
}

static void usb_dw_ep0_wait_setup(void)
{
    struct usb_endpoint_number ep = {
            .number = 0, .direction = USB_ENDPOINT_DIRECTION_OUT };
    usb_dw_start_rx(ep, ep0_buffer.raw, 64);
}

static void usb_dw_start_tx(struct usb_endpoint_number ep, const void* buf, int size)
{
    struct usb_dw_ep_state* endpoint = &usb_dw_ep[ep.number].in.state;

    if ((uint32_t)buf & 3)
        logf("%s(): IN%d %lx unaligned!", __func__, ep.number, (uint32_t)buf);

    endpoint->busy = true;
    endpoint->status = -1;
    endpoint->sizeleft = size;
    size = MIN(size, usb_dw_get_maxxfersize(ep));
    endpoint->size = size;

    int packets = usb_dw_calc_packets(size, usb_dw_get_maxpktsize(ep));

    /* Set up data source */
    usb_dw_ep[ep.number].in.txaddr = (uint32_t*)buf;
#ifndef USB_DW_ARCH_SLAVE
    DESIGNWARE_DIEPDMA(ep.number) = (uint32_t)buf;
    COMMIT_DCACHE_RANGE(buf, size);
#endif

#ifdef USB_DW_SHARED_FIFO
    DESIGNWARE_DIEPTSIZ(ep.number) = MCCNT(IS_PERIODIC(ep.number)) | PKTCNT(packets) | size;
#else
    DESIGNWARE_DIEPTSIZ(ep.number) = PKTCNT(packets) | size;
#endif

    /* Enable the endpoint */
    DESIGNWARE_DIEPCTL(ep.number) |= EPENA|CNAK;

#ifdef USB_DW_ARCH_SLAVE
    /* Enable interrupts to start pushing data into the FIFO */
    if (size)
#ifdef USB_DW_SHARED_FIFO
        DESIGNWARE_GINTMSK |= (IS_PERIODIC(ep.number) ? PTXFE : NPTXFE);
#else
        DESIGNWARE_DIEPEMPMSK |= (1 << GET_DTXFNUM(ep.number));
#endif
#endif
}

static void usb_dw_handle_setup_received(int ep)
{
    static struct usb_ctrlrequest usb_dw_ctrlreq;

    if (ep) {
        logf("%s(): OUT%d ignored", __func__, ep);
        return;
    }

    usb_dw_flush_in_endpoint(0);

    if (ep0_buffer.setup.bmRequestType.recipient == USB_SETUP_BMREQUESTTYPE_RECIPIENT_DEVICE
     && ep0_buffer.setup.bmRequestType.type == USB_SETUP_BMREQUESTTYPE_TYPE_STANDARD
     && ep0_buffer.setup.bRequest.req == USB_SETUP_BREQUEST_SET_ADDRESS)
        usb_dw_set_address(ep0_buffer.setup.wValue);

    memcpy(&usb_dw_ctrlreq, ep0_buffer.raw, sizeof(usb_dw_ctrlreq));
    usb_core_control_request(&usb_dw_ctrlreq);

    usb_dw_ep0_wait_setup();
}

static void usb_dw_abort_in_endpoint(int ep)
{
    if (usb_dw_ep[ep].in.state.busy)
    {
        usb_dw_flush_in_endpoint(ep);
        usb_core_transfer_complete(ep, USB_DIR_IN, -1, 0);
    }
}

static void usb_dw_handle_xfer_complete(struct usb_endpoint_number ep)
{
    struct usb_dw_ep_state* endpoint =
        (ep.direction == USB_ENDPOINT_DIRECTION_IN)
                ? &usb_dw_ep[ep.number].in.state
                : &usb_dw_ep[ep.number].out.state;

    if (endpoint->busy)
    {
        if (ep.direction == USB_ENDPOINT_DIRECTION_IN)
        {
            uint32_t bytesleft = DESIGNWARE_DIEPTSIZ(ep.number) & 0x7ffff;
            endpoint->sizeleft -= (endpoint->size - bytesleft);

            if (!bytesleft && endpoint->sizeleft)
            {
#ifndef USB_DW_ARCH_SLAVE
                usb_dw_ep[ep.number].in.txaddr += (endpoint->size >> 2); /* words */
#endif
                usb_dw_start_tx(ep, usb_dw_ep[ep.number].in.txaddr, endpoint->sizeleft);
                return;
            }

            /* SNAK the disabled EP, otherwise IN tokens for this
               EP could raise unwanted EPMIS interrupts. Useful for
               usbserial when there is no data to send. */
            DESIGNWARE_DIEPCTL(ep.number) |= SNAK;

#ifdef USB_DW_SHARED_FIFO
            /* see usb-s5l8701.c */
            if (usb_dw_config.use_ptxfifo_as_plain_buffer)
            {
                int dtxfnum = GET_DTXFNUM(ep.number);
                if (dtxfnum)
                    usb_dw_flush_txfifo(dtxfnum);
            }
#endif
        }
        else
        {
            uint32_t bytesleft = DESIGNWARE_DOEPTSIZ(ep.number) & 0x7ffff;

            if (!ep.number)
            {
                int recvbytes = 64 - bytesleft;
                endpoint->sizeleft = endpoint->req_size - recvbytes;
                if (endpoint->req_addr)
                    memcpy(endpoint->req_addr, ep0_buffer.raw, endpoint->req_size);
            }
            else
            {
                endpoint->sizeleft -= (endpoint->size - bytesleft);
                if (!bytesleft && endpoint->sizeleft)
                {
#ifndef USB_DW_ARCH_SLAVE
                    usb_dw_ep[ep.number].out.rxaddr += (endpoint->size >> 2); /* words */
#endif
                    usb_dw_start_rx(ep, usb_dw_ep[ep.number].out.rxaddr, endpoint->sizeleft);
                    return;
                }
            }
        }

        endpoint->busy = false;
        endpoint->status = 0;
        semaphore_release(&endpoint->complete);

        int transfered = endpoint->req_size - endpoint->sizeleft;
        usb_core_transfer_complete(ep.number,
                ep.direction == USB_ENDPOINT_DIRECTION_OUT ? USB_DIR_OUT : USB_DIR_IN,
                endpoint->status, transfered);
    }

    if (ep.direction == USB_ENDPOINT_DIRECTION_OUT)
        if (!ep.number)
            usb_dw_ep0_wait_setup();
}

#ifdef USB_DW_SHARED_FIFO
static int usb_dw_get_epmis(void)
{
    unsigned epmis;
    uint32_t gnptxsts = DESIGNWARE_GNPTXSTS;

    if (((gnptxsts>>16) & 0xff) >= hw_nptxqdepth)
        return -1;  /* empty queue */

    /* get the EP on the top of the queue, 0 < idx < number of available IN EPs */
    int idx = (gnptxsts >> 27) & 0xf;
    for (epmis = 0; epmis < USB_NUM_ENDPOINTS; epmis++)
        if ((USB_ENDPOINTS & (1 << epmis)) && !idx--)
            break;

    /* the maximum EP mismatch counter is configured, so we verify all nptx
       queue entries, 4 bits per entry, first entry at DTKQNR1[11:8] */
    uint32_t volatile *dtknqr = &DESIGNWARE_DTKNQR1;
    for (int i = 2; i < hw_nptxqdepth + 2; i++)
        if (((*(dtknqr+(i>>3)) >> ((i & 0x7)*4)) & 0xf) == epmis)
            return -1;

    return epmis;
}

static void usb_dw_handle_token_mismatch(void)
{
    usb_dw_ginak_effective(true);
    int epmis = usb_dw_get_epmis();
    if (epmis >= 0)
    {
        /* the EP is disabled, unqueued, and reconfitured to re-reenable it
           later when a token is received, (or it will be cancelled by
           timeout if it was a blocking request). */
        usb_dw_nptx_unqueue(epmis);

        epmis_msk |= (1 << epmis);
        if (epmis_msk)
            DESIGNWARE_DIEPMSK |= ITTXFE;

        /* Be sure the status is clear */
        DESIGNWARE_DIEPINT(epmis) = ITTXFE;

        /* Must disable NAK to allow to get ITTXFE interrupts for this EP */
        DESIGNWARE_DIEPCTL(epmis) |= CNAK;
    }
    usb_dw_ginak_effective(false);
}
#endif /* USB_DW_SHARED_FIFO */

void usb_dw_irq(void)
{
    int ep;
    uint32_t daint;

#ifdef USB_DW_ARCH_SLAVE
    /* Handle one packet at a time, the IRQ will re-trigger if there's something left. */
    if (DESIGNWARE_GINTSTS & RXFLVL)
    {
        usb_dw_handle_rxfifo();
    }
#endif

#ifdef USB_DW_SHARED_FIFO
    if (DESIGNWARE_GINTSTS & EPMIS)
    {
        usb_dw_handle_token_mismatch();
        DESIGNWARE_GINTSTS = EPMIS;
    }

#ifdef USB_DW_ARCH_SLAVE
    uint32_t gintsts = DESIGNWARE_GINTSTS & DESIGNWARE_GINTMSK;
    if (gintsts & PTXFE)
    {
        /* First disable the IRQ, it will be re-enabled later if there is anything left to be done. */
        DESIGNWARE_GINTMSK &= ~PTXFE;
        /* Check all periodic endpoints for anything to be transmitted */
        for (ep = 1; ep < USB_NUM_ENDPOINTS; ep++)
            if ((USB_ENDPOINTS & (1 << ep)) && IS_PERIODIC(ep))
                usb_dw_try_push(ep);
    }

    if (gintsts & NPTXFE)
    {
        /* First disable the IRQ, it will be re-enabled later if there is anything left to be done. */
        DESIGNWARE_GINTMSK &= ~NPTXFE;
        /* Check all non-periodic endpoints for anything to be transmitted */
        for (ep = 0; ep < USB_NUM_ENDPOINTS; ep++)
            if ((USB_ENDPOINTS & (1 << ep)) && !IS_PERIODIC(ep))
                usb_dw_try_push(ep);
    }
#endif /* USB_DW_ARCH_SLAVE */
#endif /* USB_DW_SHARED_FIFO */

    daint = DESIGNWARE_DAINT;

    /* IN */
    for (ep = 0; ep < USB_NUM_ENDPOINTS; ep++)
    {
        if (daint & (1 << ep))
        {
            uint32_t epints = DESIGNWARE_DIEPINT(ep);

            if (epints & TOC)
            {
                usb_dw_abort_in_endpoint(ep);
            }

#if defined(USB_DW_ARCH_SLAVE) && !defined(USB_DW_SHARED_FIFO)
            if (epints & TXFE)
            {
                usb_dw_handle_dtxfifo(ep);
            }
#endif

#ifdef USB_DW_SHARED_FIFO
            if (epints & ITTXFE & DESIGNWARE_DIEPMSK)
            //if (epints & ITTXFE)
            {
                if (epmis_msk & (1 << ep))
                {
                    DESIGNWARE_DIEPCTL(ep) |= EPENA;

                    epmis_msk &= ~(1 << ep);
                    if (!epmis_msk)
                        DESIGNWARE_DIEPMSK &= ~ITTXFE;
                }
            }
#endif

            /* Clear XFRC here, if this is a 'multi-transfer' request the a new transfer is
               going to be launched,, this ensures it will not miss a single interrupt. */
            DESIGNWARE_DIEPINT(ep) = epints;

            if (epints & XFRC)
            {
                struct usb_endpoint_number epnum = {
                        .direction = USB_ENDPOINT_DIRECTION_IN, .number = ep };
                usb_dw_handle_xfer_complete(epnum);
            }
        }
    }

    /* OUT */
    for (ep = 0; ep < USB_NUM_ENDPOINTS; ep++)
    {
        if (daint & ((1<<ep)<<16))
        {
            uint32_t epints = DESIGNWARE_DOEPINT(ep);

            if (epints & STUP)
            {
                usb_dw_handle_setup_received(ep);
                /* Clear interrupt after the current SETUP packed is handled */
                DESIGNWARE_DOEPINT(ep) = epints;
            }
            else if (epints & XFRC)
            {
                DESIGNWARE_DOEPINT(ep) = epints;
                struct usb_endpoint_number epnum = {
                        .direction = USB_ENDPOINT_DIRECTION_OUT, .number = ep };
                usb_dw_handle_xfer_complete(epnum);
            }
        }
    }

    if (DESIGNWARE_GINTSTS & USBRST)
    {
        DESIGNWARE_GINTSTS = USBRST;
        usb_dw_set_address(0);
        usb_dw_reset_endpoints();
        usb_dw_ep0_wait_setup();
        usb_core_bus_reset();
    }

    if (DESIGNWARE_GINTSTS & ENUMDNE)
    {
        DESIGNWARE_GINTSTS = ENUMDNE;
        /* Nothing to do? */
    }
}

void usb_dw_check_hw(void)
{
    uint32_t ghwcfg2 = DESIGNWARE_GHWCFG2;
    uint32_t ghwcfg3 = DESIGNWARE_GHWCFG3;
    uint32_t ghwcfg4 = DESIGNWARE_GHWCFG4;

    int hw_maxtxfifos;  /* periodic or dedicated */

    /* check for supported HW */
    if (((ghwcfg2 >> 10) & 0xf)+1 < USB_NUM_ENDPOINTS)
        panicf("%s(): USB_NUM_ENDPOINTS too big", __func__);
#ifndef USB_DW_ARCH_SLAVE
    if (((ghwcfg2 >> 3) & 3) != 2)
        panicf("%s(): internal DMA not supported", __func__);
#endif
#ifdef USB_DW_SHARED_FIFO
    if ((ghwcfg4 >> 25) & 1)
        panicf("%s(): shared TxFIFO not supported", __func__);
    hw_maxtxfifos = ghwcfg4 & 0xf;
    hw_nptxqdepth = (1 << (((ghwcfg2 >> 22) & 3) + 1));
#else
    if (!((ghwcfg4 >> 25) & 1))
        panicf("%s(): dedicated TxFIFO not supported", __func__);
    hw_maxtxfifos = (ghwcfg4 >> 26) & 0xf;
#endif
    hw_maxbytes = (1 << (((ghwcfg3 >> 0) & 0xf) + 11)) - 1;
    hw_maxpackets = (1 << (((ghwcfg3 >> 4) & 0x7) + 4)) - 1;
    uint16_t hw_fifomem = ghwcfg3 >> 16;

    /* configure FIFOs, sizes are 32-bit words, we will need at least
       one periodic or dedicated Tx FIFO (really the periodic Tx FIFO
       is not needed if !USB_ENABLE_HID). */
    const struct usb_dw_config *c = &usb_dw_config;
    if (c->rx_fifosz + c->nptx_fifosz + c->ptx_fifosz > hw_fifomem)
        panicf("%s(): insufficient FIFO memory", __func__);
    n_ptxfifos = (hw_fifomem - c->rx_fifosz - c->nptx_fifosz) / c->ptx_fifosz;
    if (n_ptxfifos > hw_maxtxfifos)
        n_ptxfifos = hw_maxtxfifos;
    logf("%s(): FIFO mem=%d rx=%d nptx=%d ptx=%dx%d", __func__,
        hw_fifomem, c->rx_fifosz, c->nptx_fifosz, n_ptxfifos, c->ptx_fifosz);

    // TODO: autodetect USB_ENDPOINTS, this should be done using HWCFG registers
    //       but next code also works fine.
    DESIGNWARE_DAINTMSK = 0xffffffff;
    if (DESIGNWARE_DAINTMSK != USB_ENDPOINTS)
        panicf("%s(): USB_ENDPOINTS mismatch (%lx)", __func__, DESIGNWARE_DAINTMSK);
    DESIGNWARE_DAINTMSK = 0;

    /* HWCFG registers are not checked, if an option is not supported
       then the related bits should be RO. */
    DESIGNWARE_GUSBCFG = c->phytype;
    if (DESIGNWARE_GUSBCFG != c->phytype)
        panicf("%s(): phytype=%x not supported", __func__, c->phytype);
}

void usb_dw_init(void)
{
    static bool initialized = false;
    const struct usb_dw_config *c = &usb_dw_config;

    if (!initialized)
    {
        for (int i = 0; i < USB_NUM_ENDPOINTS; i++)
        {
            semaphore_init(&usb_dw_ep[i].out.state.complete, 1, 0);
            semaphore_init(&usb_dw_ep[i].in.state.complete, 1, 0);
        }
        initialized = true;
    }

    /* Disable IRQ during setup */
    usb_dw_target_disable_irq();

    /* Enable OTG clocks */
    usb_dw_target_enable_clocks();

    /* Enable PHY clocks */
    DESIGNWARE_PCGCCTL = 0;

    usb_dw_check_hw();

    /* Configure PHY type (must be done before reset) */
    #ifndef USB_DW_TURNAROUND
    /*
     * Turnarount time (in PHY clocks) = 4*AHB clocks + 1*PHY clock,
     * worst cases are:
     *  16-bit UTMI+: PHY=30MHz, AHB=30Mhz -> 5
     *  8-bit UTMI+:  PHY=60MHz, AHB=30MHz -> 9
     */
    int USB_DW_TURNAROUND = (c->phytype == DWC_PHYTYPE_UTMI_16) ? 5 : 9;
    #endif
    uint32_t gusbcfg = c->phytype|TRDT(USB_DW_TURNAROUND)|USB_DW_TOUTCAL;

    DESIGNWARE_GUSBCFG = gusbcfg;

    /* Reset the whole USB core */
    udelay(100);
    while (!(DESIGNWARE_GRSTCTL & AHBIDL));
    DESIGNWARE_GRSTCTL = CSRST;
    while (DESIGNWARE_GRSTCTL & CSRST);
    while (!(DESIGNWARE_GRSTCTL & AHBIDL));

    /* configure FIFOs */
    DESIGNWARE_GRXFSIZ = c->rx_fifosz;
    DESIGNWARE_GNPTXFSIZ_TX0FSIZ = (c->nptx_fifosz << 16) | c->rx_fifosz;
    for (int i = 0; i < n_ptxfifos; i++)
        DESIGNWARE_DIEPTXF(i) = (c->ptx_fifosz << 16) |
                        (c->nptx_fifosz + c->rx_fifosz + c->ptx_fifosz*i);
	/*
	 * according to p428 of the design guide, we need to ensure that
	 * all fifos are flushed before continuing
	 */
    DESIGNWARE_GRSTCTL = GRSTCTL_TXFNUM(0x10) | TXFFLSH | RXFFLSH;
    while (DESIGNWARE_GRSTCTL & (TXFFLSH|RXFFLSH));
    //udelay(3); /* Wait 3 PHY cycles */

    ptxfifo_usage = 0;

    /* Configure the core */
    DESIGNWARE_GUSBCFG = gusbcfg;

    uint32_t gahbcfg = GINT;
#ifdef USB_DW_ARCH_SLAVE
#ifdef USB_DW_SHARED_FIFO
    if (c->use_ptxfifo_as_plain_buffer)
        gahbcfg |= PTXFELVL;
#endif
    if (c->disable_double_buffering)
        gahbcfg |= TXFELVL;
#else
    gahbcfg |= HBSTLEN(c->ahb_burst_len)|DMAEN;
#endif
    DESIGNWARE_GAHBCFG = gahbcfg;

    DESIGNWARE_DCFG = NZLSOHSK;
#ifdef USB_DW_SHARED_FIFO
    /* set EP mismatch counter to the maximum */
    //DESIGNWARE_DCFG |= EPMISCNT(0x1f);
    DESIGNWARE_DCFG |= EPMISCNT(8);
#endif

#if !defined(USB_DW_ARCH_SLAVE) && !defined(USB_DW_SHARED_FIFO)
    if (c->ahb_threshold)
        DESIGNWARE_DTHRCTL = ARPEN|RXTHRLEN(c->ahb_threshold)|RXTHREN;
#endif

    /* Set up interrupts */
    DESIGNWARE_DOEPMSK = STUP|XFRC;
    DESIGNWARE_DIEPMSK = TOC|XFRC;

    DESIGNWARE_DAINTMSK = 0xffffffff;

    uint32_t gintmsk = USBRST|ENUMDNE|IEPINT|OEPINT;
#ifdef USB_DW_ARCH_SLAVE
    gintmsk |= RXFLVL;
#endif
#ifdef USB_DW_SHARED_FIFO
    gintmsk |= EPMIS;
#endif
    DESIGNWARE_GINTMSK = gintmsk;

    usb_dw_reset_endpoints();

    /* Soft disconnect */
    DESIGNWARE_DCTL = SDIS;

    usb_dw_target_clear_irq();
    usb_dw_target_enable_irq();

    /* Soft reconnect */
    udelay(3000);
    DESIGNWARE_DCTL &= ~SDIS;
}

void usb_dw_exit(void)
{
    /* Soft disconnect */
    DESIGNWARE_DCTL = SDIS;
    udelay(10);

    DESIGNWARE_PCGCCTL = 1; /* stop Phy clock */

    /* Disable IRQs */
    usb_dw_target_disable_irq();

    /* Disable clocks */
    usb_dw_target_disable_clocks();
}


/*
 * RB API
 */

/* cancel transfers on configured EPs */
// TODO: cancel IN0 ???, xfer_complete() ???
void usb_drv_cancel_all_transfers()
{
    usb_dw_target_disable_irq();
    for (int i = 1; i < USB_NUM_ENDPOINTS; i++)
    {
        if (USB_ENDPOINTS & (1 << i) &&
                        usb_dw_ep[i].in.state.active)
        {
            usb_dw_flush_in_endpoint(i);
            DESIGNWARE_DIEPCTL(i) |= SD0PID;
        }
        if (USB_ENDPOINTS & (1 << (i + 16)) &&
                        usb_dw_ep[i].out.state.active)
        {
            usb_dw_flush_out_endpoint(i);
            DESIGNWARE_DOEPCTL(i) |= SD0PID;
        }
    }
    usb_dw_target_enable_irq();
}

bool usb_drv_stalled(int endpoint, bool in)
{
    struct usb_endpoint_number ep = {
        .number = endpoint & 0xf,
        .direction = in ? USB_ENDPOINT_DIRECTION_IN
                        : USB_ENDPOINT_DIRECTION_OUT };

    return usb_dw_get_stall(ep);
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
    struct usb_endpoint_number ep = {
       .number = endpoint & 0xf,
       .direction = in ? USB_ENDPOINT_DIRECTION_IN
                       : USB_ENDPOINT_DIRECTION_OUT };
    usb_dw_target_disable_irq();
    usb_dw_set_stall(ep, stall);
    usb_dw_target_enable_irq();
}

void usb_drv_set_address(int address)
{
#if 1
    /* Ignored intentionally, because the controller requires us to set the
       new address before sending the response for some reason. So we'll
       already set it when the control request arrives, before passing that
       into the USB core, which will then call this dummy function. */
    (void)address;
#else
    usb_dw_target_disable_irq();
    usb_dw_set_address(address);
    usb_dw_target_enable_irq();
#endif
}

int usb_drv_port_speed(void)
{
    return ((DESIGNWARE_DSTS & 0x6) == 0);
}

void usb_drv_set_test_mode(int mode)
{
    (void)mode;
    /* Ignore this for now */
}

void usb_attach(void)
{
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
    int req_ep = -1;
    struct usb_endpoint_number ep;

    usb_dw_target_disable_irq();
    if (dir == USB_DIR_IN)
    {
        ep.direction = USB_ENDPOINT_DIRECTION_IN;
        for (int i = 1; i < USB_NUM_ENDPOINTS; i++)
            if (USB_ENDPOINTS & (1 << i) &&
                            !usb_dw_ep[i].in.state.active)
            {
                ep.number = i;
                if (usb_dw_configure_ep(ep, type,
                                usb_drv_port_speed() ? 512 : 64))
                {
                    usb_dw_ep[i].in.state.active = true;
                    req_ep = ep.number | USB_DIR_IN;
                }
                break;
            }
    }
    else
    {
        ep.direction = USB_ENDPOINT_DIRECTION_OUT;
        for (int i = 1; i < USB_NUM_ENDPOINTS; i++)
            if (USB_ENDPOINTS & (1 << (16 + i)) &&
                            !usb_dw_ep[i].out.state.active)
            {
                ep.number = i;
                if (usb_dw_configure_ep(ep, type,
                                usb_drv_port_speed() ? 512 : 64))
                {
                    usb_dw_ep[i].out.state.active = true;
                    req_ep = ep.number | USB_DIR_OUT;
                }
                break;
            }
    }
    usb_dw_target_enable_irq();
    return req_ep;
}

void usb_drv_release_endpoint(int endpoint)
{
    struct usb_endpoint_number ep = {
        .number = endpoint & 0xf,
        .direction = ((endpoint & 0x80) == USB_DIR_IN)
                ? USB_ENDPOINT_DIRECTION_IN
                : USB_ENDPOINT_DIRECTION_OUT };

    if (!ep.number)
        return;

    struct usb_dw_ep_state* ep_state =
        (ep.direction == USB_ENDPOINT_DIRECTION_IN)
                ? &usb_dw_ep[ep.number].in.state
                : &usb_dw_ep[ep.number].out.state;

    usb_dw_target_disable_irq();
    if (ep_state->active)
    {
        usb_dw_unconfigure_ep(ep);
        ep_state->active = false;
    }
    usb_dw_target_enable_irq();
}

int usb_drv_recv(int epnum, void* ptr, int length)
{
    struct usb_endpoint_number ep = {
        .number = epnum & 0xf, .direction = USB_ENDPOINT_DIRECTION_OUT };
    struct usb_dw_ep_state* endpoint = &usb_dw_ep[ep.number].out.state;

    usb_dw_target_disable_irq();
    if (endpoint->active)
    {
        endpoint->req_addr = ptr;
        endpoint->req_size = length;
        /* OUT0 is always launched waiting for STUP packed,
           it is CNAKed to receive app data */
        if (ep.number == 0)
            DESIGNWARE_DOEPCTL(0) |= CNAK;
        else
            usb_dw_start_rx(ep, ptr, length);
    }
    usb_dw_target_enable_irq();
    return 0;
}

int usb_drv_send_nonblocking(int epnum, void *ptr, int length)
{
    struct usb_endpoint_number ep = {
        .number = epnum & 0xf, .direction = USB_ENDPOINT_DIRECTION_IN };
    struct usb_dw_ep_state *endpoint = &usb_dw_ep[ep.number].in.state;

    usb_dw_target_disable_irq();
    if (endpoint->active)
    {
        endpoint->req_addr = ptr;
        endpoint->req_size = length;
        usb_dw_start_tx(ep, ptr, length);
    }
    usb_dw_target_enable_irq();
    return 0;
}

int usb_drv_send(int epnum, void *ptr, int length)
{
    struct usb_endpoint_number ep = {
        .number = epnum & 0xf, .direction = USB_ENDPOINT_DIRECTION_IN };
    struct usb_dw_ep_state *endpoint = &usb_dw_ep[ep.number].in.state;

    semaphore_wait(&endpoint->complete, 0);

    usb_drv_send_nonblocking(epnum, ptr, length);

    if (semaphore_wait(&endpoint->complete, HZ) == OBJ_WAIT_TIMEDOUT)
    {
        usb_dw_target_disable_irq();
        usb_dw_abort_in_endpoint(ep.number);
        usb_dw_target_enable_irq();
    }

    return endpoint->status;
}
