/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

#include "usb-tcc7xx.h"

#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "panic.h"

#ifdef HAVE_USBSTACK
#include "usb_ch9.h"
#include "usb_core.h"

#define TCC7xx_USB_EPIF_IRQ_MASK 0xf

static int dbg_level = 0x02;
static int global_ep_irq_mask = 0x1;
#define DEBUG(level, fmt, args...) do { if (dbg_level & (level)) printf(fmt, ## args); } while (0)

#include <inttypes.h>


#include "sprintf.h"
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
    volatile uint16_t  *ep; /* hw ep buffer */
    int id;       /* Endpoint id */
    int mask;     /* Endpoint bit mask */
    char *buf;    /* user buffer to store data */
    int max_len;  /* how match data will fit */
    int count;    /* actual data count */
    bool busy;
} ;

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

static int usb_drv_write_packet(volatile unsigned short *buf, unsigned char *data, int len, int max);
static void usb_set_speed(int);

int usb_drv_request_endpoint(int dir)
{
    int flags = disable_irq_save();
    size_t ep;
    int ret = 0;

    if (dir == USB_DIR_IN)
        ep = 1;
    else
        ep = 2;

    if (!tcc_endpoints[ep].busy) {
        tcc_endpoints[ep].busy = true;
        tcc_endpoints[ep].dir = dir;
        ret = ep | dir;
    } else {
        ret = -1;
    }

    restore_irq(flags);
    return ret;
}

void usb_drv_release_endpoint(int ep)
{
    int flags;
    ep = ep & 0x7f;

    if (ep < 1 || ep > NUM_ENDPOINTS)
        return ;

    flags = disable_irq_save();

    tcc_endpoints[ep].busy = false;
    tcc_endpoints[ep].dir = -1;

    restore_irq(flags);
}

static void udelay(unsigned long msecs)
{
    /* TODO: implement me other way */
    msecs*=126;
    while (msecs--)
        asm("nop;");
}

static inline void pullup_on(void)
{
    TCC7xx_USB_PHY_CFG = 0x000c;
}

static inline void pullup_off(void)
{
    TCC7xx_USB_PHY_CFG = 0x3e4c;
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
    int i;
    int type;

    /* select control endpoint */
    TCC7xx_USB_INDEX = 0x00;
    stat = TCC7xx_USB_EP0_STAT;

    if (stat & 0x10) {
        DEBUG(2, "stall");
        TCC7xx_USB_EP0_STAT = 0x10;
    }

    if (TCC7xx_USB_EP0_STAT & 0x01) { /* RX */
        uint16_t *ptr = (uint16_t *) ep_control;

        count = TCC7xx_USB_EP_BRCR;

        if (TCC7xx_USB_EP0_STAT & 0x2)
            TCC7xx_USB_EP0_STAT = 0x02;

        if (count != 4) { /* bad control? */
            unsigned short dummy;

            while (count--)
                dummy = TCC7xx_USB_EP0_BUF;
                DEBUG(1, "WTF: count = %d", count);
            } else {
                /* simply read control packet */
                for (i = 0; i < count; i++)
                    ptr[i] = TCC7xx_USB_EP0_BUF;
            }

            count *= 2;
            TCC7xx_USB_EP0_STAT = 0x01;
            DEBUG(1, "CTRL: len = %d %04x", count, stat);
    } else if (TCC7xx_USB_EP0_STAT & 0x02) { /* TX */
        TCC7xx_USB_EP0_STAT = 0x02;
        DEBUG(2, "TX Done\n");
    } else {
        DEBUG(1, "stat: %04x", stat);
    }

    TCC7xx_USB_EPIF = 1;

    if (0 == (stat & 0x1) || count != 8)
        return ;
#if 1 /* TODO: remove me someday */
    {
        int i;
        uint16_t *ptr = (uint16_t *) ep_control;
        for (i = 1; i < (count>>1); i++) {
            if (ptr[i] != ptr[0])
                break;
        }
        if (i == (count>>1)) {
            /*DEBUG(2, */panicf_my("sanity failed");
            return ;
        }
    }
#endif
    type = req->bRequestType;

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

    usb_core_control_request(req);
}

