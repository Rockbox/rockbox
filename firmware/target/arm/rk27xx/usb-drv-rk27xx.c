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

/*#define LOGF_ENABLE*/
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

#define USB_FULL_SPEED 0
#define USB_HIGH_SPEED 1

/* max allowed packet size definitions */
#define CTL_MAX_SIZE    64
#define BLK_HS_MAX_SIZE 512
#define BLK_FS_MAX_SIZE 64
#define INT_HS_MAX_SIZE 1024
#define INT_FS_MAX_SIZE 64

#ifdef LOGF_ENABLE
#define XFER_DIR_STR(dir) ((dir) ? "IN" : "OUT")
#define XFER_TYPE_STR(type) \
    ((type) == USB_ENDPOINT_XFER_CONTROL ? "CTRL" : \
     ((type) == USB_ENDPOINT_XFER_ISOC ? "ISOC" : \
      ((type) == USB_ENDPOINT_XFER_BULK ? "BULK" : \
       ((type) == USB_ENDPOINT_XFER_INT ? "INTR" : "INVL"))))
#endif

struct endpoint_t {
    const int type;              /* EP type */
    const int dir;               /* DIR_IN/DIR_OUT */
    const unsigned int intr_mask;
    bool allocated;              /* flag to mark EPs taken */
    volatile void *buf;          /* tx/rx buffer address */
    volatile int len;            /* size of the transfer (bytes) */
    volatile int cnt;            /* number of bytes transfered/received  */
    volatile bool block;         /* flag indicating that transfer is blocking */ 
    struct semaphore complete;   /* semaphore for blocking transfers */
};

static struct endpoint_t ctrlep[2] = {
    {USB_ENDPOINT_XFER_CONTROL, DIR_OUT, 0, true, NULL, 0, 0, true, {0, 0, 0}},
    {USB_ENDPOINT_XFER_CONTROL, DIR_IN,  0, true, NULL, 0, 0, true, {0, 0, 0}}
};

static struct endpoint_t endpoints[16] = {
    {USB_ENDPOINT_XFER_CONTROL,    3,          0,   true, NULL, 0, 0,  true, {0, 0, 0}},  /* stub   */
    {USB_ENDPOINT_XFER_BULK, DIR_OUT, BOUT1_INTR,  false, NULL, 0, 0, false, {0, 0, 0}},  /* BOUT1  */
    {USB_ENDPOINT_XFER_BULK, DIR_IN,  BIN2_INTR,   false, NULL, 0, 0, false, {0, 0, 0}},  /* BIN2   */
    {USB_ENDPOINT_XFER_INT,  DIR_IN,  IIN3_INTR,   false, NULL, 0, 0, false, {0, 0, 0}},  /* IIN3   */
    {USB_ENDPOINT_XFER_BULK, DIR_OUT, BOUT4_INTR,  false, NULL, 0, 0, false, {0, 0, 0}},  /* BOUT4  */
    {USB_ENDPOINT_XFER_BULK, DIR_IN,  BIN5_INTR,   false, NULL, 0, 0, false, {0, 0, 0}},  /* BIN5   */
    {USB_ENDPOINT_XFER_INT,  DIR_IN,  IIN6_INTR,   false, NULL, 0, 0, false, {0, 0, 0}},  /* IIN6   */
    {USB_ENDPOINT_XFER_BULK, DIR_OUT, BOUT7_INTR,  false, NULL, 0, 0, false, {0, 0, 0}},  /* BOUT7  */
    {USB_ENDPOINT_XFER_BULK, DIR_IN,  BIN8_INTR,   false, NULL, 0, 0, false, {0, 0, 0}},  /* BIN8   */
    {USB_ENDPOINT_XFER_INT,  DIR_IN,  IIN9_INTR,   false, NULL, 0, 0, false, {0, 0, 0}},  /* IIN9   */
    {USB_ENDPOINT_XFER_BULK, DIR_OUT, BOUT10_INTR, false, NULL, 0, 0, false, {0, 0, 0}},  /* BOUT10 */
    {USB_ENDPOINT_XFER_BULK, DIR_IN,  BIN11_INTR,  false, NULL, 0, 0, false, {0, 0, 0}},  /* BIN11  */
    {USB_ENDPOINT_XFER_INT,  DIR_IN,  IIN12_INTR,  false, NULL, 0, 0, false, {0, 0, 0}},  /* IIN12  */
    {USB_ENDPOINT_XFER_BULK, DIR_OUT, BOUT13_INTR, false, NULL, 0, 0, false, {0, 0, 0}},  /* BOUT13 */
    {USB_ENDPOINT_XFER_BULK, DIR_IN,  BIN14_INTR,  false, NULL, 0, 0, false, {0, 0, 0}},  /* BIN14  */
    {USB_ENDPOINT_XFER_INT,  DIR_IN,  IIN15_INTR,  false, NULL, 0, 0, false, {0, 0, 0}},  /* IIN15  */
};

