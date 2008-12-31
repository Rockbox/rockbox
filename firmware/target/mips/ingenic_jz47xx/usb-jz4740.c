/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
#define LOGF_ENABLE
#include "logf.h"
#include "system.h"
#include "usb_ch9.h"
#include "usb_drv.h"
#include "usb_core.h"
#include "usb-target.h"
#include "jz4740.h"
#include "thread.h"

#define USB_EP0_IDLE      0
#define USB_EP0_RX        1
#define USB_EP0_TX        2

#define EP_BUF_LEFT(ep)  ((ep)->length - (ep)->sent)
#define EP_PTR(ep)       ((void*)((unsigned int)(ep)->buf + (ep)->sent))
#define EP_NUMBER(ep)    (((int)(ep) - (int)&endpoints[0])/sizeof(struct usb_endpoint))
#define EP_NUMBER2(ep)   (EP_NUMBER((ep))/2)
#define TOTAL_EP()       (sizeof(endpoints)/sizeof(struct usb_endpoint))
#define EP_IS_IN(ep)     (EP_NUMBER((ep))%2)

enum ep_type
{
    ep_control, ep_bulk, ep_interrupt
};

struct usb_endpoint
{
    void *buf;
    unsigned int length;
    union
    {
        unsigned int sent;
        unsigned int received;
    };

    const enum ep_type type;
    const bool use_dma;

    const unsigned int fifo_addr;
    unsigned short fifo_size;
};

static unsigned char ep0_rx_buf[64];
static unsigned char ep0state = USB_EP0_IDLE;
static struct usb_endpoint endpoints[] =
{
  /*    buf       length sent       type     use_dma   fifo_addr  fifo_size */
    {&ep0_rx_buf,    0,  {0},   ep_control,   false, USB_FIFO_EP0, 64 },
    {NULL,           0,  {0},   ep_control,   false, USB_FIFO_EP0, 64 },
    {NULL,           0,  {0},   ep_bulk,      false, USB_FIFO_EP1, 512},
    {NULL,           0,  {0},   ep_bulk,      false, USB_FIFO_EP1, 512},
    {NULL,           0,  {0},   ep_interrupt, false, USB_FIFO_EP2, 64 }
};

static inline void select_endpoint(int ep)
{
    REG_USB_REG_INDEX = ep;
}

static void readFIFO(struct usb_endpoint *ep, unsigned int size)
{
    logf("readFIFO(EP%d, %d)", EP_NUMBER2(ep), size);
    
    register unsigned char *ptr = (unsigned char*)EP_PTR(ep);
    register unsigned int *ptr32 = (unsigned int*)ptr;
    register unsigned int s = size >> 2;
    register unsigned int x;
    
    if(size > 0)
    {
        if( ((int)ptr & 3) == 0 )
        {
            while(s--)
                *ptr32++ = REG32(ep->fifo_addr);
            
            ptr = (unsigned char*)ptr32;
        }
        else
        {
            while(s--)
            {
                x = REG32(ep->fifo_addr);
                *ptr++ = (x >> 0)  & 0xff;
                *ptr++ = (x >> 8)  & 0xff;
                *ptr++ = (x >> 16) & 0xff;
                *ptr++ = (x >> 24) & 0xff;
            }
        }
        
        s = size & 3;
        while(s--)
            *ptr++ = REG8(ep->fifo_addr);
    }
}

static void writeFIFO(struct usb_endpoint *ep, unsigned int size)
{
    //logf("writeFIFO(EP%d, %d)", EP_NUMBER2(ep), size);
    
    register unsigned int *d = (unsigned int *)EP_PTR(ep);
    register unsigned char *c;
    register int s;

    if(size > 0)
    {
        s = size >> 2;
        while (s--)
            REG32(ep->fifo_addr) = *d++;
        
        if( (s = size & 3) )
        {
            c = (unsigned char *)d;
            while (s--)
                REG8(ep->fifo_addr) = *c++;
        }
    }
    else
        REG32(ep->fifo_addr) = 0;
}

static void flushFIFO(struct usb_endpoint *ep)
{
    //logf("flushFIFO(%d)", EP_NUMBER(ep));

    switch (ep->type)
    {
        case ep_control:
        break;
        
        case ep_bulk:
        case ep_interrupt:
            if(EP_IS_IN(ep))
                REG_USB_REG_INCSR |= USB_INCSR_FF;
            else
                REG_USB_REG_OUTCSR |= USB_OUTCSR_FF;
        break;
    }
}

