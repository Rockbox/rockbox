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

#include <string.h>
#include "arcotg_dcd.h"

/*-------------------------------------------------------------------------*/

static struct arcotg_dcd dcd_controller;
struct usb_response res;

/* datastructes to controll transfers */
struct dtd dev_td[USB_MAX_PIPES] IBSS_ATTR;
struct dqh dev_qh[USB_MAX_PIPES] __attribute((aligned (1 << 11))) IBSS_ATTR;

/* shared memory used by rockbox and dcd to exchange data */
#define BUFFER_SIZE 512
unsigned char buffer[BUFFER_SIZE] IBSS_ATTR;

/*-------------------------------------------------------------------------*/

/* description of our device driver operations */
struct usb_dcd_controller_ops arotg_dcd_ops = {
    .enable          = usb_arcotg_dcd_enable,
    .disable         = usb_arcotg_dcd_disable,
    .set_halt        = usb_arcotg_dcd_set_halt,
    .send            = usb_arcotg_dcd_send,
    .receive         = usb_arcotg_dcd_receive,
    .ep0             = &dcd_controller.endpoints[0],
};

/* description of our usb controller driver */
struct usb_controller arcotg_dcd = {
    .name            = "arcotg_dcd",
    .type            = DEVICE,
    .speed           = USB_SPEED_UNKNOWN,
    .init            = usb_arcotg_dcd_init,
    .shutdown        = usb_arcotg_dcd_shutdown,
    .irq             = usb_arcotg_dcd_irq,
    .start           = usb_arcotg_dcd_start,
    .stop            = usb_arcotg_dcd_stop,
    .controller_ops  = (void*)&arotg_dcd_ops,
};

static struct usb_response response;

/*-------------------------------------------------------------------------*/

/* TODO hmmm */

struct timer {
  unsigned long s;
  unsigned long e;
};

void
timer_set(struct timer * timer, unsigned long val)
{
  timer->s = USEC_TIMER;
  timer->e = timer->s + val + 1;
}

int
timer_expired(struct timer * timer)
{
  unsigned long val = USEC_TIMER;

  if (timer->e > timer->s) {
    return !(val >= timer->s && val <= timer->e);
  } else {
    return (val > timer->e && val < timer->s);
  }
}

#define MAX_PACKET_SIZE USB_MAX_CTRL_PAYLOAD

#define ERROR_TIMEOUT  (-3)
#define ERROR_UNKNOWN  (-7)

#define PRIME_TIMER    100000
#define TRANSFER_TIMER 1000000
#define RESET_TIMER    5000000
#define SETUP_TIMER    200000

/*-------------------------------------------------------------------------*/

/* gets called by usb_stack_init() to register 
 * this arcotg device controller driver in the 
 * stack. */
void usb_dcd_init(void)
{
    usb_controller_register(&arcotg_dcd);
}

/*-------------------------------------------------------------------------*/

void usb_arcotg_dcd_init(void)
{
    struct timer t;
    int i, ep_num = 0;

    logf("arcotg_dcd: init");
    memset(&dcd_controller, 0, sizeof(struct arcotg_dcd));

    /* setup list of aviable endpoints */
    INIT_LIST_HEAD(&arcotg_dcd.endpoints.list);

    for (i = 0; i < USB_MAX_PIPES; i++) {

        dcd_controller.endpoints[i].pipe_num = i;

        if (i % 2 == 0) {
            dcd_controller.endpoints[i].ep_num = ep_num;
        } else {
            dcd_controller.endpoints[i].ep_num = ep_num;
            ep_num++;
        }

        logf("pipe %d -> ep %d %s", dcd_controller.endpoints[i].pipe_num,
                                    dcd_controller.endpoints[i].ep_num,
                                    dcd_controller.endpoints[i].name);

        if (ep_name[i] != NULL) {
            memcpy(&dcd_controller.endpoints[i].name, ep_name[i],
            sizeof(dcd_controller.endpoints[i].name));

            if (i != 0) {
                /* add to list of configurable endpoints */
                list_add_tail(&dcd_controller.endpoints[i].list,
                              &arcotg_dcd.endpoints.list);
            }
        }
    }

    /* ep0 is special */
    arcotg_dcd.ep0 = &dcd_controller.endpoints[0];
    arcotg_dcd.ep0->maxpacket = USB_MAX_CTRL_PAYLOAD;

    /* stop */
    UDC_USBCMD &= ~USB_CMD_RUN;

    udelay(50000);
    timer_set(&t, RESET_TIMER);

    /* reset */
    UDC_USBCMD |= USB_CMD_CTRL_RESET;

    while ((UDC_USBCMD & USB_CMD_CTRL_RESET)) {
        if (timer_expired(&t)) {
            logf("TIMEOUT->init");
        }
    }

    /* put controller in device mode */
    UDC_USBMODE |= USB_MODE_CTRL_MODE_DEVICE;

    /* init queue heads */
    qh_init(0, USB_RECV, USB_ENDPOINT_XFER_CONTROL, USB_MAX_CTRL_PAYLOAD, 0, 0);
    qh_init(0, USB_SEND, USB_ENDPOINT_XFER_CONTROL, USB_MAX_CTRL_PAYLOAD, 0, 0);

    UDC_ENDPOINTLISTADDR = (unsigned int)dev_qh;
}

