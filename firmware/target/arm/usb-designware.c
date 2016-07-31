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
 * Copyright (C) 2014 by Marcin Bukat
 * Copyright (C) 2016 by Cástor Muñoz
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

#include "usb-designware.h"

/* Define LOGF_ENABLE to enable logf output in this file */
/*#define LOGF_ENABLE*/
#include "logf.h"


/* The ARM940T uses a subset of the ARMv4 functions, not
 * supporting clean/invalidate cache entries using MVA.
 */
#if CONFIG_CPU == S5L8701
#define DISCARD_DCACHE_RANGE(b,s)   commit_discard_dcache()
#define COMMIT_DCACHE_RANGE(b,s)    commit_dcache()
#else
#define DISCARD_DCACHE_RANGE(b,s)   discard_dcache_range(b,s)
#define COMMIT_DCACHE_RANGE(b,s)    commit_dcache_range(b,s)
#endif

#ifndef USB_DW_TOUTCAL
#define USB_DW_TOUTCAL 0
#endif

#define GET_DTXFNUM(ep) ((DWC_DIEPCTL(ep)>>22) & 0xf)

#define USB_DW_NUM_DIRS 2
#define USB_DW_DIR_OFF(dir) (((dir) == USB_DW_EPDIR_IN) ? 0 : 16)

enum usb_dw_epdir
{
    USB_DW_EPDIR_IN = 0,
    USB_DW_EPDIR_OUT = 1,
};

union usb_ep0_buffer
{
    struct usb_ctrlrequest setup;
    uint8_t raw[64];
};

static union usb_ep0_buffer ep0_buffer USB_DEVBSS_ATTR;

/* Internal EP state/info */
struct usb_dw_ep
{
    struct semaphore complete;
    uint32_t* req_addr;
    uint32_t req_size;
    uint32_t* addr;
    uint32_t sizeleft;
    uint32_t size;
    int8_t status;
    uint8_t active;
    uint8_t busy;
};

static struct usb_dw_ep usb_dw_ep_list[USB_NUM_ENDPOINTS][USB_DW_NUM_DIRS];

static uint32_t usb_endpoints;  /* available EPs mask */

/* For SHARED_FIFO mode this is the number of periodic Tx FIFOs
   (usually 1), otherwise it is the number of dedicated Tx FIFOs
   (not counting NPTX FIFO that is always dedicated for IN0). */
static int n_ptxfifos;
static uint16_t ptxfifo_usage;

static uint32_t hw_maxbytes;
static uint32_t hw_maxpackets;
#ifdef USB_DW_SHARED_FIFO
static uint8_t hw_nptxqdepth;
static uint32_t epmis_msk;
static uint32_t ep_periodic_msk;
#endif

static const char *dw_dir_str[USB_DW_NUM_DIRS] =
{
    [USB_DW_EPDIR_IN] = "IN",
    [USB_DW_EPDIR_OUT] = "OUT",
};


static struct usb_dw_ep *usb_dw_get_ep(int epnum, enum usb_dw_epdir epdir)
{
    return &usb_dw_ep_list[epnum][epdir];
}

static int usb_dw_maxpktsize(int epnum, enum usb_dw_epdir epdir)
{
    return epnum ? DWC_EPCTL(epnum, epdir) & 0x3ff : 64;
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
    return !!(DWC_EPCTL(epnum, epdir) & STALL);
}

static void usb_dw_set_stall(int epnum, enum usb_dw_epdir epdir, int stall)
{
    if (stall)
    {
        DWC_EPCTL(epnum, epdir) |= STALL;
    }
    else
    {
        DWC_EPCTL(epnum, epdir) &= ~STALL;
        DWC_EPCTL(epnum, epdir) |= SD0PID;
    }
}

static void usb_dw_set_address(uint8_t address)
{
    DWC_DCFG = (DWC_DCFG & ~(0x7f0)) | DAD(address);
}

static void usb_dw_wait_for_ahb_idle(void)
{
    while (!(DWC_GRSTCTL & AHBIDL));
}