static volatile bool set_address = false;
static volatile bool set_configuration = false;

static void setup_irq_handler(void)
{
    static uint32_t setup_data[2];
    
    /* copy setup data from packet */
    setup_data[0] = SETUP1;
    setup_data[1] = SETUP2;

    /* pass setup data to the upper layer */
    usb_core_control_request((struct usb_ctrlrequest*)setup_data);
}

/* service ep0 IN transaction */
static void ctr_write(void)
{
    int xfer_size = MIN(ctrlep[DIR_IN].cnt, CTL_MAX_SIZE);
    unsigned int timeout = current_tick + HZ/10;
    
    while (TX0BUF & TXFULL) /* TX0FULL flag */
    {
        if (TIME_AFTER(current_tick, timeout))
            break;
    }

    TX0STAT = xfer_size;                           /* size of the transfer */
    TX0DMALM_IADDR = (uint32_t)ctrlep[DIR_IN].buf; /* local buffer address */
    TX0DMAINCTL = DMA_START;                       /* start DMA */
    TX0CON &= ~TXNAK;                              /* clear NAK */
    
    /* Decrement by max packet size is intentional.
     * This way if we have final packet short one we will get negative len
     * after transfer, which in turn indicates we *don't* need to send
     * zero length packet. If the final packet is max sized packet we will
     * get zero len after transfer which indicates we need to send
     * zero length packet to signal host end of the transfer.
     */
    ctrlep[DIR_IN].cnt -= CTL_MAX_SIZE;
    ctrlep[DIR_IN].buf += xfer_size;
}

static void ctr_read(void)
{
    int xfer_size = RX0STAT & 0xffff;
    
    /* clear NAK bit */
    RX0CON &= ~RXNAK;
    
    ctrlep[DIR_OUT].cnt -= xfer_size;
    ctrlep[DIR_OUT].buf += xfer_size;
    
    RX0DMAOUTLMADDR = (uint32_t)ctrlep[DIR_OUT].buf; /* buffer address */
    RX0DMACTLO = DMA_START;                          /* start DMA */
}