static
void handle_ep_in(struct tcc_ep *tcc_ep, uint16_t stat)
{
    uint8_t *buf = tcc_ep->buf;
    uint16_t *wbuf = (uint16_t *) buf;
    int wcount;
    int count;
    int i;

    if (tcc_ep->dir != USB_DIR_OUT) {
        panicf_my("ep%d: is input only", tcc_ep->id);
    }

    wcount = TCC7xx_USB_EP_BRCR;

    DEBUG(2, "ep%d: %04x %04x", tcc_ep->id, stat, wcount);

    /* read data */
    count = wcount * 2;
    if (stat & TCC7xx_USP_EP_STAT_LWO) {
        count--;
        wcount--;
    }

    if (buf == NULL)
        panicf_my("ep%d: Unexpected packet! %d %x", tcc_ep->id, count, TCC7xx_USB_EP_CTRL);
    if (tcc_ep->max_len < count)
        panicf_my("Too big packet: %d excepted %d %x", count, tcc_ep->max_len, TCC7xx_USB_EP_CTRL);

    for (i = 0; i < wcount; i++)
        wbuf[i] = *tcc_ep->ep;

    if (count & 1) { /* lwo */
        uint16_t tmp = *tcc_ep->ep;
        buf[count - 1] = tmp & 0xff;
    }

    tcc_ep->buf = NULL;

    TCC7xx_USB_EP_STAT = TCC7xx_USB_EP_STAT;
    TCC7xx_USB_EPIF = tcc_ep->mask;
    TCC7xx_USB_EPIE &= ~tcc_ep->mask; /* TODO: use INGLD? */
    global_ep_irq_mask &= ~tcc_ep->mask;

    if (TCC7xx_USB_EP_STAT & 0x1)
        panicf_my("One more packet?");

    TCC7xx_USB_EP_CTRL |= TCC7xx_USB_EP_CTRL_OUTHD;

    usb_core_transfer_complete(tcc_ep->id, USB_DIR_OUT, 0, count);
}