#ifdef USB_DW_SHARED_FIFO
static unsigned usb_dw_bytes_in_txfifo(int epnum, uint32_t *sentbytes)
{
    uint32_t size = usb_dw_get_ep(epnum, USB_DW_EPDIR_IN)->size;
    if (sentbytes) *sentbytes = size;
    uint32_t dieptsiz = DWC_DIEPTSIZ(epnum);
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
#endif

#ifdef USB_DW_ARCH_SLAVE
/* Read one packet/token from Rx FIFO */
static void usb_dw_handle_rxfifo(void)
{
    uint32_t rxsts = DWC_GRXSTSP;
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
                    *dw_ep->addr++ = DWC_DFIFO(0);
            }
            else
            {
                /* Discard data */
                while (words--)
                    (void) DWC_DFIFO(0);
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

    uint32_t wordsleft = ((DWC_DIEPTSIZ(epnum) & 0x7ffff) + 3) >> 2;
    if (!wordsleft) return;

    /* Get fifo space for NPTXFIFO or PTXFIFO */
    uint32_t fifospace;
    int dtxfnum = GET_DTXFNUM(epnum);
    if (dtxfnum)
    {
        uint32_t fifosize = DWC_DIEPTXF(dtxfnum - 1) >> 16;
        fifospace = fifosize - ((usb_dw_bytes_in_txfifo(epnum, NULL) + 3) >> 2);
    }
    else
    {
        uint32_t gnptxsts = DWC_GNPTXSTS;
        fifospace = ((gnptxsts >> 16) & 0xff) ? (gnptxsts & 0xffff) : 0;
    }

    uint32_t maxpktsize = usb_dw_maxpktsize(epnum, USB_DW_EPDIR_IN);
    uint32_t words = MIN((maxpktsize + 3) >> 2, wordsleft);

    if (fifospace >= words)
    {
        wordsleft -= words;
        while (words--)
            DWC_DFIFO(epnum) = *dw_ep->addr++;
    }

    if (wordsleft)
        DWC_GINTMSK |= (dtxfnum ? PTXFE : NPTXFE);
}

#else /* !USB_DW_SHARED_FIFO */
static void usb_dw_handle_dtxfifo(int epnum)
{
    struct usb_dw_ep* dw_ep = usb_dw_get_ep(epnum, USB_DW_EPDIR_IN);

    if (!dw_ep->busy)
        return;

    uint32_t wordsleft = ((DWC_DIEPTSIZ(epnum) & 0x7ffff) + 3) >> 2;

    while (wordsleft)
    {
        uint32_t words = wordsleft;
        uint32_t fifospace = DWC_DTXFSTS(epnum) & 0xffff;

        if (fifospace < words)
        {
            /* We push whole packets to read consistent info on DIEPTSIZ
               (i.e. when FIFO size is not maxpktsize multiplo). */
            int maxpktwords = usb_dw_maxpktsize(epnum, USB_DW_EPDIR_IN) >> 2;
            words = (fifospace / maxpktwords) * maxpktwords;
        }

        if (!words)
            break;

        wordsleft -= words;
        while (words--)
            DWC_DFIFO(epnum) = *dw_ep->addr++;
    }

    if (!wordsleft)
        DWC_DIEPEMPMSK &= ~(1 << GET_DTXFNUM(epnum));
}
#endif /* !USB_DW_SHARED_FIFO */
#endif /* USB_DW_ARCH_SLAVE */

static void usb_dw_flush_fifo(uint32_t fflsh, int fnum)
{
#ifdef USB_DW_ARCH_SLAVE
    /* Rx queue must be emptied before flushing Rx FIFO */
    if (fflsh & RXFFLSH)
        while (DWC_GINTSTS & RXFLVL)
            usb_dw_handle_rxfifo();
#else
    /* Wait for any DMA activity to stop */
    usb_dw_wait_for_ahb_idle();
#endif
    DWC_GRSTCTL = TXFNUM(fnum) | fflsh;
    while (DWC_GRSTCTL & fflsh);
    udelay(1);  /* Wait 3 PHY cycles */
}

/* These are the conditions that must be met so that the application can
 * disable an endpoint avoiding race conditions:
 *
 * 1) The endpoint must be enabled when EPDIS is written, otherwise the
 *    core will never raise EPDISD interrupt (thus EPDIS remains enabled).
 *
 * 2) - Periodic (SHARED_FIFO) or dedicated (!SHARED_FIFO) IN endpoints:
 *      IN NAK must be effective, to ensure that the core is not going
 *      to disable the EP just before EPDIS is written.
 *    - Non-periodic (SHARED_FIFO) IN endpoints: use usb_dw_nptx_unqueue().
 *    - OUT endpoints: GONAK must be effective, this also ensures that the
 *      core is not going to disable the EP.
 */
static void usb_dw_disable_ep(int epnum, enum usb_dw_epdir epdir)
{
    if (!epnum && (epdir == USB_DW_EPDIR_OUT))
        return;  /* The application cannot disable OUT0 */

    if (DWC_EPCTL(epnum, epdir) & EPENA)
    {
        int tmo = 50;
        DWC_EPCTL(epnum, epdir) |= EPDIS;
        while (DWC_EPCTL(epnum, epdir) & EPDIS)
        {
            if (!tmo--)
                panicf("%s: %s%d failed!", __func__, dw_dir_str[epdir], epnum);
            udelay(1);
        }
    }
}

static void usb_dw_gonak_effective(bool enable)
{
    if (enable)
    {
        if (!(DWC_DCTL & GONSTS))
            DWC_DCTL |= SGONAK;

        /* Wait for global IN NAK effective */
        int tmo = 50;
        while (~DWC_GINTSTS & GOUTNAKEFF)
        {
            if (!tmo--) panicf("%s: failed!", __func__);
#ifdef USB_DW_ARCH_SLAVE
            /* Pull Rx queue until GLOBALOUTNAK token is received. */
            if (DWC_GINTSTS & RXFLVL)
                usb_dw_handle_rxfifo();
            else
#endif
            udelay(1);
        }
    }
    else
    {
        if (DWC_DCTL & GONSTS)
            DWC_DCTL |= CGONAK;
    }
}

static void usb_dw_set_innak_effective(int epnum)
{
    if (~DWC_DIEPCTL(epnum) & NAKSTS)
    {
        /* Wait for IN NAK effective avoiding race conditions, if the
         * endpoint is disabled by the core (or it was already disabled)
         * then INEPNE is never raised.
         */
        int tmo = 50;
        DWC_DIEPCTL(epnum) |= SNAK;
        while ((DWC_DIEPCTL(epnum) & EPENA) && !(DWC_DIEPINT(epnum) & INEPNE))
        {
            if (!tmo--) panicf("%s: IN%d failed!", __func__, epnum);
            udelay(1);
        }
    }
}

#ifdef USB_DW_SHARED_FIFO
static void usb_dw_ginak_effective(bool enable)
{
    if (enable)
    {
        if (!(DWC_DCTL & GINSTS))
            DWC_DCTL |= SGINAK;

        /* Wait for global IN NAK effective */
        int tmo = 50;
        while (~DWC_GINTSTS & GINAKEFF)
        {
            if (!tmo--) panicf("%s: failed!", __func__);
            udelay(1);
        }
#ifndef USB_DW_ARCH_SLAVE
        /* Wait for any DMA activity to stop. */
        usb_dw_wait_for_ahb_idle();
#endif
    }
    else
    {
        if (DWC_DCTL & GINSTS)
            DWC_DCTL |= CGINAK;
    }
}

static void usb_dw_nptx_unqueue(int epnum)
{
    uint32_t reenable_msk = 0;

    usb_dw_ginak_effective(true);

    /* Disable EPs */
    for (int ep = 0; ep < USB_NUM_ENDPOINTS; ep++)
    {
        if (usb_endpoints & ~ep_periodic_msk & (1 << ep))
        {
            /* Disable */
            if (~DWC_DIEPCTL(ep) & EPENA)
                continue;
            DWC_DIEPCTL(ep) |= EPDIS|SNAK;

            /* Adjust */
            uint32_t packetsleft = (DWC_DIEPTSIZ(ep) >> 19) & 0x3ff;
            if (!packetsleft) continue;

            struct usb_dw_ep* dw_ep = usb_dw_get_ep(ep, USB_DW_EPDIR_IN);
            uint32_t sentbytes;
            uint32_t bytesinfifo = usb_dw_bytes_in_txfifo(ep, &sentbytes);

#ifdef USB_DW_ARCH_SLAVE
            dw_ep->addr -= (bytesinfifo + 3) >> 2;
#else
            (void) bytesinfifo;
            DWC_DIEPDMA(ep) = (uint32_t)(dw_ep->addr) + sentbytes;
#endif
            DWC_DIEPTSIZ(ep) = PKTCNT(packetsleft) | (dw_ep->size - sentbytes);

            /* Do not re-enable the EP we are going to unqueue */
            if (ep == epnum)
                continue;

            /* Mark EP to be re-enabled later */
            reenable_msk |= (1 << ep);
        }
    }

    /* Flush NPTXFIFO */
    usb_dw_flush_fifo(TXFFLSH, 0);

    /* Re-enable EPs */
    for (int ep = 0; ep < USB_NUM_ENDPOINTS; ep++)
        if (reenable_msk & (1 << ep))
            DWC_DIEPCTL(ep) |= EPENA|CNAK;

#ifdef USB_DW_ARCH_SLAVE
    if (reenable_msk)
        DWC_GINTMSK |= NPTXFE;
#endif

    usb_dw_ginak_effective(false);
}
#endif /* USB_DW_SHARED_FIFO */

static void usb_dw_flush_endpoint(int epnum, enum usb_dw_epdir epdir)
{
    struct usb_dw_ep* dw_ep = usb_dw_get_ep(epnum, epdir);
    dw_ep->busy = false;
    dw_ep->status = -1;
    semaphore_release(&dw_ep->complete);

    if (DWC_EPCTL(epnum, epdir) & EPENA)
    {
        if (epdir == USB_DW_EPDIR_IN)
        {
            /* We are shutting down an endpoint that might still have IN
             * packets in the FIFO. Disable the endpoint, wait for things
             * to settle, and flush the relevant FIFO.
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
                /* Wait for IN NAK effective to avoid race conditions
                   while shutting down the endpoint. */
                usb_dw_set_innak_effective(epnum);

                /* Disable the EP we are going to flush */
                usb_dw_disable_ep(epnum, epdir);

                /* Flush it all the way down! */
                usb_dw_flush_fifo(TXFFLSH, dtxfnum);

#if !defined(USB_DW_SHARED_FIFO) && defined(USB_DW_ARCH_SLAVE)
                DWC_DIEPEMPMSK &= ~(1 << dtxfnum);
#endif
            }
        }
        else
        {
            /* We are waiting for an OUT packet on this endpoint, which
             * might arrive any moment. Assert a global output NAK to
             * avoid race conditions while shutting down the endpoint.
             * Global output NAK also flushes the Rx FIFO.
             */
            usb_dw_gonak_effective(true);
            usb_dw_disable_ep(epnum, epdir);
            usb_dw_gonak_effective(false);
        }
    }

    /* At this point the endpoint is disabled, SNAK it (in case it is not
     * already done), it is needed for Tx shared FIFOs (to not to raise
     * unwanted EPMIS interrupts) and recomended for dedicated FIFOs.
     */
    DWC_EPCTL(epnum, epdir) |= SNAK;

#ifdef USB_DW_SHARED_FIFO
    if (epdir == USB_DW_EPDIR_IN)
    {
        epmis_msk &= ~(1 << epnum);
        if (!epmis_msk)
            DWC_DIEPMSK &= ~ITTXFE;
    }
#endif

    /* Clear all channel interrupts to avoid to process
       pending tokens for the flushed EP. */
    DWC_EPINT(epnum, epdir) = DWC_EPINT(epnum, epdir);
}