void usb_arcotg_dcd_shutdown(void)
{

}

void usb_arcotg_dcd_start(void)
{
    logf("start");

    /* clear stopped bit */
    dcd_controller.stopped = false;

    UDC_USBCMD |= USB_CMD_RUN;
}

void usb_arcotg_dcd_stop(void)
{
    logf("stop");

    /* set stopped bit */
    dcd_controller.stopped = true;

    UDC_USBCMD &= ~USB_CMD_RUN;
}

void usb_arcotg_dcd_irq(void)
{
    if (dcd_controller.stopped == true) {
        return;
    }

    /* check if we need to wake up from suspend */
    if (!(UDC_USBSTS & USB_STS_SUSPEND) && dcd_controller.resume_state) {
        resume_int();
    }

    /* USB Interrupt */
    if (UDC_USBSTS & USB_STS_INT) {

        /* setup packet, we only support ep0 as control ep */
        if (UDC_ENDPTSETUPSTAT & EP_SETUP_STATUS_EP0) {
            /* copy data from queue head to local buffer */
            memcpy(&dcd_controller.local_setup_buff,
                   (uint8_t *) &dev_qh[0].setup_buffer, 8);

            /* ack setup packet */
            UDC_ENDPTSETUPSTAT = UDC_ENDPTSETUPSTAT;
            setup_received_int(&dcd_controller.local_setup_buff);
        }

        if (UDC_ENDPTCOMPLETE) {
            dtd_complete();
        }
    }

    if (UDC_USBSTS & USB_STS_PORT_CHANGE) {
        port_change_int();
    }

    if (UDC_USBSTS & USB_STS_SUSPEND) {
        suspend_int();
    }

    if (UDC_USBSTS & USB_STS_RESET) {
        reset_int();
    }

    if (UDC_USBSTS & USB_STS_ERR) {
        logf("!!! error !!!");
    }

    if (UDC_USBSTS & USB_STS_SYS_ERR) {
        logf("!!! sys error !!!");
    }
}

/*-------------------------------------------------------------------------*/
/* interrupt handlers */

