/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2008 by Vitja Makarov
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

#include "usb-tcc.h"

#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "panic.h"

#ifdef HAVE_USBSTACK
#include "usb_ch9.h"
#include "usb_core.h"
#include "usb_drv.h"

#define TCC7xx_USB_EPIF_IRQ_MASK 0xf

static int dbg_level = 0x00;
static int global_ep_irq_mask = 0x1;
#define DEBUG(level, fmt, args...) do { if (dbg_level & (level)) printf(fmt, ## args); } while (0)

#include "power.h"

#ifndef BOOTLOADER
#define printf(...) do {} while (0)
#define panicf_my panicf
#else
int printf(const char *fmt, ...);
#define panicf_my(fmt, args...) {               \
    int flags = disable_irq_save();             \
    printf("*** PANIC ***");                    \
    printf(fmt, ## args);                       \
    printf("*** PANIC ***");                    \
    while (usb_detect() == USB_INSERTED)        \
        ;                                       \
    power_off();                                \
    while(1);                                   \
    restore_irq(flags);                         \
}
#endif

struct tcc_ep {
    unsigned char dir;      /* endpoint direction */
    volatile TCC7xx_USB_FIFO_TYPE *ep; /* hw ep buffer */
    int id;       /* Endpoint id */
    int mask;     /* Endpoint bit mask */
    char *buf;    /* user buffer to store data */
    int max_len;  /* how match data will fit */
    int count;    /* actual data count */
} ;

struct usb_drv_ep_spec usb_drv_ep_specs[USB_NUM_ENDPOINTS] = {
    {.type = {USB_ENDPOINT_XFER_CONTROL, USB_ENDPOINT_XFER_CONTROL}},
    {.type = {USB_ENDPOINT_TYPE_NONE, USB_ENDPOINT_XFER_BULK}},
    {.type = {USB_ENDPOINT_XFER_BULK, USB_ENDPOINT_TYPE_NONE}},
};
uint8_t usb_drv_ep_specs_flags = 0;

static struct tcc_ep tcc_endpoints[] = {
    /* control */
    {
        .dir = -1,
        .ep = &TCC7xx_USB_EP0_BUF,
    }, {  /* bulk */
        .dir = -1,
        .ep = &TCC7xx_USB_EP1_BUF,
    }, { /* bulk */
        .dir = -1,
        .ep = &TCC7xx_USB_EP2_BUF,
    }, { /* interrupt */
        .dir = -1,
        .ep = &TCC7xx_USB_EP3_BUF,
    },
} ;

static bool usb_drv_write_ep(struct tcc_ep *ep);
static void usb_set_speed(int);

static bool address_status;
static bool ep0_expect_status_out;
static bool ep0_tx_pending;
static bool ep0_tx_notify;
static bool ep0_tx_zlp;
static unsigned short ep0_request_length;
static int ep0_tx_result;
static struct semaphore ep0_tx_complete;
static bool handle_set_address(struct usb_ctrlrequest *req);
static void ep0_tx_ack(void);

static void ep0_cancel_tx(void)
{
    if (ep0_tx_pending) {
        ep0_tx_pending = false;
        ep0_tx_result = -1;
        semaphore_release(&ep0_tx_complete);
    }
    ep0_tx_zlp = false;
    tcc_endpoints[0].buf = NULL;
}

#ifdef CPU_TCC780X
static struct usb_ctrlrequest ep0_last_setup;

/* An OUT status event can retain BRCR=4. Accept a fresh standard SETUP
 * from its FIFO and reject repeats of the preceding request. */
static bool ep0_is_new_standard_setup(const struct usb_ctrlrequest *req)
{
    if (req->bRequestType == ep0_last_setup.bRequestType &&
        req->bRequest == ep0_last_setup.bRequest &&
        req->wValue == ep0_last_setup.wValue &&
        req->wIndex == ep0_last_setup.wIndex &&
        req->wLength == ep0_last_setup.wLength)
        return false;

    if (req->bRequestType == USB_DIR_IN &&
        req->bRequest == USB_REQ_GET_DESCRIPTOR)
        return req->wValue >> 8 && req->wLength > 0 &&
               req->wLength <= 256;

    if (req->bRequestType == 0 && req->bRequest == USB_REQ_SET_CONFIGURATION)
        return req->wLength == 0;

    return false;
}
#endif