static void usb_dw_unconfigure_ep(int epnum, enum usb_dw_epdir epdir)
{
    uint32_t epctl = 0;

    if (epdir == USB_DW_EPDIR_IN)
    {
#ifdef USB_DW_SHARED_FIFO
#ifndef USB_DW_ARCH_SLAVE
        int next;
        for (next = epnum + 1; next < USB_NUM_ENDPOINTS; next++)
            if (usb_endpoints & (1 << next))
                break;
        epctl = NEXTEP(next % USB_NUM_ENDPOINTS);
#endif
        ep_periodic_msk &= ~(1 << epnum);
#endif
        ptxfifo_usage &= ~(1 << GET_DTXFNUM(epnum));
    }

    usb_dw_flush_endpoint(epnum, epdir);
    DWC_EPCTL(epnum, epdir) = epctl;
}

static int usb_dw_configure_ep(int epnum,
                enum usb_dw_epdir epdir, int type, int maxpktsize)
{
    uint32_t epctl = SD0PID|EPTYP(type)|USBAEP|maxpktsize;

    if (epdir == USB_DW_EPDIR_IN)
    {
        /*
         * If the hardware has dedicated fifos, we must give each
         * IN EP a unique tx-fifo even if it is non-periodic.
         */
#ifdef USB_DW_SHARED_FIFO
#ifndef USB_DW_ARCH_SLAVE
        epctl |= DWC_DIEPCTL(epnum) & NEXTEP(0xf);
#endif
        if (type == USB_ENDPOINT_XFER_INT)
#endif
        {
            int fnum;
            for (fnum = 1; fnum <= n_ptxfifos; fnum++)
                if (~ptxfifo_usage & (1 << fnum))
                    break;
            if (fnum > n_ptxfifos)
                return -1; /* no available fifos */
            ptxfifo_usage |= (1 << fnum);
            epctl |= DTXFNUM(fnum);
#ifdef USB_DW_SHARED_FIFO
            ep_periodic_msk |= (1 << epnum);
#endif
        }
    }

