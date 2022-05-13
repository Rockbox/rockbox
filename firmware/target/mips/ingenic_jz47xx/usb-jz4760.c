/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 Roman Stolyarov
 * Copyright (C) 2020 Solomon Peachy
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
//#define LOGF_ENABLE
#include "logf.h"
#include "system.h"
#include "usb_ch9.h"
#include "usb_drv.h"
#include "usb_core.h"
#include "cpu.h"
#include "thread.h"

#define USE_USB_DMA

#define PIN_USB_DET      (32*4+19)
#define IRQ_USB_DET      GPIO_IRQ(PIN_USB_DET)
#define GPIO_USB_DET     GPIO147

#define PIN_USB_DRVVBUS  (32*4+10)
#define PIN_USB_OTG_ID   (32*3+7)

#define EP_BUF_LEFT(ep)  ((ep)->length - (ep)->sent)
#define EP_PTR(ep)       ((void*)((unsigned int)(ep)->buf + (ep)->sent))
#define EP_NUMBER(ep)    (((int)(ep) - (int)&endpoints[0])/sizeof(struct usb_endpoint))
#define EP_NUMBER2(ep)   (EP_NUMBER((ep))/2)
#define TOTAL_EP()       (sizeof(endpoints)/sizeof(struct usb_endpoint))
#define EP_IS_IN(ep)     (EP_NUMBER((ep))%2)

#define TXCSR_WZC_BITS (USB_INCSR_SENTSTALL | USB_INCSR_UNDERRUN | USB_INCSR_FFNOTEMPT | USB_INCSR_INCOMPTX)
#define RXCSR_WZC_BITS (USB_OUTCSR_SENTSTALL | USB_OUTCSR_OVERRUN | USB_OUTCSR_OUTPKTRDY)

/* NOTE:  IN/OUT is from the HOST perspective.  We're a peripheral, so:
     IN = DEV->HOST, (ie we send)
     OUT = HOST->DEV, (ie we recv)
*/

enum ep_type {
    ep_control,
    ep_bulk,
    ep_interrupt,
    ep_isochronous
};

struct usb_endpoint {
    const enum ep_type type;
    const long fifo_addr;
    unsigned short fifo_size;
    bool allocated;
    int use_dma;  /* -1 = no, 0 = mode_0, 1 = mode_1 */

    struct semaphore complete;

    uint8_t config;

    volatile void *buf;
    volatile size_t length;
    union {
        volatile size_t sent;
        volatile size_t received;
    };
    volatile int rc;
    volatile bool busy;
    volatile bool wait;
};

#define EP_INIT(_type, _fifo_addr, _fifo_size, _buf) \
    { .type = (_type), .fifo_addr = (_fifo_addr), .fifo_size = (_fifo_size), \
      .buf = (_buf), .use_dma = -1, \
      .length = 0, .busy = false, .wait = false, .allocated = false }

#define short_not_ok 1   /* only works for mass storage.. */
#define ep_doublebuf(__ep) 0

static union {
    int buf[64 / sizeof(int)];
    struct usb_ctrlrequest request;
} ep0_rx;

static volatile bool ep0_data_supplied = false;
static volatile bool ep0_data_requested = false;

static struct usb_endpoint endpoints[] =
{
    EP_INIT(ep_control,   USB_FIFO_EP(0),  64, NULL),
    EP_INIT(ep_control,   USB_FIFO_EP(0),  64, NULL),
    EP_INIT(ep_bulk,      USB_FIFO_EP(1), 512, NULL),
    EP_INIT(ep_bulk,      USB_FIFO_EP(1), 512, NULL),
    EP_INIT(ep_interrupt, USB_FIFO_EP(2), 512, NULL),
    EP_INIT(ep_interrupt, USB_FIFO_EP(2), 512, NULL),
};

static inline void select_endpoint(int ep)
{
    REG_USB_INDEX = ep;
}

static void readFIFO(struct usb_endpoint *ep, unsigned int size)
{
    logf("%s(EP%d, %d)", __func__, EP_NUMBER2(ep), size);

    register unsigned char *ptr = (unsigned char*)EP_PTR(ep);
    register unsigned int *ptr32 = (unsigned int*)ptr;
    register unsigned int s = size >> 2;
    register unsigned int x;

    if(size > 0) {
        if (((unsigned int)ptr & 3) == 0) {
            while(s--) {
                *ptr32++ = REG32(ep->fifo_addr);
            }
            ptr = (unsigned char*)ptr32;
        } else {
            while(s--) {
                x = REG32(ep->fifo_addr);
                *ptr++ = x & 0xFF; x >>= 8;
                *ptr++ = x & 0xFF; x >>= 8;
                *ptr++ = x & 0xFF; x >>= 8;
                *ptr++ = x;
            }
        }
        s = size & 3;
        while(s--) {
            *ptr++ = REG8(ep->fifo_addr);
        }
    }
}

static void writeFIFO(struct usb_endpoint *ep, size_t size)
{
    logf("%s(EP%d, %d)", __func__, EP_NUMBER2(ep), size);

    register unsigned int *d32 = (unsigned int *)EP_PTR(ep);
    register unsigned char *d8 = (unsigned char *)d32;
    register size_t s = size >> 2;
    register unsigned int x;

    if (size > 0) {
        if (((unsigned int)d8 & 3) == 0) {
            while (s--) {
                REG32(ep->fifo_addr) = *d32++;
            }
            d8 = (unsigned char *)d32;
        } else {
            while (s--) {
                x = (unsigned int)(*d8++) & 0xff;
                x |= ((unsigned int)(*d8++) & 0xff) << 8;
                x |= ((unsigned int)(*d8++) & 0xff) << 16;
                x |= ((unsigned int)(*d8++) & 0xff) << 24;
                REG32(ep->fifo_addr) = x;
            }
        }
	s = size & 3;
        while (s--) {
            REG8(ep->fifo_addr) = *d8++;
        }
    }
}

