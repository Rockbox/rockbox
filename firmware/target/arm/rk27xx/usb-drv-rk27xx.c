/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Marcin Bukat
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

#include "usb_ch9.h"
#include "usb_core.h"
#include <inttypes.h>
#include "power.h"

#define LOGF_ENABLE
#include "logf.h"

typedef volatile uint32_t reg32;

#ifdef LOGF_ENABLE
#define XFER_DIR_STR(dir) ((dir) ? "IN" : "OUT")
#define XFER_TYPE_STR(type) \
    ((type) == USB_ENDPOINT_XFER_CONTROL ? "CTRL" : \
     ((type) == USB_ENDPOINT_XFER_ISOC ? "ISOC" : \
      ((type) == USB_ENDPOINT_XFER_BULK ? "BULK" : \
       ((type) == USB_ENDPOINT_XFER_INT ? "INTR" : "INVL"))))
#endif

struct endpoint_t
{
    const int ep_num;            /* EP number */
    const int type;              /* EP type */
    const int dir;               /* DIR_IN/DIR_OUT */
    volatile unsigned long *stat; /* RXSTAT/TXSTAT register */
    bool allocated;              /* flag to mark EPs taken */
    volatile void *buf;          /* tx/rx buffer address */
    volatile int len;            /* size of the transfer (bytes) */
    volatile int cnt;            /* number of bytes transfered/received  */
    volatile bool block;         /* flag indicating that transfer is blocking */ 
    struct semaphore complete;   /* semaphore for blocking transfers */
};

/* compute RXCON address from RXSTAT, and so on */
#define RXSTAT(endp)        *((endp)->stat)
#define RXCON(endp)         *(1 + (endp)->stat)
#define DMAOUTCTL(endp)     *(2 + (endp)->stat)
#define DMAOUTLMADDR(endp)  *(3 + (endp)->stat)
/* compute TXCON address from TXSTAT, and so on */
#define TXSTAT(endp)        *((endp)->stat)
#define TXCON(endp)         *(1 + (endp)->stat)
#define TXBUF(endp)         *(2 + (endp)->stat)
#define DMAINCTL(endp)      *(3 + (endp)->stat)
#define DMAINLMADDR(endp)   *(4 + (endp)->stat)

