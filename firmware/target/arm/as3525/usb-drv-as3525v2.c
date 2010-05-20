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

#if 0
static struct usb_endpoint endpoints[USB_NUM_ENDPOINTS*2];
#endif

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
    CCU_USB_THINGY &= ~0x300000;
    usb_delay();
    /* 12) reset usb core parameters (dev addr, speed, ...) */
    USB_DCFG = 0;
    usb_delay();
}

static void usb_enable_common_interrupts(void)
{
    /* Clear any pending otg interrupt */
    USB_GOTGINT = 0xffffffff;
    /* Clear any pending interrupt */
    USB_GINTSTS = 0Xffffffff;
    /* Enable interrupts */
    USB_GINTMSK |= USB_GINTMSK_modemismatch |
                    USB_GINTMSK_otgintr |
                    USB_GINTMSK_rxstsqlvl | /* for dma */
                    USB_GINTMSK_conidstschng |
                    USB_GINTMSK_wkupintr |
                    USB_GINTMSK_disconnect |
                    USB_GINTMSK_usbsuspend |
                    USB_GINTMSK_sessreqintr;
}

static void usb_enable_device_interrupts(void)
{
    /* Disable all interrupts */
    USB_GINTMSK = 0;
    /* Clear any pending interrupt */
    USB_GINTSTS = 0xffffffff;
    /* Enable common interrupts */
    usb_enable_common_interrupts();
    /* Enable interrupts */
    USB_GINTMSK |=
          USB_GINTMSK_usb_rst
        | USB_GINTMSK_enumdone
        | USB_GINTMSK_inepintr
        | USB_GINTMSK_outepintr
        | USB_GINTMSK_erlysuspend
        | USB_GINTMSK_epmismatch /* only if multiple tx fifos enabled */
#if 0 /* only if periodic fifo used */
        | USB_GINTMSK_isooutdrop
        | USB_GINTMSK_eopframe
        | USB_GINTMSK_incomplisoin
        | USB_GINTMSK_incomplisoout
#endif
        ;
}

static void usb_flush_tx_fifos(int nums)
{
    unsigned int i = 0;
    
    USB_GRSTCTL = (USB_GRSTCTL & (~USB_GRSTCTL_txfnum_bits))
        | (nums << USB_GRSTCTL_txfnum_bit_pos)
        | USB_GRSTCTL_txfflsh_flush;
    while(USB_GRSTCTL & USB_GRSTCTL_txfflsh_flush && i < 0x300)
        i++;
    if(USB_GRSTCTL & USB_GRSTCTL_txfflsh_flush)
        panicf("usb: hang of flush tx fifos (%x)", nums);
    /* wait 3 phy clocks */
    sleep(1);
}

static void usb_flush_rx_fifo(void)
{
    unsigned int i = 0;
    
    USB_GRSTCTL |= USB_GRSTCTL_rxfflsh_flush;
    while(USB_GRSTCTL & USB_GRSTCTL_rxfflsh_flush && i < 0x300)
        i++;
    if(USB_GRSTCTL & USB_GRSTCTL_rxfflsh_flush)
        panicf("usb: hang of flush rx fifo");
    /* wait 3 phy clocks */
    sleep(1);
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
        panicf("oops, usb core soft reset hang :(");

    /* Wait for 3 PHY Clocks */
    /*mdelay(100);*/
    sleep(1);
}

