/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2011 by Marcin Bukat
 * Copyright (C) 2012 by Amaury Pouly
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
#include "rk27xx.h"

typedef volatile uint32_t reg32;

#define USB_FULL_SPEED 0
#define USB_HIGH_SPEED 1

/* max allowed packet size definitions */
#define CTL_MAX_SIZE   64

struct endpoint_t {
    const int type;              /* EP type */
    const int dir;               /* DIR_IN/DIR_OUT */
    const unsigned int intr_mask;
    bool allocated;              /* flag to mark EPs taken */
    volatile void *buf;          /* tx/rx buffer address */
    volatile int len;            /* size of the transfer (bytes) */
    volatile int cnt;            /* number of bytes transfered/received  */
};

static struct endpoint_t ctrlep[2] = {
    {USB_ENDPOINT_XFER_CONTROL, DIR_OUT, 0, true, NULL, 0, 0},
    {USB_ENDPOINT_XFER_CONTROL, DIR_IN,  0, true, NULL, 0, 0}
};

volatile bool setup_data_valid = false;
static volatile uint32_t setup_data[2];

static volatile bool usb_drv_send_done = false;
static volatile bool usb_drv_rcv_done = false;

void usb_drv_configure_endpoint(int ep_num, int type)
{
    /* not needed as we use EP0 only */
    (void)ep_num;
    (void)type;
}

int usb_drv_recv_setup(struct usb_ctrlrequest *req)
{
    while (!setup_data_valid)
        ;

    memcpy(req, (void *)setup_data, sizeof(struct usb_ctrlrequest));
    setup_data_valid = false;
    return 0;
}

static void setup_irq_handler(void)
{
    /* copy setup data from packet */
    setup_data[0] = SETUP1;
    setup_data[1] = SETUP2;

    /* ack upper layer we have setup data */
    setup_data_valid = true;
}

/* service ep0 IN transaction */
static void ep0_in_dma_setup(void)
{
    int xfer_size = MIN(ctrlep[DIR_IN].cnt, CTL_MAX_SIZE);

    while (TX0BUF & TXFULL) /* TX0FULL flag */
        ;

    TX0STAT = xfer_size;                           /* size of the transfer */
    TX0DMALM_IADDR = (uint32_t)ctrlep[DIR_IN].buf; /* local buffer address */
    TX0DMAINCTL = DMA_START;                       /* start DMA */

    ctrlep[DIR_IN].cnt -= CTL_MAX_SIZE;
    ctrlep[DIR_IN].buf += xfer_size;

    TX0CON &= ~TXNAK;                              /* clear NAK */
}

static void ep0_out_dma_setup(void)
{
    RX0DMAOUTLMADDR = (uint32_t)ctrlep[DIR_OUT].buf; /* buffer address */
    RX0DMACTLO = DMA_START;                          /* start DMA */

    /* clear NAK bit */
    RX0CON &= ~RXNAK;
}

static void udc_phy_reset(void)
{
    DEV_CTL |= SOFT_POR;
    target_mdelay(10); /* min 10ms */
    DEV_CTL &= ~SOFT_POR;
}

static void udc_soft_connect(void)
{
    DEV_CTL |= CSR_DONE    |
               DEV_SOFT_CN |
               DEV_SELF_PWR;
}