/* This driver handles the SET_ADDRESS status phase. */
static void complete_transfer(int ep, int dir, int status, int length)
{
    if(ep == EP_CONTROL && address_status)
        return;
    usb_core_transfer_complete(ep, dir, status, length);
}

void usb_drv_ep_init(const struct usb_drv_ep_alloc_ctx* ctx, int ep)
{
    /* FIXME: support max packet size override */
    (void)ctx;
    tcc_endpoints[EP_NUM(ep)].dir = EP_DIR(ep) == DIR_IN ? USB_DIR_IN : USB_DIR_OUT;
}

void usb_drv_ep_deinit(const struct usb_drv_ep_alloc_ctx* ctx, int ep)
{
    (void)ctx;
    tcc_endpoints[EP_NUM(ep)].dir = -1;
}

static inline void pullup_on(void)
{
    TCC7xx_USB_PHY_CFG = 0x000c;
}

static inline void pullup_off(void)
{
    TCC7xx_USB_PHY_CFG = 0x3e4c;
}

/* On TCC7801, an indexed-register read completes the FIFO pop and makes
 * the next data-port entry visible. */
static inline unsigned int usb_fifo_read_word(
    volatile TCC7xx_USB_FIFO_TYPE *fifo)
{
    unsigned int value = *fifo;
#ifdef CPU_TCC780X
    unsigned int remaining = TCC7xx_USB_EP_BRCR;
    (void)remaining;
#endif
    return value;
}

/* SETUP and class buffers support byte alignment. Unpack the controller's
 * word-sized FIFO reads into bytes to handle odd addresses and short tails
 * on ARM926. */
static void usb_fifo_read_packet(volatile TCC7xx_USB_FIFO_TYPE *fifo,
                                unsigned char *buf, unsigned int count)
{
    unsigned int i = 0;

    while (i < count) {
        unsigned int word = usb_fifo_read_word(fifo);
        buf[i++] = word;
        if (i < count)
            buf[i++] = word >> 8;
    }
}

#if 0
static
char *dump_data(char *data, int count)
{
    static char buf[1024];
    char *dump = buf;
    int i;

    for (i = 0; i < count; i++)
        dump += snprintf(dump, sizeof(buf) - (dump - buf), "%02x", data[i]);
    return buf;
}
#endif