static void setup_received_int(struct usb_ctrlrequest* request)
{
    int error = 0;
    uint8_t address = 0;
    bool set_config = false;
    int handled = 0;    /* set to zero if we do not handle the message, */
                        /* and should pass it to the driver */

    into_usb_ctrlrequest(request);

    /* handle all requests we support */
    switch (request->bRequestType & USB_TYPE_MASK) {
    case USB_TYPE_STANDARD:

        switch (request->bRequest) {
        case USB_REQ_SET_ADDRESS:
            /* store address as we need to ack before setting it */
            address = (uint8_t)request->wValue;

            handled = 1;
            break;

        case USB_REQ_SET_CONFIGURATION:
            set_config = true;
            break;

        case USB_REQ_GET_STATUS:
            response.buf = &dcd_controller.usb_state;
            response.length = 2;

            handled = usb_arcotg_dcd_send(NULL, &response);
            break;
        }

        case USB_REQ_CLEAR_FEATURE:
        case USB_REQ_SET_FEATURE:
            if (request->bRequestType != USB_RECIP_ENDPOINT) {
                break;
            }

            int dir = (request->wIndex & 0x0080) ? EP_DIR_IN : EP_DIR_OUT;
            int num = (request->wIndex & 0x000f);
            struct usb_ep *ep;

            if (request->wValue != 0 ||
                request->wLength != 0 ||
                (num * 2 + dir) > USB_MAX_PIPES) {
                break;
            }
            ep = &dcd_controller.endpoints[num * 2 + dir];

            if (request->bRequest == USB_REQ_SET_FEATURE) {
                logf("HALT");
                handled = usb_arcotg_dcd_set_halt(ep, true);
            } else {
                logf("UNHALT");
                handled = usb_arcotg_dcd_set_halt(ep, false);
            }

            if (handled == 0) {
                handled = 1; /* dont pass it to driver */
            }

            break;
    }

    /* if dcd can not handle reqeust, ask driver */
    if (handled == 0) {
        if (arcotg_dcd.device_driver != NULL && 
            arcotg_dcd.device_driver->request != NULL) {
            handled = arcotg_dcd.device_driver->request(request);
            logf("result from driver %d", handled);
        }
    }

    if (handled <= 0) {
        error = handled;
    }

    /* ack transfer */
    usb_ack(request, error);

    /* set address and usb state after USB_REQ_SET_ADDRESS */
    if (address != 0) {
        logf("setting address to %d", address);
        UDC_DEVICEADDR = address << 25;
        dcd_controller.usb_state = USB_STATE_ADDRESS;
    }

    /* update usb state after successfull USB_REQ_SET_CONFIGURATION */
    if (set_config) {
        if (handled > 0) {
            dcd_controller.usb_state = USB_STATE_CONFIGURED;
        }
    }
}

static void port_change_int(void)
{
    uint32_t tmp;
    enum usb_device_speed speed = USB_SPEED_UNKNOWN; 

    /* bus resetting is finished */
    if (!(UDC_PORTSC1 & PORTSCX_PORT_RESET)) {
        /* Get the speed */
        tmp = (UDC_PORTSC1 & PORTSCX_PORT_SPEED_MASK);
        switch (tmp) {
        case PORTSCX_PORT_SPEED_HIGH:
            speed = USB_SPEED_HIGH;
            break;
        case PORTSCX_PORT_SPEED_FULL:
            speed = USB_SPEED_FULL;
            break;
        case PORTSCX_PORT_SPEED_LOW:
            speed = USB_SPEED_LOW;
            break;
        default:
            speed = USB_SPEED_UNKNOWN;
            break;
        }
    }

    if (arcotg_dcd.speed == speed) {
        return;
    }

    /* update speed */
    arcotg_dcd.speed = speed;

    /* update USB state */
    if (!dcd_controller.resume_state) {
        dcd_controller.usb_state = USB_STATE_DEFAULT;
    }

    /* inform device driver */
    if (arcotg_dcd.device_driver != NULL &&
        arcotg_dcd.device_driver->speed != NULL) {
        arcotg_dcd.device_driver->speed(speed);
    }
}

static void dtd_complete(void) {

    uint32_t bit_pos;
    int i, ep_num, direction, bit_mask /*, status*/;

    /* clear the bits in the register */
    bit_pos = UDC_ENDPTCOMPLETE;
    UDC_ENDPTCOMPLETE = bit_pos;

    if (!bit_pos) {
        return;
    }

    for (i = 0; i < USB_MAX_ENDPOINTS * 2; i++) {
        ep_num = i >> 1;
        direction = i % 2;

        bit_mask = 1 << (ep_num + 16 * direction);

        if (!(bit_pos & bit_mask)) {
            continue;
        }

        logf("   ");
        logf("TRAFFIC");
        logf("  -> on ep %d dir %d", i, direction);
    }
}

