/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for ARC USBOTG Device Controller
 *
 * Copyright (C) 2007 by Björn Stenberg
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

#define MAX_PKT_SIZE        1024
#define MAX_PKT_SIZE_EP0    64

/* USB device mode registers (Little Endian) */
#define REG_USBCMD           (*(volatile unsigned int *)(USB_BASE+0x140))
#define REG_DEVICEADDR       (*(volatile unsigned int *)(USB_BASE+0x154))
#define REG_ENDPOINTLISTADDR (*(volatile unsigned int *)(USB_BASE+0x158))
#define REG_PORTSC1          (*(volatile unsigned int *)(USB_BASE+0x184))
#define REG_USBMODE          (*(volatile unsigned int *)(USB_BASE+0x1a8))
#define REG_ENDPTSETUPSTAT   (*(volatile unsigned int *)(USB_BASE+0x1ac))
#define REG_ENDPTPRIME       (*(volatile unsigned int *)(USB_BASE+0x1b0))
#define REG_ENDPTSTATUS      (*(volatile unsigned int *)(USB_BASE+0x1b8))
#define REG_ENDPTCOMPLETE    (*(volatile unsigned int *)(USB_BASE+0x1bc))
#define REG_ENDPTCTRL0       (*(volatile unsigned int *)(USB_BASE+0x1c0))
#define REG_ENDPTCTRL1       (*(volatile unsigned int *)(USB_BASE+0x1c4))
#define REG_ENDPTCTRL2       (*(volatile unsigned int *)(USB_BASE+0x1c8))
#define REG_ENDPTCTRL(_x_)   (*(volatile unsigned int *)(USB_BASE+0x1c0+4*(_x_)))

/* USB CMD  Register Bit Masks */
#define USBCMD_RUN                            (0x00000001)
#define USBCMD_CTRL_RESET                     (0x00000002)
#define USBCMD_PERIODIC_SCHEDULE_EN           (0x00000010)
#define USBCMD_ASYNC_SCHEDULE_EN              (0x00000020)
#define USBCMD_INT_AA_DOORBELL                (0x00000040)
#define USBCMD_ASP                            (0x00000300)
#define USBCMD_ASYNC_SCH_PARK_EN              (0x00000800)
#define USBCMD_SUTW                           (0x00002000)
#define USBCMD_ATDTW                          (0x00004000)
#define USBCMD_ITC                            (0x00FF0000)

/* Device Address bit masks */
#define USBDEVICEADDRESS_MASK                 (0xFE000000)
#define USBDEVICEADDRESS_BIT_POS              (25)

/* Endpoint Setup Status bit masks */
#define EPSETUP_STATUS_EP0                    (0x00000001)

/* PORTSCX  Register Bit Masks */
#define PORTSCX_CURRENT_CONNECT_STATUS         (0x00000001)
#define PORTSCX_CONNECT_STATUS_CHANGE          (0x00000002)
#define PORTSCX_PORT_ENABLE                    (0x00000004)
#define PORTSCX_PORT_EN_DIS_CHANGE             (0x00000008)
#define PORTSCX_OVER_CURRENT_ACT               (0x00000010)
#define PORTSCX_OVER_CURRENT_CHG               (0x00000020)
#define PORTSCX_PORT_FORCE_RESUME              (0x00000040)
#define PORTSCX_PORT_SUSPEND                   (0x00000080)
#define PORTSCX_PORT_RESET                     (0x00000100)
#define PORTSCX_LINE_STATUS_BITS               (0x00000C00)
#define PORTSCX_PORT_POWER                     (0x00001000)
#define PORTSCX_PORT_INDICTOR_CTRL             (0x0000C000)
#define PORTSCX_PORT_TEST_CTRL                 (0x000F0000)
#define PORTSCX_WAKE_ON_CONNECT_EN             (0x00100000)
#define PORTSCX_WAKE_ON_CONNECT_DIS            (0x00200000)
#define PORTSCX_WAKE_ON_OVER_CURRENT           (0x00400000)
#define PORTSCX_PHY_LOW_POWER_SPD              (0x00800000)
#define PORTSCX_PORT_FORCE_FULL_SPEED          (0x01000000)
#define PORTSCX_PORT_SPEED_MASK                (0x0C000000)
#define PORTSCX_PORT_WIDTH                     (0x10000000)
#define PORTSCX_PHY_TYPE_SEL                   (0xC0000000)

