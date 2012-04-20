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

//#include "usb-s3c6400x.h"

#include "usb_ch9.h"
#include "usb_core.h"
#include <inttypes.h>
#include "power.h"

#include "logf.h"

typedef volatile uint32_t reg32;

/* Bulk OUT: ep1, ep4, ep7, ep10, ep13 */
#define BOUT_RXSTAT(ep_num)       (*(reg32*)(AHB0_UDC+0x54+0x38*(ep_num/3)))
#define BOUT_RXCON(ep_num)        (*(reg32*)(AHB0_UDC+0x58+0x38*(ep_num/3)))
#define BOUT_DMAOUTCTL(ep_num)    (*(reg32*)(AHB0_UDC+0x5C+0x38*(ep_num/3)))
#define BOUT_DMAOUTLMADDR(ep_num) (*(reg32*)(AHB0_UDC+0x60+0x38*(ep_num/3)))

/* Bulk IN: ep2, ep5, ep8, ep11, ep4 */
#define BIN_TXSTAT(ep_num)        (*(reg32*)(AHB0_UDC+0x64+0x38*(ep_num/3)))
#define BIN_TXCON(ep_num)         (*(reg32*)(AHB0_UDC+0x68+0x38*(ep_num/3)))
#define BIN_TXBUF(ep_num)         (*(reg32*)(AHB0_UDC+0x6C+0x38*(ep_num/3)))
#define BIN_DMAINCTL(ep_num)      (*(reg32*)(AHB0_UDC+0x70+0x38*(ep_num/3)))
#define BIN_DMAINLMADDR(ep_num)   (*(reg32*)(AHB0_UDC+0x74+0x38*(ep_num/3)))

/* INTERRUPT IN: ep3, ep6, ep9, ep12, ep15 */
#define IIN_TXSTAT(ep_num)        (*(reg32*)(AHB0_UDC+0x78+0x38*((ep_num/3)-1)))
#define IIN_TXCON(ep_num)         (*(reg32*)(AHB0_UDC+0x7C+0x38*((ep_num/3)-1)))
#define IIN_TXBUF(ep_num)         (*(reg32*)(AHB0_UDC+0x80+0x38*((ep_num/3)-1)))
#define IIN_DMAINCTL(ep_num)      (*(reg32*)(AHB0_UDC+0x84+0x38*((ep_num/3)-1)))
#define IIN_DMAINLMADDR(ep_num)   (*(reg32*)(AHB0_UDC+0x88+0x38*((ep_num/3)-1)))

#ifdef LOGF_ENABLE
#define XFER_DIR_STR(dir) ((dir) ? "IN" : "OUT")
#endif

struct endpoint_t {
    const int type;              /* EP type */
    const int dir;               /* DIR_IN/DIR_OUT */
    bool allocated;              /* flag to mark EPs taken */
    volatile void *buf;          /* tx/rx buffer address */
    volatile int len;            /* size of the transfer (bytes) */
    volatile int cnt;            /* number of bytes transfered/received  */
    volatile bool block;         /* flag indicating that transfer is blocking */ 
    struct semaphore complete;   /* semaphore for blocking transfers */
};

static struct endpoint_t ctrlep[2] = {
    {USB_ENDPOINT_XFER_CONTROL, DIR_OUT, true, NULL, 0, 0, true, {0, 0, 0}},
    {USB_ENDPOINT_XFER_CONTROL, DIR_IN, true, NULL, 0, 0, true, {0, 0, 0}}
};