static
void handle_control(void)
{
    /* control are always 8 bytes len */
    static unsigned char ep_control[8];
    struct usb_ctrlrequest *req =
        (struct usb_ctrlrequest *) ep_control;
    unsigned short stat;
    unsigned short count = 0;
    bool possible_status_out;

    /* select control endpoint */
    TCC7xx_USB_INDEX = 0x00;
    /* Clear the IRQ latch before servicing the packet. A new event during
     * the handler must remain pending when we return. */
    TCC7xx_USB_EPIF = 1;
    stat = TCC7xx_USB_EP0_STAT;

    if (stat & 0x10) {
        DEBUG(2, "stall");
        TCC7xx_USB_EP0_STAT = 0x10;
    }

    if (TCC7xx_USB_EP0_STAT & 0x01) { /* RX */
        count = TCC7xx_USB_EP_BRCR;
        possible_status_out = ep0_expect_status_out && (stat & 0x20);

        /* An empty RX indication can follow SETUP. Complete the status
         * phase when the core has armed it. */
        if (count == 0) {
            bool complete_status = ep0_expect_status_out;

            ep0_expect_status_out = false;
            if (stat & 0x2) {
                TCC7xx_USB_EP0_STAT = 0x02;
                ep0_tx_ack();
            }
            TCC7xx_USB_EP0_STAT = 0x01;
            if (complete_status) {
                complete_transfer(EP_CONTROL, USB_DIR_OUT, 0, 0);
            }
            return;
        }

        /* Samsung's TCC7801 handler clears bit 1 of EP0_CTRL before
         * draining every nonempty EP0 RX event. */
#ifdef CPU_TCC780X
        TCC7xx_USB_EP0_CTRL &= ~0x2;
#endif

        /* Acknowledge the preceding TX completion before handling SETUP,
         * matching Samsung's EP0 interrupt path. Queue the response later. */
        if (stat & 0x2)
            TCC7xx_USB_EP0_STAT = 0x02;

        if (count != 4) { /* bad control? */
            unsigned short dummy;

            while (count > 0) {
                dummy = usb_fifo_read_word(&TCC7xx_USB_EP0_BUF);
                count--;
            }
            (void)dummy;
            DEBUG(1, "WTF: count = %d", count);
        } else {
            usb_fifo_read_packet(&TCC7xx_USB_EP0_BUF, ep_control, count * 2);
        }

        count *= 2;
        TCC7xx_USB_EP0_STAT = 0x01;
        DEBUG(1, "CTRL: len = %d %04x", count, stat);

#ifdef CPU_TCC780X
        if (possible_status_out) {
            if (count != 8 || !ep0_is_new_standard_setup(req)) {
                ep0_expect_status_out = false;
                if (stat & 0x2)
                    ep0_tx_ack();
                complete_transfer(EP_CONTROL, USB_DIR_OUT, 0, 0);
                return;
            }
        }
#else
        (void)possible_status_out;
#endif
        /* A new SETUP supersedes an uncompleted status phase. */
        ep0_expect_status_out = false;
    } else if (TCC7xx_USB_EP0_STAT & 0x02) { /* TX */
        TCC7xx_USB_EP0_STAT = 0x02;
        ep0_tx_ack();
        DEBUG(2, "TX Done\n");
    } else {
        DEBUG(1, "stat: %04x", stat);
    }

    if (0 == (stat & 0x1) || count != 8)
        return ;
#ifdef CPU_TCC780X
    ep0_last_setup = *req;
#endif
    ep0_cancel_tx();
    ep0_request_length = req->wLength;

    /* Cancel the preceding data/status phase so the core can process
     * the new SETUP immediately. */
    usb_core_control_cancelled();

    /* TODO: don't pass some kinds of requests to upper level */
    switch (req->bRequest) {
    case USB_REQ_CLEAR_FEATURE:
        DEBUG(2, "USB_REQ_CLEAR_FEATURE");
        DEBUG(2, "...%04x %04x", req->wValue, req->wIndex);
        break;
    case USB_REQ_SET_ADDRESS:
        //DEBUG(2, "USB_REQ_SET_ADDRESS, %d %d", req->wValue, TCC7xx_USB_FUNC);
        /* seems we don't have to set it manually
           TCC7xx_USB_FUNC = req->wValue; */
        break;
    case USB_REQ_GET_DESCRIPTOR:
        DEBUG(2, "gd, %02x %02x", req->wValue, req->wIndex);
        break;
    case USB_REQ_GET_CONFIGURATION:
        DEBUG(2, "USB_REQ_GET_CONFIGURATION");
        break;
    default:
        DEBUG(2, "req: %02x %02d", req->bRequestType, req->bRequest);
    }

    if(!handle_set_address(req))
        usb_core_setup_received(req);
}