static void EP0_send(void)
{
    register struct usb_endpoint* ep = &endpoints[1];
    register unsigned int length;
    
    if(ep->length == 0)
    {
        select_endpoint(0);
        REG_USB_REG_CSR0 |= (USB_CSR0_SVDOUTPKTRDY | USB_CSR0_DATAEND);
        return;
    }
    
    if(ep->sent == 0)
        length = (ep->length <= ep->fifo_size ? ep->length : ep->fifo_size);
    else
        length = (EP_BUF_LEFT(ep) <= ep->fifo_size ? EP_BUF_LEFT(ep) : ep->fifo_size);
    
    select_endpoint(0);
    writeFIFO(ep, length);
    ep->sent += length;
    
    if(ep->sent >= ep->length)
    {
        REG_USB_REG_CSR0 |= (USB_CSR0_INPKTRDY | USB_CSR0_DATAEND); /* Set data end! */
        ep0state = USB_EP0_IDLE;
    }
    else
        REG_USB_REG_CSR0 |= USB_CSR0_INPKTRDY;
}

static void EP0_handler(void)
{
logf("EP0_handler");
    register unsigned char csr0;

    /* Read CSR0 */
    select_endpoint(0);
    csr0 = REG_USB_REG_CSR0;

    /* Check for SentStall:
        This bit is set when a STALL handshake is transmitted. The CPU should clear this bit.
     */
    if (csr0 & USB_CSR0_SENTSTALL)
    {
        REG_USB_REG_CSR0 &= ~USB_CSR0_SENTSTALL;
        ep0state = USB_EP0_IDLE;
        return;
    }

    /* Check for SetupEnd:
        This bit will be set when a control transaction ends before the DataEnd bit has been set.
        An interrupt will be generated and the FIFO flushed at this time.
        The bit is cleared by the CPU writing a 1 to the ServicedSetupEnd bit.
     */
    if (csr0 & USB_CSR0_SETUPEND) 
    {
        REG_USB_REG_CSR0 |= USB_CSR0_SVDSETUPEND;
        ep0state = USB_EP0_IDLE;
        return;
    }
    
    /* Call relevant routines for endpoint 0 state */
    if (ep0state == USB_EP0_IDLE) 
    {
        if (csr0 & USB_CSR0_OUTPKTRDY)   /* There is data in the fifo */
        {
            readFIFO(&endpoints[0], REG_USB_REG_COUNT0);
            REG_USB_REG_CSR0 |= USB_CSR0_SVDOUTPKTRDY; /* clear OUTPKTRDY bit */
            usb_core_control_request((struct usb_ctrlrequest*)endpoints[0].buf);
        }
    }
    else if (ep0state == USB_EP0_TX)
        EP0_send();
}

static void EPIN_handler(unsigned int endpoint)
{
logf("EPIN_handler(%d)", endpoint);
    struct usb_endpoint* ep = &endpoints[endpoint*2+1];
    unsigned int length, csr;
    
    select_endpoint(endpoint);
    csr = REG_USB_REG_INCSR;
    
    if(ep->length == 0)
    {
        REG_USB_REG_INCSR = csr | USB_INCSR_INPKTRDY;
        return;
    }
    
    if(csr & USB_INCSR_SENTSTALL)
    {
        REG_USB_REG_INCSR = csr & ~USB_INCSR_SENTSTALL;
        return;
    }
    
    if(ep->sent == 0)
        length = (ep->length <= ep->fifo_size ? ep->length : ep->fifo_size);
    else
        length = (EP_BUF_LEFT(ep) <= ep->fifo_size ? EP_BUF_LEFT(ep) : ep->fifo_size);
    
    writeFIFO(ep, length);
    ep->sent += length;
    
    REG_USB_REG_INCSR = csr | USB_INCSR_INPKTRDY;
    if(ep->sent >= ep->length)
        usb_core_transfer_complete(endpoint|USB_DIR_IN, USB_DIR_IN, 0, ep->sent);
}