#define ENDPOINT(num, type, dir, reg) \
    {num, USB_ENDPOINT_XFER_##type, USB_DIR_##dir, reg, false, NULL, 0, 0, true, {{0, 0}, 0, 0}}

static struct endpoint_t ctrlep[2] =
{
    ENDPOINT(0, CONTROL, OUT, &RX0STAT),
    ENDPOINT(0, CONTROL, IN, &TX0STAT),
};

static struct endpoint_t endpoints[16] =
{
    ENDPOINT(0,  CONTROL, OUT, NULL),  /* stub */
    ENDPOINT(1,  BULK, OUT, &RX1STAT),  /* BOUT1 */
    ENDPOINT(2,  BULK, IN,  &TX2STAT),  /* BIN2 */
    ENDPOINT(3,  INT,  IN,  &TX3STAT),  /* IIN3 */
    ENDPOINT(4,  BULK, OUT, &RX4STAT),  /* BOUT4 */
    ENDPOINT(5,  BULK, IN,  &TX5STAT),  /* BIN5 */
    ENDPOINT(6,  INT,  IN,  &TX6STAT),  /* IIN6 */
    ENDPOINT(7,  BULK, OUT, &RX7STAT),  /* BOUT7 */
    ENDPOINT(8,  BULK, IN,  &TX8STAT),  /* BIN8 */
    ENDPOINT(9,  INT,  IN,  &TX9STAT),  /* IIN9 */
    ENDPOINT(10, BULK, OUT, &RX10STAT), /* BOUT10 */
    ENDPOINT(11, BULK, IN,  &TX11STAT), /* BIN11 */
    ENDPOINT(12, INT,  IN,  &TX12STAT), /* IIN12 */
    ENDPOINT(13, BULK, OUT, &RX13STAT), /* BOUT13 */
    ENDPOINT(14, BULK, IN,  &TX14STAT), /* BIN14 */
    ENDPOINT(15, INT,  IN,  &TX15STAT), /* IIN15 */
};

static volatile bool set_address = false;
static volatile bool set_configuration = false;

#undef ENDPOINT

static void setup_received(void)
{
    static uint32_t setup_data[2];
    logf("udc: setup");

    /* copy setup data from packet */
    setup_data[0] = SETUP1;
    setup_data[1] = SETUP2;

    /* pass setup data to the upper layer */
    usb_core_control_request((struct usb_ctrlrequest*)setup_data);
}

static int max_pkt_size(struct endpoint_t *endp)
{
    switch(endp->type)
    {
        case USB_ENDPOINT_XFER_CONTROL: return 64;
        case USB_ENDPOINT_XFER_BULK: return usb_drv_port_speed() ? 512 : 64;
        case USB_ENDPOINT_XFER_INT: return usb_drv_port_speed() ? 1024 : 64;
        default: panicf("die"); return 0;
    }
}

static void ep_write(struct endpoint_t *endp)
{
    int xfer_size = MIN(max_pkt_size(endp), endp->cnt);
    unsigned int timeout = current_tick + HZ/10;

    while(TXBUF(endp) & TXFULL) /* TXFULL flag */
    {
        if(TIME_AFTER(current_tick, timeout))
            break;
    }

    /* setup transfer size and DMA */
    TXSTAT(endp) = xfer_size;
    DMAINLMADDR(endp) = (uint32_t)endp->buf; /* local buffer address */
    DMAINCTL(endp) = DMA_START;
    /* Decrement by max packet size is intentional.
     * This way if we have final packet short one we will get negative len
     * after transfer, which in turn indicates we *don't* need to send
     * zero length packet. If the final packet is max sized packet we will
     * get zero len after transfer which indicates we need to send
     * zero length packet to signal host end of the transfer.
     */
    endp->cnt -= max_pkt_size(endp);
    endp->buf += xfer_size;
    /* clear NAK */
    TXCON(endp) &= ~TXNAK;
}

static void ep_read(struct endpoint_t *endp)
{
    /* setup DMA */
    DMAOUTLMADDR(endp) = (uint32_t)endp->buf; /* local buffer address */
    DMAOUTCTL(endp) = DMA_START;
    /* clear NAK */
    RXCON(endp) &= ~RXNAK;
}

static void in_intr(struct endpoint_t *endp)
{
    uint32_t txstat = TXSTAT(endp);
    /* check if clear feature was sent by host */
    if(txstat & TXCFINT)
    {
        logf("clear_stall: %d", endp->ep_num);
        usb_drv_stall(endp->ep_num, false, true);
    }
    /* check if a transfer has finished */
    if(txstat & TXACK)
    {
        logf("udc: ack(%d)", endp->ep_num);
        /* finished ? */
        if(endp->cnt <= 0)
        {
            usb_core_transfer_complete(endp->ep_num, endp->dir, 0, endp->len);
            /* release semaphore for blocking transfer */
            if(endp->block)
                semaphore_release(&endp->complete);
        }
        else /* more data to send */
            ep_write(endp);
    }
}

static void out_intr(struct endpoint_t *endp)
{
    uint32_t rxstat = RXSTAT(endp);
    logf("udc: out intr(%d)", endp->ep_num);
    /* check if clear feature was sent by host */
    if(rxstat & RXCFINT)
    {
        logf("clear_stall: %d", endp->ep_num);
        usb_drv_stall(endp->ep_num, false, false);
    }
    /* check if a transfer has finished */
    if(rxstat & RXACK)
    {
        int xfer_size = rxstat & 0xffff;
        endp->cnt -= xfer_size;
        endp->buf += xfer_size;
        logf("udc: ack(%d) -> %d/%d", endp->ep_num, xfer_size, endp->cnt);
        /* finished ? */
        if(endp->cnt <= 0 || xfer_size < max_pkt_size(endp))
            usb_core_transfer_complete(endp->ep_num, endp->dir, 0, endp->len);
        else
            ep_read(endp);
    }
}

static void udc_phy_reset(void)
{
    DEV_CTL |= SOFT_POR;
    udelay(10000);                 /* min 10ms */
    DEV_CTL &= ~SOFT_POR;
}

static void udc_soft_connect(void)
{
    DEV_CTL |= CSR_DONE    |
               DEV_SOFT_CN |
               DEV_SELF_PWR;
}

static void udc_helper(void)
{
    uint32_t dev_info = DEV_INFO;

    /* This polls for DEV_EN bit set in DEV_INFO  register
     * as well as tracks current requested configuration
     * (DEV_INFO [11:8]). On state change it notifies usb stack
     * about it.
     */

    /* SET ADDRESS request */
    if(!set_address)
        if(dev_info & 0x7f)
        {
            set_address = true;
            usb_core_notify_set_address(dev_info & 0x7f);
        }

    /* SET CONFIGURATION request */
    if(!set_configuration)
        if(dev_info & DEV_EN)
        {
            set_configuration = true;
            usb_core_notify_set_config(((dev_info >> 7) & 0xf) + 1);
        }
}

/* return port speed FS=0, HS=1 */
int usb_drv_port_speed(void)
{
    return (DEV_INFO & DEV_SPEED) ? 0 : 1;
}

/* Reserve endpoint */
int usb_drv_request_endpoint(int type, int dir)
{
    logf("req: %s %s", XFER_DIR_STR(dir), XFER_TYPE_STR(type));

    /* Find an available ep/dir pair */
    for(int ep_num = 1; ep_num<USB_NUM_ENDPOINTS;ep_num++)
    {
        struct endpoint_t *endp = &endpoints[ep_num];

        if(endp->allocated || endp->type != type || endp->dir != dir)
            continue;
        /* allocate endpoint and enable interrupt */
        endp->allocated = true;
        if(dir == USB_DIR_IN)
            TXCON(endp) = (ep_num << 8) | TXEPEN | TXNAK | TXACKINTEN | TXCFINTE;
        else
            RXCON(endp) = (ep_num << 8) | RXEPEN | RXNAK | RXACKINTEN | RXCFINTE | RXERRINTEN;
        EN_INT |= 1 << (ep_num + 7);

        logf("add: ep%d %s", ep_num, XFER_DIR_STR(dir));
        return ep_num | dir;
    }
    return -1;
}

/* Free endpoint */
void usb_drv_release_endpoint(int ep)
{
    int ep_num = EP_NUM(ep);

    logf("rel: ep%d", ep_num);
    endpoints[ep_num].allocated = false;

    /* disable interrupt from this endpoint */
    EN_INT &= ~(1 << (ep_num + 7));
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
    /* UDC seems to set this automaticaly */
}

static int _usb_drv_send(int endpoint, void *ptr, int length, bool block)
{
    logf("udc: send(%x)", endpoint);
    struct endpoint_t *ep;
    int ep_num = EP_NUM(endpoint);

    if (ep_num == 0)
        ep = &ctrlep[DIR_IN];
    else
        ep = &endpoints[ep_num];

    /* for send transfers, make sure the data is committed */
    commit_discard_dcache_range(ptr, length);
    ep->buf = ptr;
    ep->len = ep->cnt = length;
    ep->block = block;

    ep_write(ep);

    /* wait for transfer to end */
    if(block)
        semaphore_wait(&ep->complete, TIMEOUT_BLOCK);

    return 0;
}

/* Setup a send transfer. (blocking) */
int usb_drv_send(int endpoint, void *ptr, int length)
{
    return _usb_drv_send(endpoint, ptr, length, true);
}

/* Setup a send transfer. (non blocking) */
int usb_drv_send_nonblocking(int endpoint, void *ptr, int length)
{
    return _usb_drv_send(endpoint, ptr, length, false);
}

/* Setup a receive transfer. (non blocking) */
int usb_drv_recv(int endpoint, void* ptr, int length)
{
    logf("udc: recv(%x)", endpoint);
    struct endpoint_t *ep;
    int ep_num = EP_NUM(endpoint);

    if(ep_num == 0)
        ep = &ctrlep[DIR_OUT];
    else
        ep = &endpoints[ep_num];

    /* for recv, discard the cache lines related to the buffer */
    commit_discard_dcache_range(ptr, length);
    ep->buf = ptr;
    ep->len = ep->cnt = length;
    ep_read(ep);
    return 0;
}

/* Kill all transfers. Usually you need to set a bit for each endpoint
 *  and flush fifos. You should also call the completion handler with 
 * error status for everything
 */
void usb_drv_cancel_all_transfers(void)
{
}

/* Set test mode, you can forget that for now, usually it's sufficient
 * to bit copy the argument into some register of the controller
 */
void usb_drv_set_test_mode(int mode)
{
    (void)mode;
}

/* Check if endpoint is in stall state */
bool usb_drv_stalled(int endpoint, bool in)
{
    struct endpoint_t *endp = &endpoints[EP_NUM(endpoint)];

    if(in)
        return !!(TXCON(endp) & TXSTALL);
    else
        return !!(RXCON(endp) & RXSTALL);
}

/* Stall the endpoint. Usually set a flag in the controller */
void usb_drv_stall(int endpoint, bool stall, bool in)
{
    struct endpoint_t *endp = &endpoints[EP_NUM(endpoint)];
    if(in)
    {
        if(stall)
            TXCON(endp) |= TXSTALL;
        else
            TXCON(endp) &= ~TXSTALL;
    }
    else
    {
        if(stall)
            RXCON(endp) |= RXSTALL;
        else
            RXCON(endp) &= ~RXSTALL;
    }
}

/* one time init (once per connection) - basicaly enable usb core */
void usb_drv_init(void)
{
    /* init semaphore of ep0 */
    semaphore_init(&ctrlep[DIR_OUT].complete, 1, 0);
    semaphore_init(&ctrlep[DIR_IN].complete, 1, 0);

    for(int ep_num = 1; ep_num < USB_NUM_ENDPOINTS; ep_num++)
        semaphore_init(&endpoints[ep_num].complete, 1, 0);
}

/* turn off usb core */
void usb_drv_exit(void)
{
    DEV_CTL = DEV_SELF_PWR;

    /* disable USB interrupts in interrupt controller */
    INTC_IMR &= ~IRQ_ARM_UDC;
    INTC_IECR &= ~IRQ_ARM_UDC;

    /* we cannot disable UDC clock since this causes data abort
     * when reading DEV_INFO in order to check usb connect event
     */
}

int usb_detect(void)
{
    if(DEV_INFO & VBUS_STS)
        return USB_INSERTED;
    else
        return USB_EXTRACTED;
}

/* UDC ISR function */
void INT_UDC(void)
{
    /* read what caused UDC irq */
    uint32_t intsrc = INT2FLAG & 0x7fffff;

    if(intsrc & USBRST_INTR) /* usb reset */
    {
        logf("udc_int: reset, %ld", current_tick);

        EN_INT = EN_SUSP_INTR   |  /* Enable Suspend Irq */
                 EN_RESUME_INTR |  /* Enable Resume Irq */
                 EN_USBRST_INTR |  /* Enable USB Reset Irq */
                 EN_OUT0_INTR   |  /* Enable OUT Token receive Irq EP0 */
                 EN_IN0_INTR    |  /* Enable IN Token transmit Irq EP0 */
                 EN_SETUP_INTR;    /* Enable SETUP Packet Receive Irq */

        INTCON = UDC_INTHIGH_ACT | /* interrupt high active */
                 UDC_INTEN;        /* enable EP0 irqs */

        TX0CON = TXACKINTEN |      /* Set as one to enable the EP0 tx irq */
                 TXNAK;            /* Set as one to response NAK handshake */

        RX0CON = RXACKINTEN |
                 RXEPEN     |      /* Endpoint 0 Enable. When cleared the
                                    * endpoint does not respond to an SETUP
                                    * or OUT token */
                 RXNAK;            /* Set as one to response NAK handshake */

        set_address = false;
        set_configuration = false;
    }
    /* This needs to be processed AFTER usb reset */
    udc_helper();

    if(intsrc & SETUP_INTR) /* setup interrupt */
    {
        setup_received();
    }
    if(intsrc & IN0_INTR)
    {
        /* EP0 IN done */
        in_intr(&ctrlep[DIR_IN]);
    }
    if(intsrc & OUT0_INTR)
    {
        /* EP0 OUT done */
        out_intr(&ctrlep[DIR_OUT]);
    }
    if(intsrc & USBRST_INTR)
    {
        /* usb reset */
        usb_drv_init();
    }
    if(intsrc & RESUME_INTR)
    {
        /* usb resume */
        TX0CON |=  TXCLR;  /* TxClr */
        TX0CON &= ~TXCLR;
        RX0CON |=  RXCLR; /* RxClr */
        RX0CON &= ~RXCLR;
    }
    if(intsrc & SUSP_INTR)
    {
        /* usb suspend */
    }
    if(intsrc & CONN_INTR)
    {
        /* usb connect */
        udc_phy_reset();
        udelay(10000);         /* wait at least 10ms */
        udc_soft_connect();
    }
    /* other endpoints */
    for(int ep_num = 1; ep_num < 16; ep_num++)
    {
        if(!(intsrc & (1 << (ep_num + 7))))
            continue;
        struct endpoint_t *endp = &endpoints[ep_num];
        if(endp->dir == USB_DIR_IN)
            in_intr(endp);
        else
            out_intr(endp);
    }
}