/* bit 11-10 are line status */
#define PORTSCX_LINE_STATUS_SE0                (0x00000000)
#define PORTSCX_LINE_STATUS_JSTATE             (0x00000400)
#define PORTSCX_LINE_STATUS_KSTATE             (0x00000800)
#define PORTSCX_LINE_STATUS_UNDEF              (0x00000C00)
#define PORTSCX_LINE_STATUS_BIT_POS            (10)

/* bit 15-14 are port indicator control */
#define PORTSCX_PIC_OFF                        (0x00000000)
#define PORTSCX_PIC_AMBER                      (0x00004000)
#define PORTSCX_PIC_GREEN                      (0x00008000)
#define PORTSCX_PIC_UNDEF                      (0x0000C000)
#define PORTSCX_PIC_BIT_POS                    (14)

/* bit 19-16 are port test control */
#define PORTSCX_PTC_DISABLE                    (0x00000000)
#define PORTSCX_PTC_JSTATE                     (0x00010000)
#define PORTSCX_PTC_KSTATE                     (0x00020000)
#define PORTSCX_PTC_SE0NAK                     (0x00030000)
#define PORTSCX_PTC_PACKET                     (0x00040000)
#define PORTSCX_PTC_FORCE_EN                   (0x00050000)
#define PORTSCX_PTC_BIT_POS                    (16)

/* bit 27-26 are port speed */
#define PORTSCX_PORT_SPEED_FULL                (0x00000000)
#define PORTSCX_PORT_SPEED_LOW                 (0x04000000)
#define PORTSCX_PORT_SPEED_HIGH                (0x08000000)
#define PORTSCX_PORT_SPEED_UNDEF               (0x0C000000)
#define PORTSCX_SPEED_BIT_POS                  (26)

/* bit 28 is parallel transceiver width for UTMI interface */
#define PORTSCX_PTW                            (0x10000000)
#define PORTSCX_PTW_8BIT                       (0x00000000)
#define PORTSCX_PTW_16BIT                      (0x10000000)

/* bit 31-30 are port transceiver select */
#define PORTSCX_PTS_UTMI                       (0x00000000)
#define PORTSCX_PTS_CLASSIC                    (0x40000000)
#define PORTSCX_PTS_ULPI                       (0x80000000)
#define PORTSCX_PTS_FSLS                       (0xC0000000)
#define PORTSCX_PTS_BIT_POS                    (30)

/* USB MODE Register Bit Masks */
#define USBMODE_CTRL_MODE_IDLE                (0x00000000)
#define USBMODE_CTRL_MODE_DEVICE              (0x00000002)
#define USBMODE_CTRL_MODE_HOST                (0x00000003)
#define USBMODE_CTRL_MODE_RSV                 (0x00000001)
#define USBMODE_SETUP_LOCK_OFF                (0x00000008)
#define USBMODE_STREAM_DISABLE                (0x00000010)

/* ENDPOINTCTRLx  Register Bit Masks */
#define EPCTRL_TX_ENABLE                       (0x00800000)
#define EPCTRL_TX_DATA_TOGGLE_RST              (0x00400000)    /* Not EP0 */
#define EPCTRL_TX_DATA_TOGGLE_INH              (0x00200000)    /* Not EP0 */
#define EPCTRL_TX_TYPE                         (0x000C0000)
#define EPCTRL_TX_DATA_SOURCE                  (0x00020000)    /* Not EP0 */
#define EPCTRL_TX_EP_STALL                     (0x00010000)
#define EPCTRL_RX_ENABLE                       (0x00000080)
#define EPCTRL_RX_DATA_TOGGLE_RST              (0x00000040)    /* Not EP0 */
#define EPCTRL_RX_DATA_TOGGLE_INH              (0x00000020)    /* Not EP0 */
#define EPCTRL_RX_TYPE                         (0x0000000C)
#define EPCTRL_RX_DATA_SINK                    (0x00000002)    /* Not EP0 */
#define EPCTRL_RX_EP_STALL                     (0x00000001)

/* bit 19-18 and 3-2 are endpoint type */
#define EPCTRL_TX_EP_TYPE_SHIFT                (18)
#define EPCTRL_RX_EP_TYPE_SHIFT                (2)

#define QH_MULT_POS                            (30)
#define QH_ZLT_SEL                             (0x20000000)
#define QH_MAX_PKT_LEN_POS                     (16)
#define QH_IOS                                 (0x00008000)
#define QH_NEXT_TERMINATE                      (0x00000001)
#define QH_IOC                                 (0x00008000)
#define QH_MULTO                               (0x00000C00)
#define QH_STATUS_HALT                         (0x00000040)
#define QH_STATUS_ACTIVE                       (0x00000080)
#define EP_QUEUE_CURRENT_OFFSET_MASK         (0x00000FFF)
#define EP_QUEUE_HEAD_NEXT_POINTER_MASK      (0xFFFFFFE0)
#define EP_QUEUE_FRINDEX_MASK                (0x000007FF)
#define EP_MAX_LENGTH_TRANSFER               (0x4000)

