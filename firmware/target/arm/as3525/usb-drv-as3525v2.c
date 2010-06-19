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
#include "stdbool.h"
#include "string.h"
#include "stdio.h"
#include "panic.h"
#include "mmu-arm.h"
#include "system.h"
#define LOGF_ENABLE
#include "logf.h"
#include "usb-drv-as3525v2.h"
#include "usb_core.h"

static int __in_ep_list[USB_NUM_IN_EP] = {USB_IN_EP_LIST};
static int __out_ep_list[USB_NUM_OUT_EP] = {USB_OUT_EP_LIST};

/* iterate through each in/out ep except EP0
 * 'counter' is the counter, 'ep' is the actual value */
#define FOR_EACH_IN_EP(counter, ep) \
    for(counter = 0, ep = __in_ep_list[0]; counter < USB_NUM_IN_EP; counter++, ep = __in_ep_list[counter])

#define FOR_EACH_OUT_EP(counter, ep) \
    for(counter = 0, ep = __out_ep_list[0]; counter < USB_NUM_OUT_EP; counter++, ep = __out_ep_list[counter])

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
static struct usb_ctrlrequest ep0_setup_pkt;

void usb_attach(void)
{
    logf("usb: attach");
    usb_enable(true);
}

static void usb_delay(void)
{
    int i = 0;
    while(i < 0x300)
    {
        asm volatile("nop");
        i++;
    }
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
    USB_DCTL |= USB_DCTL_pwronprgdone;
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
    USB_GUSBCFG &= ~USB_GUSBCFG_force_host_mode;
    USB_GUSBCFG |= USB_GUSBCFG_force_device_mode;
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

static void usb_enable_device_interrupts(void)
{
    /* Clear any pending interrupt */
    USB_GINTSTS = 0xffffffff;
    /* Clear any pending otg interrupt */
    USB_GOTGINT = 0xffffffff;
    /* Enable interrupts */
    USB_GINTMSK = USB_GINTMSK_usbreset
                | USB_GINTMSK_enumdone
                | USB_GINTMSK_inepintr
                | USB_GINTMSK_outepintr
                | USB_GINTMSK_otgintr
                | USB_GINTMSK_disconnect;
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
    udelay(1);
}

static void usb_flush_rx_fifo(void)
{
    unsigned int i = 0;
    
    USB_GRSTCTL = USB_GRSTCTL_rxfflsh_flush;
    while(USB_GRSTCTL & USB_GRSTCTL_rxfflsh_flush && i < 0x300)
        i++;
    if(USB_GRSTCTL & USB_GRSTCTL_rxfflsh_flush)
        panicf("usb: hang of flush rx fifo");
    /* wait 3 phy clocks */
    udelay(1);
}

static void core_reset(void)
{
    unsigned int i = 0;
    /* Wait for AHB master IDLE state. */
    while((USB_GRSTCTL & USB_GRSTCTL_ahbidle) == 0)
        udelay(10);
    /* Core Soft Reset */
    USB_GRSTCTL |= USB_GRSTCTL_csftrst;
    /* Waits for the hardware to clear reset bit */
    while(USB_GRSTCTL & USB_GRSTCTL_csftrst && i < 0x300)
        i++;

    if(USB_GRSTCTL & USB_GRSTCTL_csftrst)
        panicf("oops, usb core soft reset hang :(");

    /* Wait for 3 PHY Clocks */
    udelay(1);
}

static void reset_endpoints(void)
{
    int i, ep;
    /* disable all endpoints except EP0 */
    FOR_EACH_IN_EP(i, ep)
        if(USB_DIEPCTL(ep) & USB_DEPCTL_epena)
            USB_DIEPCTL(ep) =  USB_DEPCTL_epdis | USB_DEPCTL_snak;
        else
            USB_DIEPCTL(ep) = 0;
    
    FOR_EACH_OUT_EP(i, ep)
        if(USB_DOEPCTL(ep) & USB_DEPCTL_epena)
            USB_DOEPCTL(ep) =  USB_DEPCTL_epdis | USB_DEPCTL_snak;
        else
            USB_DOEPCTL(ep) = 0;
    /* Setup EP0 OUT with the following parameters:
     * packet count = 1
     * setup packet count = 1
     * transfer size = 64
     * Setup EP0 IN/OUT with 64 byte maximum packet size and activate both. Enable transfer on EP0 OUT
     */
    
    USB_DOEPTSIZ(0) = (1 << USB_DEPTSIZ0_supcnt_bit_pos)
                    | (1 << USB_DEPTSIZ0_pkcnt_bit_pos)
                    | 8;

    /* setup DMA */
    clean_dcache_range((void*)&ep0_setup_pkt, sizeof ep0_setup_pkt);  /* force write back */
    USB_DOEPDMA(0) = (unsigned long)&ep0_setup_pkt; /* virtual address=physical address */

    /* Enable endpoint, clear nak */
    USB_DOEPCTL(0) = USB_DEPCTL_epena | USB_DEPCTL_cnak | USB_DEPCTL_usbactep
                    | (USB_DEPCTL_MPS_8 << USB_DEPCTL_mps_bit_pos);

    /* 64 bytes packet size, active endpoint */
    USB_DIEPCTL(0) = (USB_DEPCTL_MPS_8 << USB_DEPCTL_mps_bit_pos)
                    | USB_DEPCTL_usbactep;

    USB_DCTL = USB_DCTL_cgnpinnak | USB_DCTL_cgoutnak;
}

static void core_dev_init(void)
{
    unsigned int usb_num_in_ep = 0;
    unsigned int usb_num_out_ep = 0;
    unsigned int i;
    /* Restart the phy clock */
    USB_PCGCCTL = 0;
    /* Set phy speed : high speed */
    USB_DCFG = (USB_DCFG & ~USB_DCFG_devspd_bits) | USB_DCFG_devspd_hs_phy_hs;
    
    /* Check hardware capabilities */
    if(USB_GHWCFG2_ARCH != USB_INT_DMA_ARCH)
        panicf("usb: wrong architecture (%ld)", USB_GHWCFG2_ARCH);
    if(USB_GHWCFG2_HS_PHY_TYPE != USB_PHY_TYPE_UTMI)
        panicf("usb: wrong HS phy type (%ld)", USB_GHWCFG2_HS_PHY_TYPE);
    if(USB_GHWCFG2_FS_PHY_TYPE != USB_PHY_TYPE_UNSUPPORTED)
        panicf("usb: wrong FS phy type (%ld)", USB_GHWCFG2_FS_PHY_TYPE);
    if(USB_GHWCFG4_UTMI_PHY_DATA_WIDTH != 0x2)
        panicf("usb: wrong utmi data width (%ld)", USB_GHWCFG4_UTMI_PHY_DATA_WIDTH);
    if(USB_GHWCFG4_DED_FIFO_EN != 1) /* it seems to be multiple tx fifo support */
        panicf("usb: no multiple tx fifo");

    #ifdef USB_USE_CUSTOM_FIFO_LAYOUT
    if(USB_GHWCFG2_DYN_FIFO != 1)
        panicf("usb: no dynamic fifo");
    if(USB_GRXFSIZ != USB_DATA_FIFO_DEPTH)
        panicf("usb: wrong data fifo size");
    #endif /* USB_USE_CUSTOM_FIFO_LAYOUT */

    /* do some logging */
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

    if(adr > USB_DATA_FIFO_DEPTH)
        panicf("usb: total data fifo size exceeded");
    #endif /* USB_USE_CUSTOM_FIFO_LAYOUT */
    
    for(i = 1; i <= USB_NUM_IN_EP; i++)
    {
        logf("  dieptx fifo(%2u): [%04lx,+%4lx]", i,
            USB_GET_FIFOSIZE_START_ADR(USB_DIEPTXFSIZ(i)),
            USB_GET_FIFOSIZE_DEPTH(USB_DIEPTXFSIZ(i)));
    }

    /* Setup interrupt masks for endpoints */
    /* Setup interrupt masks */
    USB_DOEPMSK = USB_DOEPINT_setup | USB_DOEPINT_xfercompl | USB_DOEPINT_ahberr
                | USB_DOEPINT_epdisabled;
    USB_DIEPMSK = USB_DIEPINT_xfercompl | USB_DIEPINT_timeout
                | USB_DIEPINT_epdisabled | USB_DIEPINT_ahberr;
    USB_DAINTMSK = 0xffffffff;

    reset_endpoints();

    /* fixme: threshold tweaking only takes place if we use multiple tx fifos it seems */
    /* only dump them for now, leave threshold disabled */
    /*
    logf("threshold control:");
    logf("  non_iso_thr_en: %d", (USB_DTHRCTL & USB_DTHRCTL_non_iso_thr_en) ? 1 : 0);
    logf("  iso_thr_en: %d", (USB_DTHRCTL & USB_DTHRCTL_iso_thr_en) ? 1 : 0);
    logf("  tx_thr_len: %lu", (USB_DTHRCTL & USB_DTHRCTL_tx_thr_len_bits) >> USB_DTHRCTL_tx_thr_len_bit_pos);
    logf("  rx_thr_en: %d", (USB_DTHRCTL & USB_DTHRCTL_rx_thr_en) ? 1 : 0);
    logf("  rx_thr_len: %lu", (USB_DTHRCTL & USB_DTHRCTL_rx_thr_len_bits) >> USB_DTHRCTL_rx_thr_len_bit_pos);
    */

    USB_DTHRCTL = 0;

    /* enable USB interrupts */
    usb_enable_device_interrupts();
}

static void core_init(void)
{
    /* Disconnect */
    USB_DCTL |= USB_DCTL_sftdiscon;
    /* Select UTMI+ 16 */
    USB_GUSBCFG |= USB_GUSBCFG_phy_if;
    
    /* fixme: the current code is for internal DMA only, the clip+ architecture
     *        define the internal DMA model */
    /* Set burstlen and enable DMA*/
    USB_GAHBCFG = (USB_GAHBCFG_INT_DMA_BURST_INCR4 << USB_GAHBCFG_hburstlen_bit_pos)
                | USB_GAHBCFG_dma_enable;
    /* Disable HNP and SRP, not sure it's useful because we already forced dev mode */
    USB_GUSBCFG &= ~(USB_GUSBCFG_srpcap | USB_GUSBCFG_hnpcapp);

    /* perform device model specific init */
    core_dev_init();
    
    /* Reconnect */
    USB_DCTL &= ~USB_DCTL_sftdiscon;
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

static bool handle_usb_reset(void)
{
    logf("usb: bus reset");

    /* Clear the Remote Wakeup Signalling */
    USB_DCTL &= ~USB_DCTL_rmtwkupsig;

    /* Flush FIFOs */
    usb_flush_tx_fifos(0x10);

    /* Flush the Learning Queue */
    USB_GRSTCTL = USB_GRSTCTL_intknqflsh;

    reset_endpoints();

    /* Reset Device Address */
    USB_DCFG &= ~USB_DCFG_devadr_bits;

    usb_core_bus_reset();

    return true;
}

static bool handle_enum_done(void)
{
    logf("usb: enum done");

    /* read speed */
    logf("DSTS: %lx", USB_DSTS);
    logf("DOEPCTL0=%lx", USB_DOEPCTL(0));
    logf("DOEPTSIZ=%lx", USB_DOEPTSIZ(0));
    logf("DIEPCTL0=%lx", USB_DIEPCTL(0));
    logf("DOEPMSK=%lx", USB_DOEPMSK);
    logf("DIEPMSK=%lx", USB_DIEPMSK);
    logf("DAINTMSK=%lx", USB_DAINTMSK);
    logf("DAINT=%lx", USB_DAINT);
    logf("GINTSTS=%lx", USB_GINTSTS);
    logf("GINTMSK=%lx", USB_GINTMSK);
    logf("DCTL=%lx", USB_DCTL);
    logf("GAHBCFG=%lx", USB_GAHBCFG);
    logf("GUSBCFG=%lx", USB_GUSBCFG);
    logf("DCFG=%lx", USB_DCFG);
    logf("DTHRCTL=%lx", USB_DTHRCTL);

    switch((USB_DSTS & USB_DSTS_enumspd_bits) >> USB_DSTS_enumspd_bit_pos)
    {
        case USB_DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ:
            logf("usb: HS");
            break;
        case USB_DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ:
        case USB_DSTS_ENUMSPD_FS_PHY_48MHZ:
            logf("usb: FS");
            break;
        case USB_DSTS_ENUMSPD_LS_PHY_6MHZ:
            panicf("usb: LS is not supported");
    }

    USB_DOEPCTL(0) = (USB_DOEPCTL(0) & ~USB_DEPCTL_mps_bits)
                | (USB_DEPCTL_MPS_64 << USB_DEPCTL_mps_bit_pos);
    USB_DIEPCTL(0) = (USB_DIEPCTL(0) & ~USB_DEPCTL_mps_bits)
                | (USB_DEPCTL_MPS_64 << USB_DEPCTL_mps_bit_pos);

    unsigned i, ep;
    FOR_EACH_IN_EP(i, ep)
        USB_DIEPCTL(ep) = (USB_DIEPCTL(ep) & ~USB_DEPCTL_mps_bits)
                | (512 << USB_DEPCTL_mps_bit_pos);

    FOR_EACH_OUT_EP(i, ep)
        USB_DOEPCTL(ep) = (USB_DOEPCTL(ep) & ~USB_DEPCTL_mps_bits)
                | (512 << USB_DEPCTL_mps_bit_pos);

    USB_DOEPTSIZ(0) = (1 << USB_DEPTSIZ0_supcnt_bit_pos)
                    | (1 << USB_DEPTSIZ0_pkcnt_bit_pos)
                    | 64;
    
    return true;
}

static bool handle_in_ep_int(void)
{
    panicf("usb: in ep int");
    return false;
}

static bool handle_out_ep_int(void)
{
    panicf("usb: out ep int");
    return false;
}

static void dump_intsts(char *buffer, size_t size, unsigned long sts)
{
    (void) size;
    buffer[0] = 0;
    #define DUMP_CASE(name) \
        if(sts & USB_GINTMSK_##name) strcat(buffer, #name " ");

    DUMP_CASE(modemismatch)
    DUMP_CASE(otgintr)
    DUMP_CASE(sofintr)
    DUMP_CASE(rxstsqlvl)
    DUMP_CASE(nptxfempty)
    DUMP_CASE(ginnakeff)
    DUMP_CASE(goutnakeff)
    DUMP_CASE(i2cintr)
    DUMP_CASE(erlysuspend)
    DUMP_CASE(usbsuspend)
    DUMP_CASE(usbreset)
    DUMP_CASE(enumdone)
    DUMP_CASE(isooutdrop)
    DUMP_CASE(eopframe)
    DUMP_CASE(epmismatch)
    DUMP_CASE(inepintr)
    DUMP_CASE(outepintr)
    DUMP_CASE(incomplisoin)
    DUMP_CASE(incomplisoout)
    DUMP_CASE(portintr)
    DUMP_CASE(hcintr)
    DUMP_CASE(ptxfempty)
    DUMP_CASE(conidstschng)
    DUMP_CASE(disconnect)
    DUMP_CASE(sessreqintr)
    DUMP_CASE(wkupintr)
    
    buffer[strlen(buffer) - 1] = 0;
}

/* interrupt service routine */
void INT_USB(void)
{
    /* some bits in GINTSTS can be set even though we didn't enable the interrupt source
     * so AND it with the actual mask */
    unsigned long sts = USB_GINTSTS & USB_GINTMSK;
    unsigned long handled_one = 0; /* mask of all listed one (either handled or not) */
    
    #define HANDLED_CASE(bitmask, callfn) \
        handled_one |= bitmask; \
        if(sts & bitmask) \
        { \
            if(!callfn()) \
                goto Lerr; \
        }

    #define UNHANDLED_CASE(bitmask) \
        handled_one |= bitmask; \
        if(sts & bitmask) \
            goto Lunhandled;

    /* device part */
    HANDLED_CASE(USB_GINTMSK_usbreset, handle_usb_reset)
    HANDLED_CASE(USB_GINTMSK_enumdone, handle_enum_done)
    /*
    HANDLED_CASE(USB_GINTMSK_inepintr, handle_in_ep_int)
    HANDLED_CASE(USB_GINTMSK_outepintr, handle_out_ep_int)
    */
    UNHANDLED_CASE(USB_GINTMSK_outepintr)
    UNHANDLED_CASE(USB_GINTMSK_inepintr)

    /* common part */
    UNHANDLED_CASE(USB_GINTMSK_otgintr)
    UNHANDLED_CASE(USB_GINTMSK_conidstschng)
    UNHANDLED_CASE(USB_GINTMSK_disconnect)

    /* unlisted ones */
    if(sts & ~handled_one)
        goto Lunhandled;

    USB_GINTSTS = USB_GINTSTS;

    return;

    Lunhandled:
    panicf("unhandled usb int: %lx", sts);

    Lerr:
    panicf("error in usb int: %lx", sts);
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