static void flushFIFO(struct usb_endpoint *ep)
{
    logf("%s(%d)", __func__, EP_NUMBER(ep));

    switch (ep->type) {
    case ep_control:
        break;
    case ep_bulk:
    case ep_interrupt:
    case ep_isochronous:
        if (EP_IS_IN(ep)) {
            REG_USB_INCSR |= (USB_INCSR_FF | USB_INCSR_CDT);
        } else {
            REG_USB_OUTCSR |= (USB_OUTCSR_FF | USB_OUTCSR_CDT);
        }
        break;
    }
}

static inline void ep_transfer_completed(struct usb_endpoint* ep)
{
    ep->sent   = 0;
    ep->length = 0;
    ep->buf    = NULL;
    ep->busy   = false;
    if (ep->wait) {
        semaphore_release(&ep->complete);
    }
}

static void EP0_send(void)
{
    struct usb_endpoint* ep = &endpoints[0];
    unsigned int length;
    unsigned short csr0;

    select_endpoint(0);
    csr0 = REG_USB_CSR0;

    logf("%s(): 0x%x %d %d", __func__, csr0, ep->sent, ep->length);

    if (ep->sent == 0) {
        length = MIN(ep->length, ep->fifo_size);
        REG_USB_CSR0 = (csr0 | USB_CSR0_FLUSHFIFO);
    } else {
        length = MIN(EP_BUF_LEFT(ep), ep->fifo_size);
    }

    writeFIFO(ep, length);
    ep->sent += length;

    if (ep->sent >= ep->length) {
        REG_USB_CSR0 = (csr0 | USB_CSR0_INPKTRDY | USB_CSR0_DATAEND); /* Set data end! */
        if (!ep->wait) {
            usb_core_transfer_complete(EP_CONTROL, USB_DIR_IN, 0, ep->sent);
	}
        ep->rc = 0;
        ep_transfer_completed(ep);
    } else {
        REG_USB_CSR0 = (csr0 | USB_CSR0_INPKTRDY);
    }
}

static void EP0_handler(void)
{
    unsigned short csr0;
    struct usb_endpoint *ep_send = &endpoints[0];
    struct usb_endpoint *ep_recv = &endpoints[1];

    /* Read CSR0 */
    select_endpoint(0);
    csr0 = REG_USB_CSR0;

    logf("%s(): 0x%x %d", __func__, csr0, ep_send->busy);

    /* Check for SentStall:
        This bit is set when a STALL handshake is transmitted. The CPU should clear this bit.
     */
    if (csr0 & USB_CSR0_SENTSTALL) {
        REG_USB_CSR0 = csr0 & ~USB_CSR0_SENTSTALL;
        return;
    }

    /* Check for SetupEnd:
        This bit will be set when a control transaction ends before the DataEnd bit has been set.
        An interrupt will be generated and the FIFO flushed at this time.
        The bit is cleared by the CPU writing a 1 to the ServicedSetupEnd bit.
     */
    if (csr0 & USB_CSR0_SETUPEND) {
        csr0 |= USB_CSR0_SVDSETUPEND;
        REG_USB_CSR0 = csr0;
        ep0_data_supplied = false;
        ep0_data_requested = false;
        if (ep_send->busy) {
            if (!ep_send->wait)
                usb_core_transfer_complete(EP_CONTROL, USB_DIR_IN, -1, 0);
            ep_transfer_completed(ep_send);
        }
        if (ep_recv->busy) {
            usb_core_transfer_complete(EP_CONTROL, USB_DIR_OUT, -1, 0);
            ep_transfer_completed(ep_recv);
        }
    }

    /* Call relevant routines for endpoint 0 state */
    if (csr0 & USB_CSR0_OUTPKTRDY) { /* There is a packet in the fifo */
        if (ep_send->busy) {
            if (!ep_send->wait) {
                usb_core_transfer_complete(EP_CONTROL, USB_DIR_IN, -1, 0);
            }
            ep_transfer_completed(ep_send);
        }
        if (ep_recv->busy && ep_recv->buf && ep_recv->length) {
            unsigned int size = REG_USB_COUNT0;
            readFIFO(ep_recv, size);
            ep_recv->received += size;
            if (size < ep_recv->fifo_size || ep_recv->received >= ep_recv->length) {
                REG_USB_CSR0 = csr0 | USB_CSR0_SVDOUTPKTRDY | USB_CSR0_DATAEND; /* Set data end! */
                usb_core_transfer_complete(EP_CONTROL, USB_DIR_OUT, 0, ep_recv->received);
                ep_transfer_completed(ep_recv);
            } else {
                REG_USB_CSR0 = csr0 | USB_CSR0_SVDOUTPKTRDY; /* clear OUTPKTRDY bit */
            }
        } else if (!ep0_data_supplied) {
            ep_recv->buf = ep0_rx.buf;
            readFIFO(ep_recv, REG_USB_COUNT0);
            csr0 |= USB_CSR0_SVDOUTPKTRDY;
            if (!ep0_rx.request.wLength) {
                csr0 |= USB_CSR0_DATAEND; /* Set data end! */
                ep0_data_requested = false;
                ep0_data_supplied = false;
            } else if (ep0_rx.request.bRequestType & USB_DIR_IN) {
                ep0_data_requested = true;
            } else {
        	ep0_data_supplied = true;
	    }
            REG_USB_CSR0 = csr0;
            usb_core_legacy_control_request(&ep0_rx.request);
            ep_transfer_completed(ep_recv);
        }
    }
    else if (ep_send->busy) {
        EP0_send();
    }
}