    DWC_EPCTL(epnum, epdir) = epctl;
    return 0; /* ok */
}

static void usb_dw_reset_endpoints(void)
{
    /* Initial state for all endpoints, setting OUT EPs as not busy
     * will discard all pending data (if any) on the flush stage.
     */
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

#if CONFIG_CPU == S5L8701
    /*
     * Workaround for spurious -EPROTO when receiving bulk data on Nano2G.
     *
     * The Rx FIFO and Rx queue are currupted by the received (corrupted)
     * data, must be flushed, otherwise the core can not set GONAK effective.
     */
    usb_dw_flush_fifo(RXFFLSH, 0);
#endif

    /* Flush and initialize EPs, includes disabling USBAEP on all EPs
     * except EP0 (USB HW core keeps EP0 active on all configurations).
     */
    for (int ep = 0; ep < USB_NUM_ENDPOINTS; ep++)
    {
        if (usb_endpoints & (1 << (ep + 16)))
            usb_dw_unconfigure_ep(ep, USB_DW_EPDIR_OUT);
        if (usb_endpoints & (1 << ep))
            usb_dw_unconfigure_ep(ep, USB_DW_EPDIR_IN);
    }

    ptxfifo_usage = 0;
#ifdef USB_DW_SHARED_FIFO
    ep_periodic_msk = 0;
#endif
}