static void suspend_int(void)
{
    dcd_controller.resume_state = dcd_controller.usb_state;
    dcd_controller.usb_state = USB_STATE_SUSPENDED;

    /* report suspend to the driver */
    if (arcotg_dcd.device_driver != NULL &&
        arcotg_dcd.device_driver->suspend != NULL) {
        arcotg_dcd.device_driver->suspend();
    }
}

static void resume_int(void)
{
    dcd_controller.usb_state = dcd_controller.resume_state;
    dcd_controller.resume_state = USB_STATE_NOTATTACHED;

    /* report resume to the driver */
    if (arcotg_dcd.device_driver != NULL &&
        arcotg_dcd.device_driver->resume != NULL) {
        arcotg_dcd.device_driver->resume();
    }
}

static void reset_int(void)
{
    /* clear device address */
    UDC_DEVICEADDR = 0 << 25;

    /* update usb state */
    dcd_controller.usb_state = USB_STATE_DEFAULT;

    UDC_ENDPTSETUPSTAT = UDC_ENDPTSETUPSTAT;
    UDC_ENDPTCOMPLETE  = UDC_ENDPTCOMPLETE;

    /* prime and flush pending transfers */
    while (UDC_ENDPTPRIME);
    UDC_ENDPTFLUSH = ~0;

    if ((UDC_PORTSC1 & PORTSCX_PORT_RESET) == 0) {
        logf("TIMEOUT->port");
    }

    /* clear USB Reset status bit */
    UDC_USBSTS |= USB_STS_RESET;
}

/*-------------------------------------------------------------------------*/
/* usb controller ops */

int usb_arcotg_dcd_enable(struct usb_ep* ep,
                          struct usb_endpoint_descriptor* desc)
{
    unsigned short max = 0;
    unsigned char mult = 0, zlt = 0;
    int retval = 0;
    char *val = NULL;    /* for debug */

    /* catch bogus parameter */
    if (!ep) {
        return -EINVAL;
    }

    max = desc->wMaxPacketSize;
    retval = -EINVAL;

    /* check the max package size validate for this endpoint
     * Refer to USB2.0 spec table 9-13, */
    switch (desc->bmAttributes & 0x03) {
    case USB_ENDPOINT_XFER_BULK:
        if (strstr(ep->name, "-iso") || strstr(ep->name, "-int")) {
            goto en_done;
        }
        mult = 0;
        zlt = 1;

        switch (arcotg_dcd.speed) {
        case USB_SPEED_HIGH:
            if ((max == 128) || (max == 256) || (max == 512)) {
                break;
            }
        default:
            switch (max) {
            case 4:
            case 8:
            case 16:
            case 32:
            case 64:
                break;
            default:
            case USB_SPEED_LOW:
                goto en_done;
            }
        }
        break;

    case USB_ENDPOINT_XFER_INT:
        if (strstr(ep->name, "-iso")) { /* bulk is ok */
            goto en_done;
        }
        mult = 0;
        zlt = 1;

        switch (arcotg_dcd.speed) {
        case USB_SPEED_HIGH:
            if (max <= 1024) {
                break;
            }
        case USB_SPEED_FULL:
            if (max <= 64) {
                break;
            }
        default:
            if (max <= 8) {
                break;
            }
            goto en_done;
        }
        break;

    case USB_ENDPOINT_XFER_ISOC:
        if (strstr(ep->name, "-bulk") || strstr(ep->name, "-int")) {
            goto en_done;
        }
        mult = (unsigned char) (1 +((desc->wMaxPacketSize >> 11) & 0x03));
        zlt = 0;

        switch (arcotg_dcd.speed) {
        case USB_SPEED_HIGH:
            if (max <= 1024) {
                break;
            }
        case USB_SPEED_FULL:
            if (max <= 1023) {
                break;
            }
        default:
            goto en_done;
        }
        break;

        case USB_ENDPOINT_XFER_CONTROL:
        if (strstr(ep->name, "-iso")	|| strstr(ep->name, "-int")) {
            goto en_done;
        }
        mult = 0;
        zlt = 1;

        switch (arcotg_dcd.speed) {
        case USB_SPEED_HIGH:
        case USB_SPEED_FULL:
            switch (max) {
            case 1:
            case 2:
            case 4:
            case 8:
            case 16:
            case 32:
            case 64:
                break;
            default:
                goto en_done;
            }
        case USB_SPEED_LOW:
            switch (max) {
            case 1:
            case 2:
            case 4:
            case 8:
                break;
            default:
                goto en_done;
            }
        default:
            goto en_done;
        }
        break;

    default:
        goto en_done;
    }


