/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Christian Gmeiner
 *
 * Based on code from the Linux Target Image Builder from Freescale
 * available at http://www.bitshrine.org/ and
 * http://www.bitshrine.org/gpp/linux-2.6.16-mx31-usb-2.patch
 * Adapted for Rockbox in January 2007
 * Original file: drivers/usb/gadget/arcotg_udc.c
 *
 * USB Device Controller Driver
 * Driver for ARC OTG USB module in the i.MX31 platform, etc.
 *
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Based on mpc-udc.h
 * Author: Li Yang (leoli@freescale.com)
 *         Jiang Bo (Tanya.jiang@freescale.com)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _ARCOTG_DCD_H_
#define _ARCOTG_DCD_H_

#include "usbstack/core.h"
#include "arcotg_udc.h"

/*-------------------------------------------------------------------------*/

#define ep_is_in(EP)   (((EP)->desc->bEndpointAddress & USB_DIR_IN)==USB_DIR_IN)

#define EP_DIR_IN       1
#define EP_DIR_OUT      0

/*-------------------------------------------------------------------------*/

/* pipe direction macro from device view */
#define USB_RECV    (0) /* OUT EP */
#define USB_SEND    (1) /* IN EP */

/* Shared Bit Masks for Endpoint Queue Head and Endpoint Transfer Descriptor */
#define  TERMINATE                          (1 << 0)
#define  STATUS_ACTIVE                      (1 << 7)
#define  STATUS_HALTED                      (1 << 6)
#define  STATUS_DATA_BUFF_ERR               (1 << 5)
#define  STATUS_TRANSACTION_ERR             (1 << 4)
#define  INTERRUPT_ON_COMPLETE              (1 << 15)
#define  LENGTH_BIT_POS                     (16)
#define  ADDRESS_MASK                       (0xFFFFFFE0)
#define  ERROR_MASK                         (DTD_STATUS_HALTED | \
                                            DTD_STATUS_DATA_BUFF_ERR | \
                                            DTD_STATUS_TRANSACTION_ERR)

#define  RESERVED_FIELDS                    ((1 << 0) | (1 << 2) | (1 << 4) | \
                                            (1 << 8) | (1 << 9) | (1 << 12)| \
                                            (1 << 13)| (1 << 14)| (1 << 31))

/* Endpoint Queue Head Bit Masks */
#define  EP_QUEUE_HEAD_MULT_POS             (30)
#define  EP_QUEUE_HEAD_ZLT_SEL              (0x20000000)
#define  EP_QUEUE_HEAD_MAX_PKT_LEN(ep_info) (((ep_info)>>16)&0x07ff)
#define  EP_QUEUE_HEAD_MULTO                (0x00000C00)
#define  EP_QUEUE_CURRENT_OFFSET_MASK       (0x00000FFF)
#define  EP_QUEUE_FRINDEX_MASK              (0x000007FF)
#define  EP_MAX_LENGTH_TRANSFER             (0x4000)

/*-------------------------------------------------------------------------*/

/* ep name is important, it should obey the convention of ep_match() */
/* even numbered EPs are OUT or setup, odd are IN/INTERRUPT */
static const char* ep_name[] = {
    "ep0-control", NULL, /* everyone has ep0 */
     /* 7 configurable endpoints */
    "ep1out",
    "ep1in",
    "ep2out",
    "ep2in",
    "ep3out",
    "ep3in",
    "ep4out",
    "ep4in",
    "ep5out",
    "ep5in",
    "ep6out",
    "ep6in",
    "ep7out",
    "ep7in"
};

/*-------------------------------------------------------------------------*/

/* Endpoint Transfer Descriptor data struct */
struct dtd {
    uint32_t next_dtd;       /* Next TD pointer(31-5),
                                T(0) set indicate invalid */
    uint32_t dtd_token;      /* Total bytes (30-16), IOC (15),
                                MultO(11-10), STS (7-0)  */
    uint32_t buf_ptr0;       /* Buffer pointer Page 0 */
    uint32_t buf_ptr1;       /* Buffer pointer Page 1 */
    uint32_t buf_ptr2;       /* Buffer pointer Page 2 */
    uint32_t buf_ptr3;       /* Buffer pointer Page 3 */
    uint32_t buf_ptr4;       /* Buffer pointer Page 4 */
    uint32_t res;            /* make it an even 8 words */
} __attribute((packed));

/* Endpoint Queue Head*/
struct dqh {
    uint32_t endpt_cap;          /* Mult(31-30) , Zlt(29) , Max Pkt len
                                  * and IOS(15) */
    uint32_t cur_dtd;            /* Current dTD Pointer(31-5) */
    struct dtd dtd_ovrl;         /* Transfer descriptor */
    uint32_t setup_buffer[2];    /* Setup data 8 bytes */
    uint32_t res2[4];            /* pad out to 64 bytes */
} __attribute((packed));

#define RESPONSE_SIZE 30

/* our controller struct */
struct arcotg_dcd {
    struct usb_ctrlrequest local_setup_buff;
    struct usb_ep endpoints[USB_MAX_PIPES];
    struct usb_response response[RESPONSE_SIZE];
    enum usb_device_state usb_state;
    enum usb_device_state resume_state;
    bool stopped;
};

/*-------------------------------------------------------------------------*/

/* usb_controller functions */
void usb_arcotg_dcd_init(void);
void usb_arcotg_dcd_shutdown(void);
void usb_arcotg_dcd_irq(void);
void usb_arcotg_dcd_start(void);
void usb_arcotg_dcd_stop(void);

/* usb controller ops */
int usb_arcotg_dcd_enable(struct usb_ep* ep,
                          struct usb_endpoint_descriptor* desc);
int usb_arcotg_dcd_disable(struct usb_ep* ep);
int usb_arcotg_dcd_set_halt(struct usb_ep* ep, bool halt);
int usb_arcotg_dcd_send(struct usb_ep* ep, struct usb_response* request);
int usb_arcotg_dcd_receive(struct usb_ep* ep, struct usb_response* res);

/* interrupt handlers */
static void setup_received_int(struct usb_ctrlrequest* request);
static void port_change_int(void);
static void dtd_complete(void);
static void reset_int(void);
static void suspend_int(void);
static void resume_int(void);

/* life cycle */
static void qh_init(unsigned char ep_num, unsigned char dir,
                    unsigned char ep_type, unsigned int max_pkt_len,
                    unsigned int zlt, unsigned char mult);
static void td_init(struct dtd* td, void* buffer, uint32_t todo);
static void ep_setup(unsigned char ep_num, unsigned char dir,
                     unsigned char ep_type);

/* helpers for tx/rx */
static int td_enqueue(struct dtd* td, struct dqh* qh, unsigned int mask);
static int td_wait(struct dtd* td, unsigned int mask);
static int usb_ack(struct usb_ctrlrequest * s, int error);

#endif /*_ARCOTG_DCD_H_*/