static void core_dev_init(void)
{
    unsigned int usb_num_in_ep = 0;
    unsigned int usb_num_out_ep = 0;
    unsigned int i;
    /* Restart the phy clock */
    USB_PCGCCTL = 0;
    /* Set phy speed : high speed */
    USB_DCFG = (USB_DCFG & (~USB_DCFG_devspd_bits)) | USB_DCFG_devspd_hs_phy_hs;
    /* Set periodic frame interval */
    USB_DCFG = (USB_DCFG & (~USB_DCFG_perfrint_bits)) | (USB_DCFG_FRAME_INTERVAL_80 << USB_DCFG_perfrint_bit_pos);
    
    /* Check hardware capabilities */
    if(USB_GHWCFG2_ARCH != USB_INT_DMA_ARCH)
        panicf("usb: wrong architecture (%ld)", USB_GHWCFG2_ARCH);
    if(USB_GHWCFG2_HS_PHY_TYPE != USB_PHY_TYPE_UTMI)
        panicf("usb: wrong HS phy type (%ld)", USB_GHWCFG2_HS_PHY_TYPE);
    if(USB_GHWCFG2_FS_PHY_TYPE != USB_PHY_TYPE_UNSUPPORTED)
        panicf("usb: wrong FS phy type (%ld)", USB_GHWCFG2_FS_PHY_TYPE);
    #ifdef USB_USE_CUSTOM_FIFO_LAYOUT
    if(USB_GHWCFG2_DYN_FIFO != 1)
        panicf("usb: no dynamic fifo");
    if(USB_GRXFSIZ != USB_DATA_FIFO_DEPTH)
        panicf("usb: wrong data fifo size");
    #endif /* USB_USE_CUSTOM_FIFO_LAYOUT */
    if(USB_GHWCFG4_UTMI_PHY_DATA_WIDTH != 0x2)
        panicf("usb: wrong utmi data width (%ld)", USB_GHWCFG4_UTMI_PHY_DATA_WIDTH);
    if(USB_GHWCFG4_DED_FIFO_EN != 1) /* it seems to be multiple tx fifo support */
        panicf("usb: no multiple tx fifo");

    logf("hwcfg1: %08lx", USB_GHWCFG1);
    logf("hwcfg2: %08lx", USB_GHWCFG2);
    logf("hwcfg3: %08lx", USB_GHWCFG3);
    logf("hwcfg4: %08lx", USB_GHWCFG4);

    logf("%ld endpoints", USB_GHWCFG2_NUM_EP);
    usb_num_in_ep = 0;
    usb_num_out_ep = 0;
    for(i = 0; i < USB_GHWCFG2_NUM_EP; i++)
    {
        if(USB_GHWCFG1_IN_EP(i))
            usb_num_in_ep++;
        if(USB_GHWCFG1_OUT_EP(i))
            usb_num_out_ep++;
        logf("  EP%d: IN=%ld OUT=%ld", i, USB_GHWCFG1_IN_EP(i), USB_GHWCFG1_OUT_EP(i));
    }

    if(usb_num_in_ep != USB_GHWCFG4_NUM_IN_EP)
        panicf("usb: num in ep mismatch(%d,%lu)", usb_num_in_ep, USB_GHWCFG4_NUM_IN_EP);
    if(usb_num_in_ep != USB_NUM_IN_EP)
        panicf("usb: num in ep static mismatch(%u,%u)", usb_num_in_ep, USB_NUM_IN_EP);
    if(usb_num_out_ep != USB_NUM_OUT_EP)
        panicf("usb: num out ep static mismatch(%u,%u)", usb_num_out_ep, USB_NUM_OUT_EP);

    logf("%d in ep, %d out ep", usb_num_in_ep, usb_num_out_ep);
    logf("initial:");
    logf("  tot fifo sz: %lx", USB_GHWCFG3_DFIFO_LEN);
    logf("  rx fifo: [%04x,+%4lx]", 0, USB_GRXFSIZ);
    logf("  nptx fifo: [%04lx,+%4lx]", USB_GET_FIFOSIZE_START_ADR(USB_GNPTXFSIZ),
        USB_GET_FIFOSIZE_DEPTH(USB_GNPTXFSIZ));
    for(i = 1; i <= USB_NUM_IN_EP; i++)
    {
        logf("  dieptx fifo(%2u): [%04lx,+%4lx]", i,
            USB_GET_FIFOSIZE_START_ADR(USB_DIEPTXFSIZ(i)),
            USB_GET_FIFOSIZE_DEPTH(USB_DIEPTXFSIZ(i)));
    }

    #ifdef USB_USE_CUSTOM_FIFO_LAYOUT
    /* Setup FIFOs */
    /* Organize FIFO as follow:
     *             0           ->         rxfsize         : RX fifo
     *          rxfsize        ->   rxfsize + nptxfsize   : TX fifo for first IN ep
     *   rxfsize + nptxfsize   -> rxfsize + 2 * nptxfsize : TX fifo for second IN ep
     * rxfsize + 2 * nptxfsize -> rxfsize + 3 * nptxfsize : TX fifo for third IN ep
     * ...
     */

    unsigned short adr = 0;
    unsigned short depth = USB_RX_FIFO_SIZE;
    USB_GRXFSIZ = depth;
    adr += depth;
    depth = USB_NPTX_FIFO_SIZE;
    USB_GNPTXFSIZ = USB_MAKE_FIFOSIZE_DATA(adr, depth);
    adr += depth;

    for(i = 1; i <= USB_NUM_IN_EP; i++)
    {
        depth = USB_EPTX_FIFO_SIZE;
        USB_DIEPTXFSIZ(i) = USB_MAKE_FIFOSIZE_DATA(adr, depth);
        adr += depth;
    }

    logf("used:");
    logf("  rx fifo: [%04x,+%4lx]", 0, USB_GRXFSIZ);
    logf("  nptx fifo: [%04lx,+%4lx]", USB_GET_FIFOSIZE_START_ADR(USB_GNPTXFSIZ),
        USB_GET_FIFOSIZE_DEPTH(USB_GNPTXFSIZ));
    for(i = 1; i <= USB_NUM_IN_EP; i++)
    {
        logf("  dieptx fifo(%2u): [%04lx,+%4lx]", i,
            USB_GET_FIFOSIZE_START_ADR(USB_DIEPTXFSIZ(i)),
            USB_GET_FIFOSIZE_DEPTH(USB_DIEPTXFSIZ(i)));
    }

    if(adr > USB_DATA_FIFO_DEPTH)
        panicf("usb: total data fifo size exceeded");
    #endif /* USB_USE_CUSTOM_FIFO_LAYOUT */

    /* flush the fifos */
    usb_flush_tx_fifos(0x10); /* flush all */
    usb_flush_rx_fifo();

    /* flush learning queue */
    USB_GRSTCTL |= USB_GRSTCTL_intknqflsh;

    /* Clear all pending device interrupts */
    USB_DIEPMSK = 0;
    USB_DOEPMSK = 0;
    USB_DAINT = 0xffffffff;
    USB_DAINTMSK = 0;

    for(i = 0; i <= USB_NUM_IN_EP; i++)
    {
        /* disable endpoint if enabled */
        if(USB_DIEPCTL(i) & USB_DEPCTL_epena)
            USB_DIEPCTL(i) = USB_DEPCTL_epdis | USB_DEPCTL_snak;
        else
            USB_DIEPCTL(i) = 0;

        USB_DIEPTSIZ(i) = 0;
        USB_DIEPDMA(i) = 0;
        USB_DIEPINT(i) = 0xff;
    }

    for(i = 0; i <= USB_NUM_OUT_EP; i++)
    {
        /* disable endpoint if enabled */
        if(USB_DOEPCTL(i) & USB_DEPCTL_epena)
            USB_DOEPCTL(i) = USB_DEPCTL_epdis | USB_DEPCTL_snak;
        else
            USB_DOEPCTL(i) = 0;

        USB_DOEPTSIZ(i) = 0;
        USB_DOEPDMA(i) = 0;
        USB_DOEPINT(i) = 0xff;
    }

    /* fixme: threshold tweaking only takes place if we use multiple tx fifos it seems */
    /* only dump them for now, leave threshold disabled */
    logf("threshold control:");
    logf("  non_iso_thr_en: %d", (USB_DTHRCTL & USB_DTHRCTL_non_iso_thr_en) ? 1 : 0);
    logf("  iso_thr_en: %d", (USB_DTHRCTL & USB_DTHRCTL_iso_thr_en) ? 1 : 0);
    logf("  tx_thr_len: %lu", (USB_DTHRCTL & USB_DTHRCTL_tx_thr_len_bits) >> USB_DTHRCTL_tx_thr_len_bit_pos);
    logf("  rx_thr_en: %d", (USB_DTHRCTL & USB_DTHRCTL_rx_thr_en) ? 1 : 0);
    logf("  rx_thr_len: %lu", (USB_DTHRCTL & USB_DTHRCTL_rx_thr_len_bits) >> USB_DTHRCTL_rx_thr_len_bit_pos);

    /* enable USB interrupts */
    usb_enable_device_interrupts();

    /* enable fifo underrun interrupt ? */
    USB_DIEPMSK |= USB_DIEPINT_txfifoundrn;
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

    /* enable basic interrupts */
    usb_enable_common_interrupts();

    /* perform device model specific init */
    core_dev_init();
}

static void usb_enable_global_interrupts(void)
{
    VIC_INT_ENABLE = INTERRUPT_USB;
    USB_GAHBCFG |= USB_GAHBCFG_glblintrmsk;
}

static void usb_disable_global_interrupts(void)
{
    USB_GAHBCFG &= ~USB_GAHBCFG_glblintrmsk;
    VIC_INT_EN_CLEAR = INTERRUPT_USB;
}

void usb_drv_init(void)
{
    logf("usb_drv_init");
    /* Enable PHY and clocks (but leave pullups disabled) */
    as3525v2_connect();
    /* Disable global interrupts */
    usb_disable_global_interrupts();
    logf("usb: synopsis id: %lx", USB_GSNPSID);
    /* Core init */
    core_init();
    /* Enable global interrupts */
    usb_enable_global_interrupts();
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
}