/* Does new work */
static void EPIN_send(unsigned int endpoint)
{
    struct usb_endpoint* ep = &endpoints[endpoint*2];
    unsigned int length, csr;

    select_endpoint(endpoint);
    csr = REG_USB_INCSR;
    logf("%s(%d): 0x%x", __func__, endpoint, csr);

    if (!ep->busy) {
        logf("Entered EPIN handler without work!");
        return;
    }

    if (csr & USB_INCSR_INPKTRDY) {
        logf("PKTRDY %d", endpoint);
        return;
    }

    if (csr & USB_INCSR_SENTSTALL) {
        logf("TX SENTSTALL %d", endpoint);
        REG_USB_INCSR = csr & ~USB_INCSR_SENTSTALL;
        return;
    }

    if (csr & USB_INCSR_FFNOTEMPT) {
        logf("FIFO is not empty! 0x%x", csr);
        return;
    }

#ifdef USE_USB_DMA
    if(ep->use_dma >= 0) {
	logf("DMA busy(%x %x %x)", REG_USB_ADDR(USB_INTR_DMA_BULKIN), REG_USB_COUNT(USB_INTR_DMA_BULKIN),REG_USB_CNTL(USB_INTR_DMA_BULKIN));

        return;
    }
#endif

    logf("EP%d: %d -> %d", endpoint, ep->sent, ep->length);

#ifdef USE_USB_DMA
    /* Can we use DMA? */
    if (ep->type == ep_bulk && ep->length && (!(((unsigned long)ep->buf + ep->sent) % 4)) && !button_hold()) {
        if (ep->length >= ep->fifo_size)
            ep->use_dma = 1;
        else
            ep->use_dma = 0;
    } else {
        ep->use_dma = -1;
    }

    if (ep->use_dma >= 0) {
        commit_discard_dcache_range((void*)ep->buf + ep->sent, ep->length - ep->sent);
        /* Set up DMA */
        uint16_t dmacr = USB_CNTL_BURST_16 | USB_CNTL_EP(EP_NUMBER2(ep)) | USB_CNTL_ENA | USB_CNTL_INTR_EN | USB_CNTL_DIR_IN ;
        if (ep->use_dma > 0)
            dmacr |= USB_CNTL_MODE_1;

        REG_USB_ADDR(USB_INTR_DMA_BULKIN) = PHYSADDR((unsigned long)ep->buf + ep->sent);
        REG_USB_COUNT(USB_INTR_DMA_BULKIN) = ep->length - ep->sent;
        REG_USB_CNTL(USB_INTR_DMA_BULKIN) = dmacr;

	uint16_t csr = REG_USB_INCSR;
	if (ep->use_dma == 0) {
            csr &= ~((USB_INCSRH_AUTOSET | USB_INCSRH_DMAREQENAB) << 8);
            REG_USB_INCSR = csr | TXCSR_WZC_BITS;
            csr &= ~((USB_INCSRH_DMAREQMODE) << 8);
            csr |= ((USB_INCSRH_DMAREQENAB | USB_INCSRH_MODE) << 8);
        } else {
            csr |= ((USB_INCSRH_DMAREQENAB | USB_INCSRH_MODE | USB_INCSRH_DMAREQMODE) << 8);
            csr |= ((USB_INCSRH_AUTOSET) << 8);
        }
        csr &= ~USB_INCSR_UNDERRUN;

        logf("DMA setup(%d: %x %x %x %x - %d)", EP_NUMBER2(ep), (unsigned int)PHYSADDR((unsigned long)ep->buf), ep->length, dmacr, csr, ep->use_dma);

        REG_USB_INCSR = csr;

        return;
    }
#endif

    /* Non-DMA code */
    if (ep->sent == 0)
        length = MIN(ep->length, ep->fifo_size);
    else
        length = MIN(EP_BUF_LEFT(ep), ep->fifo_size);

    writeFIFO(ep, length);
    ep->sent += length;
    csr &= ~USB_INCSR_UNDERRUN;
    csr |= USB_INCSR_INPKTRDY;
    logf("Non-DMA TX %x", csr);
    REG_USB_INCSR = csr;
}