static void usb_dw_start_xfer(int epnum,
                enum usb_dw_epdir epdir, const void* buf, int size)
{
    if ((uint32_t)buf & ((epdir == USB_DW_EPDIR_IN) ? 3 : CACHEALIGN_SIZE-1))
        logf("%s: %s%d %p unaligned", __func__, dw_dir_str[epdir], epnum, buf);

    struct usb_dw_ep* dw_ep = usb_dw_get_ep(epnum, epdir);

    dw_ep->busy = true;
    dw_ep->status = -1;
    dw_ep->sizeleft = size;
    size = MIN(size, usb_dw_maxxfersize(epnum, epdir));
    dw_ep->size = size;

    int packets = usb_dw_calc_packets(size, usb_dw_maxpktsize(epnum, epdir));
    uint32_t eptsiz = PKTCNT(packets) | size;
    uint32_t nak;

    /* Set up data source */
    dw_ep->addr = (uint32_t*)buf;
#ifndef USB_DW_ARCH_SLAVE
    DWC_EPDMA(epnum, epdir) = (uint32_t)buf;
#endif

    if (epdir == USB_DW_EPDIR_IN)
    {
#ifndef USB_DW_ARCH_SLAVE
        COMMIT_DCACHE_RANGE(buf, size);
#endif
#ifdef USB_DW_SHARED_FIFO
        eptsiz |= MCCNT((ep_periodic_msk >> epnum) & 1);
#endif
        nak = CNAK;
    }
    else
    {
#ifndef USB_DW_ARCH_SLAVE
        DISCARD_DCACHE_RANGE(buf, size);
#endif
        eptsiz |= STUPCNT(!epnum);
        nak = epnum ? CNAK : SNAK;
    }

    DWC_EPTSIZ(epnum, epdir) = eptsiz;

    /* Enable the endpoint */
    DWC_EPCTL(epnum, epdir) |= EPENA | nak;

#ifdef USB_DW_ARCH_SLAVE
    /* Enable interrupts to start pushing data into the FIFO */
    if ((epdir == USB_DW_EPDIR_IN) && size)
#ifdef USB_DW_SHARED_FIFO
        DWC_GINTMSK |= ((ep_periodic_msk & (1 << epnum)) ? PTXFE : NPTXFE);
#else
        DWC_DIEPEMPMSK |= (1 << GET_DTXFNUM(epnum));
#endif
#endif
}

static void usb_dw_ep0_wait_setup(void)
{
    usb_dw_start_xfer(0, USB_DW_EPDIR_OUT, ep0_buffer.raw, 64);
}

static void usb_dw_handle_setup_received(void)
{
    static struct usb_ctrlrequest usb_ctrlsetup;

    usb_dw_flush_endpoint(0, USB_DW_EPDIR_IN);

    memcpy(&usb_ctrlsetup, ep0_buffer.raw, sizeof(usb_ctrlsetup));

    if (((usb_ctrlsetup.bRequestType & USB_RECIP_MASK) == USB_RECIP_DEVICE)
     && ((usb_ctrlsetup.bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD)
     && (usb_ctrlsetup.bRequest == USB_REQ_SET_ADDRESS))
        usb_dw_set_address(usb_ctrlsetup.wValue);

    usb_core_control_request(&usb_ctrlsetup);
}

static void usb_dw_abort_endpoint(int epnum, enum usb_dw_epdir epdir)
{
    struct usb_dw_ep* dw_ep = usb_dw_get_ep(epnum, epdir);
    if (dw_ep->busy)
    {
        usb_dw_flush_endpoint(epnum, epdir);
        usb_core_transfer_complete(epnum, (epdir == USB_DW_EPDIR_OUT) ?
                                        USB_DIR_OUT : USB_DIR_IN, -1, 0);
    }
}

static void usb_dw_handle_xfer_complete(int epnum, enum usb_dw_epdir epdir)
{
    struct usb_dw_ep* dw_ep = usb_dw_get_ep(epnum, epdir);

    if (!dw_ep->busy)
        return;

    uint32_t bytesleft = DWC_EPTSIZ(epnum, epdir) & 0x7ffff;

    if (!epnum && (epdir == USB_DW_EPDIR_OUT))  /* OUT0 */
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
            usb_dw_start_xfer(epnum, epdir, dw_ep->addr, dw_ep->sizeleft);
            return;
        }

        if (epdir == USB_DW_EPDIR_IN)
        {
            /* SNAK the disabled EP, otherwise IN tokens for this
               EP could raise unwanted EPMIS interrupts. Useful for
               usbserial when there is no data to send. */
            DWC_DIEPCTL(epnum) |= SNAK;

#ifdef USB_DW_SHARED_FIFO
            /* See usb-s5l8701.c */
            if (usb_dw_config.use_ptxfifo_as_plain_buffer)
            {
                int dtxfnum = GET_DTXFNUM(epnum);
                if (dtxfnum)
                    usb_dw_flush_fifo(TXFFLSH, dtxfnum);
            }
#endif
        }
    }

    dw_ep->busy = false;
    dw_ep->status = 0;
    semaphore_release(&dw_ep->complete);

    int transfered = dw_ep->req_size - dw_ep->sizeleft;
    usb_core_transfer_complete(epnum, (epdir == USB_DW_EPDIR_OUT) ?
                USB_DIR_OUT : USB_DIR_IN, dw_ep->status, transfered);
}