static
void handle_ep_in(struct tcc_ep *tcc_ep, uint16_t stat)
{
    uint8_t *buf = tcc_ep->buf;
    int wcount;
    int count;
    int max_packet;

    if (tcc_ep->dir != USB_DIR_OUT) {
        panicf_my("ep%d: is input only", tcc_ep->id);
    }

    wcount = TCC7xx_USB_EP_BRCR;
    max_packet = TCC7xx_USB_EP_MAXP;

    DEBUG(2, "ep%d: %04x %04x", tcc_ep->id, stat, wcount);

    /* read data */
    count = wcount * 2;
    if (stat & TCC7xx_USP_EP_STAT_LWO) {
        count--;
    }

    if (buf == NULL)
        panicf_my("ep%d: Unexpected packet! %d %x", tcc_ep->id, count, TCC7xx_USB_EP_CTRL);
    if (tcc_ep->max_len < count)
        panicf_my("Too big packet: %d excepted %d %x", count, tcc_ep->max_len, TCC7xx_USB_EP_CTRL);

    usb_fifo_read_packet(tcc_ep->ep, buf, count);

    tcc_ep->buf += count;
    tcc_ep->count += count;
    tcc_ep->max_len -= count;

    TCC7xx_USB_EP_STAT = TCC7xx_USB_EP_STAT;
    TCC7xx_USB_EPIF = tcc_ep->mask;

    /* A CBW ends with a short packet, but a WRITE(10) data stage may span
     * many full-size packets. Report the whole receive to the storage class
     * only when the requested length or a short packet ends it. */
    if (tcc_ep->max_len > 0 && count == max_packet)
        return;

    tcc_ep->buf = NULL;
    TCC7xx_USB_EPIE &= ~tcc_ep->mask;
    global_ep_irq_mask &= ~tcc_ep->mask;
    TCC7xx_USB_EP_CTRL |= TCC7xx_USB_EP_CTRL_OUTHD;

    complete_transfer(tcc_ep->id, USB_DIR_OUT, 0, tcc_ep->count);
}

static
void handle_ep_out(struct tcc_ep *tcc_ep, uint16_t stat)
{
    bool done;
    (void) stat;

    if (tcc_ep->dir != USB_DIR_IN) {
        panicf_my("ep%d: is out only", tcc_ep->id);
    }

//    if (tcc_ep->buf == NULL) {
//        panicf_my("%s:%d", __FILE__, __LINE__);
//    }

    done = usb_drv_write_ep(tcc_ep);

//    TCC7xx_USB_EP_STAT = 0x2; /* Clear TX stat */
    TCC7xx_USB_EPIF = tcc_ep->mask;

    if (done) { // tcc_ep->buf == NULL) {
        TCC7xx_USB_EPIE &= ~tcc_ep->mask;
        global_ep_irq_mask &= ~tcc_ep->mask;

//        complete_transfer(tcc_ep->id, USB_DIR_IN, 0, tcc_ep->count);
    }
}

static
void handle_ep(unsigned short ep_irq)
{
    if (ep_irq & 0x1) {
        handle_control();
    }

    if (ep_irq & 0xe) {
        int endpoint;

        for (endpoint = 1; endpoint < 4; endpoint++) {
            struct tcc_ep *tcc_ep = &tcc_endpoints[endpoint];
            uint16_t stat;

            if (0 == (ep_irq & (1 << endpoint)))
                continue;

            TCC7xx_USB_INDEX = endpoint;
            stat = TCC7xx_USB_EP_STAT;

            DEBUG(1, "ep%d: %04x", endpoint, stat);

            if (stat & 0x1)
                handle_ep_in(tcc_ep, stat);
            else if (stat & 0x2)
                handle_ep_out(tcc_ep, stat);
            else /* TODO: remove me? */
                panicf_my("Unhandled ep%d state: %x, %d", endpoint, TCC7xx_USB_EP_STAT, TCC7xx_USB_INDEX);
        }
    }
}

static void usb_set_speed(int high_speed)
{
    TCC7xx_USB_EP_DIR = 0x0000;

    /* control endpoint */
    TCC7xx_USB_INDEX = 0;
    TCC7xx_USB_EP0_CTRL = 0x0000;
    TCC7xx_USB_EP_MAXP = 64;
    TCC7xx_USB_EP_CTRL = TCC7xx_USB_EP_CTRL_CDP | TCC7xx_USB_EP_CTRL_FLUSH;

    /* ep1: bulk-in, to host */
    TCC7xx_USB_INDEX = 1;
    TCC7xx_USB_EP_DIR |= (1 << 1);
    TCC7xx_USB_EP_CTRL = TCC7xx_USB_EP_CTRL_CDP;

    if (high_speed)
        TCC7xx_USB_EP_MAXP = 512;
    else
        TCC7xx_USB_EP_MAXP = 64;

    TCC7xx_USB_EP_DMA_CTRL = 0x0;

    /* ep2: bulk-out, from host */
    TCC7xx_USB_INDEX = 2;
    TCC7xx_USB_EP_DIR &= ~(1 << 2);
    TCC7xx_USB_EP_CTRL = TCC7xx_USB_EP_CTRL_CDP;

    if (high_speed)
        TCC7xx_USB_EP_MAXP = 512;
    else
        TCC7xx_USB_EP_MAXP = 64;

    TCC7xx_USB_EP_DMA_CTRL = 0x0;

    /* ep3: interrupt in */
    TCC7xx_USB_INDEX = 3;
    TCC7xx_USB_EP_DIR &= ~(1 << 3);
    TCC7xx_USB_EP_CTRL = TCC7xx_USB_EP_CTRL_CDP;
    TCC7xx_USB_EP_MAXP = 64;

    TCC7xx_USB_EP_DMA_CTRL = 0x0;
}