static void EPIN_complete(unsigned int endpoint)
{
    struct usb_endpoint* ep = &endpoints[endpoint*2];
    uint16_t csr;

    select_endpoint(endpoint);
    csr = REG_USB_INCSR;
    logf("%s(%d): 0x%x", __func__, endpoint, csr);

    if (csr & USB_INCSR_SENTSTALL) {
        logf("TX SENTSTALL %d", endpoint);
        REG_USB_INCSR = csr & ~USB_INCSR_SENTSTALL; // XXX TXCSR_P_WZC_BITS
        return;
    }

    if (csr & USB_INCSR_UNDERRUN) {
        csr |= TXCSR_WZC_BITS;
        csr &= ~(USB_INCSR_UNDERRUN | USB_INCSR_INPKTRDY);
        REG_USB_INCSR = csr;
        logf("underrun! %x", csr);
    }

    if (!ep->busy) {
        logf("Entered EPIN_complete without work!");
        return;
    }

    if (ep->use_dma >= 0) {
        logf("DMA status (%x %x %x)", REG_USB_ADDR(USB_INTR_DMA_BULKIN), REG_USB_COUNT(USB_INTR_DMA_BULKIN),REG_USB_CNTL(USB_INTR_DMA_BULKIN));
        return;
    }

    /* If we get here, the operation is completed, and we need to clean up */

    /* Make sure DMA engine is idle */
    if (csr & (USB_INCSRH_DMAREQENAB << 8)) {
        csr |= TXCSR_WZC_BITS;
        csr &= ~(USB_INCSR_UNDERRUN | USB_INCSR_INPKTRDY |
                 ((USB_INCSRH_DMAREQENAB | USB_INCSRH_AUTOSET) << 8));
        REG_USB_INCSR = csr;
        csr = REG_USB_INCSR;
        logf("DMA cleanup %x", csr);
    }

    // XXX send a zero-length packet if necessary.
    // if tx complete, and ep->length > 0 and ep->length % fifo == 0,
    // REG_USB_INCSR = MODE | PKTRDY;
    // Not needed for mass storage as it counts packets but
    // if we ever enable other protocls...

    logf("EP%d: %d -> %d", endpoint, ep->sent, ep->length);

    if (ep->sent >= ep->length) {
        if (!ep->wait)
            usb_core_transfer_complete(endpoint, USB_DIR_IN, 0, ep->sent);
        ep->rc = 0;
        ep_transfer_completed(ep);
        logf("send complete");
    } else {
        EPIN_send(endpoint);
    }
}

static void EPOUT_handler(unsigned int endpoint)
{
    struct usb_endpoint* ep = &endpoints[endpoint*2+1];
    unsigned int size, csr;

    if(!ep->busy) {
        logf("Entered EPOUT handler without work!");
        return;
    }

    select_endpoint(endpoint);
    while ((csr = REG_USB_OUTCSR) & (USB_OUTCSR_SENTSTALL|USB_OUTCSR_OUTPKTRDY)) {
        logf("%s(%d): 0x%x", __func__, endpoint, csr);
        if (csr & USB_OUTCSR_SENTSTALL) {
            logf("stall sent, flushing fifo..");
            flushFIFO(ep);
            REG_USB_OUTCSR = csr & ~USB_OUTCSR_SENTSTALL;
            return;
        }

#ifdef USE_USB_DMA
        if (ep->use_dma >= 0) {
            logf("DMA busy(%x %x %x)", REG_USB_ADDR(USB_INTR_DMA_BULKOUT), REG_USB_COUNT(USB_INTR_DMA_BULKOUT),REG_USB_CNTL(USB_INTR_DMA_BULKOUT));
            return;
        }

        /* Can we use DMA? */
        if (ep->type == ep_bulk && ep->length && (!(((unsigned long)ep->buf + ep->received) % 4)) && !button_hold()) {
            if (ep->length >= ep->fifo_size && short_not_ok)
                ep->use_dma = 1;
            else
                ep->use_dma = 0;
        } else {
            ep->use_dma = -1;
        }
        logf("RX DMA? %d", ep->use_dma);

        /* Set up RX side for DMA */
        if (ep->use_dma == 1) {
            csr |= (USB_OUTCSRH_AUTOCLR) << 8;
            REG_USB_OUTCSR = csr;
            csr |= USB_OUTCSRH_DMAREQENAB << 8;
            REG_USB_OUTCSR = csr;

            csr |= (USB_OUTCSRH_DMAREQMODE << 8); // XXX
//            /* Work around HW quirk; write and clear DMAMODE */
//            REG_USB_OUTCSR = csr | (USB_OUTCSRH_DMAREQMODE << 8);

        } else if (ep->use_dma == 0) {
            if (ep_doublebuf(ep))  // XXX or isoc..
                csr |= ((USB_OUTCSRH_AUTOCLR) << 8);
            csr |= USB_OUTCSRH_DMAREQENAB << 8;
        }
        /* Set up DMA engine */
        if (ep->use_dma >= 0) {
            REG_USB_OUTCSR = csr;
            logf("DMA RX %d csr %x", ep->use_dma, csr);
            discard_dcache_range((void*)ep->buf + ep->received, ep->length - ep->received);

            /* Program actual DMA channel */
            uint16_t dmacr = USB_CNTL_BURST_16 | USB_CNTL_EP(EP_NUMBER2(ep)) | USB_CNTL_ENA | USB_CNTL_INTR_EN;
            if (ep->use_dma > 0)
                dmacr |= USB_CNTL_MODE_1;

            REG_USB_ADDR(USB_INTR_DMA_BULKOUT) = PHYSADDR((unsigned long)ep->buf + ep->received);
            REG_USB_COUNT(USB_INTR_DMA_BULKOUT) = ep->length - ep->received;
            REG_USB_CNTL(USB_INTR_DMA_BULKOUT) = dmacr;
            logf("DMA RX start %x %d %x", (unsigned int)ep->buf + ep->received,
                 (ep->length - ep->received), dmacr);

            return; /* ie wait for DMA to complete */
        }
#endif

        /* There is a packet in the fifo, copy it out via PIO */
        if (csr & USB_OUTCSR_OUTPKTRDY) {
            size = REG_USB_OUTCOUNT;

            readFIFO(ep, size);
            ep->received += size;

            /*if(csr & USB_OUTCSR_FFFULL)
                csr &= ~USB_OUTCSR_FFFULL;*/

            REG_USB_OUTCSR = csr & ~USB_OUTCSR_OUTPKTRDY;

            logf("received: %d max length: %d", ep->received, ep->length);

            if (size < ep->fifo_size || ep->received >= ep->length) {
                usb_core_transfer_complete(endpoint, USB_DIR_OUT, 0, ep->received);
                ep_transfer_completed(ep);
                logf("receive transfer_complete");
            }
        }
    }
}

