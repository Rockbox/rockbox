/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2010 Amaury Pouly
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

#include "usb.h"
#include "usb_drv.h"
#include "as3525v2.h"
#include "clock-target.h"
#include "ascodec.h"
#include "as3514.h"
#include <stdbool.h>
#include "panic.h"
#define LOGF_ENABLE
#include "logf.h"
#include "usb-drv-as3525v2.h"

struct usb_endpoint
{
    void *buf;
    unsigned int len;
    union
    {
        unsigned int sent;
        unsigned int received;
    };
    bool wait;
    bool busy;
};

static struct usb_endpoint endpoints[USB_NUM_ENDPOINTS*2];

void usb_attach(void)
{
    usb_enable(true);
}

static void usb_delay(void)
{
    int i = 0;
    while(i < 0x300)
        i++;
}

static void as3525v2_connect(void)
{
    logf("usb: init as3525v2");
    /* 1) enable usb core clock */
    CGU_PERI |= CGU_USB_CLOCK_ENABLE;
    usb_delay();
    /* 2) enable usb phy clock */
    CGU_USB |= 0x20;
    usb_delay();
    /* 3) clear "stop pclk" */
    USB_PCGCCTL &= ~0x1;
    usb_delay();
    /* 4) clear "power clamp" */
    USB_PCGCCTL &= ~0x4;
    usb_delay();
    /* 5) clear "reset power down module" */
    USB_PCGCCTL &= ~0x8;
    usb_delay();
    /* 6) set "power on program done" */
    USB_DCTL |= 0x800;
    usb_delay();
    /* 7) core soft reset */
    USB_GRSTCTL |= USB_GRSTCTL_csftrst;
    usb_delay();
    /* 8) hclk soft reset */
    USB_GRSTCTL |= USB_GRSTCTL_hsftrst;
    usb_delay();
    /* 9) flush and reset everything */
    USB_GRSTCTL |= 0x3f;
    usb_delay();
    /* 10) force device mode*/
    USB_GUSBCFG &= ~0x20000000;
    USB_GUSBCFG |= 0x40000000;
    usb_delay();
    /* 11) Do something that is probably CCU related but undocumented*/
    CCU_USB_THINGY &= ~0x1000;
    usb_delay();
    /* 12) reset usb core parameters (dev addr, speed, ...) */
    USB_DCFG = 0;
    usb_delay();
}

static void core_reset(void)
{
    unsigned int i = 0;
    /* Wait for AHB master IDLE state. */
    while((USB_GRSTCTL & USB_GRSTCTL_ahbidle) == 0);
    {
        /*udelay(10);*/
        sleep(1);
    }
    /* Core Soft Reset */
    USB_GRSTCTL |= USB_GRSTCTL_csftrst;
    /* Waits for the hardware to clear reset bit */
    while(USB_GRSTCTL & USB_GRSTCTL_csftrst && i < 0x300)
        i++;

    if(USB_GRSTCTL & USB_GRSTCTL_csftrst)
        logf("oops, usb core soft reset hang :(");

    /* Wait for 3 PHY Clocks */
    /*mdelay(100);*/
    sleep(1);

    logf("%ld endpoints", USB_GHWCFG2_NUM_EP);
    for(i = 0; i < USB_GHWCFG2_NUM_EP; i++)
        logf("  EP%d: IN=%ld OUT=%ld", i, USB_GHWCFG1_IN_EP(i), USB_GHWCFG1_OUT_EP(i));
    logf("hwcfg1: %08lx", USB_GHWCFG1);
    logf("hwcfg2: %08lx", USB_GHWCFG2);
    logf("hwcfg3: %08lx", USB_GHWCFG3);
    logf("hwcfg4: %08lx", USB_GHWCFG4);

    logf("%ld in ep", USB_GHWCFG4_NUM_IN_EP);
    logf("tot fifo sz: %ld", USB_GHWCFG3_DFIFO_LEN);
    logf("rx fifo sz: %ld", USB_GRXFSIZ);
    logf("tx fifo sz: %ld", USB_GNPTXFSIZ >> 16); /* there is no perio ep so print only non-perio */
}

static void core_init(void)
{
    /* Reset the Controller */
    core_reset();

    /* Setup phy for high speed */
    /* 1) select utmi */
    /*    fixme: the clip+ hardware support utmi only, this is useless */
    //USB_GUSBCFG &= ~USB_GUSBCFG_ulpi_utmi_sel;
    /* 2) select utmi 16-bit wide bus */
    USB_GUSBCFG |= USB_GUSBCFG_phy_if;
    /* 3) core reset */
    /*    fixme: linux patch says the phy parameters survive the soft reset so
     *           perhaps this part can be done only one type but I don't know
     *           what happened when phy goes to standby mode and clock are disabled */
    core_reset();

    /* fixme: at this point, the linux patch sets ulpi bits to 0 on utmi selection
     *        but the clip+ hardware does not support it so don't bother with
     *        that */

    /* fixme: the current code is for internal DMA only, the clip+ architecture
     *        define the internal DMA model */
    /* Set burstlen */
    USB_GAHBCFG |= USB_GAHBCFG_INT_DMA_BURST_INCR << USB_GAHBCFG_hburstlen_bit_pos;
    /* Enable DMA */
    USB_GAHBCFG |= USB_GAHBCFG_dma_enable;
    /* Disable HNP and SRP, not sure it's useful because we already forced dev mode */
    USB_GUSBCFG &= ~(USB_GUSBCFG_SRP_cap | USB_GUSBCFG_HNP_cap);
}

void usb_drv_init(void)
{
    logf("usb_drv_init");
    as3525v2_connect();

    logf("usb: synopsis id: %lx", USB_GSNPSID);

    core_init();
}

void usb_drv_exit(void)
{
    logf("usb_drv_exit");
}

int usb_drv_port_speed(void)
{
    return 0;
}

int usb_drv_request_endpoint(int type, int dir)
{
    (void) type;
    (void) dir;
    return -1;
}

void usb_drv_release_endpoint(int ep)
{
    (void) ep;
}

void usb_drv_cancel_all_transfers(void)
{
}

int usb_drv_recv(int ep, void *ptr, int len)
{
    (void) ep;
    (void) ptr;
    (void) len;
    return -1;
}

int usb_drv_send(int ep, void *ptr, int len)
{
    (void) ep;
    (void) ptr;
    (void) len;
    return -1;
}

int usb_drv_send_nonblocking(int ep, void *ptr, int len)
{
    (void) ep;
    (void) ptr;
    (void) len;
    return -1;
}

/* interrupt service routine */
void INT_USB(void)
{
    panicf("USB interrupt !");
}

/* (not essential? , not implemented in usb-tcc.c) */
void usb_drv_set_test_mode(int mode)
{
    (void) mode;
}

void usb_drv_set_address(int address)
{
    (void) address;
}

void usb_drv_stall(int ep, bool stall, bool in)
{
    (void) ep;
    (void) stall;
    (void) in;
}

bool usb_drv_stalled(int ep, bool in)
{
    (void) ep;
    (void) in;
    return true;
    return true;
}