static struct endpoint_t endpoints[16] = {
    {USB_ENDPOINT_XFER_CONTROL,    3, true,  NULL, 0, 0,  true, {0, 0, 0}},  /* stub   */
    {USB_ENDPOINT_XFER_BULK, DIR_OUT, false, NULL, 0, 0, false, {0, 0, 0}},  /* BOUT1  */
    {USB_ENDPOINT_XFER_BULK, DIR_IN,  false, NULL, 0, 0, false, {0, 0, 0}},  /* BIN2   */
    {USB_ENDPOINT_XFER_INT,  DIR_IN,  false, NULL, 0, 0, false, {0, 0, 0}},  /* IIN3   */
    {USB_ENDPOINT_XFER_BULK, DIR_OUT, false, NULL, 0, 0, false, {0, 0, 0}},  /* BOUT4  */
    {USB_ENDPOINT_XFER_BULK, DIR_IN,  false, NULL, 0, 0, false, {0, 0, 0}},  /* BIN5   */
    {USB_ENDPOINT_XFER_INT,  DIR_IN,  false, NULL, 0, 0, false, {0, 0, 0}},  /* IIN6   */
    {USB_ENDPOINT_XFER_BULK, DIR_OUT, false, NULL, 0, 0, false, {0, 0, 0}},  /* BOUT7  */
    {USB_ENDPOINT_XFER_BULK, DIR_IN,  false, NULL, 0, 0, false, {0, 0, 0}},  /* BIN8   */
    {USB_ENDPOINT_XFER_INT,  DIR_IN,  false, NULL, 0, 0, false, {0, 0, 0}},  /* IIN9   */
    {USB_ENDPOINT_XFER_BULK, DIR_OUT, false, NULL, 0, 0, false, {0, 0, 0}},  /* BOUT10 */
    {USB_ENDPOINT_XFER_BULK, DIR_IN,  false, NULL, 0, 0, false, {0, 0, 0}},  /* BIN11  */
    {USB_ENDPOINT_XFER_INT,  DIR_IN,  false, NULL, 0, 0, false, {0, 0, 0}},  /* IIN12  */
    {USB_ENDPOINT_XFER_BULK, DIR_OUT, false, NULL, 0, 0, false, {0, 0, 0}},  /* BOUT13 */
    {USB_ENDPOINT_XFER_BULK, DIR_IN,  false, NULL, 0, 0, false, {0, 0, 0}},  /* BIN14  */
    {USB_ENDPOINT_XFER_INT,  DIR_IN,  false, NULL, 0, 0, false, {0, 0, 0}},  /* IIN15  */
};

static void setup_received(void)
{
    static uint32_t setup_data[2];
    
    /* copy setup data from packet */
    setup_data[0] = SETUP1;
    setup_data[1] = SETUP2;

    /* clear all pending control transfers
     * do we need this here?
     */
    
    /* pass setup data to the upper layer */
    usb_core_control_request((struct usb_ctrlrequest*)setup_data);
}

/* service ep0 IN transaction */
static void ctr_write(void)
{
    int xfer_size = (ctrlep[DIR_IN].cnt > 64) ? 64 : ctrlep[DIR_IN].cnt;
    unsigned int timeout = current_tick + HZ/10;
    
    while (TX0BUF & (1<<0)) /* TX0FULL flag */
    {
        if(TIME_AFTER(current_tick, timeout))
            break;
    }

    TX0STAT = xfer_size;                          /* size of the transfer */
    TX0DMALM_IADDR = (uint32_t)ctrlep[DIR_IN].buf;    /* local buffer address */
    TX0DMAINCTL = (1<<1);                         /* start DMA */
    TX0CON &= ~(1<<2);                            /* clear NAK */
    
    /* Decrement by max packet size is intentional.
     * This way if we have final packet short one we will get negative len
     * after transfer, which in turn indicates we *don't* need to send
     * zero length packet. If the final packet is max sized packet we will
     * get zero len after transfer which indicates we need to send
     * zero length packet to signal host end of the transfer.
     */
    ctrlep[DIR_IN].cnt -= 64;
    ctrlep[DIR_IN].buf += xfer_size;
}

static void ctr_read(void)
{
    int xfer_size = RX0STAT & 0xffff;
    
    /* clear NAK bit */
    RX0CON &= ~(1<<3);
    
    ctrlep[DIR_OUT].cnt -= xfer_size;
    ctrlep[DIR_OUT].buf += xfer_size;
    
    RX0DMAOUTLMADDR = (uint32_t)ctrlep[DIR_OUT].buf;
    RX0DMACTLO = (1<<0);
}