/*
  Reset TCC7xx usb device
 */
static void usb_reset(void)
{
    pullup_on();

    TCC7xx_USB_DELAY_CTRL |= 0x81;

#ifdef CPU_TCC780X
    /* Matches the controller setup in Samsung YP-P2 firmware 5.10. */
    TCC7xx_USB_SYS_CTRL = 0xa000 |
        TCC7xx_USB_SYS_CTRL_RESET   |
        TCC7xx_USB_SYS_CTRL_SUSPEND |
        TCC7xx_USB_SYS_CTRL_RFRE    |
        TCC7xx_USB_SYS_CTRL_SPDEN   |
        TCC7xx_USB_SYS_CTRL_EIEN    |
        TCC7xx_USB_SYS_CTRL_DUAL    |
        TCC7xx_USB_SYS_CTRL_DMAZ;
#else
    TCC7xx_USB_SYS_CTRL = 0xa000 |
        TCC7xx_USB_SYS_CTRL_RESET |
        TCC7xx_USB_SYS_CTRL_RFRE  |
        TCC7xx_USB_SYS_CTRL_SPDEN |
        TCC7xx_USB_SYS_CTRL_VBONE |
        TCC7xx_USB_SYS_CTRL_VBOFE;
#endif

    usb_set_speed(1);
    pullup_on();

    TCC7xx_USB_EPIF = TCC7xx_USB_EPIF_IRQ_MASK;
    global_ep_irq_mask = 0x1;
    TCC7xx_USB_EPIE = global_ep_irq_mask;

    address_status = false;
    ep0_expect_status_out = false;
    ep0_cancel_tx();
    usb_core_bus_reset();
}

/* IRQ handler */
void USB_DEVICE(void)
{
    unsigned short sys_stat;
    unsigned short ep_irq;
    unsigned short index_save;

    sys_stat = TCC7xx_USB_SYS_STAT;

    if (sys_stat & TCC7xx_USB_SYS_STAT_RESET) {
        TCC7xx_USB_SYS_STAT = TCC7xx_USB_SYS_STAT_RESET;
        usb_reset();
        TCC7xx_USB_SYS_CTRL |= TCC7xx_USB_SYS_CTRL_SUSPEND;
        DEBUG(2, "reset");
    }

    if (sys_stat & TCC7xx_USB_SYS_STAT_RESUME) {
        TCC7xx_USB_SYS_STAT = TCC7xx_USB_SYS_STAT_RESUME;
        usb_reset();
        TCC7xx_USB_SYS_CTRL |= TCC7xx_USB_SYS_CTRL_SUSPEND;
        DEBUG(2, "resume");
    }

    if (sys_stat & TCC7xx_USB_SYS_STAT_SPD_END) {
        usb_set_speed(1);
        TCC7xx_USB_SYS_STAT = TCC7xx_USB_SYS_STAT_SPD_END;
        DEBUG(2, "spd end");
    }

    if (sys_stat & TCC7xx_USB_SYS_STAT_ERRORS) {
        DEBUG(2, "errors: %4x", sys_stat & TCC7xx_USB_SYS_STAT_ERRORS);
        TCC7xx_USB_SYS_STAT = sys_stat & TCC7xx_USB_SYS_STAT_ERRORS;
    }

//    TCC7xx_USB_SYS_STAT = sys_stat;

    index_save = TCC7xx_USB_INDEX;

    ep_irq = TCC7xx_USB_EPIF & global_ep_irq_mask;

    /* Service the current endpoint flags, then return so the USB thread
     * can prepare deferred SETUP responses. Pending flags assert another IRQ. */
    if (ep_irq & TCC7xx_USB_EPIF_IRQ_MASK)
        handle_ep(ep_irq);

    TCC7xx_USB_INDEX = index_save;
}

