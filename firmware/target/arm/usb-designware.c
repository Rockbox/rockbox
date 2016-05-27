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
//#define IS_PERIODIC(ep) (((DESIGNWARE_DIEPCTL(ep)>>18) & 0x3) == EPTYP_INTERRUPT)

#define USB_DW_NUM_DIRS 2
#define USB_DW_DIR_OFF(dir) (((dir) == USB_DW_EPDIR_IN) ? 0 : 16)


static union usb_ep0_buffer ep0_buffer USB_DEVBSS_ATTR;
static uint32_t usb_endpoints;  /* available EPs mask */

/* For SHARED_TXFIFO mode this is the number of periodic Tx FIFOs
   (usually 1), otherwise it is the number of dedicated Tx FIFOs
   (not counting NPTX FIFO that is always dedicated for IN0). */
static int n_ptxfifos;
static uint16_t ptxfifo_usage;

static uint32_t hw_maxbytes;
static uint32_t hw_maxpackets;

#ifdef USB_DW_SHARED_FIFO
static uint8_t hw_nptxqdepth;
static uint32_t epmis_msk;
#endif

static struct usb_dw_ep usb_dw_ep_list[USB_NUM_ENDPOINTS][USB_DW_NUM_DIRS];


static struct usb_dw_ep *usb_dw_get_ep(int epnum, enum usb_dw_epdir epdir)
{
    return &usb_dw_ep_list[epnum][epdir];
}

static int usb_dw_maxpktsize(int epnum, enum usb_dw_epdir epdir)
{
    return epnum ? ((epdir == USB_DW_EPDIR_IN)
                        ? DESIGNWARE_DIEPCTL(epnum)
                        : DESIGNWARE_DOEPCTL(epnum)) & 0x3ff : 64;
}

static int usb_dw_maxxfersize(int epnum, enum usb_dw_epdir epdir)
{
    return epnum ? ALIGN_DOWN_P2(MIN(hw_maxbytes,
        hw_maxpackets*usb_dw_maxpktsize(epnum, epdir)), CACHEALIGN_BITS) : 64;
}

/* Calculate number of packets (if size == 0 an empty packet will be sent) */
static int usb_dw_calc_packets(uint32_t size, uint32_t maxpktsize)
{
    int packets = (size + maxpktsize - 1) / maxpktsize;
    if (!packets) packets = 1;
    return packets;
}

static int usb_dw_get_stall(int epnum, enum usb_dw_epdir epdir)
{
    if (epdir == USB_DW_EPDIR_IN)
        return !!(DESIGNWARE_DIEPCTL(epnum) & STALL);
    return !!(DESIGNWARE_DOEPCTL(epnum) & STALL);
}