static void blk_write(int ep)
{
    int ep_num = EP_NUM(ep);
    int max = usb_drv_port_speed() ? 512 : 64;
    int xfer_size = (endpoints[ep_num].cnt > max) ? max : endpoints[ep_num].cnt;  
    unsigned int timeout = current_tick + HZ/10;
    
    while (BIN_TXBUF(ep_num) & (1<<0)) /* TXFULL flag */
    {
        if(TIME_AFTER(current_tick, timeout))
            break;
    }
    
    BIN_TXSTAT(ep_num) = xfer_size;           /* size of the transfer */
    BIN_DMAINLMADDR(ep_num) = (uint32_t)endpoints[ep_num].buf; /* buf address */
    BIN_DMAINCTL(ep_num) = (1<<0);            /* start DMA */
    BIN_TXCON(ep_num) &= ~(1<<2);             /* clear NAK */
    
    /* Decrement by max packet size is intentional.
     * This way if we have final packet short one we will get negative len
     * after transfer, which in turn indicates we *don't* need to send
     * zero length packet. If the final packet is max sized packet we will
     * get zero len after transfer which indicates we need to send
     * zero length packet to signal host end of the transfer.
     */
    endpoints[ep_num].cnt -= max;
    endpoints[ep_num].buf += xfer_size;
}

static void blk_read(int ep)
{
    int ep_num = EP_NUM(ep);
    int xfer_size = BOUT_RXSTAT(ep_num) & 0xffff;
    
    /* clear NAK bit */
    BOUT_RXCON(ep_num) &= ~(1<<3);
    
    endpoints[ep_num].cnt -= xfer_size;
    endpoints[ep_num].buf += xfer_size;
    
    BOUT_DMAOUTLMADDR(ep_num) = (uint32_t)endpoints[ep_num].buf;
    BOUT_DMAOUTCTL(ep_num) = (1<<1);
}

static void int_write(int ep)
{
    int ep_num = EP_NUM(ep);
    int max = usb_drv_port_speed() ? 1024 : 64;
    int xfer_size = (endpoints[ep_num].cnt > max) ? max : endpoints[ep_num].cnt;  
    unsigned int timeout = current_tick + HZ/10;
    
    while (IIN_TXBUF(ep_num) & (1<<0)) /* TXFULL flag */
    {
        if(TIME_AFTER(current_tick, timeout))
            break;
    }
    
    IIN_TXSTAT(ep_num) = xfer_size;           /* size of the transfer */
    IIN_DMAINLMADDR(ep_num) = (uint32_t)endpoints[ep_num].buf; /* buf address */
    IIN_DMAINCTL(ep_num) = (1<<0);            /* start DMA */
    IIN_TXCON(ep_num) &= ~(1<<2);             /* clear NAK */
    
    /* Decrement by max packet size is intentional.
     * This way if we have final packet short one we will get negative len
     * after transfer, which in turn indicates we *don't* need to send
     * zero length packet. If the final packet is max sized packet we will
     * get zero len after transfer which indicates we need to send
     * zero length packet to signal host end of the transfer.
     */
    endpoints[ep_num].cnt -= max;
    endpoints[ep_num].buf += xfer_size;
}