#define DTD_NEXT_TERMINATE                   (0x00000001)
#define DTD_IOC                              (0x00008000)
#define DTD_STATUS_ACTIVE                    (0x00000080)
#define DTD_STATUS_HALTED                    (0x00000040)
#define DTD_STATUS_DATA_BUFF_ERR             (0x00000020)
#define DTD_STATUS_TRANSACTION_ERR           (0x00000008)
#define DTD_RESERVED_FIELDS                  (0x80007300)
#define DTD_ADDR_MASK                        (0xFFFFFFE0)
#define DTD_PACKET_SIZE                      (0x7FFF0000)
#define DTD_LENGTH_BIT_POS                   (16)
#define DTD_ERROR_MASK                       (DTD_STATUS_HALTED | \
                                               DTD_STATUS_DATA_BUFF_ERR | \
                                               DTD_STATUS_TRANSACTION_ERR)
/*-------------------------------------------------------------------------*/
/* manual: 32.13.2 Endpoint Transfer Descriptor (dTD) */
struct transfer_descriptor {
    unsigned int next_td_ptr;           /* Next TD pointer(31-5), T(0) set
                                           indicate invalid */
    unsigned int size_ioc_sts;          /* Total bytes (30-16), IOC (15),
                                           MultO(11-10), STS (7-0)  */
    unsigned int buff_ptr0;             /* Buffer pointer Page 0 */
    unsigned int buff_ptr1;             /* Buffer pointer Page 1 */
    unsigned int buff_ptr2;             /* Buffer pointer Page 2 */
    unsigned int buff_ptr3;             /* Buffer pointer Page 3 */
    unsigned int buff_ptr4;             /* Buffer pointer Page 4 */
    unsigned int reserved;
} __attribute__ ((packed));

static struct transfer_descriptor td_array[USB_NUM_ENDPOINTS*2]
    __attribute__((aligned(32)));

/* manual: 32.13.1 Endpoint Queue Head (dQH) */
struct queue_head {
    unsigned int max_pkt_length;    /* Mult(31-30) , Zlt(29) , Max Pkt len
                                       and IOS(15) */
    unsigned int curr_dtd_ptr;      /* Current dTD Pointer(31-5) */
    struct transfer_descriptor dtd; /* dTD overlay */
    unsigned int setup_buffer[2];   /* Setup data 8 bytes */
    unsigned int reserved;          /* for software use, pointer to the first TD */
    unsigned int status;            /* for software use, status of chain in progress */
    unsigned int length;            /* for software use, transfered bytes of chain in progress */
    unsigned int wait;              /* for softwate use, indicates if the transfer is blocking */
} __attribute__((packed));

static struct queue_head qh_array[USB_NUM_ENDPOINTS*2] __attribute__((aligned(2048)));

static const unsigned int pipe2mask[] = {
    0x01, 0x010000,
    0x02, 0x020000,
    0x04, 0x040000,
    0x08, 0x080000,
    0x10, 0x100000,
};

/* return transfered size if wait=true */
static int prime_transfer(int ep_num, void *ptr, int len, bool send, bool wait)
{
    int pipe = ep_num * 2 + (send ? 1 : 0);
    unsigned mask = pipe2mask[pipe];
    struct transfer_descriptor *td = &td_array[pipe];
    struct queue_head* qh = &qh_array[pipe];

    /* prepare TD */
    td->next_td_ptr = DTD_NEXT_TERMINATE;
    td->size_ioc_sts = (len<< DTD_LENGTH_BIT_POS) | DTD_STATUS_ACTIVE;
    td->buff_ptr0 = (unsigned int)ptr;
    td->buff_ptr1 = ((unsigned int)ptr & 0xfffff000) + 0x1000;
    td->buff_ptr2 = ((unsigned int)ptr & 0xfffff000) + 0x2000;
    td->buff_ptr3 = ((unsigned int)ptr & 0xfffff000) + 0x3000;
    td->buff_ptr4 = ((unsigned int)ptr & 0xfffff000) + 0x4000;
    td->reserved = 0;
    /* prime */
    qh->dtd.next_td_ptr = (unsigned int)td;
    qh->dtd.size_ioc_sts &= ~(QH_STATUS_HALT | QH_STATUS_ACTIVE);
    REG_ENDPTPRIME |= mask;
    /* wait for priming to be taken into account */
    while(!(REG_ENDPTSTATUS & mask));
    /* wait for completion */
    if(wait)
    {
        while(!(REG_ENDPTCOMPLETE & mask));
        REG_ENDPTCOMPLETE = mask;
        /* memory barrier */
        asm volatile("":::"memory");
        /* return transfered size */
        return len - (td->size_ioc_sts >> DTD_LENGTH_BIT_POS);
    }
    else
        return 0;
}