static
void handle_ep_out(struct tcc_ep *tcc_ep, uint16_t stat)
{
    (void) stat;

    if (tcc_ep->dir != USB_DIR_IN) {
        panicf_my("ep%d: is out only", tcc_ep->id);
    }

    if (tcc_ep->buf == NULL) {
        panicf_my("%s:%d", __FILE__, __LINE__);
    }

    if (tcc_ep->max_len) {
        int count = usb_drv_write_packet(tcc_ep->ep,
                                            tcc_ep->buf,
                                            tcc_ep->max_len,
                                            512);
        tcc_ep->buf += count;
        tcc_ep->max_len -= count;
        tcc_ep->count += count;
    } else {
        tcc_ep->buf = NULL;
    }

    TCC7xx_USB_EP_STAT = 0x2; /* Clear TX stat */
    TCC7xx_USB_EPIF = tcc_ep->mask;

    if (tcc_ep->buf == NULL) {
        TCC7xx_USB_EPIE &= ~tcc_ep->mask;
        global_ep_irq_mask &= ~tcc_ep->mask;

        usb_core_transfer_complete(tcc_ep->id, USB_DIR_IN, 0, tcc_ep->count);
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
            if (!tcc_ep->busy)
                panicf_my("ep%d: wasn't requested", endpoint);

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

    TCC7xx_USB_SYS_CTRL = 0xa000 |
        TCC7xx_USB_SYS_CTRL_RESET |
        TCC7xx_USB_SYS_CTRL_RFRE  |
        TCC7xx_USB_SYS_CTRL_SPDEN |
        TCC7xx_USB_SYS_CTRL_VBONE |
        TCC7xx_USB_SYS_CTRL_VBOFE;

    usb_set_speed(1);
    pullup_on();

    TCC7xx_USB_EPIF = TCC7xx_USB_EPIF_IRQ_MASK;
    global_ep_irq_mask = 0x1;
    TCC7xx_USB_EPIE = global_ep_irq_mask;

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

    while (ep_irq & TCC7xx_USB_EPIF_IRQ_MASK) {
        handle_ep(ep_irq);

        /* is that really needed, btw not a problem for rockbox */
        udelay(50);
        ep_irq = TCC7xx_USB_EPIF & global_ep_irq_mask;
    }

    TCC7xx_USB_INDEX = index_save;
}

void usb_drv_set_address(int address)
{
    DEBUG(2, "setting address %d %d", address, TCC7xx_USB_FUNC);
}

int usb_drv_port_speed(void)
{
    return (TCC7xx_USB_SYS_STAT & 0x10) ? 1 : 0;
}

static int usb_drv_write_packet(volatile unsigned short *buf, unsigned char *data, int len, int max)
{
    uint16_t  *wbuf = (uint16_t *) data;
    int count, i;

    len = MIN(len, max);
    count = (len + 1) / 2;

    TCC7xx_USB_EP_BWCR = len;

    for (i = 0; i < count; i++)
        *buf = *wbuf++;

    return len;
}

int usb_drv_send(int endpoint, void *ptr, int length)
{
    int flags = disable_irq_save();
    int rc = 0;
    char *data = (unsigned char*) ptr;;

    DEBUG(2, "%s(%d,%d)" , __func__, endpoint, length);

    if (endpoint != 0)
        panicf_my("%s(%d,%d)", __func__, endpoint, length);

    TCC7xx_USB_INDEX = 0;
    while (length > 0) {
        int ret;

        ret = usb_drv_write_packet(&TCC7xx_USB_EP0_BUF, data, length, 64);
        length -= ret;
        data += ret;

        while (0 == (TCC7xx_USB_EP0_STAT & 0x2))
            ;
        TCC7xx_USB_EP0_STAT = 0x2;
    }

    restore_irq(flags);
    return rc;
}

int usb_drv_send_nonblocking(int endpoint, void *ptr, int length)
{
    int flags;
    int rc = 0, count = length;
    char *data = (unsigned char*) ptr;
    struct tcc_ep *ep = &tcc_endpoints[endpoint & 0x7f];

    if (ep->dir != USB_DIR_IN || length == 0)
        panicf_my("%s(%d,%d): Not supported", __func__, endpoint, length);

    DEBUG(2, "%s(%d,%d):", __func__, endpoint, length);

    flags = disable_irq_save();

    if(ep->buf != NULL) {
        panicf_my("%s: ep is already busy", __func__);
    }

    TCC7xx_USB_INDEX = ep->id;

    count = usb_drv_write_packet(ep->ep, data, length, 512);

    data += count;
    length -= count;

    ep->buf = data;
    ep->max_len = length;
    ep->count = count;

    TCC7xx_USB_EPIE |= ep->mask;
    global_ep_irq_mask |= ep->mask;

    restore_irq(flags);

    DEBUG(2, "%s end", __func__);

    return rc;
}

int usb_drv_recv(int endpoint, void* ptr, int length)
{
    volatile struct tcc_ep *tcc_ep = &tcc_endpoints[endpoint & 0x7f];
    int flags;

    if (length == 0) {
        if (endpoint != 0)
            panicf_my("%s(%d,%d) zero length?", __func__, endpoint, length);
        return 0;
    }
    // TODO: check ep
    if (tcc_ep->dir != USB_DIR_OUT)
        panicf_my("%s(%d,%d)", __func__, endpoint, length);

    DEBUG(2, "%s(%d,%d)", __func__, endpoint, length);

    flags = disable_irq_save();

    if (tcc_ep->buf) {
        panicf_my("%s: overrun: %x %x", __func__, tcc_ep->buf, tcc_ep);
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
    for (endpoint = 0; endpoint < 4; endpoint++) {
        if (tcc_endpoints[endpoint].buf) {
/*            usb_core_transfer_complete(tcc_endpoints[endpoint].id,
                                        tcc_endpoints[endpoint].dir, -1, 0); */
            tcc_endpoints[endpoint].buf = NULL;
        }
    }

    global_ep_irq_mask = 1;
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
}

void usb_drv_stall(int endpoint, bool stall,bool in)
{
    printf("%s(%d,%d,%d)", __func__, endpoint, stall, in);
}

void usb_drv_init(void)
{
    size_t i;

    DEBUG(2, "%s", __func__);

    for (i = 0; i < sizeof(tcc_endpoints)/sizeof(struct tcc_ep); i++) {
        tcc_endpoints[i].id = i;
        tcc_endpoints[i].mask = 1 << i;
        tcc_endpoints[i].buf = NULL;
        tcc_endpoints[i].busy = false;
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
    usb_acknowledge(SYS_USB_CONNECTED_ACK);

    while (1) {
        sleep(HZ);
//        usb_serial_send("Hello\r\n", 7);
    }
}
#endif
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