static void usb_dw_set_stall(int epnum, enum usb_dw_epdir epdir, int stall)
{
    if (epdir == USB_DW_EPDIR_IN)
    {
        if (stall)
        {
            DESIGNWARE_DIEPCTL(epnum) |= STALL;
        }
        else
        {
            DESIGNWARE_DIEPCTL(epnum) &= ~STALL;
            DESIGNWARE_DIEPCTL(epnum) |= SD0PID;
        }
    }
    else
    {
        if (stall)
        {
            DESIGNWARE_DOEPCTL(epnum) |= STALL;
        }
        else
        {
            DESIGNWARE_DOEPCTL(epnum) &= ~STALL;
            DESIGNWARE_DOEPCTL(epnum) |= SD0PID;
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
    /* Wait for any DMA activity to stop. */
    while (!(DESIGNWARE_GRSTCTL & AHBIDL));
    #endif
    /* Flush FIFO */
    DESIGNWARE_GRSTCTL = GRSTCTL_TXFNUM(fnum) | TXFFLSH;
    while (DESIGNWARE_GRSTCTL & TXFFLSH);
    udelay(3);  /* Wait 3 PHY cycles */
}

#ifdef USB_DW_SHARED_FIFO
static unsigned usb_dw_bytes_in_txfifo(int epnum, uint32_t *sentbytes)
{
    uint32_t size = usb_dw_get_ep(epnum, USB_DW_EPDIR_IN)->size;
    if (sentbytes) *sentbytes = size;
    uint32_t dieptsiz = DESIGNWARE_DIEPTSIZ(epnum);
    uint32_t packetsleft = (dieptsiz >> 19) & 0x3ff;
    if (!packetsleft) return 0;
    int maxpktsize = usb_dw_maxpktsize(epnum, USB_DW_EPDIR_IN);
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
            struct usb_dw_ep* dw_ep = usb_dw_get_ep(ep, USB_DW_EPDIR_OUT);
            if (dw_ep->busy)
            {
                while (words--)
                    *dw_ep->addr++ = DESIGNWARE_DFIFO(0);
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
static void usb_dw_try_push(int epnum)
{
    struct usb_dw_ep* dw_ep = usb_dw_get_ep(epnum, USB_DW_EPDIR_IN);

    if (!dw_ep->busy)
        return;

    if (epmis_msk & (1 << epnum))
        return;

    uint32_t wordsleft = ((DESIGNWARE_DIEPTSIZ(epnum) & 0x7ffff) + 3) >> 2;
    if (!wordsleft) return;

    /* Get fifo space for NPTXFIFO or PTXFIFO */
    uint32_t fifospace;
    int dtxfnum = GET_DTXFNUM(epnum);
    if (dtxfnum)
    {
        uint32_t fifosize = DESIGNWARE_DIEPTXF(dtxfnum - 1) >> 16;
        fifospace = fifosize - ((usb_dw_bytes_in_txfifo(epnum, NULL) + 3) >> 2);
    }
    else
    {
        uint32_t gnptxsts = DESIGNWARE_GNPTXSTS;
        fifospace = ((gnptxsts >> 16) & 0xff) ? (gnptxsts & 0xffff) : 0;
    }

    uint32_t maxpktsize = usb_dw_maxpktsize(epnum, USB_DW_EPDIR_IN);
    uint32_t words = MIN((maxpktsize + 3) >> 2, wordsleft);

    if (fifospace >= words)
    {
        wordsleft -= words;
        while (words--)
            DESIGNWARE_DFIFO(epnum) = *dw_ep->addr++;
    }

    if (wordsleft)
        DESIGNWARE_GINTMSK |= (dtxfnum ? PTXFE : NPTXFE);
}

#else /* !USB_DW_SHARED_FIFO */
static void usb_dw_handle_dtxfifo(int epnum)
{
    struct usb_dw_ep* dw_ep = usb_dw_get_ep(epnum, USB_DW_EPDIR_IN);

    if (!dw_ep->busy)
        return;

    uint32_t wordsleft = ((DESIGNWARE_DIEPTSIZ(epnum) & 0x7ffff) + 3) >> 2;

    while (wordsleft)
    {
        uint32_t words = wordsleft;
        uint32_t fifospace = DESIGNWARE_DTXFSTS(epnum) & 0xffff;

        if (fifospace < words)
        {
            /* we push whole packets to read consistent info on DIEPTSIZ
               (i.e. when FIFO size is not maxpktsize multiplo). */
            int maxpktwords = usb_dw_maxpktsize(epnum, USB_DW_EPDIR_IN) >> 2;
            words = (fifospace / maxpktwords) * maxpktwords;
        }

        if (!words)
            break;

        wordsleft -= words;
        while (words--)
            DESIGNWARE_DFIFO(epnum) = *dw_ep->addr++;
    }

    if (!wordsleft)
        DESIGNWARE_DIEPEMPMSK &= ~(1 << GET_DTXFNUM(epnum));
}
#endif /* !USB_DW_SHARED_FIFO */
#endif /* USB_DW_ARCH_SLAVE */

static void usb_dw_disable_ep(int epnum, enum usb_dw_epdir epdir)
{
    int tmo = 50;
    if (epdir == USB_DW_EPDIR_IN)
    {
        DESIGNWARE_DIEPCTL(epnum) |= EPDIS;
        while (DESIGNWARE_DIEPCTL(epnum) & EPDIS)
        {
            if (!tmo--) panicf("%s: IN%d failed!", __func__, epnum);
            else udelay(1);
        }
    }
    else
    {
        if (!epnum) return;  /* The application cannot disable OUT0 */

        DESIGNWARE_DOEPCTL(epnum) |= EPDIS;
        while (DESIGNWARE_DOEPCTL(epnum) & EPDIS)
        {
            if (!tmo--) panicf("%s: OUT%d failed!", __func__, epnum);
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
            if (!tmo--) panicf("%s: failed!", __func__);
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
        {
            if (!tmo--) panicf("%s: failed!", __func__);
            else udelay(1);
        }
        #ifndef USB_DW_ARCH_SLAVE
        /* Wait for any DMA activity to stop. */
        while (!(DESIGNWARE_GRSTCTL & AHBIDL));
        #endif
    }
    else
    {
        if (DESIGNWARE_DCTL & GINSTS)
            DESIGNWARE_DCTL |= CGINAK;
    }
}

static void usb_dw_nptx_unqueue(int epnum)
{
    uint32_t reenable_msk = 0;

    usb_dw_ginak_effective(true);

    /* Disable EPs */
    for (int ep = 0; ep < USB_NUM_ENDPOINTS; ep++)
    {
        if (usb_endpoints & (1 << ep))
        {
            /* Skip periodic EPs */
            if (IS_PERIODIC(ep))
                continue;

            if (~DESIGNWARE_DIEPCTL(ep) & EPENA)
                continue;

            /* Disable */
            DESIGNWARE_DIEPCTL(ep) |= EPDIS|SNAK;

            /* Adjust */
            uint32_t packetsleft = (DESIGNWARE_DIEPTSIZ(ep) >> 19) & 0x3ff;
            if (!packetsleft) continue;

            struct usb_dw_ep* dw_ep = usb_dw_get_ep(ep, USB_DW_EPDIR_IN);
            uint32_t sentbytes;
            uint32_t bytesinfifo = usb_dw_bytes_in_txfifo(ep, &sentbytes);

#ifdef USB_DW_ARCH_SLAVE
            dw_ep->addr -= (bytesinfifo + 3) >> 2;
#else
            (void) bytesinfifo;
            DESIGNWARE_DIEPDMA(ep) = (uint32_t)(dw_ep->addr) + sentbytes;
#endif
            DESIGNWARE_DIEPTSIZ(ep) =
                        PKTCNT(packetsleft) | (dw_ep->size - sentbytes);

            /* do not re-enable the EP we are going to unqueue */
            if (ep == epnum)
                continue;

            /* Mark EP to be re-enabled later */
            reenable_msk |= (1 << ep);
        }
    }

    /* Flush NPTXFIFO */
    usb_dw_flush_txfifo(0);

    /* Re-enable EPs */
    for (int ep = 0; ep < USB_NUM_ENDPOINTS; ep++)
        if (reenable_msk & (1 << ep))
            DESIGNWARE_DIEPCTL(ep) |= EPENA|CNAK;

#ifdef USB_DW_ARCH_SLAVE
    if (reenable_msk)
        DESIGNWARE_GINTMSK |= NPTXFE;
#endif

    usb_dw_ginak_effective(false);
}
#endif /* USB_DW_SHARED_FIFO */

static void usb_dw_flush_out_endpoint(int epnum)
{
    struct usb_dw_ep* dw_ep = usb_dw_get_ep(epnum, USB_DW_EPDIR_OUT);
    /* for ARCH_SLAVE, setting !busy will discard already received
       packets on this endpoint. */
    dw_ep->busy = false;
    dw_ep->status = -1;
    semaphore_release(&dw_ep->complete);

    DESIGNWARE_DOEPCTL(epnum) |= SNAK;

    if (DESIGNWARE_DOEPCTL(epnum) & EPENA)
    {
        /* We are waiting for an OUT packet on this endpoint, which might
           arrive any moment. Assert a global output NAK to avoid race
           conditions while shutting down the endpoint. */
        usb_dw_gonak_effective(true);
        usb_dw_disable_ep(epnum, USB_DW_EPDIR_OUT);
        usb_dw_gonak_effective(false);
    }

    /* Clear all channel interrupts to avoid to process
       pending STUP or XFRC on already flushed packets */
    DESIGNWARE_DOEPINT(epnum) = DESIGNWARE_DOEPINT(epnum);
}

static void usb_dw_flush_in_endpoint(int epnum)
{
    struct usb_dw_ep* dw_ep = usb_dw_get_ep(epnum, USB_DW_EPDIR_IN);
    dw_ep->busy = false;
    dw_ep->status = -1;
    semaphore_release(&dw_ep->complete);

    DESIGNWARE_DIEPCTL(epnum) |= SNAK;

    if (DESIGNWARE_DIEPCTL(epnum) & EPENA)
    {
        /* We are shutting down an endpoint that might still have IN packets
         * in the FIFO. Disable the endpoint, wait for things to settle, and
         * flush the relevant FIFO.
         */
        int dtxfnum = GET_DTXFNUM(epnum);

#ifdef USB_DW_SHARED_FIFO
        if (!dtxfnum)
        {
            usb_dw_nptx_unqueue(epnum);
        }
        else
#endif
        {
            /* Disable the EP we are going to flush */
            usb_dw_disable_ep(epnum, USB_DW_EPDIR_IN);

            /* Flush it all the way down! */
            usb_dw_flush_txfifo(dtxfnum);

#if !defined(USB_DW_SHARED_FIFO) && defined(USB_DW_ARCH_SLAVE)
            DESIGNWARE_DIEPEMPMSK &= ~(1 << dtxfnum);
#endif
        }

    }

#ifdef USB_DW_SHARED_FIFO
    epmis_msk &= ~(1 << epnum);
    if (!epmis_msk)
        DESIGNWARE_DIEPMSK &= ~ITTXFE;
#endif

    /* Clear all channel interrupts to avoid to process
       pending XFRC for the flushed EP */
    DESIGNWARE_DIEPINT(epnum) = DESIGNWARE_DIEPINT(epnum);
}

static void usb_dw_reset_endpoints(void)
{
    for (int ep = 0; ep < USB_NUM_ENDPOINTS; ep++)
    {
        for (int dir = 0; dir < USB_DW_NUM_DIRS; dir++)
        {
            struct usb_dw_ep* dw_ep = usb_dw_get_ep(ep, dir);
            dw_ep->active = !ep;
            dw_ep->busy = false;
            dw_ep->status = -1;
            semaphore_release(&dw_ep->complete);
        }
    }

    /* Workaround for spurious -EPROTO on Nano2G.
       Actually solved, see usb-s5l8701.c */
    /* #define EPROTO_WORKAROUND */
#ifdef EPROTO_WORKAROUND
    #if defined(USB_DW_ARCH_SLAVE)
    while (DESIGNWARE_GINTSTS & RXFLVL)
        usb_dw_handle_rxfifo();
    #else
    /* Wait for any DMA activity to stop. */
    while (!(DESIGNWARE_GRSTCTL & AHBIDL));
    #endif
    /* Flush Rx FIFO */
    DESIGNWARE_GRSTCTL = RXFFLSH;
    while (DESIGNWARE_GRSTCTL & RXFFLSH);
    udelay(3);  /* Wait 3 PHY cycles */
#endif

    /* initialize EPs, includes disabling USBAEP on all EPs except
       EP0 (USB HW core keeps EP0 active on all configurations). */
    for (int ep = 0; ep < USB_NUM_ENDPOINTS; ep++)
    {
        if (usb_endpoints & (1 << (ep + 16)))
        {
            usb_dw_flush_out_endpoint(ep);
            DESIGNWARE_DOEPCTL(ep) = 0;
        }
        if (usb_endpoints & (1 << ep))
        {
            usb_dw_flush_in_endpoint(ep);
#if !defined(USB_DW_ARCH_SLAVE) && defined(USB_DW_SHARED_FIFO)
            int next;
            for (next = ep + 1; next < USB_NUM_ENDPOINTS; next++)
                if (usb_endpoints & (1 << next))
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

static void usb_dw_unconfigure_ep(int epnum, enum usb_dw_epdir epdir)
{
    if (epdir == USB_DW_EPDIR_IN)
    {
        ptxfifo_usage &= ~(1 << GET_DTXFNUM(epnum));
        usb_dw_flush_in_endpoint(epnum);
#if defined(USB_DW_SHARED_FIFO) && !defined(USB_DW_ARCH_SLAVE)
        DESIGNWARE_DIEPCTL(epnum) &= NEXTEP(0xf);
#else
        DESIGNWARE_DIEPCTL(epnum) = 0;
#endif
    }
    else
    {
        usb_dw_flush_out_endpoint(epnum);
        DESIGNWARE_DOEPCTL(epnum) = 0;
    }
}

static int usb_dw_configure_ep(int epnum,
                enum usb_dw_epdir epdir, int type, int maxpktsize)
{
    uint32_t epctl = SD0PID|EPTYP(type)|USBAEP|maxpktsize;

    if (epdir == USB_DW_EPDIR_IN)
    {
        /*
         * if the hardware has dedicated fifos, we must give each IN EP
         * a unique tx-fifo even if it is non-periodic.
         */
#ifdef USB_DW_SHARED_FIFO
        if (type == USB_ENDPOINT_XFER_INT)
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
        epctl |= DESIGNWARE_DIEPCTL(epnum) & NEXTEP(0xf);
#endif
        DESIGNWARE_DIEPCTL(epnum) = epctl;
    }
    else
    {
        DESIGNWARE_DOEPCTL(epnum) = epctl;
    }

    return 1; /* ok */
}

static void usb_dw_start_rx(int epnum, void* buf, int size)
{
    struct usb_dw_ep* dw_ep = usb_dw_get_ep(epnum, USB_DW_EPDIR_OUT);

    if ((uint32_t)buf & (CACHEALIGN_SIZE-1))
        logf("%s: OUT%d %lx unaligned!", __func__, epnum, (uint32_t)buf);

    dw_ep->busy = true;
    dw_ep->status = -1;
    dw_ep->sizeleft = size;
    size = MIN(size, usb_dw_maxxfersize(epnum, USB_DW_EPDIR_OUT));
    dw_ep->size = size;

    int packets = usb_dw_calc_packets(size,
                    usb_dw_maxpktsize(epnum, USB_DW_EPDIR_OUT));

    /* Set up data destination */
    dw_ep->addr = (uint32_t*)buf;
#ifndef USB_DW_ARCH_SLAVE
    DESIGNWARE_DOEPDMA(epnum) = (uint32_t)buf;
    DISCARD_DCACHE_RANGE(buf, size);
#endif

    DESIGNWARE_DOEPTSIZ(epnum) = STUPCNT(!epnum) | PKTCNT(packets) | size;

    /* Enable the endpoint */
    DESIGNWARE_DOEPCTL(epnum) |= EPENA | (!epnum ? SNAK:CNAK);
}

static void usb_dw_ep0_wait_setup(void)
{
    usb_dw_start_rx(0, ep0_buffer.raw, 64);
}

static void usb_dw_start_tx(int epnum, const void* buf, int size)
{
    struct usb_dw_ep* dw_ep = usb_dw_get_ep(epnum, USB_DW_EPDIR_IN);

    if ((uint32_t)buf & 3)
        logf("%s: IN%d %lx unaligned!", __func__, epnum, (uint32_t)buf);

    dw_ep->busy = true;
    dw_ep->status = -1;
    dw_ep->sizeleft = size;
    size = MIN(size, usb_dw_maxxfersize(epnum, USB_DW_EPDIR_IN));
    dw_ep->size = size;

    int packets = usb_dw_calc_packets(size,
                    usb_dw_maxpktsize(epnum, USB_DW_EPDIR_IN));

    /* Set up data source */
    dw_ep->addr = (uint32_t*)buf;
#ifndef USB_DW_ARCH_SLAVE
    DESIGNWARE_DIEPDMA(epnum) = (uint32_t)buf;
    COMMIT_DCACHE_RANGE(buf, size);
#endif

#ifdef USB_DW_SHARED_FIFO
    DESIGNWARE_DIEPTSIZ(epnum) = MCCNT(IS_PERIODIC(epnum)) | PKTCNT(packets) | size;
#else
    DESIGNWARE_DIEPTSIZ(epnum) = PKTCNT(packets) | size;
#endif

    /* Enable the endpoint */
    DESIGNWARE_DIEPCTL(epnum) |= EPENA|CNAK;

#ifdef USB_DW_ARCH_SLAVE
    /* Enable interrupts to start pushing data into the FIFO */
    if (size)
#ifdef USB_DW_SHARED_FIFO
        DESIGNWARE_GINTMSK |= (IS_PERIODIC(epnum) ? PTXFE : NPTXFE);
#else
        DESIGNWARE_DIEPEMPMSK |= (1 << GET_DTXFNUM(epnum));
#endif
#endif
}

static void usb_dw_handle_setup_received(int epnum)
{
    static struct usb_ctrlrequest usb_dw_ctrlreq;

    if (epnum) {
        logf("%s: OUT%d ignored", __func__, epnum);
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

static void usb_dw_abort_in_endpoint(int epnum)
{
    struct usb_dw_ep* dw_ep = usb_dw_get_ep(epnum, USB_DW_EPDIR_IN);
    if (dw_ep->busy)
    {
        usb_dw_flush_in_endpoint(epnum);
        usb_core_transfer_complete(epnum, USB_DIR_IN, -1, 0);
    }
}

static void usb_dw_handle_xfer_complete(int epnum, enum usb_dw_epdir epdir)
{
    struct usb_dw_ep* dw_ep = usb_dw_get_ep(epnum, epdir);

    if (dw_ep->busy)
    {
        if (epdir == USB_DW_EPDIR_IN)
        {
            uint32_t bytesleft = DESIGNWARE_DIEPTSIZ(epnum) & 0x7ffff;
            dw_ep->sizeleft -= (dw_ep->size - bytesleft);

            if (!bytesleft && dw_ep->sizeleft)
            {
#ifndef USB_DW_ARCH_SLAVE
                dw_ep->addr += (dw_ep->size >> 2); /* words */
#endif
                usb_dw_start_tx(epnum, dw_ep->addr, dw_ep->sizeleft);
                return;
            }

            /* SNAK the disabled EP, otherwise IN tokens for this
               EP could raise unwanted EPMIS interrupts. Useful for
               usbserial when there is no data to send. */
            DESIGNWARE_DIEPCTL(epnum) |= SNAK;

#ifdef USB_DW_SHARED_FIFO
            /* see usb-s5l8701.c */
            if (usb_dw_config.use_ptxfifo_as_plain_buffer)
            {
                int dtxfnum = GET_DTXFNUM(epnum);
                if (dtxfnum)
                    usb_dw_flush_txfifo(dtxfnum);
            }
#endif
        }
        else
        {
            uint32_t bytesleft = DESIGNWARE_DOEPTSIZ(epnum) & 0x7ffff;

            if (!epnum)
            {
                int recvbytes = 64 - bytesleft;
                dw_ep->sizeleft = dw_ep->req_size - recvbytes;
                if (dw_ep->req_addr)
                    memcpy(dw_ep->req_addr, ep0_buffer.raw, dw_ep->req_size);
            }
            else
            {
                dw_ep->sizeleft -= (dw_ep->size - bytesleft);
                if (!bytesleft && dw_ep->sizeleft)
                {
#ifndef USB_DW_ARCH_SLAVE
                    dw_ep->addr += (dw_ep->size >> 2); /* words */
#endif
                    usb_dw_start_rx(epnum, dw_ep->addr, dw_ep->sizeleft);
                    return;
                }
            }
        }

        dw_ep->busy = false;
        dw_ep->status = 0;
        semaphore_release(&dw_ep->complete);

        int transfered = dw_ep->req_size - dw_ep->sizeleft;
        usb_core_transfer_complete(epnum, (epdir == USB_DW_EPDIR_IN) ?
                    USB_DIR_IN : USB_DIR_OUT, dw_ep->status, transfered);
    }

    if (!epnum && (epdir == USB_DW_EPDIR_OUT))
        usb_dw_ep0_wait_setup();
}

#ifdef USB_DW_SHARED_FIFO
static int usb_dw_get_epmis(void)
{
    unsigned epmis;
    uint32_t gnptxsts = DESIGNWARE_GNPTXSTS;

    if (((gnptxsts >> 16) & 0xff) >= hw_nptxqdepth)
        return -1;  /* empty queue */

    /* get the EP on the top of the queue, 0 < idx < number of available IN EPs */
    int idx = (gnptxsts >> 27) & 0xf;
    for (epmis = 0; epmis < USB_NUM_ENDPOINTS; epmis++)
        if ((usb_endpoints & (1 << epmis)) && !idx--)
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

static void usb_dw_irq(void)
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
            if ((usb_endpoints & (1 << ep)) && IS_PERIODIC(ep))
                usb_dw_try_push(ep);
    }

    if (gintsts & NPTXFE)
    {
        /* First disable the IRQ, it will be re-enabled later if there is anything left to be done. */
        DESIGNWARE_GINTMSK &= ~NPTXFE;
        /* Check all non-periodic endpoints for anything to be transmitted */
        for (ep = 0; ep < USB_NUM_ENDPOINTS; ep++)
            if ((usb_endpoints & (1 << ep)) && !IS_PERIODIC(ep))
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

#ifdef USB_DW_SHARED_FIFO
            if (epints & ITTXFE)
            {
                if (epmis_msk & (1 << ep))
                {
                    DESIGNWARE_DIEPCTL(ep) |= EPENA;

                    epmis_msk &= ~(1 << ep);
                    if (!epmis_msk)
                        DESIGNWARE_DIEPMSK &= ~ITTXFE;
                }
            }

#elif defined(USB_DW_ARCH_SLAVE)
            if (epints & TXFE)
            {
                usb_dw_handle_dtxfifo(ep);
            }
#endif

            /* Clear XFRC here, if this is a 'multi-transfer' request the a
               new transfer is going to be launched,, this ensures it will
               not miss a single interrupt. */
            DESIGNWARE_DIEPINT(ep) = epints;

            if (epints & XFRC)
            {
                usb_dw_handle_xfer_complete(ep, USB_DW_EPDIR_IN);
            }
        }
    }

    /* OUT */
    for (ep = 0; ep < USB_NUM_ENDPOINTS; ep++)
    {
        if (daint & (1 << (ep + 16)))
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
                usb_dw_handle_xfer_complete(ep, USB_DW_EPDIR_OUT);
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

static void usb_dw_check_hw(void)
{
    uint32_t ghwcfg2 = DESIGNWARE_GHWCFG2;
    uint32_t ghwcfg3 = DESIGNWARE_GHWCFG3;
    uint32_t ghwcfg4 = DESIGNWARE_GHWCFG4;

    int hw_maxtxfifos;  /* periodic or dedicated */

    /* check for supported HW */
    if (((ghwcfg2 >> 10) & 0xf)+1 < USB_NUM_ENDPOINTS)
        panicf("%s: USB_NUM_ENDPOINTS too big", __func__);
#ifndef USB_DW_ARCH_SLAVE
    if (((ghwcfg2 >> 3) & 3) != 2)
        panicf("%s: internal DMA not supported", __func__);
#endif
#ifdef USB_DW_SHARED_FIFO
    if ((ghwcfg4 >> 25) & 1)
        panicf("%s: shared TxFIFO not supported", __func__);
    hw_maxtxfifos = ghwcfg4 & 0xf;
    hw_nptxqdepth = (1 << (((ghwcfg2 >> 22) & 3) + 1));
#else
    if (!((ghwcfg4 >> 25) & 1))
        panicf("%s: dedicated TxFIFO not supported", __func__);
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
        panicf("%s: insufficient FIFO memory", __func__);
    n_ptxfifos = (hw_fifomem - c->rx_fifosz - c->nptx_fifosz) / c->ptx_fifosz;
    if (n_ptxfifos > hw_maxtxfifos)
        n_ptxfifos = hw_maxtxfifos;
    logf("%s: FIFO mem=%d rx=%d nptx=%d ptx=%dx%d", __func__,
        hw_fifomem, c->rx_fifosz, c->nptx_fifosz, n_ptxfifos, c->ptx_fifosz);

    /* HWCFG registers are not checked, if an option is not supported
       then the related bits should be Read-Only. */
    DESIGNWARE_GUSBCFG = c->phytype;
    if (DESIGNWARE_GUSBCFG != c->phytype)
        panicf("%s: phytype=%x not supported", __func__, c->phytype);
}

static void usb_dw_init(void)
{
    static bool initialized = false;
    const struct usb_dw_config *c = &usb_dw_config;

    if (!initialized)
    {
        for (int ep = 0; ep < USB_NUM_ENDPOINTS; ep++)
            for (int dir = 0; dir < USB_DW_NUM_DIRS; dir++)
                semaphore_init(&usb_dw_get_ep(ep, dir)->complete, 1, 0);
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
     * According to p428 of the design guide, we need to ensure that
     * fifos are flushed before continuing.
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
    DESIGNWARE_DCFG |= EPMISCNT(0x1f);
#endif

#if !defined(USB_DW_ARCH_SLAVE) && !defined(USB_DW_SHARED_FIFO)
    if (c->ahb_threshold)
        DESIGNWARE_DTHRCTL = ARPEN|RXTHRLEN(c->ahb_threshold)|RXTHREN;
#endif

    /* Set up interrupts */
    DESIGNWARE_DOEPMSK = STUP|XFRC;
    DESIGNWARE_DIEPMSK = TOC|XFRC;

    /* Unmask all available endpoints */
    DESIGNWARE_DAINTMSK = 0xffffffff;
    usb_endpoints = DESIGNWARE_DAINTMSK;

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

static void usb_dw_exit(void)
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
    for (int ep = 1; ep < USB_NUM_ENDPOINTS; ep++)
    {
        if ((usb_endpoints & (1 << ep)) &&
                        usb_dw_get_ep(ep, USB_DW_EPDIR_IN)->active)
        {
            usb_dw_flush_in_endpoint(ep);
            DESIGNWARE_DIEPCTL(ep) |= SD0PID;
        }
        if ((usb_endpoints & (1 << (ep + 16))) &&
                        usb_dw_get_ep(ep, USB_DW_EPDIR_OUT)->active)
        {
            usb_dw_flush_out_endpoint(ep);
            DESIGNWARE_DOEPCTL(ep) |= SD0PID;
        }
    }
    usb_dw_target_enable_irq();
}

bool usb_drv_stalled(int endpoint, bool in)
{
    return usb_dw_get_stall(EP_NUM(endpoint),
                    in ? USB_DW_EPDIR_IN : USB_DW_EPDIR_OUT);
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
    usb_dw_target_disable_irq();
    usb_dw_set_stall(EP_NUM(endpoint),
                    in ? USB_DW_EPDIR_IN : USB_DW_EPDIR_OUT, stall);
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
    enum usb_dw_epdir epdir = (EP_DIR(dir) == DIR_IN) ?
                                USB_DW_EPDIR_IN : USB_DW_EPDIR_OUT;

    usb_dw_target_disable_irq();
    for (int ep = 1; ep < USB_NUM_ENDPOINTS; ep++)
    {
        if (usb_endpoints & (1 << (ep + USB_DW_DIR_OFF(epdir))))
        {
            struct usb_dw_ep* dw_ep = usb_dw_get_ep(ep, epdir);
            if (!dw_ep->active)
            {
                if (usb_dw_configure_ep(ep, epdir, type,
                                usb_drv_port_speed() ? 512 : 64))
                {
                    dw_ep->active = true;
                    req_ep = ep | dir;
                }
                break;
            }
        }
    }
    usb_dw_target_enable_irq();
    return req_ep;
}

void usb_drv_release_endpoint(int endpoint)
{
    int epnum = EP_NUM(endpoint);
    if (!epnum) return;
    enum usb_dw_epdir epdir = (EP_DIR(endpoint) == DIR_IN) ?
                                USB_DW_EPDIR_IN : USB_DW_EPDIR_OUT;
    struct usb_dw_ep* dw_ep = usb_dw_get_ep(epnum, epdir);

    usb_dw_target_disable_irq();
    if (dw_ep->active)
    {
        usb_dw_unconfigure_ep(epnum, epdir);
        dw_ep->active = false;
    }
    usb_dw_target_enable_irq();
}

int usb_drv_recv(int endpoint, void* ptr, int length)
{
    int epnum = EP_NUM(endpoint);
    struct usb_dw_ep* dw_ep = usb_dw_get_ep(epnum, USB_DW_EPDIR_OUT);

    usb_dw_target_disable_irq();
    if (dw_ep->active)
    {
        dw_ep->req_addr = ptr;
        dw_ep->req_size = length;
        /* OUT0 is always launched waiting for STUP packed,
           it is CNAKed to receive app data */
        if (epnum == 0)
            DESIGNWARE_DOEPCTL(0) |= CNAK;
        else
            usb_dw_start_rx(epnum, ptr, length);
    }
    usb_dw_target_enable_irq();
    return 0;
}

int usb_drv_send_nonblocking(int endpoint, void *ptr, int length)
{
    int epnum = EP_NUM(endpoint);
    struct usb_dw_ep* dw_ep = usb_dw_get_ep(epnum, USB_DW_EPDIR_IN);

    usb_dw_target_disable_irq();
    if (dw_ep->active)
    {
        dw_ep->req_addr = ptr;
        dw_ep->req_size = length;
        usb_dw_start_tx(epnum, ptr, length);
    }
    usb_dw_target_enable_irq();
    return 0;
}

int usb_drv_send(int endpoint, void *ptr, int length)
{
    int epnum = EP_NUM(endpoint);
    struct usb_dw_ep* dw_ep = usb_dw_get_ep(epnum, USB_DW_EPDIR_IN);

    semaphore_wait(&dw_ep->complete, 0);

    usb_drv_send_nonblocking(endpoint, ptr, length);

    if (semaphore_wait(&dw_ep->complete, HZ) == OBJ_WAIT_TIMEDOUT)
    {
        usb_dw_target_disable_irq();
        usb_dw_abort_in_endpoint(epnum);
        usb_dw_target_enable_irq();
    }

    return dw_ep->status;
}