/* return port speed  */
int usb_drv_port_speed(void)
{
    return ((DEV_INFO & DEV_SPEED) ? USB_FULL_SPEED : USB_HIGH_SPEED);
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

int usb_drv_send(int endpoint, void *ptr, int length)
{
    (void)endpoint;
    struct endpoint_t *ep = &ctrlep[DIR_IN];

    ep->buf = ptr;
    ep->len = ep->cnt = length;

    ep0_in_dma_setup();

    /* wait for transfer to end */
    while(!usb_drv_send_done)
        ;

    usb_drv_send_done = false;

    return 0;
}

/* Setup a receive transfer. (blocking) */
int usb_drv_recv(int endpoint, void* ptr, int length)
{
    (void)endpoint;
    struct endpoint_t *ep = &ctrlep[DIR_OUT];

    ep->buf = ptr;
    ep->len = ep->cnt = length;

    if (length)
    {
        usb_drv_rcv_done = false;

        ep0_out_dma_setup();

        /* block here until the transfer is finished */
        while (!usb_drv_rcv_done)
            ;
    }
    else
    {
        /* ZLP, clear NAK bit */
        RX0CON &= ~RXNAK;
    }

    return (length - ep->cnt);
}

/* Stall the endpoint. Usually set a flag in the controller */
void usb_drv_stall(int endpoint, bool stall, bool in)
{
    /* ctrl only anyway */
    (void)endpoint;

    if(in)
    {
        if(stall)
            TX0CON |= TXSTALL;
        else
            TX0CON &= ~TXSTALL;
    }
    else
    {
        if (stall)
            RX0CON |= RXSTALL;
        else
            RX0CON &= ~RXSTALL; /* doc says Auto clear by UDC 2.0 */
    }
}

/* one time init (once per connection) - basicaly enable usb core */
void usb_drv_init(void)
{
        udc_phy_reset();
        target_mdelay(10);      /* wait at least 10ms */
        udc_soft_connect();

        EN_INT = EN_SUSP_INTR   |  /* Enable Suspend Irq */
                 EN_RESUME_INTR |  /* Enable Resume Irq */
                 EN_USBRST_INTR |  /* Enable USB Reset Irq */
                 EN_OUT0_INTR   |  /* Enable OUT Token receive Irq EP0 */
                 EN_IN0_INTR    |  /* Enable IN Token transmit Irq EP0 */
                 EN_SETUP_INTR;    /* Enable SETUP Packet Receive Irq */

        INTCON = UDC_INTHIGH_ACT | /* interrupt high active */
                 UDC_INTEN;        /* enable EP0 irqs */
}

/* turn off usb core */
void usb_drv_exit(void)
{
    /* udc module reset */
    SCU_RSTCFG |= (1<<1);
    target_udelay(10);
    SCU_RSTCFG &= ~(1<<1);
}

/* UDC ISR function */
void INT_UDC(void)
{
    uint32_t txstat, rxstat;

    /* read what caused UDC irq */
    uint32_t intsrc = INT2FLAG & 0x7fffff;

   if (intsrc & USBRST_INTR) /* usb reset */
    {
        EN_INT = EN_SUSP_INTR   |  /* Enable Suspend Irq */
                 EN_RESUME_INTR |  /* Enable Resume Irq */
                 EN_USBRST_INTR |  /* Enable USB Reset Irq */
                 EN_OUT0_INTR   |  /* Enable OUT Token receive Irq EP0 */
                 EN_IN0_INTR    |  /* Enable IN Token transmit Irq EP0 */
                 EN_SETUP_INTR;    /* Enable SETUP Packet Receive Irq */

        TX0CON = TXACKINTEN |      /* Set as one to enable the EP0 tx irq */
                 TXNAK;            /* Set as one to response NAK handshake */

        RX0CON = RXACKINTEN |
                 RXEPEN     |      /* Endpoint 0 Enable. When cleared the
                                    * endpoint does not respond to an SETUP
                                    * or OUT token */
                 RXNAK;            /* Set as one to response NAK handshake */

        usb_drv_rcv_done = true;
    }

    if (intsrc & SETUP_INTR)       /* setup interrupt */
    {
        setup_irq_handler();
    }

    if (intsrc & IN0_INTR)         /* ep0 in interrupt */
    {
        txstat = TX0STAT;          /* read clears flags */

        /* TODO handle errors */
        if (txstat & TXACK)        /* check TxACK flag */
        {
            /* Decrement by max packet size is intentional.
             * This way if we have final packet short one we will get negative len
             * after transfer, which in turn indicates we *don't* need to send
             * zero length packet. If the final packet is max sized packet we will
             * get zero len after transfer which indicates we need to send
             * zero length packet to signal host end of the transfer.
             */
            if (ctrlep[DIR_IN].cnt > 0)
            {
                /* we still have data to send */
                ep0_in_dma_setup();

            }
            else
            {
                if (ctrlep[DIR_IN].cnt == 0)
                {
                    ep0_in_dma_setup();
                }

                /* final ack received */
                usb_drv_send_done = true;
            }
        }
    }

    if (intsrc & OUT0_INTR)        /* ep0 out interrupt */
    {
        rxstat = RX0STAT;

        /* TODO handle errors */
        if (rxstat & RXACK)        /* RxACK */
        {
            int xfer_size = rxstat & 0x7ff;
            ctrlep[DIR_OUT].cnt -= xfer_size;

            if (ctrlep[DIR_OUT].cnt > 0 && xfer_size == 64)
            {
                /* advance the buffer */
                ctrlep[DIR_OUT].buf += xfer_size;
                ep0_out_dma_setup();
            }
            else
                usb_drv_rcv_done = true;
        }
    }

    if (intsrc & RESUME_INTR)      /* usb resume */
    {
        TX0CON |=  TXCLR;          /* TxClr */
        TX0CON &= ~TXCLR;

        RX0CON |=  RXCLR;          /* RxClr */
        RX0CON &= ~RXCLR;
    }
}