    /* here initialize variable of ep */
    ep->maxpacket = max;
    ep->desc = desc;

    /* hardware special operation */

    /* Init EPx Queue Head (Ep Capabilites field in QH
     * according to max, zlt, mult) */
    qh_init(ep->ep_num,
            (desc->bEndpointAddress & USB_DIR_IN) ? USB_RECV : USB_SEND, 
            (unsigned char) (desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK),
            max, zlt, mult);

    /* Init endpoint x at here */
    ep_setup(ep->ep_num,
            (unsigned char)(desc->bEndpointAddress & USB_DIR_IN) ?
                                                            USB_RECV : USB_SEND,
            (unsigned char)(desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK));

    /* Now HW will be NAKing transfers to that EP,
     * until a buffer is queued to it. */

    retval = 0;
    switch (desc->bmAttributes & 0x03) {
    case USB_ENDPOINT_XFER_BULK:
        val = "bulk";
        break;
    case USB_ENDPOINT_XFER_ISOC:
        val = "iso";
        break;
    case USB_ENDPOINT_XFER_INT:
        val = "intr";
        break;
    default:
        val = "ctrl";
        break;
    }

    logf("ep num %d", (int)ep->ep_num);

    logf("enabled %s (ep%d%s-%s)", ep->name,
         desc->bEndpointAddress & 0x0f,
        (desc->bEndpointAddress & USB_DIR_IN) ? "in" : "out", val);
    logf(" maxpacket %d", max);

en_done:
    return retval;
}

int usb_arcotg_dcd_disable(struct usb_ep* ep)
{
    if (ep == NULL || ep->desc == NULL) {
        logf("failed to disabled %s",  ep ? ep->name : NULL);
        return -EINVAL;
    }

    ep->desc = NULL;

    logf("disabled %s", ep->name);
    return 0;
}

int usb_arcotg_dcd_set_halt(struct usb_ep* ep, bool halt)
{
    int status = -EOPNOTSUPP; /* operation not supported */
    unsigned char dir = 0;
    unsigned int tmp_epctrl = 0;

    if (!ep) {
        status = -EINVAL;
        goto out;
    }

    if (ep->desc->bmAttributes == USB_ENDPOINT_XFER_ISOC) {
        status = -EOPNOTSUPP;
        goto out;
    }

    dir = ep_is_in(ep) ? USB_RECV : USB_SEND;

    logf("modify halt of %d", ep->ep_num);
    tmp_epctrl = UDC_ENDPTCTRL(ep->ep_num);
    logf("reg %x", tmp_epctrl);

    if (halt) {
        logf("halting...");
        /* set the stall bit */
        if (dir) {
            logf("..tx..");
            tmp_epctrl |= EPCTRL_TX_EP_STALL;
        } else {
            logf("..rx..");
            tmp_epctrl |= EPCTRL_RX_EP_STALL;
        }
    } else {
        logf("UNhalting...");
        /* clear the stall bit and reset data toggle */
        if (dir) {
            logf("..tx..");
            tmp_epctrl &= ~EPCTRL_TX_EP_STALL;
            tmp_epctrl |= EPCTRL_TX_DATA_TOGGLE_RST;
        } else {
            logf("..rx..");
            tmp_epctrl &= ~EPCTRL_RX_EP_STALL;
            tmp_epctrl |= EPCTRL_RX_DATA_TOGGLE_RST;
        }
    }
    UDC_ENDPTCTRL(ep->ep_num) = tmp_epctrl;
    logf("reg %x", tmp_epctrl);

out:
    logf("%s %s halt", ep->name, halt ? "set" : "clear");
    return 0;
}