static void EPOUT_handler(unsigned int endpoint)
{
logf("EPOUT_handler(%d)", endpoint);
    struct usb_endpoint* ep = &endpoints[endpoint*2];
    unsigned int size, csr;

    select_endpoint(endpoint);
    csr = REG_USB_REG_OUTCSR;
    
    if(ep->buf == NULL && ep->length == 0)
    {
        REG_USB_REG_OUTCSR = csr & ~USB_OUTCSR_OUTPKTRDY;
        return;
    }
    
    if(csr & USB_OUTCSR_SENTSTALL)
    {
        REG_USB_REG_OUTCSR = csr & ~USB_OUTCSR_SENTSTALL;
        return;
    }
    
    size = REG_USB_REG_OUTCOUNT;
    ep->received += size;
    
    readFIFO(ep, size);

    REG_USB_REG_OUTCSR = csr & ~USB_OUTCSR_OUTPKTRDY;
    
    logf("received: %d length: %d", ep->received, ep->length);
    
    //if(ep->received >= ep->length)
        usb_core_transfer_complete(endpoint|USB_DIR_OUT, USB_DIR_OUT, 0, ep->received);
}

static void setup_endpoint(struct usb_endpoint *ep)
{
    int csr;
    ep->sent = 0;
    ep->length = 0;
    
    select_endpoint(EP_NUMBER2(ep));
    
    if(ep->type == ep_bulk)
    {
        if(REG_USB_REG_POWER & USB_POWER_HSMODE)
            ep->fifo_size = 512;
        else
            ep->fifo_size = 64;
    }
    
    if(EP_IS_IN(ep))
    {
        REG_USB_REG_INMAXP = ep->fifo_size;
        csr = (USB_INCSR_FF | USB_INCSR_CDT | USB_INCSRH_MODE);
        if(ep->use_dma)
            csr |= (USB_INCSRH_DMAREQENAB | USB_INCSRH_AUTOSET);
        else
            REG_USB_REG_INTRINE |= USB_INTR_EP(EP_NUMBER2(ep));
        
        REG_USB_REG_INCSR = csr;
    }
    else
    {    
        REG_USB_REG_OUTMAXP = ep->fifo_size;
        csr = (USB_OUTCSR_FF | USB_OUTCSR_CDT);
        
        if(ep->type == ep_interrupt)
            csr |= USB_OUTCSRH_DNYT;
        
        if(ep->use_dma)
            csr |= (USB_OUTCSRH_DMAREQENAB | USB_OUTCSRH_AUTOCLR | USB_OUTCSRH_DMAREQMODE);
        else
            REG_USB_REG_INTROUTE |= USB_INTR_EP(EP_NUMBER2(ep));
        
        csr = REG_USB_REG_OUTCSR;
    }
    
    //flushFIFO(ep);
}

static void udc_reset(void)
{
    logf("udc_reset()");

    register unsigned int i;
    
    /* EP0 init */
    ep0state = USB_EP0_IDLE;
    
    /* Disable interrupts */
    REG_USB_REG_INTRINE  = 0;
    REG_USB_REG_INTROUTE = 0;
    REG_USB_REG_INTRUSBE = 0;
    
    /* Disable DMA */
    REG_USB_REG_CNTL1 = 0;
    REG_USB_REG_CNTL2 = 0;
    
    /* Reset address */
    REG_USB_REG_FADDR = 0;
    
    /* High speed and softconnect */
    //REG_USB_REG_POWER = (USB_POWER_SOFTCONN | USB_POWER_HSENAB);
    REG_USB_REG_POWER = USB_POWER_SOFTCONN;
    
    /* Enable SUSPEND */
    /* REG_USB_REG_POWER |= USB_POWER_SUSPENDM; */
    
    /* Reset EP0 */
    select_endpoint(0);
    REG_USB_REG_CSR0 = (USB_CSR0_SVDOUTPKTRDY | USB_CSR0_SVDSETUPEND);
    
    for(i=2; i<TOTAL_EP(); i++) /* Skip EP0 */
        setup_endpoint(&endpoints[i]);
    
    /* Enable interrupts */
    REG_USB_REG_INTRINE |= USB_INTR_EP0;
    REG_USB_REG_INTRUSBE |= USB_INTR_RESET;
    
    usb_core_bus_reset();
}