static void EPOUT_ready(unsigned int endpoint)
{
    logf("%s(%d)", __func__, endpoint);

#ifdef USE_USB_DMA
    struct usb_endpoint* ep = &endpoints[endpoint*2+1];
    unsigned int csr;

    select_endpoint(endpoint);
    csr = REG_USB_OUTCSR;

    if (!ep->busy) {
        logf("Entered EPOUT handler without work!");
        return;
    }

    // check for stall
    // check for overrun
    // check for incomprx

    /* If DMA engine is enabled, handle and clean up */
    if (ep->use_dma >= 0 && csr & (USB_OUTCSRH_DMAREQENAB << 8)) {
        int size = VIRTADDR(REG_USB_ADDR(USB_INTR_DMA_BULKOUT)) - ((unsigned int)ep->buf + ep->received);

        csr &= ~((USB_OUTCSRH_AUTOCLR | USB_OUTCSRH_DMAREQENAB | USB_OUTCSRH_DMAREQMODE) << 8);
        REG_USB_OUTCSR = csr | RXCSR_WZC_BITS;
        logf("EPOUT DMA RX %x %d @%d/%d", csr, size, ep->received, ep->length);
        ep->received += size;

        /* Autoclear doesn't clear OutPktRdy for short packets.. */
        if ((ep->use_dma == 0 && !ep_doublebuf(ep)) || size % ep->fifo_size) {
            csr &= ~USB_OUTCSR_OUTPKTRDY;
            REG_USB_OUTCSR = csr;
            logf("Cleanup after short RX %x", csr);
        }
        // XXX what about 0-length transfers?

        /* If we're incomplete, wait for the next one.. */
        if (ep->received < ep->length && size == ep->fifo_size) {
            csr = REG_USB_OUTCSR;
            if (csr & USB_OUTCSR_OUTPKTRDY && ep_doublebuf(ep))
               goto exit;
            return;
        }

        /* It we're done, clean up */
        if (size < ep->fifo_size || ep->received >= ep->length) {
            ep->use_dma = -1;
            usb_core_transfer_complete(endpoint, USB_DIR_OUT, 0, ep->received);
            ep_transfer_completed(ep);
            logf("DMA RX transfer_complete");
            return;
        }
    }

exit:
#endif
    EPOUT_handler(endpoint);
}

#ifdef USE_USB_DMA
static void EPDMA_handler(int number)
{
    int endpoint = -1;
    int size = 0;
    struct usb_endpoint* ep = NULL;

    endpoint = (REG_USB_CNTL(number) >> 4) & 0xF;
    ep = &endpoints[endpoint*2];
    if (!(REG_USB_CNTL(number) & USB_CNTL_DIR_IN))
        ep++;  /* RX endpoint is +1 in the array */
    size = VIRTADDR(REG_USB_ADDR(number)) - ((unsigned int)ep->buf + ep->sent);

    if (number == USB_INTR_DMA_BULKIN) {
        if ((ep->use_dma == 0) || (size % ep->fifo_size)) {
            /* DMA is completed, but the final (short) packet needs to
               be manually initiated! */
            uint16_t incsr;
            select_endpoint(endpoint);
            incsr = REG_USB_INCSR;

            if (ep->use_dma == 1) {
                /* Switch to Mode 0 DMA */
                incsr &= ~((USB_INCSRH_AUTOSET | USB_INCSRH_DMAREQENAB) << 8);
                REG_USB_INCSR = incsr;
                incsr &= ~((USB_INCSRH_DMAREQMODE) << 8);
                incsr |= ((USB_INCSRH_DMAREQENAB) << 8);
            }
            incsr |= USB_INCSR_INPKTRDY;
            logf("DMA dangling %x", incsr);
            REG_USB_INCSR = incsr;
        }
        logf("DMA TX%d %d @%d/%d", number, size, ep->sent, ep->length);
        ep->sent += size;
        ep->use_dma = -1; /* DMA is complete, mark channel as idle */

        EPIN_complete(endpoint);
    } else if (number == USB_INTR_DMA_BULKOUT) {
        /* RX DMA completed */
        logf("DMA RX%d %d @%d/%d", number, size, ep->received, ep->length);
        EPOUT_ready(endpoint);
    } else if (ep) {
        ep->use_dma = -1;
    }
}
#endif

static void setup_endpoint(struct usb_endpoint *ep)
{
    int endpoint = EP_NUMBER2(ep);
    unsigned char csr, csrh;

    select_endpoint(endpoint);

    if (ep->busy) {
        if (EP_IS_IN(ep)) {
            if (ep->wait)
                semaphore_release(&ep->complete);
            else
                usb_core_transfer_complete(endpoint, USB_DIR_IN, -1, 0);
        } else {
             usb_core_transfer_complete(endpoint, USB_DIR_OUT, -1, 0);
        }
    }

    ep->busy = false;
    ep->wait = false;
    ep->sent = 0;
    ep->length = 0;

    if (ep->type != ep_control)
        ep->fifo_size = usb_drv_port_speed() ? 512 : 64;

    ep->config = REG_USB_CONFIGDATA;

    if(EP_IS_IN(ep)) {
        csr = (USB_INCSR_FF | USB_INCSR_CDT);
        csrh = USB_INCSRH_MODE;

        if (ep->type == ep_interrupt)
            csrh |= USB_INCSRH_FRCDATATOG;

        REG_USB_INMAXP   = ep->fifo_size;
        REG_USB_INCSR    = csr;
        REG_USB_INCSRH   = csrh;

	logf("IN %d (%x %x %x)", endpoint, ep->fifo_size, csr, csrh);

        if (ep->allocated)
            REG_USB_INTRINE |= USB_INTR_EP(EP_NUMBER2(ep));
    } else {
        csr = (USB_OUTCSR_FF | USB_OUTCSR_CDT);
        csrh = 0;

        if(ep->type == ep_interrupt)
            csrh |= USB_OUTCSRH_DNYT;

        REG_USB_OUTMAXP   = ep->fifo_size;
        REG_USB_OUTCSR    = csr;
        REG_USB_OUTCSRH   = csrh;

	logf("OUT %d (%x %x %x)", endpoint, ep->fifo_size, csr, csrh);

        if (ep->allocated)
            REG_USB_INTROUTE |= USB_INTR_EP(EP_NUMBER2(ep));
    }
}