/* UDC ISR function */
void INT_UDC(void)
{
    uint32_t txstat, rxstat;
    int tmp, ep_num;
    
    /* read what caused UDC irq */
    uint32_t intsrc = INT2FLAG & 0x7fffff;
    
    if (intsrc & (1<<1)) /* setup interrupt */
    {
        setup_received();
    }
    else if (intsrc & (1<<2)) /* ep0 in interrupt */
    {
        txstat = TX0STAT; /* read clears flags */
        
        /* TODO handle errors */
        if (txstat & (1<<18)) /* check TxACK flag */
        {
            if (ctrlep[DIR_IN].cnt >= 0)
            {
                /* we still have data to send (or ZLP) */
                ctr_write();
            }
            else
            {
                /* final ack received */
                usb_core_transfer_complete(0,                   /* ep */
                                           USB_DIR_IN,          /* dir */
                                           0,                   /* status */
                                           ctrlep[DIR_IN].len); /* length */
                
                /* release semaphore for blocking transfer */
                if (ctrlep[DIR_IN].block)
                    semaphore_release(&ctrlep[DIR_IN].complete);
            }
        }
    }
    else if (intsrc & (1<<3)) /* ep0 out interrupt */
    {
        rxstat = RX0STAT;

        /* TODO handle errors */
        if (rxstat & (1<<18)) /* RxACK */
        {
            if (ctrlep[DIR_OUT].cnt > 0)
                ctr_read();
            else
                usb_core_transfer_complete(0,                    /* ep */
                                           USB_DIR_OUT,          /* dir */
                                           0,                    /* status */
                                           ctrlep[DIR_OUT].len); /* length */                
        }
    }
    else if (intsrc & (1<<4)) /* usb reset */
    {
        usb_drv_init();
    }
    else if (intsrc & (1<<5)) /* usb resume */
    {
        TX0CON |= (1<<0); /* TxClr */
        TX0CON &= ~(1<<0);
        RX0CON |= (1<<1); /* RxClr */
        RX0CON &= (1<<1);
    }
    else if (intsrc & (1<<6)) /* usb suspend */
    {
    }
    else if (intsrc & (1<<7)) /* usb connect */
    {
    }
    else
    {
        /* lets figure out which ep generated irq */
        tmp = intsrc >> 7;
        for (ep_num=1; ep_num < 15; ep_num++)
        {
            tmp >>= ep_num;
            if (tmp & 0x01)
                break;
        }
        
        if (intsrc & ((1<<8)|(1<<11)|(1<<14)|(1<<17)|(1<<20)))
        {
            /* bulk out */
            rxstat = BOUT_RXSTAT(ep_num);
            
            /* TODO handle errors */
            if (rxstat & (1<<18)) /* RxACK */
            {
                if (endpoints[ep_num].cnt > 0)
                    blk_read(ep_num);
                else
                    usb_core_transfer_complete(ep_num,               /* ep */
                                               USB_DIR_OUT,          /* dir */
                                               0,                    /* status */
                                               endpoints[ep_num].len); /* length */                
            }
        }
        else if (intsrc & ((1<<9)|(1<<12)|(1<<15)|(1<<18)|(1<<21)))
        {
            /* bulk in */
            txstat = BIN_TXSTAT(ep_num);
            
            /* TODO handle errors */
            if (txstat & (1<<18)) /* check TxACK flag */
            {
                if (endpoints[ep_num].cnt >= 0)
                {
                    /* we still have data to send (or ZLP) */
                    blk_write(ep_num);
                }
                else
                {
                    /* final ack received */
                    usb_core_transfer_complete(ep_num,                   /* ep */
                                               USB_DIR_IN,          /* dir */
                                               0,                   /* status */
                                               endpoints[ep_num].len); /* length */
                
                    /* release semaphore for blocking transfer */
                    if (endpoints[ep_num].block)
                        semaphore_release(&endpoints[ep_num].complete);
                }
            }
        }
        else if (intsrc & ((1<<10)|(1<13)|(1<<16)|(1<<19)|(1<<22)))
        {
            /* int in */
            txstat = IIN_TXSTAT(ep_num);

            /* TODO handle errors */
            if (txstat & (1<<18)) /* check TxACK flag */
            {
                if (endpoints[ep_num].cnt >= 0)
                {
                    /* we still have data to send (or ZLP) */
                    int_write(ep_num);
                }
                else
                {
                    /* final ack received */
                    usb_core_transfer_complete(ep_num,                   /* ep */
                                               USB_DIR_IN,          /* dir */
                                               0,                   /* status */
                                               endpoints[ep_num].len); /* length */
                
                    /* release semaphore for blocking transfer */
                    if (endpoints[ep_num].block)
                        semaphore_release(&endpoints[ep_num].complete);
                }
            }
        }
    }
}

/* return port speed FS=0, HS=1 */
int usb_drv_port_speed(void)
{
    return ((DEV_INFO & (3<<21)) == 0) ? 0 : 1;
}