/* Interrupt handler */
void UDC(void)
{
    /* Read interrupt registers */
    register unsigned char    intrUSB = REG_USB_REG_INTRUSB & 0x07; /* Mask SOF */
    register unsigned short   intrIn  = REG_USB_REG_INTRIN;
    register unsigned short   intrOut = REG_USB_REG_INTROUT;
    register unsigned char    intrDMA = REG_USB_REG_INTR;

    if(intrUSB == 0 && intrIn == 0 && intrOut == 0 && intrDMA == 0)
        return;

    /* EPIN & EPOUT are all handled in DMA */
    if(intrIn & USB_INTR_EP0)
        EP0_handler();
    if(intrIn & USB_INTR_INEP1)
        EPIN_handler(1);
    if(intrIn & USB_INTR_INEP2)
        EPIN_handler(2);
    if(intrOut & USB_INTR_OUTEP1)
        EPOUT_handler(1);
    if(intrOut & USB_INTR_OUTEP2)
        EPOUT_handler(2);
    if(intrUSB & USB_INTR_RESET)
        udc_reset();
    //if(intrUSB & USB_INTR_SUSPEND);
    //if(intrUSB & USB_INTR_RESUME);
    if(intrDMA & USB_INTR_DMA_BULKIN)
    {
        logf("DMA_BULKIN %d", ((REG_USB_REG_CNTL1 >> 4) & 0xF));
        usb_core_transfer_complete(1 | USB_DIR_IN, USB_DIR_IN, 0, 0);
    }
    if(intrDMA & USB_INTR_DMA_BULKOUT)
    {
        logf("DMA_BULKOUT %d", ((REG_USB_REG_CNTL2 >> 4) & 0xF));
        
        select_endpoint(1);
        REG_USB_REG_OUTCSR &= ~(USB_OUTCSRH_DMAREQENAB | USB_OUTCSR_OUTPKTRDY);
        
        usb_core_transfer_complete(1 | USB_DIR_OUT, USB_DIR_OUT, 0, 0);
    }
}

bool usb_drv_stalled(int endpoint, bool in)
{
    logf("usb_drv_stalled(%d, %s)", endpoint, in?"IN":"OUT");
    
    select_endpoint(endpoint & 0x7F);
    
    if(endpoint == 0)
        return (REG_USB_REG_CSR0 & USB_CSR0_SENDSTALL) != 0;
    else
    {
        if(in)
            return (REG_USB_REG_INCSR & USB_INCSR_SENDSTALL) != 0;
        else
            return (REG_USB_REG_OUTCSR & USB_OUTCSR_SENDSTALL) != 0;
    }
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
    logf("usb_drv_stall(%d,%s,%s)", endpoint, stall?"Y":"N", in?"IN":"OUT");
    
    select_endpoint(endpoint);
    
    if(endpoint == EP_CONTROL)
    {
        if(stall)
            REG_USB_REG_CSR0 |= USB_CSR0_SENDSTALL;
        else
            REG_USB_REG_CSR0 &= ~USB_CSR0_SENDSTALL;
    }
    else
    {
        if(in)
        {
            if(stall)
                REG_USB_REG_INCSR |= USB_INCSR_SENDSTALL;
            else
                REG_USB_REG_INCSR = (REG_USB_REG_INCSR & ~USB_INCSR_SENDSTALL) | USB_INCSR_CDT;
        }
        else
        {
            if(stall)
                REG_USB_REG_OUTCSR |= USB_OUTCSR_SENDSTALL;
            else
                REG_USB_REG_OUTCSR = (REG_USB_REG_OUTCSR & ~USB_OUTCSR_SENDSTALL) | USB_OUTCSR_CDT;
        }
    }
}

bool usb_drv_connected(void)
{
    return USB_DRV_CONNECTED();
}

int usb_detect(void)
{
    return usb_drv_connected() ? USB_INSERTED : USB_EXTRACTED;
}

void usb_init_device(void)
{
    USB_INIT_GPIO();
    system_enable_irq(IRQ_UDC);
}

void usb_enable(bool on)
{
    if(on)
        usb_core_init();
    else
        usb_core_exit();
}

void usb_drv_init(void)
{
    logf("usb_drv_init()");
    
    /* Set this bit to allow the UDC entering low-power mode when
     * there are no actions on the USB bus.
     * UDC still works during this bit was set.
     */
    //__cpm_stop_udc();
    __cpm_start_udc();

    /* Enable the USB PHY */
    REG_CPM_SCR |= CPM_SCR_USBPHY_ENABLE;
    
    udc_reset();
}