int usb_drv_port_speed(void)
{
    return (TCC7xx_USB_SYS_STAT & 0x10) ? 1 : 0;
}

static int usb_drv_write_packet(volatile TCC7xx_USB_FIFO_TYPE *buf,
                                unsigned char *data, int len, int max)
{
    int count, i;

    len = MIN(len, max);
    count = (len + 1) / 2;

    TCC7xx_USB_EP_BWCR = len;

    for (i = 0; i < count; i++) {
        unsigned int word = data[2 * i];
        if (2 * i + 1 < len)
            word |= data[2 * i + 1] << 8;
        *buf = word;
    }

    return len;
}

static bool usb_drv_write_ep(struct tcc_ep *ep)
{
    int count;

    if (ep->max_len == 0) {
#ifdef CPU_TCC780X
        /* Complete the transfer after the final packet's TX interrupt.
         * BOT retains its CSW buffer until this host ACK. */
        if (ep->buf != NULL) {
            ep->buf = NULL;
            complete_transfer(ep->id, USB_DIR_IN, 0, ep->count);
        }
#endif
        return true;
    }

    count = usb_drv_write_packet(ep->ep, ep->buf, ep->max_len, 512);
    TCC7xx_USB_EP_STAT = 0x2; /* Clear TX stat */

    ep->buf += count;
    ep->count += count;
    ep->max_len -= count;

#ifndef CPU_TCC780X
    if (ep->max_len == 0) {
        complete_transfer(ep->id, USB_DIR_IN, 0, ep->count);
        ep->buf = NULL;
//        return true;
    }
#endif

    return false;
}

static void ep0_tx_packet(void)
{
    struct tcc_ep *ep = &tcc_endpoints[EP_CONTROL];

    TCC7xx_USB_INDEX = 0;
    if (ep->max_len == 0) {
        ep0_tx_zlp = false;
        /* Samsung's EP0 writer uses indexed FLUSH for an empty packet. */
        TCC7xx_USB_EP_CTRL |= TCC7xx_USB_EP_CTRL_FLUSH;
    } else {
        int count = usb_drv_write_packet(&TCC7xx_USB_EP0_BUF,
                                         ep->buf, ep->max_len, 64);
        ep->buf += count;
        ep->count += count;
        ep->max_len -= count;
    }
}

/* Advance the transfer when the controller reports a transmitted packet. */
static void ep0_tx_ack(void)
{
    struct tcc_ep *ep = &tcc_endpoints[EP_CONTROL];

    if (!ep0_tx_pending)
        return;
    if (ep->max_len > 0 || ep0_tx_zlp) {
        ep0_tx_packet();
        return;
    }

    bool notify = ep0_tx_notify;
    int count = ep->count;
    ep->buf = NULL;
    ep0_tx_pending = false;
    ep0_tx_result = 0;
    semaphore_release(&ep0_tx_complete);
    if (notify)
        complete_transfer(EP_CONTROL, USB_DIR_IN, 0, count);
}

static int ep0_start_tx(void *ptr, int length, bool notify)
{
    int flags = disable_irq_save();
    struct tcc_ep *ep = &tcc_endpoints[EP_CONTROL];

    if (ep0_tx_pending || length < 0 || (length && !ptr)) {
        restore_irq(flags);
        return -1;
    }
    /* Discard the semaphore token from the preceding asynchronous send. */
    semaphore_wait(&ep0_tx_complete, TIMEOUT_NOBLOCK);
    ep->buf = ptr;
    ep->count = 0;
    ep->max_len = length;
    ep0_tx_pending = true;
    ep0_tx_notify = notify;
    ep0_tx_result = -1;
    ep0_tx_zlp = length > 0 && (length % 64) == 0 &&
                 length < ep0_request_length;
    ep0_tx_packet();
    restore_irq(flags);
    return 0;
}