int usb_arcotg_dcd_send(struct usb_ep* ep, struct usb_response* res)
{
    char* ptr;
    int todo, error, size, done = 0;
    int index = 1; /* use as default ep0 tx qh and td */
    struct dtd* td;
    struct dqh* qh;
    unsigned int mask;

    if (res == NULL) {
        logf("invalid input");
        return -EINVAL;
    }

    if (ep != NULL) {
        index = ep->pipe_num;
    }

    ptr = res->buf;
    size = res->length;

    td = &dev_td[index];
    qh = &dev_qh[index];
    mask = 1 << (15 + index);

    do {
        /* calculate how much to copy and send */
        todo = MIN(size, BUFFER_SIZE);

        /* copy data to shared memory area */
        memcpy(buffer, ptr, todo);

        /* init transfer descriptor */
        td_init(td, buffer, todo);

        /* start transfer*/
        error = td_enqueue(td, qh, mask);

        if (error == 0) {
            /* waiting for finished transfer */
            error = td_wait(td, mask);
        }

        if (error) {
            done = error;
            break;
        }

        size -= todo;
        ptr  += todo;
        done += todo;

    } while (size > 0);

    logf("usb_send done %d",done);
    return done;
}

int usb_arcotg_dcd_receive(struct usb_ep* ep, struct usb_response* res)
{
    char* ptr;
    int todo, error, size, done = 0;
    int index = 0; /* use as default ep0 rx qh and td */
    struct dtd* td;
    struct dqh* qh;
    unsigned int mask;

    if (res == NULL) {
        logf("invalid input");
        return -EINVAL;
    }

    if (ep != NULL) {
        index = ep->pipe_num;
    }

    ptr = res->buf;
    size = res->length;

    td = &dev_td[index];
    qh = &dev_qh[index];
    mask = 1 << index;

    do {
        /* calculate how much to receive in one step */
        todo = MIN(size, BUFFER_SIZE);

        /* init transfer descritpor */
        td_init(td, buffer, size);

        /* start transfer */
        error = td_enqueue(td, qh, mask);

        if (error == 0) {
            /* wait until transfer is finished */
            error = td_wait(td, mask);
        }

        if (error) {
            done = error;
            break;
        }

        /* copy receive data to buffer */
        memcpy(ptr, buffer, todo);

        size -= todo;
        ptr  += todo;
        done += todo;

    } while (size > 0);

    logf("usb_recive done %d",done);
    return done;
}

/*-------------------------------------------------------------------------*/
/* lifecylce */

static void qh_init(unsigned char ep_num, unsigned char dir,
                    unsigned char ep_type, unsigned int max_pkt_len,
                    unsigned int zlt, unsigned char mult)
{
    struct dqh *qh = &dev_qh[2 * ep_num + dir];
    uint32_t tmp = 0;
    memset(qh, 0, sizeof(struct dqh));

    /* set the Endpoint Capabilites Reg of QH */
    switch (ep_type) {
    case USB_ENDPOINT_XFER_CONTROL:
        /* Interrupt On Setup (IOS). for control ep  */
        tmp = (max_pkt_len << LENGTH_BIT_POS) | INTERRUPT_ON_COMPLETE;
        break;
    case USB_ENDPOINT_XFER_ISOC:
        tmp = (max_pkt_len << LENGTH_BIT_POS) |
              (mult << EP_QUEUE_HEAD_MULT_POS);
        break;
    case USB_ENDPOINT_XFER_BULK:
    case USB_ENDPOINT_XFER_INT:
        tmp = max_pkt_len << LENGTH_BIT_POS;
        if (zlt) {
            tmp |= EP_QUEUE_HEAD_ZLT_SEL;
        }
        break;
    default:
        logf("error ep type is %d", ep_type);
            return;
    }

    /* see 32.14.4.1 Queue Head Initialization */

    /* write the wMaxPacketSize field as required by the USB Chapter9 or
       application specific portocol */
    qh->endpt_cap = tmp;

    /* write the next dTD Terminate bit fild to 1 */
    qh->dtd_ovrl.next_dtd = 1;

    /* write the Active bit in the status field to 0 */
    qh->dtd_ovrl.dtd_token &= ~STATUS_ACTIVE;

    /* write the Hald bit in the status field to 0 */
    qh->dtd_ovrl.dtd_token &= ~STATUS_HALTED;

    logf("qh: init %d", (2 * ep_num + dir));
}