void usb_drv_exit(void)
{
    logf("usb_drv_exit()");
    
    /* Disable interrupts */
    REG_USB_REG_INTRINE = 0;
    REG_USB_REG_INTROUTE = 0;
    REG_USB_REG_INTRUSBE = 0;

    /* Disable DMA */
    REG_USB_REG_CNTL1 = 0;
    REG_USB_REG_CNTL2 = 0;
    
    /* Disconnect from USB */
    REG_USB_REG_POWER &= ~USB_POWER_SOFTCONN;
    
    /* Disable the USB PHY */
    REG_CPM_SCR &= ~CPM_SCR_USBPHY_ENABLE;
    
    __cpm_stop_udc();
}

void usb_drv_set_address(int address)
{
    logf("set adr: %d", address);
    
    REG_USB_REG_FADDR = address;
}

int usb_drv_send(int endpoint, void* ptr, int length)
{
    int flags;
    endpoint &= 0x7F;
    
    logf("usb_drv_send(%d, 0x%x, %d)", endpoint, (int)ptr, length);
  
    if(endpoint == EP_CONTROL)
    {
        flags = disable_irq_save();
        endpoints[1].buf = ptr;
        endpoints[1].sent = 0;
        endpoints[1].length = length;
        ep0state = USB_EP0_TX;
        EP0_send();
        
        restore_irq(flags);
        return 0;
    }
    else if(endpoint == 1)
    {
#if 0
        select_endpoint(endpoint);
        
        REG_USB_REG_ADDR2 = ((unsigned long)ptr) & 0x7fffffff;
        REG_USB_REG_COUNT2 = length;
        REG_USB_REG_CNTL2 = 1;
#else
        flags = disable_irq_save();
        endpoints[3].buf = ptr;
        endpoints[3].sent = 0;
        endpoints[3].length = length;
        EPIN_handler(1);
        restore_irq(flags);
#endif
        return 0;
    }
    else
        return -1;
}

int usb_drv_send_nonblocking(int endpoint, void* ptr, int length)
{
    return usb_drv_send(endpoint, ptr, length);
}

int usb_drv_recv(int endpoint, void* ptr, int length)
{
    int flags;
    endpoint &= 0x7F;
    
    logf("usb_drv_recv(%d, 0x%x, %d)", endpoint, (int)ptr, length);

    if(endpoint == EP_CONTROL && ptr == NULL && length == 0)
        return 0; /* ACK request, handled by the USB controller */
    else if(endpoint == 1)
    {
        logf("EP1 handled: %d", length);
        flags = disable_irq_save();
        endpoints[2].buf = ptr;
        endpoints[2].received = 0;
        endpoints[2].length = length;
        restore_irq(flags);
        return 0;
    }
    else
        return -1;
}

void usb_drv_set_test_mode(int mode)
{
    logf("usb_drv_set_test_mode(%d)", mode);
    
    switch(mode)
    {
        case 0:
            REG_USB_REG_TESTMODE &= ~USB_TEST_ALL;
            break;
        case 1:
            REG_USB_REG_TESTMODE |= USB_TEST_J;
            break;
        case 2:
            REG_USB_REG_TESTMODE |= USB_TEST_K;
            break;
        case 3:
            REG_USB_REG_TESTMODE |= USB_TEST_SE0NAK;
            break;
        case 4:
            REG_USB_REG_TESTMODE |= USB_TEST_PACKET;
            break;
    }
}

int usb_drv_port_speed(void)
{
    return ((REG_USB_REG_POWER & USB_POWER_HSMODE) != 0) ? 1 : 0;
}

void usb_drv_cancel_all_transfers(void)
{
    logf("usb_drv_cancel_all_transfers()");
    
    unsigned int i, flags;
    flags = disable_irq_save();
    
    for(i=0; i<TOTAL_EP(); i++)
    {
        endpoints[i].sent = 0;
        endpoints[i].length = 0;
        
        select_endpoint(i/2);
        flushFIFO(&endpoints[i]);
    }
    restore_irq(flags);
    
    ep0state = USB_EP0_IDLE;
}

void usb_drv_release_endpoint(int ep)
{
    //logf("usb_drv_release_endpoint(%d)", ep);
    
    (void)ep;
}

int usb_drv_request_endpoint(int dir)
{
    logf("usb_drv_request_endpoint(%s)", dir == USB_DIR_IN ? "IN" : "OUT");
    
    /* There are only 3+2 endpoints, so hardcode this ... */
    /* Currently only BULK endpoints ... */
    if(dir == USB_DIR_OUT)
        return (1 | USB_DIR_OUT);
    else if(dir == USB_DIR_IN)
        return (1 | USB_DIR_IN);
    else
        return -1;
}