static void blk_write(int ep_num)
{
    unsigned int timeout = current_tick + HZ/10;
    int max = usb_drv_port_speed() ? BLK_HS_MAX_SIZE : BLK_FS_MAX_SIZE;
    int xfer_size = MIN(endpoints[ep_num].cnt, max);

    while (BIN_TXBUF(ep_num) & TXFULL) /* TXFULL flag */
    {
        if (TIME_AFTER(current_tick, timeout))
            break;
    }

    BIN_TXSTAT(ep_num) = xfer_size;                            /* size */
    BIN_DMAINLMADDR(ep_num) = (uint32_t)endpoints[ep_num].buf; /* buf address */
    BIN_DMAINCTL(ep_num) = DMA_START;                          /* start DMA */
    BIN_TXCON(ep_num) &= ~TXNAK;                               /* clear NAK */

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

static void blk_in_irq_handler(int ep_num)
{
    uint32_t txstat = BIN_TXSTAT(ep_num);
    struct endpoint_t *endp = &endpoints[ep_num];

    if (txstat & TXERR)
    {
        panicf("error condition ep%d, %ld", ep_num, current_tick);
    }

    if (txstat & TXCFINT)
    {
        logf("blk_write: cf(0x%x), %ld", ep_num, current_tick);
        /* bit cleared by read */
        usb_drv_stall(ep_num, false, true);
    }

    if (txstat & TXACK) /* check TxACK flag */
    {
        if (endp->cnt > 0)
        {
            logf("blk_write: ack(0x%x), %ld", ep_num, current_tick);
            /* we still have data to send (or ZLP) */
            blk_write(ep_num);
        }
        else
        {
            logf("udc_intr: usb_core_transfer_complete(0x%x, USB_DIR_IN, 0, 0x%x), %ld",
                 ep_num, endp->len, current_tick);

            /* final ack received */
            usb_core_transfer_complete(ep_num,     /* ep */
                                       USB_DIR_IN, /* dir */
                                       0,          /* status */
                                       endp->len); /* length */

            /* release semaphore for blocking transfer */
            if (endp->block)
            {
                logf("udc_intr: ep=0x%x, semaphore_release(), %ld",
                     ep_num, current_tick);

                semaphore_release(&endp->complete);
            }
        }
    }
}

static void blk_out_irq_handler(int ep_num)
{
    uint32_t rxstat = BOUT_RXSTAT(ep_num);
    int xfer_size = rxstat & 0xffff;
    int max = usb_drv_port_speed() ? BLK_HS_MAX_SIZE : BLK_FS_MAX_SIZE;
    struct endpoint_t *endp = &endpoints[ep_num];

    if (rxstat & RXERR)
    {
        panicf("error condition ep%d, %ld", ep_num, current_tick);
    }

    if (rxstat & RXOVF)
    {
        panicf("rxovf ep%d, %ld", ep_num, current_tick);
    }

    if (rxstat & RXCFINT)
    {
        logf("blk_read:cf(0x%x), %ld", ep_num, current_tick);
        usb_drv_stall(ep_num, false, false);
    }

    if ((rxstat & RXACK) && endp->cnt > 0)
    {
        logf("blk_read:ack(0x%x,0x%x), %ld", ep_num, xfer_size, current_tick);

        /* clear NAK bit */
        BOUT_RXCON(ep_num) &= ~RXNAK;

        endp->cnt -= xfer_size;
        endp->buf += xfer_size;

        /* if the transfer was short or if the total count is zero,
         * transfer is complete
         */
        if (endp->cnt == 0 || xfer_size < max)
        {
            logf("udc_intr: usb_core_transfer_complete(0x%x, USB_DIR_OUT, 0, 0x%x), %ld",
                 ep_num, endp->len, current_tick);

            usb_core_transfer_complete(ep_num,                 /* ep */
                                       USB_DIR_OUT,            /* dir */
                                       0,                      /* status */
                                       endp->len - endp->cnt); /* length */
            endp->cnt = 0;
        }
        else
        {
            BOUT_DMAOUTLMADDR(ep_num) = (uint32_t)endp->buf;
            BOUT_DMAOUTCTL(ep_num) = DMA_START;
        }
    }
}

static void int_write(int ep)
{
    int ep_num = EP_NUM(ep);
    int max = usb_drv_port_speed() ? INT_HS_MAX_SIZE : INT_FS_MAX_SIZE;
    int xfer_size = MIN(endpoints[ep_num].cnt, max);
    unsigned int timeout = current_tick + HZ/10;
    
    while (IIN_TXBUF(ep_num) & TXFULL)
    {
        if (TIME_AFTER(current_tick, timeout))
            break;
    }
    
    IIN_TXSTAT(ep_num) = xfer_size;                            /* size */
    IIN_DMAINLMADDR(ep_num) = (uint32_t)endpoints[ep_num].buf; /* buf address */
    IIN_DMAINCTL(ep_num) = DMA_START;                          /* start DMA */
    IIN_TXCON(ep_num) &= ~TXNAK;                               /* clear NAK */
    
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

static void int_in_irq_handler(int ep_num)
{
    uint32_t txstat = IIN_TXSTAT(ep_num);
    struct endpoint_t *endp = &endpoints[ep_num];

    if (txstat & TXCFINT)
    {
        logf("int_write: cf(0x%x), %ld", ep_num, current_tick);
        /* bit cleared by read */
        usb_drv_stall(ep_num, false, true);
    }

    if (txstat & TXERR)
    {
        panicf("error condition ep%d, %ld", ep_num, current_tick);
    }

    if (txstat & TXACK) /* check TxACK flag */
    {
        if (endp->cnt > 0)
        {
            logf("int_write: ack(0x%x), %ld", ep_num, current_tick);
            /* we still have data to send (or ZLP) */
            int_write(ep_num);
        }
        else
        {
            logf("udc_intr: usb_core_transfer_complete(0x%x, USB_DIR_IN, 0, 0x%x), %ld",
                 ep_num, endp->len, current_tick);

            /* final ack received */
            usb_core_transfer_complete(ep_num,     /* ep */
                                       USB_DIR_IN, /* dir */
                                       0,          /* status */
                                       endp->len); /* length */

            /* release semaphore for blocking transfer */
            if (endp->block)
            {
                logf("udc_intr: ep=0x%x, semaphore_release(), %ld",
                     ep_num, current_tick);

                semaphore_release(&endp->complete);
            }
        }
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
    if (set_address == false)
        if ((dev_info & 0x7f))
        {
            set_address = true;
            usb_core_notify_set_address(dev_info & 0x7f);
        }

    /* SET CONFIGURATION request */
    if (set_configuration == false)
        if (dev_info & DEV_EN)
        {
            set_configuration = true;
            usb_core_notify_set_config(((dev_info >> 7) & 0xf) + 1);
        }
}

/* return port speed  */
int usb_drv_port_speed(void)
{
    return ((DEV_INFO & DEV_SPEED) ? USB_FULL_SPEED : USB_HIGH_SPEED);
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
    for (ep_num = 1; ep_num < USB_NUM_ENDPOINTS; ep_num++)
    {
        struct endpoint_t* endpoint = &endpoints[ep_num];
        
        if (endpoint->type == ep_type &&
            endpoint->dir == ep_dir &&
            !endpoint->allocated)
        {
            /* mark endpoint as taken */
            endpoint->allocated = true;

            if (ep_num%3 == 0) /* IIN 3, 6, 9, 12, 15 */
            {
                IIN_TXCON(ep_num) = (ep_num<<8) | /* set ep number */
                                     TXERRINTEN |
                                         TXEPEN | /* endpoint enable */
                                          TXNAK | /* respond with NAK */
                                     TXACKINTEN | /* irq on ACK */
                                        TXCFINTE; /* irq on Clear feature */
            }
            else if (ep_num%3 == 1) /* BOUT 1, 4, 7, 10, 13 */
            {
                BOUT_RXCON(ep_num) = (ep_num<<8) | /* set ep number */
                                      RXERRINTEN |
                                          RXEPEN | /* endpoint enable */
                                           RXNAK | /* respond with NAK */
                                      RXACKINTEN | /* irq on ACK */
                                         RXCFINTE; /* irq on Clear feature */
            }
            else if (ep_num%3 == 2) /* BIN 2, 5, 8, 11, 14 */
            {
                BIN_TXCON(ep_num) = (ep_num<<8) | /* set ep number */
                                     TXERRINTEN |
                                         TXEPEN | /* endpoint enable */
                                          TXNAK | /* respond with NAK */
                                     TXACKINTEN | /* irq on ACK */
                                        TXCFINTE; /* irq on Clear feature */
            }

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
    logf("rel: ep%d %s", ep_num, XFER_DIR_STR(EP_DIR(ep)));
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
    /* UDC sets this automaticaly */
}

static int _usb_drv_send(int endpoint, void *ptr, int length, bool block)
{
    struct endpoint_t *ep;
    int ep_num = EP_NUM(endpoint);

    /* for send transfers, make sure the data is committed */
    commit_discard_dcache_range(ptr, length);

    logf("_usb_drv_send: endpt=0x%x, len=0x%x, block=%d",
         endpoint, length, block);

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
    
    logf("usb_drv_recv: endpt=0x%x, len=0x%x, %ld",
         endpoint, length, current_tick);

    /* for recv, discard the cache lines related to the buffer */
    commit_discard_dcache_range(ptr, length);

    if (ep_num == 0)
    {
        ep = &ctrlep[DIR_OUT];
        ep->buf = ptr;
        ep->len = ep->cnt = length;

        /* clear NAK bit */
        RX0CON &= ~RXNAK;
        RX0DMAOUTLMADDR = (uint32_t)ptr; /* buffer address */
        RX0DMACTLO = DMA_START;          /* start DMA */
    }
    else
    {
        ep = &endpoints[ep_num];
        ep->buf = ptr;
        ep->len = ep->cnt = length;

        /* clear NAK bit */
        BOUT_RXCON(ep_num) &= ~RXNAK;
        BOUT_DMAOUTLMADDR(ep_num) = (uint32_t)ptr;
        BOUT_DMAOUTCTL(ep_num) = DMA_START;
    }

    return 0;
}

/* Kill all transfers. Usually you need to set a bit for each endpoint
 * and flush fifos. You should also call the completion handler with
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
                return (TX0CON & TXSTALL) ? true : false;
            else
                return (RX0CON & RXSTALL) ? true : false;

            break;

        case USB_ENDPOINT_XFER_BULK:
            if (in)
                return (BIN_TXCON(ep_num) & TXSTALL) ? true : false;
            else
                return (BOUT_RXCON(ep_num) & RXSTALL) ? true : false;

            break;
            
        case USB_ENDPOINT_XFER_INT:
            if (in)
                return (IIN_TXCON(ep_num) & TXSTALL) ? true : false;
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
    logf("usb_drv: %sstall EP%d %s",
         stall ? "": "un", ep_num, in ? "IN" : "OUT");

    switch (endpoints[ep_num].type)
    {
        case USB_ENDPOINT_XFER_CONTROL:
            if (in)
            {
                if (stall)
                    TX0CON |=  TXSTALL;
                else
                    TX0CON &= ~TXSTALL;
            }
            else
            {
                if (stall)
                    RX0CON |=  RXSTALL;
                else
                    RX0CON &= ~RXSTALL; /* doc says Auto clear by UDC 2.0 */
            }
            break;

        case USB_ENDPOINT_XFER_BULK:
            if (in)
            {
                if (stall)
                    BIN_TXCON(ep_num) |=  TXSTALL;
                else
                    BIN_TXCON(ep_num) &= ~TXSTALL;
            }
            else
            {
                if (stall)
                    BOUT_RXCON(ep_num) |=  RXSTALL;
                else
                    BOUT_RXCON(ep_num) &= ~RXSTALL;
            }
            break;
            
        case USB_ENDPOINT_XFER_INT:
            if (in)
            {
                if (stall)
                    IIN_TXCON(ep_num) |=  TXSTALL;
                else
                    IIN_TXCON(ep_num) &= ~TXSTALL;
            }
            break;
    }
}

/* one time init (once per connection) - basicaly enable usb core */
void usb_drv_init(void)
{
    int ep_num;

    /* init semaphore of ep0 */
    semaphore_init(&ctrlep[DIR_OUT].complete, 1, 0);
    semaphore_init(&ctrlep[DIR_IN].complete, 1, 0);

    /* init semaphores for other endpoints */
    for (ep_num = 1; ep_num < USB_NUM_ENDPOINTS; ep_num++)
        semaphore_init(&endpoints[ep_num].complete, 1, 0);
}

/* turn off usb core */
void usb_drv_exit(void)
{
    /* udc module reset */
    SCU_RSTCFG |= (1<<1);
    udelay(10);
    SCU_RSTCFG &= ~(1<<1);
}

int usb_detect(void)
{
    if (DEV_INFO & VBUS_STS)
        return USB_INSERTED;
    else
        return USB_EXTRACTED;
}

/* UDC ISR function */
void INT_UDC(void)
{
    uint32_t txstat, rxstat;
    int ep_num;

    /* read what caused UDC irq */
    uint32_t intsrc = INT2FLAG & 0x7fffff;

   if (intsrc & USBRST_INTR) /* usb reset */
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

    if (intsrc & OUT0_INTR)        /* ep0 out interrupt */
    {
        rxstat = RX0STAT;

        /* TODO handle errors */
        if (rxstat & RXACK)        /* RxACK */
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

    if (intsrc & RESUME_INTR)      /* usb resume */
    {
        TX0CON |=  TXCLR;          /* TxClr */
        TX0CON &= ~TXCLR;

        RX0CON |=  RXCLR;          /* RxClr */
        RX0CON &= ~RXCLR;
    }

    if (intsrc & SUSP_INTR)        /* usb suspend */
    {
    }

    if (intsrc & CONN_INTR)        /* usb connect */
    {
        udc_phy_reset();
        udelay(10000);         /* wait at least 10ms */
        udc_soft_connect();
    }

    /* endpoints other then EP0 */
    if (intsrc & 0x7fff00)
    {
        /* lets figure out which ep generated irq */
        for (ep_num = 1; ep_num < USB_NUM_ENDPOINTS; ep_num++)
        {
            struct endpoint_t *ep = &endpoints[ep_num];

            if ((intsrc & ep->intr_mask) && ep->allocated)
            {
                switch (ep->type)
                {
                    case USB_ENDPOINT_XFER_BULK:
                        if (ep->dir == DIR_OUT)
                            blk_out_irq_handler(ep_num);
                        else
                            blk_in_irq_handler(ep_num);

                        break;

                    case USB_ENDPOINT_XFER_INT:
                        int_in_irq_handler(ep_num);

                        break;
                }
            }
        }
    }
}