static void udc_reset(void)
{
    /* From the datasheet:

       When a reset condition is detected on the USB, the controller performs the following actions:
           * Sets FAddr to 0.
           * Sets Index to 0.
           * Flushes all endpoint FIFOs.
           * Clears all control/status registers.
           * Enables all endpoint interrupts.
           * Generates a Reset interrupt.
    */

    logf("%s()", __func__);

    unsigned int i;

    REG_USB_FADDR = 0;
    REG_USB_INDEX = 0;

    /* Disable interrupts */
    REG_USB_INTRINE  = 0;
    REG_USB_INTROUTE = 0;
    REG_USB_INTRUSBE = 0;

    /* Disable DMA */
    REG_USB_CNTL(USB_INTR_DMA_BULKIN) = 0;
    REG_USB_CNTL(USB_INTR_DMA_BULKOUT) = 0;

    /* High speed, softconnect */
    REG_USB_POWER = (USB_POWER_SOFTCONN | USB_POWER_HSENAB);

    /* Reset EP0 */
    select_endpoint(0);
    REG_USB_CSR0 = (USB_CSR0_SVDOUTPKTRDY | USB_CSR0_SVDSETUPEND | USB_CSR0_FLUSHFIFO);

    endpoints[0].config = REG_USB_CONFIGDATA;
    endpoints[1].config = REG_USB_CONFIGDATA;

    if (endpoints[0].busy) {
        if (endpoints[0].wait)
            semaphore_release(&endpoints[0].complete);
        else
            usb_core_transfer_complete(EP_CONTROL, USB_DIR_IN, -1, 0);
    }

    endpoints[0].busy = false;
    endpoints[0].wait = false;
    endpoints[0].sent = 0;
    endpoints[0].length = 0;
    endpoints[0].allocated = true;

    if (endpoints[1].busy)
        usb_core_transfer_complete(EP_CONTROL, USB_DIR_OUT, -1, 0);

    endpoints[1].busy = false;
    endpoints[1].wait = false;
    endpoints[1].received = 0;
    endpoints[1].length = 0;
    endpoints[1].allocated = true;

    /* Reset other endpoints */
    for (i=2; i<TOTAL_EP(); i++)
        setup_endpoint(&endpoints[i]);

    ep0_data_supplied = false;
    ep0_data_requested = false;

    /* Enable interrupts */
    REG_USB_INTRINE  |= USB_INTR_EP(0);
    REG_USB_INTRUSBE |= USB_INTR_RESET;

    usb_core_bus_reset();
}

/* Interrupt handler */
void OTG(void)
{
    /* Read interrupt registers */
    unsigned char    intrUSB = REG_USB_INTRUSB;
    unsigned short   intrIn  = REG_USB_INTRIN;
    unsigned short   intrOut = REG_USB_INTROUT;
#ifdef USE_USB_DMA
    unsigned char    intrDMA = REG_USB_INTR;
#endif

    logf("IRQ %x %x %x %x", intrUSB, intrIn, intrOut, intrDMA);

    /* EPIN & EPOUT are all handled in DMA */
    if (intrIn & USB_INTR_EP(0))
        EP0_handler();
    if (intrIn & USB_INTR_EP(1))
        EPIN_complete(1);
    if (intrIn & USB_INTR_EP(2))
        EPIN_complete(2);
    if (intrOut & USB_INTR_EP(1))
        EPOUT_ready(1);
    if (intrOut & USB_INTR_EP(2))
        EPOUT_ready(2);
    if (intrUSB & USB_INTR_RESET)
        udc_reset();
    if (intrUSB & USB_INTR_SUSPEND)
        logf("USB suspend");
    if (intrUSB & USB_INTR_RESUME)
        logf("USB resume");
#ifdef USE_USB_DMA
    if (intrDMA & (1<<USB_INTR_DMA_BULKIN))
        EPDMA_handler(USB_INTR_DMA_BULKIN);
    if (intrDMA & (1<<USB_INTR_DMA_BULKOUT))
        EPDMA_handler(USB_INTR_DMA_BULKOUT);
#endif
}