static void td_init(struct dtd* td, void* buffer, uint32_t todo)
{
    /* see 32.14.5.2 Building a Transfer Descriptor */

    /* init first 7 dwords with 0 */
    memset(td, 0, sizeof(struct dtd)); /* set set all to 0 */

    /* set terminate bit to 1*/
    td->next_dtd = 1;

    /* fill in total bytes with transfer size */
    td->dtd_token = (todo << 16);

    /* set interrupt on compilte if desierd */
    td->dtd_token |= INTERRUPT_ON_COMPLETE;

    /* initialize the status field with the active bit set to 1 and all
       remaining status bits to 0 */
    td->dtd_token |= STATUS_ACTIVE;

    td->buf_ptr0 = (uint32_t)buffer;
}

static void ep_setup(unsigned char ep_num, unsigned char dir,
                     unsigned char ep_type)
{
    unsigned int tmp_epctrl = 0;
    struct timer t;

    tmp_epctrl = UDC_ENDPTCTRL(ep_num);
    if (dir) {
        if (ep_num) {
            tmp_epctrl |= EPCTRL_TX_DATA_TOGGLE_RST;
        }
        logf("tx enablde");
        tmp_epctrl |= EPCTRL_TX_ENABLE;
        tmp_epctrl |= ((unsigned int)(ep_type) << EPCTRL_TX_EP_TYPE_SHIFT);
    } else {
        if (ep_num) {
            tmp_epctrl |= EPCTRL_RX_DATA_TOGGLE_RST;
        }
        logf("rx enablde");
        tmp_epctrl |= EPCTRL_RX_ENABLE;
        tmp_epctrl |= ((unsigned int)(ep_type) << EPCTRL_RX_EP_TYPE_SHIFT);
    }

    UDC_ENDPTCTRL(ep_num) = tmp_epctrl;

    /* wait for the write reg to finish */

    timer_set(&t, SETUP_TIMER);
    while (!(UDC_ENDPTCTRL(ep_num) &
             (tmp_epctrl & (EPCTRL_TX_ENABLE | EPCTRL_RX_ENABLE)))) {
        if (timer_expired(&t)) {
            logf("TIMEOUT: enable ep");
            return;
        }
    }
}

/*-------------------------------------------------------------------------*/
/* helpers for sending/receiving */

static int td_enqueue(struct dtd* td, struct dqh* qh, unsigned int mask)
{
    struct timer t;

    qh->dtd_ovrl.next_dtd   = (unsigned int)td;
    qh->dtd_ovrl.dtd_token &= ~0xc0;

    timer_set(&t, PRIME_TIMER);
    UDC_ENDPTPRIME |= mask;

    while ((UDC_ENDPTPRIME & mask)) {
        if (timer_expired(&t)) {
            logf("timeout->prime");
        }
    }

    if ((UDC_ENDPTSTAT & mask) == 0) {
        logf("Endptstat 0x%x", UDC_ENDPTSTAT);
        logf("HW_ERROR");
    }

    return 0;
}

static int td_wait(struct dtd* td, unsigned int mask)
{
    struct timer t;
    timer_set(&t, TRANSFER_TIMER);

    for (;;) {
        if ((UDC_ENDPTCOMPLETE & mask) != 0) {
            UDC_ENDPTCOMPLETE |= mask;
        }

        if ((td->dtd_token & (1 << 7)) == 0) {
            return 0;
        }

        if (timer_expired(&t)) {
            return ERROR_TIMEOUT;
        }
    }
}

static int usb_ack(struct usb_ctrlrequest * s, int error)
{
    if (error) {
        logf("STALLing ep0");
        UDC_ENDPTCTRL0 |= 1 << 16; /* stall */
        return 0;
    }

    res.buf = NULL;
    res.length = 0;

    if (s->bRequestType & 0x80) {
        return usb_arcotg_dcd_receive(NULL, &res);
    } else {
        return usb_arcotg_dcd_send(NULL, &res);
    }
}