int usb_drv_send(int endpoint, void *ptr, int length)
{
    if ((endpoint & 0x7f) != EP_CONTROL)
        panicf_my("%s(%d,%d)", __func__, endpoint, length);
    int result = ep0_start_tx(ptr, length, false);
    if (result != 0)
        return result;
    if (semaphore_wait(&ep0_tx_complete, HZ) != OBJ_WAIT_SUCCEEDED) {
        int flags = disable_irq_save();
        ep0_cancel_tx();
        restore_irq(flags);
        return -1;
    }
    return ep0_tx_result;
}

int usb_drv_send_nonblocking(int endpoint, void *ptr, int length)
{
    int flags;
    int rc = 0;
    char *data = (unsigned char*) ptr;
    struct tcc_ep *ep = &tcc_endpoints[endpoint & 0x7f];

    if ((endpoint & 0x7f) == EP_CONTROL)
        return ep0_start_tx(ptr, length, true);

    if (ep->dir != USB_DIR_IN || length == 0)
        panicf_my("%s(%d,%d): Not supported", __func__, endpoint, length);

    DEBUG(2, "%s(%d,%d):", __func__, endpoint, length);

    flags = disable_irq_save();

    if(ep->buf != NULL) {
        panicf_my("%s: ep is already busy", __func__);
    }

    ep->buf = data;
    ep->max_len = length;
    ep->count = 0;

    TCC7xx_USB_INDEX = ep->id;
#ifndef CPU_TCC780X
    TCC7xx_USB_EP_STAT = 0x2;
    /* TODO: use interrupts instead */
    while (!usb_drv_write_ep(ep)) {
        while (0==(TCC7xx_USB_EP_STAT & 0x2))
            ;
    }
#else
    /* Use interrupt-driven bulk IN on TCC780X. A host can defer
     * reading CSW until its OUT data stage finishes. Returning here keeps
     * EP0, reset and bulk OUT serviceable while awaiting the ACK. */
    if (!usb_drv_write_ep(ep)) {
        TCC7xx_USB_EPIE |= ep->mask;
        global_ep_irq_mask |= ep->mask;
    }
#endif
    restore_irq(flags);

    DEBUG(2, "%s end", __func__);

    return rc;
}

int usb_drv_recv_nonblocking(int endpoint, void* ptr, int length)
{
    volatile struct tcc_ep *tcc_ep = &tcc_endpoints[endpoint & 0x7f];
    int flags;

    if (length == 0) {
        if (endpoint != 0)
            panicf_my("%s(%d,%d) zero length?", __func__, endpoint, length);
        ep0_expect_status_out = true;
        return 0;
    }
    // TODO: check ep
    if (tcc_ep->dir != USB_DIR_OUT)
        panicf_my("%s(%d,%d)", __func__, endpoint, length);

    DEBUG(2, "%s(%d,%d)", __func__, endpoint, length);

    flags = disable_irq_save();

    if (tcc_ep->buf) {
        panicf_my("%s: overrun: %x %x", __func__,
                  (unsigned int)tcc_ep->buf, (unsigned int)tcc_ep);
    }

    tcc_ep->buf = ptr;
    tcc_ep->max_len = length;
    tcc_ep->count = 0;

    TCC7xx_USB_INDEX = tcc_ep->id;

    TCC7xx_USB_EP_CTRL &= ~TCC7xx_USB_EP_CTRL_OUTHD;
    TCC7xx_USB_EPIE |= tcc_ep->mask;
    global_ep_irq_mask |= tcc_ep->mask;

    restore_irq(flags);

    return 0;
}