bool usb_drv_stalled(int endpoint, bool in)
{
    endpoint &= 0x7F;

    logf("%s(%d, %s)", __func__, endpoint, in?"IN":"OUT");

    select_endpoint(endpoint);

    if (endpoint == EP_CONTROL) {
        return (REG_USB_CSR0 & USB_CSR0_SENDSTALL) != 0;
    } else {
        if (in)
            return (REG_USB_INCSR & USB_INCSR_SENDSTALL) != 0;
        else
            return (REG_USB_OUTCSR & USB_OUTCSR_SENDSTALL) != 0;
    }
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
    endpoint &= 0x7F;

    logf("%s(%d,%s,%s)", __func__, endpoint, stall?"Y":"N", in?"IN":"OUT");

    select_endpoint(endpoint);

    if(endpoint == EP_CONTROL) {
        if(stall)
            REG_USB_CSR0 |= USB_CSR0_SENDSTALL;
        else
            REG_USB_CSR0 &= ~USB_CSR0_SENDSTALL;
    } else {
        if(in) {
            if(stall)
                REG_USB_INCSR |= USB_INCSR_SENDSTALL;
            else
                REG_USB_INCSR = (REG_USB_INCSR & ~USB_INCSR_SENDSTALL) | USB_INCSR_CDT;
        } else {
            if(stall)
                REG_USB_OUTCSR |= USB_OUTCSR_SENDSTALL;
            else
                REG_USB_OUTCSR = (REG_USB_OUTCSR & ~USB_OUTCSR_SENDSTALL) | USB_OUTCSR_CDT;
        }
    }
}

int usb_detect(void)
{
    return (__gpio_get_pin(PIN_USB_DET) == 1)
        ? USB_INSERTED : USB_EXTRACTED;
}

void usb_init_device(void)
{
    __gpio_clear_pin(PIN_USB_DRVVBUS);
    __gpio_as_output(PIN_USB_DRVVBUS);

    __gpio_as_input(PIN_USB_OTG_ID);
    __gpio_as_input(PIN_USB_DET);

    __gpio_disable_pull(PIN_USB_OTG_ID);
    __gpio_disable_pull(PIN_USB_DET);

#ifdef USB_STATUS_BY_EVENT
    __gpio_as_irq_rise_edge(PIN_USB_DET);
    system_enable_irq(IRQ_USB_DET);
#endif

    system_enable_irq(IRQ_OTG);

    for (unsigned i=0; i<TOTAL_EP(); i++)
        semaphore_init(&endpoints[i].complete, 1, 0);
}

#ifdef USB_STATUS_BY_EVENT
static int usb_oneshot_callback(struct timeout *tmo)
{
    (void)tmo;
    int state = usb_detect();

    /* This is called only if the state was stable for HZ/16 - check state
     * and post appropriate event. */
    usb_status_event(state);

    if (state == USB_EXTRACTED)
        __gpio_as_irq_rise_edge(PIN_USB_DET);
    else
        __gpio_as_irq_fall_edge(PIN_USB_DET);

    return 0;
}

void GPIO_USB_DET(void)
{
    static struct timeout usb_oneshot;
    timeout_register(&usb_oneshot, usb_oneshot_callback, (HZ/16), 0);
}
#endif

void usb_enable(bool on)
{
    if (on)
        usb_core_init();
    else
        usb_core_exit();
}

void usb_attach(void)
{
    usb_enable(true);
}

void usb_drv_init(void)
{
    logf("%s()", __func__);

    /* Dis- and reconnect from USB */
    REG_USB_POWER &= ~USB_POWER_SOFTCONN;
    mdelay(20);
    REG_USB_POWER |= USB_POWER_SOFTCONN;
    mdelay(20);

    udc_reset();
}

void usb_drv_exit(void)
{
    logf("%s()", __func__);

    REG_USB_FADDR = 0;
    REG_USB_INDEX = 0;

    /* Disable interrupts */
    REG_USB_INTRINE  = 0;
    REG_USB_INTROUTE = 0;
    REG_USB_INTRUSBE = 0;

#ifdef USE_USB_DMA
    select_endpoint(1);

    logf("X DMA RX (%x %x %x %x)", REG_USB_ADDR(USB_INTR_DMA_BULKOUT), REG_USB_COUNT(USB_INTR_DMA_BULKOUT),REG_USB_CNTL(USB_INTR_DMA_BULKOUT), REG_USB_OUTCSR);
    logf("X DMA TX (%x %x %x %x)", REG_USB_ADDR(USB_INTR_DMA_BULKIN), REG_USB_COUNT(USB_INTR_DMA_BULKIN),REG_USB_CNTL(USB_INTR_DMA_BULKIN), REG_USB_INCSR);

    /* Disable DMA */
    REG_USB_CNTL(USB_INTR_DMA_BULKIN) = 0;
    REG_USB_CNTL(USB_INTR_DMA_BULKOUT) = 0;
#endif

    /* Disconnect from USB */
    REG_USB_POWER &= ~USB_POWER_SOFTCONN;
}

void usb_drv_set_address(int address)
{
    logf("%s(%d)", __func__, address);

    REG_USB_FADDR = address;
}

static void usb_drv_send_internal(struct usb_endpoint* ep, void* ptr, int length, bool blocking)
{
    int flags = disable_irq_save();

    if (ep->type == ep_control) {
        if ((ptr == NULL && length == 0) || !ep0_data_requested) {
            restore_irq(flags);
            return;
        }
        ep0_data_requested = false;
    }

    ep->buf = ptr;
    ep->sent = 0;
    ep->length = length;
    ep->busy = true;
    if (blocking) {
        ep->rc = -1;
        ep->wait = true;
    } else {
        ep->rc = 0;
    }

    if (ep->type == ep_control) {
        EP0_send();
    } else {
        EPIN_send(EP_NUMBER2(ep));
    }

    restore_irq(flags);

    if (blocking) {
        semaphore_wait(&ep->complete, HZ);
        ep->wait = false;
    }
}