/* Reserve endpoint */
int usb_drv_request_endpoint(int type, int dir)
{
    int ep_num, ep_dir;
    int ep_type;

    /* Safety */
    ep_dir = EP_DIR(dir);
    ep_type = type & USB_ENDPOINT_XFERTYPE_MASK;

    logf("req: %s %s", XFER_DIR_STR(ep_dir), XFER_TYPE_STR(ep_type));
    
    /* Find an available ep/dir pair */
    for (ep_num=1;ep_num<USB_NUM_ENDPOINTS;ep_num++)
    {
        struct endpoint_t* endpoint = &endpoints[ep_num];
        
        if (endpoint->type == ep_type &&
            endpoint->dir == ep_dir &&
            !endpoint->allocated)
        {
            /* mark endpoint as taken */
            endpoint->allocated = true;
            
            /* enable interrupt from this endpoint */
            EN_INT |= (1<<(ep_num+7));
            
            logf("add: ep%d %s", ep_num, XFER_DIR_STR(ep_dir));
            return (ep_num | (dir & USB_ENDPOINT_DIR_MASK));
        }
    }
    return -1;
}

/* Free endpoint */
void usb_drv_release_endpoint(int ep)
{
    int ep_num = EP_NUM(ep);
    int ep_dir = EP_DIR(ep);
    (void) ep_dir;

    logf("rel: ep%d %s", ep_num, XFER_DIR_STR(ep_dir));
    endpoints[ep_num].allocated = false;
    
    /* disable interrupt from this endpoint */
    EN_INT &= ~(1<<(ep_num+7));
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
    struct endpoint_t *ep;
    int ep_num = EP_NUM(endpoint);
    
    if (ep_num == 0)
        ep = &ctrlep[DIR_IN];
    else
        ep = &endpoints[ep_num];

    ep->buf = ptr;
    ep->len = ep->cnt = length;
    
    if (block)
        ep->block = true;
    else
        ep->block = false;
    
    switch (ep->type)
    {
        case USB_ENDPOINT_XFER_CONTROL:                     
            ctr_write();
            break;
        
        case USB_ENDPOINT_XFER_BULK:
            blk_write(ep_num);
            break;
        
        case USB_ENDPOINT_XFER_INT:
            int_write(ep_num);
            break;
    }
    
    if (block)
        /* wait for transfer to end */
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
    struct endpoint_t *ep;
    int ep_num = EP_NUM(endpoint);
    
    if (ep_num == 0)
    {
        ep = &ctrlep[DIR_OUT];
        
        ctr_read();
    }
    else
    {
        ep = &endpoints[ep_num];
        
        /* clear NAK bit */
        BOUT_RXCON(ep_num) &= ~(1<<3);
        BOUT_DMAOUTLMADDR(ep_num) = (uint32_t)ptr;
        BOUT_DMAOUTCTL(ep_num) = (1<<1);
    }
       
    ep->buf = ptr;
    ep->len = ep->cnt = length;

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
    int ep_num = EP_NUM(endpoint);

    switch (endpoints[ep_num].type)
    {
        case USB_ENDPOINT_XFER_CONTROL:
            if (in)
                return (TX0CON & (1<<1)) ? true : false;
            else
                return (RX0CON & (1<<2)) ? true : false;

            break;

        case USB_ENDPOINT_XFER_BULK:
            if (in)
                return (BIN_TXCON(ep_num) & (1<<1)) ? true : false;
            else
                return (BOUT_RXCON(ep_num) & (1<<2)) ? true : false;

            break;
            
        case USB_ENDPOINT_XFER_INT:
            if (in)
                return (IIN_TXCON(ep_num) & (1<<1)) ? true : false;
            else
                return false;    /* we don't have such endpoint anyway */
                
            break;
    }

    return false;
}

/* Stall the endpoint. Usually set a flag in the controller */
void usb_drv_stall(int endpoint, bool stall, bool in)
{
    int ep_num = EP_NUM(endpoint);

    switch (endpoints[ep_num].type)
    {
        case USB_ENDPOINT_XFER_CONTROL:
            if (in)
            {
                if (stall)
                    TX0CON |= (1<<1);
                else
                    TX0CON &= ~(1<<1);
            }
            else
            {
                if (stall)
                    RX0CON |= (1<<2);
                else
                    RX0CON &= ~(1<<2); /* doc says Auto clear by UDC 2.0 */
            }
            break;

        case USB_ENDPOINT_XFER_BULK:
            if (in)
            {
                if (stall)
                    BIN_TXCON(ep_num) |= (1<<1);
                else
                    BIN_TXCON(ep_num) &= ~(1<<1);
            }
            else
            {
                if (stall)
                    BOUT_RXCON(ep_num) |= (1<<2);
                else
                    BOUT_RXCON(ep_num) &= ~(1<<2);
            }
            break;
            
        case USB_ENDPOINT_XFER_INT:
            if (in)
            {
                if (stall)
                    IIN_TXCON(ep_num) |= (1<<1);
                else
                    IIN_TXCON(ep_num) &= ~(1<<1);
            }
            break;
    }
}