#ifdef USB_DW_SHARED_FIFO
static int usb_dw_get_epmis(void)
{
    unsigned epmis;
    uint32_t gnptxsts = DWC_GNPTXSTS;

    if (((gnptxsts >> 16) & 0xff) >= hw_nptxqdepth)
        return -1;  /* empty queue */

    /* Get the EP on the top of the queue, 0 < idx < number of available
       IN endpoints */
    int idx = (gnptxsts >> 27) & 0xf;
    for (epmis = 0; epmis < USB_NUM_ENDPOINTS; epmis++)
        if ((usb_endpoints & (1 << epmis)) && !idx--)
            break;

    /* The maximum EP mismatch counter is configured, so we verify all NPTX
       queue entries, 4 bits per entry, first entry at DTKQNR1[11:8] */
    uint32_t volatile *dtknqr = &DWC_DTKNQR1;
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
        /* The EP is disabled, unqueued, and reconfigured to re-reenable it
           later when a token is received, (or it will be cancelled by
           timeout if it was a blocking request). */
        usb_dw_nptx_unqueue(epmis);

        epmis_msk |= (1 << epmis);
        if (epmis_msk)
            DWC_DIEPMSK |= ITTXFE;

        /* Be sure the status is clear */
        DWC_DIEPINT(epmis) = ITTXFE;

        /* Must disable NAK to allow to get ITTXFE interrupts for this EP */
        DWC_DIEPCTL(epmis) |= CNAK;
    }
    usb_dw_ginak_effective(false);
}
#endif /* USB_DW_SHARED_FIFO */

static void usb_dw_irq(void)
{
    int ep;
    uint32_t daint;

#ifdef USB_DW_ARCH_SLAVE
    /* Handle one packet at a time, the IRQ will re-trigger if there's
       something left. */
    if (DWC_GINTSTS & RXFLVL)
    {
        usb_dw_handle_rxfifo();
    }
#endif

#ifdef USB_DW_SHARED_FIFO
    if (DWC_GINTSTS & EPMIS)
    {
        usb_dw_handle_token_mismatch();
        DWC_GINTSTS = EPMIS;
    }

#ifdef USB_DW_ARCH_SLAVE
    uint32_t gintsts = DWC_GINTSTS & DWC_GINTMSK;
    if (gintsts & PTXFE)
    {
        /* First disable the IRQ, it will be re-enabled later if there
           is anything left to be done. */
        DWC_GINTMSK &= ~PTXFE;
        /* Check all periodic endpoints for anything to be transmitted */
        for (ep = 1; ep < USB_NUM_ENDPOINTS; ep++)
            if (usb_endpoints & ep_periodic_msk & (1 << ep))
                usb_dw_try_push(ep);
    }

    if (gintsts & NPTXFE)
    {
        /* First disable the IRQ, it will be re-enabled later if there
           is anything left to be done. */
        DWC_GINTMSK &= ~NPTXFE;
        /* Check all non-periodic endpoints for anything to be transmitted */
        for (ep = 0; ep < USB_NUM_ENDPOINTS; ep++)
            if (usb_endpoints & ~ep_periodic_msk & (1 << ep))
                usb_dw_try_push(ep);
    }
#endif /* USB_DW_ARCH_SLAVE */
#endif /* USB_DW_SHARED_FIFO */

    daint = DWC_DAINT;

    /* IN */
    for (ep = 0; ep < USB_NUM_ENDPOINTS; ep++)
    {
        if (daint & (1 << ep))
        {
            uint32_t epints = DWC_DIEPINT(ep);

            if (epints & TOC)
            {
                usb_dw_abort_endpoint(ep, USB_DW_EPDIR_IN);
            }

#ifdef USB_DW_SHARED_FIFO
            if (epints & ITTXFE)
            {
                if (epmis_msk & (1 << ep))
                {
                    DWC_DIEPCTL(ep) |= EPENA;
                    epmis_msk &= ~(1 << ep);
                    if (!epmis_msk)
                        DWC_DIEPMSK &= ~ITTXFE;
                }
            }

#elif defined(USB_DW_ARCH_SLAVE)
            if (epints & TXFE)
            {
                usb_dw_handle_dtxfifo(ep);
            }
#endif

            /* Clear XFRC here, if this is a 'multi-transfer' request then
               a new transfer is going to be launched, this ensures it will
               not miss a single interrupt. */
            DWC_DIEPINT(ep) = epints;

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
            uint32_t epints = DWC_DOEPINT(ep);

            if (!ep)
            {
                if (epints & STUP)
                {
                    usb_dw_handle_setup_received();
                }
                else if (epints & XFRC)
                {
                    usb_dw_handle_xfer_complete(0, USB_DW_EPDIR_OUT);
                }
                usb_dw_ep0_wait_setup();
                /* Clear interrupt after the current EP0 packet is handled */
                DWC_DOEPINT(0) = epints;
            }
            else
            {
                DWC_DOEPINT(ep) = epints;
                if (epints & XFRC)
                {
                    usb_dw_handle_xfer_complete(ep, USB_DW_EPDIR_OUT);
                }
            }
        }
    }

    if (DWC_GINTSTS & USBRST)
    {
        DWC_GINTSTS = USBRST;
        usb_dw_set_address(0);
        usb_dw_reset_endpoints();
        usb_dw_ep0_wait_setup();
        usb_core_bus_reset();
    }

    if (DWC_GINTSTS & ENUMDNE)
    {
        DWC_GINTSTS = ENUMDNE;
        /* Nothing to do? */
    }
}