int usb_drv_send_nonblocking(int endpoint, void* ptr, int length)
{
    struct usb_endpoint *ep = &endpoints[(endpoint & 0x7F)*2];

    logf("%s(%d, 0x%x, %d)", __func__, endpoint, (int)ptr, length);

    if (ep->allocated) {
        usb_drv_send_internal(ep, ptr, length, false);
        return 0;
    }

    return -1;
}

int usb_drv_send(int endpoint, void* ptr, int length)
{
    struct usb_endpoint *ep = &endpoints[(endpoint & 0x7F)*2];

    logf("%s(%d, 0x%x, %d)", __func__, endpoint, (int)ptr, length);

    if (ep->allocated) {
        usb_drv_send_internal(ep, ptr, length, true);
        return ep->rc;
    }

    return -1;
}

int usb_drv_recv_nonblocking(int endpoint, void* ptr, int length)
{
    int flags;
    struct usb_endpoint *ep;
    endpoint &= 0x7F;

    logf("%s(%d, 0x%x, %d)", __func__, endpoint, (int)ptr, length);

    if (ptr == NULL || length == 0)
        return 0;

    ep = &endpoints[endpoint*2+1];

    if (!ep->allocated)
        return -1;

    flags = disable_irq_save();

    ep->buf = ptr;
    ep->received = 0;
    ep->length = length;
    ep->busy = true;

    if (endpoint == EP_CONTROL) {
        ep0_data_supplied = false;
        EP0_handler();
    } else {
        EPOUT_handler(endpoint);
    }

    restore_irq(flags);
    return 0;
}

void usb_drv_set_test_mode(int mode)
{
    logf("%s(%d)", __func__, mode);

    switch(mode)
    {
        case 0:
            REG_USB_TESTMODE &= ~USB_TEST_ALL;
            break;
        case 1:
            REG_USB_TESTMODE |= USB_TEST_J;
            break;
        case 2:
            REG_USB_TESTMODE |= USB_TEST_K;
            break;
        case 3:
            REG_USB_TESTMODE |= USB_TEST_SE0NAK;
            break;
        case 4:
            REG_USB_TESTMODE |= USB_TEST_PACKET;
            break;
    }
}

int usb_drv_port_speed(void)
{
    return (REG_USB_POWER & USB_POWER_HSMODE) ? 1 : 0;
}

void usb_drv_cancel_all_transfers(void)
{
    logf("%s()", __func__);

    unsigned int i;
    unsigned int flags = disable_irq_save();

#ifdef USE_USB_DMA
    /* Disable DMA */
    REG_USB_CNTL(USB_INTR_DMA_BULKIN) = 0;
    REG_USB_CNTL(USB_INTR_DMA_BULKOUT) = 0;
#endif

    for(i=0; i<TOTAL_EP(); i++)
    {
        if (endpoints[i].busy)
        {
            if (i & 1)
                usb_core_transfer_complete(i >> 1, USB_DIR_OUT, -1, 0);
            else if (endpoints[i].wait)
                semaphore_release(&endpoints[i].complete);
            else
                usb_core_transfer_complete(i >> 1, USB_DIR_IN, -1, 0);
        }

        if(i != 1) /* ep0 out needs special handling */
            endpoints[i].buf = NULL;

        endpoints[i].sent = 0;
        endpoints[i].length = 0;

        select_endpoint(i/2);
        flushFIFO(&endpoints[i]);
    }

    restore_irq(flags);
}

void usb_drv_release_endpoint(int ep)
{
    int n = ep & 0x7f;

    logf("%s(%d, %s)", __func__, (ep & 0x7F), (ep >> 7) ? "IN" : "OUT");

    if (n)
    {
        int dir = ep & USB_ENDPOINT_DIR_MASK;

        if(dir == USB_DIR_IN)
        {
            REG_USB_INTRINE &= ~USB_INTR_EP(n);
            endpoints[n << 1].allocated = false;
        }
        else
        {
            REG_USB_INTROUTE &= ~USB_INTR_EP(n);
            endpoints[(n << 1) + 1].allocated = false;
        }
    }
}

int usb_drv_request_endpoint(int type, int dir)
{
    logf("%s(%d, %s)", __func__, type, (dir == USB_DIR_IN) ? "IN" : "OUT");

    dir  &= USB_ENDPOINT_DIR_MASK;
    type &= USB_ENDPOINT_XFERTYPE_MASK;

    /* There are only 3+2 endpoints, so hardcode this ... */
    switch(type)
    {
        case USB_ENDPOINT_XFER_BULK:
            if(dir == USB_DIR_IN)
            {
                if (endpoints[2].allocated)
                    break;
                endpoints[2].allocated = true;
                REG_USB_INTRINE |= USB_INTR_EP(1);
                return (1 | USB_DIR_IN);
            }
            else
            {
                if (endpoints[3].allocated)
                    break;
                endpoints[3].allocated = true;
                REG_USB_INTROUTE |= USB_INTR_EP(1);
                return (1 | USB_DIR_OUT);
            }

        case USB_ENDPOINT_XFER_INT:
            if(dir == USB_DIR_IN)
            {
                if (endpoints[4].allocated)
                    break;
                endpoints[4].allocated = true;
                REG_USB_INTRINE |= USB_INTR_EP(2);
                return (2 | USB_DIR_IN);
            }
            else
            {
                if (endpoints[5].allocated)
                    break;
                endpoints[5].allocated = true;
                REG_USB_INTROUTE |= USB_INTR_EP(2);
                return (2 | USB_DIR_OUT);
            }

        default:
            break;
    }

    return -1;
}