void usb_drv_cancel_all_transfers(void)
{
    int endpoint;
    int flags;

    DEBUG(2, "%s", __func__);

    flags = disable_irq_save();
    ep0_cancel_tx();
    for (endpoint = 0; endpoint < 4; endpoint++) {
        if (tcc_endpoints[endpoint].buf) {
/*            complete_transfer(tcc_endpoints[endpoint].id,
                                        tcc_endpoints[endpoint].dir, -1, 0); */
            tcc_endpoints[endpoint].buf = NULL;
        }
    }

    global_ep_irq_mask = 1;
    ep0_expect_status_out = false;
    TCC7xx_USB_EPIE = global_ep_irq_mask;
    TCC7xx_USB_EPIF = TCC7xx_USB_EPIF_IRQ_MASK;
    restore_irq(flags);
}

void usb_drv_set_test_mode(int mode)
{
    panicf_my("%s(%d)", __func__, mode);
}

bool usb_drv_stalled(int endpoint, bool in)
{
    panicf_my("%s(%d,%d)", __func__, endpoint, in);
    return false;
}

void usb_drv_stall(int endpoint, bool stall,bool in)
{
    (void) endpoint;
    (void) stall;
    (void) in;
    printf("%s(%d,%d,%d)", __func__, endpoint, stall, in);
}

void usb_drv_init(void)
{
    size_t i;
    semaphore_init(&ep0_tx_complete, 1, 0);

    DEBUG(2, "%s", __func__);

    for (i = 0; i < sizeof(tcc_endpoints)/sizeof(struct tcc_ep); i++) {
        tcc_endpoints[i].id = i;
        tcc_endpoints[i].mask = 1 << i;
        tcc_endpoints[i].buf = NULL;
        tcc_endpoints[i].dir = -1;
    }

    /* Enable USB clock */
    BCLKCTR |= DEV_USBD;

    /* switch USB to host and then reset */
    TCC7xx_USB_PHY_CFG = 0x3e4c;
    SWRESET |= DEV_USBD;
    udelay(50);
    SWRESET &= ~DEV_USBD;

    usb_reset();

    /* unmask irq */
    CREQ = USBD_IRQ_MASK;
    IRQSEL |= USBD_IRQ_MASK;
    TMODE &= ~USBD_IRQ_MASK;
    IEN |= USBD_IRQ_MASK;
}

void usb_drv_exit(void)
{
    TCC7xx_USB_EPIE = 0;
    BCLKCTR &= ~DEV_USBD;

    SWRESET |= DEV_USBD;
    udelay(50);
    SWRESET &= ~DEV_USBD;

    pullup_off();
}

void usb_init_device(void)
{
}

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

int usb_detect(void)
{
    /* TODO: not correct for all targets, we should poll VBUS
       signal on USB bus. */
    if (charger_inserted())
        return USB_INSERTED;
    return USB_EXTRACTED;
}

#ifdef BOOTLOADER
#include "ata.h"
void usb_test(void)
{
    int rc;

    printf("ATA");
    rc = ata_init();

    if(rc) {
        panicf("ata_init failed");
    }

    usb_init();
    usb_start_monitoring();
    usb_acknowledge(SYS_USB_CONNECTED_ACK, 0);

    while (1) {
        sleep(HZ);
//        usb_serial_send("Hello\r\n", 7);
    }
}
#endif
static bool handle_set_address(struct usb_ctrlrequest *req)
{
    address_status = false;
    if(!usb_drv_is_set_address(req))
        return false;

    const uint8_t address = req->wValue & 0x7f;
    address_status = true;
    usb_drv_cancel_all_transfers();
#ifdef CPU_TCC780X
    /* TCC7801 handles the SET_ADDRESS status phase in hardware.
     * FUNC reports the captured address; update the software state below. */
#else
    usb_drv_send_nonblocking(EP_CONTROL, NULL, 0);
#endif
    usb_core_notify_set_address(address);
    return true;
}

#else
void usb_init_device(void)
{
    /* simply switch USB off for now */
    BCLKCTR |= DEV_USBD;
    TCC7xx_USB_PHY_CFG = 0x3e4c;
    BCLKCTR &= ~DEV_USBD;
}

void usb_enable(bool on)
{
    (void)on;
}

/* Always return false for now */
int usb_detect(void)
{
    return USB_EXTRACTED;
}
#endif