static void usb_dw_check_hw(void)
{
    uint32_t ghwcfg2 = DWC_GHWCFG2;
    uint32_t ghwcfg3 = DWC_GHWCFG3;
    uint32_t ghwcfg4 = DWC_GHWCFG4;
    const struct usb_dw_config *c = &usb_dw_config;
    int hw_numeps;
    int hw_maxtxfifos;  /* periodic or dedicated */
    char *err;

    hw_numeps = ((ghwcfg2 >> 10) & 0xf) + 1;

    if (hw_numeps < USB_NUM_ENDPOINTS)
    {
        err = "USB_NUM_ENDPOINTS too big";
        goto panic;
    }
    /* HWCFG registers are not checked to detect the PHY, if an option
       is not supported then the related bits should be Read-Only. */
    DWC_GUSBCFG = c->phytype;
    if (DWC_GUSBCFG != c->phytype)
    {
        err = "PHY type not supported";
        goto panic;
    }
#ifndef USB_DW_ARCH_SLAVE
    if (((ghwcfg2 >> 3) & 3) != 2)
    {
        err = "internal DMA not supported";
        goto panic;
    }
#endif
#ifdef USB_DW_SHARED_FIFO
    if ((ghwcfg4 >> 25) & 1)
    {
        err = "shared TxFIFO not supported";
        goto panic;
    }
    hw_maxtxfifos = ghwcfg4 & 0xf;
    hw_nptxqdepth = (1 << (((ghwcfg2 >> 22) & 3) + 1));
#else
    if (!((ghwcfg4 >> 25) & 1))
    {
        err = "dedicated TxFIFO not supported";
        goto panic;
    }
    hw_maxtxfifos = (ghwcfg4 >> 26) & 0xf;
#endif
    hw_maxbytes = (1 << (((ghwcfg3 >> 0) & 0xf) + 11)) - 1;
    hw_maxpackets = (1 << (((ghwcfg3 >> 4) & 0x7) + 4)) - 1;
    uint16_t hw_fifomem = ghwcfg3 >> 16;

    /* Configure FIFOs, sizes are 32-bit words, we will need at least
       one periodic or dedicated Tx FIFO (really the periodic Tx FIFO
       is not needed if !USB_ENABLE_HID). */
    if (c->rx_fifosz + c->nptx_fifosz + c->ptx_fifosz > hw_fifomem)
    {
        err = "insufficient FIFO memory";
        goto panic;
    }
    n_ptxfifos = (hw_fifomem - c->rx_fifosz - c->nptx_fifosz) / c->ptx_fifosz;
    if (n_ptxfifos > hw_maxtxfifos) n_ptxfifos = hw_maxtxfifos;

    logf("%s():", __func__);
    logf(" HW version: %4lx, num EPs: %d", DWC_GSNPSID & 0xffff, hw_numeps);
    logf(" FIFO mem=%d rx=%d nptx=%d ptx=%dx%d", hw_fifomem,
                    c->rx_fifosz, c->nptx_fifosz, n_ptxfifos, c->ptx_fifosz);

    return;

panic:
    panicf("%s: %s", __func__, err);
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
    DWC_PCGCCTL = 0;

    usb_dw_check_hw();

    /* Configure PHY type (must be done before reset) */
#ifndef USB_DW_TURNAROUND
    /*
     * Turnaround time (in PHY clocks) = 4*AHB clocks + 1*PHY clock,
     * worst cases are:
     *  16-bit UTMI+: PHY=30MHz, AHB=30Mhz -> 5
     *  8-bit UTMI+:  PHY=60MHz, AHB=30MHz -> 9
     */
    int USB_DW_TURNAROUND = (c->phytype == DWC_PHYTYPE_UTMI_16) ? 5 : 9;
#endif
    uint32_t gusbcfg = c->phytype|TRDT(USB_DW_TURNAROUND)|USB_DW_TOUTCAL;
    DWC_GUSBCFG = gusbcfg;

    /* Reset the whole USB core */
    udelay(100);
    usb_dw_wait_for_ahb_idle();
    DWC_GRSTCTL = CSRST;
    while (DWC_GRSTCTL & CSRST);
    usb_dw_wait_for_ahb_idle();

    /* Configure FIFOs */
    DWC_GRXFSIZ = c->rx_fifosz;
#ifdef USB_DW_SHARED_FIFO
    DWC_GNPTXFSIZ = (c->nptx_fifosz << 16) | c->rx_fifosz;
#else
    DWC_TX0FSIZ = (c->nptx_fifosz << 16) | c->rx_fifosz;
#endif
    for (int i = 0; i < n_ptxfifos; i++)
        DWC_DIEPTXF(i) = (c->ptx_fifosz << 16) |
                        (c->nptx_fifosz + c->rx_fifosz + c->ptx_fifosz*i);
    /*
     * According to p428 of the design guide, we need to ensure that
     * fifos are flushed before continuing.
     */
    usb_dw_flush_fifo(TXFFLSH|RXFFLSH, 0x10);

    /* Configure the core */
    DWC_GUSBCFG = gusbcfg;

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
    DWC_GAHBCFG = gahbcfg;

    DWC_DCFG = NZLSOHSK;
#ifdef USB_DW_SHARED_FIFO
    /* Set EP mismatch counter to the maximum */
    DWC_DCFG |= EPMISCNT(0x1f);
#endif

#if !defined(USB_DW_ARCH_SLAVE) && !defined(USB_DW_SHARED_FIFO)
    if (c->ahb_threshold)
        DWC_DTHRCTL = ARPEN|RXTHRLEN(c->ahb_threshold)|RXTHREN;
#endif

    /* Set up interrupts */
    DWC_DOEPMSK = STUP|XFRC;
    DWC_DIEPMSK = TOC|XFRC;

    /* Unmask all available endpoints */
    DWC_DAINTMSK = 0xffffffff;
    usb_endpoints = DWC_DAINTMSK;

    uint32_t gintmsk = USBRST|ENUMDNE|IEPINT|OEPINT;
#ifdef USB_DW_ARCH_SLAVE
    gintmsk |= RXFLVL;
#endif
#ifdef USB_DW_SHARED_FIFO
    gintmsk |= EPMIS;
#endif
    DWC_GINTMSK = gintmsk;

    usb_dw_reset_endpoints();

    /* Soft disconnect */
    DWC_DCTL = SDIS;

    usb_dw_target_clear_irq();
    usb_dw_target_enable_irq();

    /* Soft reconnect */
    udelay(3000);
    DWC_DCTL &= ~SDIS;
}