void usb_drv_set_address(int address)
{
    REG_DEVICEADDR = address << USBDEVICEADDRESS_BIT_POS;
}

/* endpoints */

int usb_drv_send_nonblocking(int endpoint, void* ptr, int length)
{
    return prime_transfer(EP_NUM(endpoint), ptr, length, true, false);
}

int usb_drv_send(int endpoint, void* ptr, int length)
{
    return prime_transfer(EP_NUM(endpoint), ptr, length, true, true);
}

int usb_drv_recv(int endpoint, void* ptr, int length)
{
    return prime_transfer(EP_NUM(endpoint), ptr, length, false, true);
}

int usb_drv_recv_nonblocking(int endpoint, void* ptr, int length)
{
    return prime_transfer(EP_NUM(endpoint), ptr, length, false, false);
}

int usb_drv_port_speed(void)
{
    return (REG_PORTSC1 & 0x08000000) ? 1 : 0;
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
    int ep_num = EP_NUM(endpoint);

    if(in)
    {
        if(stall)
            REG_ENDPTCTRL(ep_num) |= EPCTRL_TX_EP_STALL;
        else
            REG_ENDPTCTRL(ep_num) &= ~EPCTRL_TX_EP_STALL;
    }
    else
    {
        if (stall)
            REG_ENDPTCTRL(ep_num) |= EPCTRL_RX_EP_STALL;
        else
            REG_ENDPTCTRL(ep_num) &= ~EPCTRL_RX_EP_STALL;
    }
}

void usb_drv_configure_endpoint(int ep_num, int type)
{
    REG_ENDPTCTRL(ep_num) =
        EPCTRL_RX_DATA_TOGGLE_RST | EPCTRL_RX_ENABLE |
        EPCTRL_TX_DATA_TOGGLE_RST | EPCTRL_TX_ENABLE |
            (type << EPCTRL_RX_EP_TYPE_SHIFT) |
            (type << EPCTRL_TX_EP_TYPE_SHIFT);
}

void usb_drv_init(void)
{
    /* we don't know if USB was connected or not. In USB recovery mode it will
     * but in other cases it might not be. In doubt, disconnect */
    REG_USBCMD &= ~USBCMD_RUN;
    /* wait a short time for the host to realise */
    target_mdelay(50);
    /* reset the controller */
    REG_USBCMD |= USBCMD_CTRL_RESET;
    while(REG_USBCMD & USBCMD_CTRL_RESET);
    /* put it in device mode */
    REG_USBMODE = USBMODE_CTRL_MODE_DEVICE;
    /* reset address */
    REG_DEVICEADDR = 0;
    /* prepare qh array */
    qh_array[0].max_pkt_length = 1 << 29 | MAX_PKT_SIZE_EP0 << 16;
    qh_array[1].max_pkt_length = 1 << 29 | MAX_PKT_SIZE_EP0 << 16;
    qh_array[2].max_pkt_length = 1 << 29 | MAX_PKT_SIZE << 16;
    qh_array[3].max_pkt_length = 1 << 29 | MAX_PKT_SIZE << 16;
    /* setup qh */
    REG_ENDPOINTLISTADDR = (unsigned int)qh_array;
    /* clear setup status */
    REG_ENDPTSETUPSTAT = EPSETUP_STATUS_EP0;
    /* run! */
    REG_USBCMD |= USBCMD_RUN;
}

void usb_drv_exit(void)
{
    REG_USBCMD &= ~USBCMD_RUN;
    REG_USBCMD |= USBCMD_CTRL_RESET;
}

int usb_drv_recv_setup(struct usb_ctrlrequest *req)
{
    /* wait for setup */
    while(!(REG_ENDPTSETUPSTAT & EPSETUP_STATUS_EP0))
        ;
    /* clear setup status */
    REG_ENDPTSETUPSTAT = EPSETUP_STATUS_EP0;
    /* check request */
    asm volatile("":::"memory");
    /* copy */
    memcpy(req, (void *)&qh_array[0].setup_buffer[0], sizeof(struct usb_ctrlrequest));
    return 0;
}