/* one time init (once per connection) - basicaly enable usb core */
void usb_drv_init(void)
{
    int ep_num;
        
    /* enable USB clock */
    SCU_CLKCFG &= ~(1<<6);
    
    /* 1. do soft disconnect */
    DEV_CTL = (1<<3); /* DEV_SELF_PWR */

    /* 2. do power on reset to PHY */
    DEV_CTL = (1<<3) | /* DEV_SELF_PWR */
              (1<<7);  /* SOFT_POR */
    
    /* 3. wait more than 10ms */
    udelay(20000);
    
    /* 4. clear SOFT_POR bit */
    DEV_CTL  &= ~(1<<7);
    
    /* 5. configure minimal EN_INT */
    EN_INT = (1<<6) |  /* Enable Suspend Interrupt */
             (1<<5) |  /* Enable Resume Interrupt */
             (1<<4) |  /* Enable USB Reset Interrupt */
             (1<<3) |  /* Enable OUT Token receive Interrupt EP0 */
             (1<<2) |  /* Enable IN Token transmits Interrupt EP0 */
             (1<<1);   /* Enable SETUP Packet Receive Interrupt */
             
    /* 6. configure INTCON */
    INTCON = (1<<2) |  /* interrupt high active */
             (1<<0);   /* enable EP0 interrupts */
    
    /* 7. configure EP0 control registers */
    TX0CON = (1<<6) |  /* Set as one to enable the EP0 tx irq */
             (1<<2);   /* Set as one to response NAK handshake */
             
    RX0CON = (1<<7) |
             (1<<4) |  /* Endpoint 0 Enable. When cleared the endpoint does
                        * not respond to an SETUP or OUT token
                        */

             (1<<3);   /* Set as one to response NAK handshake */
             
    /* 8. write final bits to DEV_CTL */
    DEV_CTL = (1<<8) | /* Configure CSR done */
              (1<<6) | /* 16-bit data path enabled. udc_clk = 30MHz */
              (1<<4) | /* Device soft connect */
              (1<<3);  /* Device self power */

    /* init semaphore of ep0 */
    semaphore_init(&ctrlep[DIR_OUT].complete, 1, 0);
    semaphore_init(&ctrlep[DIR_IN].complete, 1, 0);
    
    for (ep_num = 1; ep_num < USB_NUM_ENDPOINTS; ep_num++)
    {
        semaphore_init(&endpoints[ep_num].complete, 1, 0);
        
        if (ep_num%3 == 0) /* IIN 3, 6, 9, 12, 15 */
        {
            IIN_TXCON(ep_num) |= (ep_num<<8)|(1<<3)|(1<<2); /* ep_num, enable, NAK */
        }
        else if (ep_num%3 == 1) /* BOUT 1, 4, 7, 10, 13 */
        {
            BOUT_RXCON(ep_num) |= (ep_num<<8)|(1<<4)|(1<<3); /* ep_num, NAK, enable */
        }
        else if (ep_num%3 == 2) /* BIN 2, 5, 8, 11, 14 */
        {
            BIN_TXCON(ep_num) |= (ep_num<<8)|(1<<3)|(1<<2); /* ep_num, enable, NAK */
        }
    }
}

/* turn off usb core */
void usb_drv_exit(void)
{
    DEV_CTL = (1<<3); /* DEV_SELF_PWR */
    
    /* disable USB interrupts in interrupt controller */
    INTC_IMR &= ~(1<<16);
    INTC_IECR &= ~(1<<16);
    
    /* we cannot disable UDC clock since this causes data abort
     * when reading DEV_INFO in order to check usb connect event
     */
}

int usb_detect(void)
{
    if (DEV_INFO & (1<<20))
        return USB_INSERTED;
    else
        return USB_EXTRACTED;
}