static void usb_dw_exit(void)
{
    /* Soft disconnect */
    DWC_DCTL = SDIS;
    udelay(10);

    DWC_PCGCCTL = 1; /* Stop Phy clock */

    /* Disable IRQs */
    usb_dw_target_disable_irq();

    /* Disable clocks */
    usb_dw_target_disable_clocks();
}


/*
 * API functions
 */

/* Cancel transfers on configured EPs */
void usb_drv_cancel_all_transfers()
{
    usb_dw_target_disable_irq();
    for (int ep = 1; ep < USB_NUM_ENDPOINTS; ep++)
        for (int dir = 0; dir < USB_DW_NUM_DIRS; dir++)
            if (usb_endpoints & (1 << (ep + USB_DW_DIR_OFF(dir))))
                if (usb_dw_get_ep(ep, dir)->active)
                {
                    //usb_dw_flush_endpoint(ep, dir);
                    usb_dw_abort_endpoint(ep, dir);
                    DWC_EPCTL(ep, dir) |= SD0PID;
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
    return ((DWC_DSTS & 0x6) == 0);
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
    int request_ep = -1;
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
                                usb_drv_port_speed() ? 512 : 64) >= 0)
                {
                    dw_ep->active = true;
                    request_ep = ep | dir;
                }
                break;
            }
        }
    }
    usb_dw_target_enable_irq();
    return request_ep;
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
        /* OUT0 is always launched waiting for SETUP packet,
           it is CNAKed to receive app data */
        if (epnum == 0)
            DWC_DOEPCTL(0) |= CNAK;
        else
            usb_dw_start_xfer(epnum, USB_DW_EPDIR_OUT, ptr, length);
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
        usb_dw_start_xfer(epnum, USB_DW_EPDIR_IN, ptr, length);
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
        usb_dw_abort_endpoint(epnum, USB_DW_EPDIR_IN);
        usb_dw_target_enable_irq();
    }

    return dw_ep->status;
}
