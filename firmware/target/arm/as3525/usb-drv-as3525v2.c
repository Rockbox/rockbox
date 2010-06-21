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

static int __in_ep_list[NUM_IN_EP] = {IN_EP_LIST};
static int __out_ep_list[NUM_OUT_EP] = {OUT_EP_LIST};

/* iterate through each in/out ep except EP0
 * 'counter' is the counter, 'ep' is the actual value */
#define FOR_EACH_IN_EP(counter, ep) \
    for(counter = 0, ep = __in_ep_list[0]; counter < NUM_IN_EP; counter++, ep = __in_ep_list[counter])

#define FOR_EACH_OUT_EP(counter, ep) \
    for(counter = 0, ep = __out_ep_list[0]; counter < NUM_OUT_EP; counter++, ep = __out_ep_list[counter])

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
static struct usb_ctrlrequest ep0_setup_pkt __attribute__((aligned(16)));

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
    /* PHY clock */
    #if 1
    CGU_USB = 1<<5 /* enable */
        | (CLK_DIV(AS3525_PLLA_FREQ, 60000000)) << 2
        | 1; /* source = PLLA */
    #else
    CGU_USB = 0x20;
    #endif
    usb_delay();
    /* 3) clear "stop pclk" */
    PCGCCTL &= ~0x1;
    usb_delay();
    /* 4) clear "power clamp" */
    PCGCCTL &= ~0x4;
    usb_delay();
    /* 5) clear "reset power down module" */
    PCGCCTL &= ~0x8;
    usb_delay();
    /* 6) set "power on program done" */
    DCTL |= DCTL_pwronprgdone;
    usb_delay();
    /* 7) core soft reset */
    GRSTCTL |= GRSTCTL_csftrst;
    usb_delay();
    /* 8) hclk soft reset */
    GRSTCTL |= GRSTCTL_hsftrst;
    usb_delay();
    /* 9) flush and reset everything */
    GRSTCTL |= 0x3f;
    usb_delay();
    /* 10) force device mode*/
    GUSBCFG &= ~GUSBCFG_force_host_mode;
    GUSBCFG |= GUSBCFG_force_device_mode;
    usb_delay();
    /* 11) Do something that is probably CCU related but undocumented*/
    CCU_USB_THINGY &= ~0x1000;
    usb_delay();
    CCU_USB_THINGY &= ~0x300000;
    usb_delay();
    /* 12) reset usb core parameters (dev addr, speed, ...) */
    DCFG = 0;
    usb_delay();
}

static void enable_device_interrupts(void)
{
    /* Clear any pending interrupt */
    GINTSTS = 0xffffffff;
    /* Clear any pending otg interrupt */
    GOTGINT = 0xffffffff;
    /* Enable interrupts */
    GINTMSK = GINTMSK_usbreset
                | GINTMSK_enumdone
                | GINTMSK_inepintr
                | GINTMSK_outepintr
                | GINTMSK_otgintr
                | GINTMSK_disconnect;
}

static void flush_tx_fifos(int nums)
{
    unsigned int i = 0;
    
    GRSTCTL = (nums << GRSTCTL_txfnum_bitp)
            | GRSTCTL_txfflsh_flush;
    while(GRSTCTL & GRSTCTL_txfflsh_flush && i < 0x300)
        i++;
    if(GRSTCTL & GRSTCTL_txfflsh_flush)
        panicf("usb: hang of flush tx fifos (%x)", nums);
    /* wait 3 phy clocks */
    udelay(1);
}

static void reset_endpoints(void)
{
    int i, ep;
    /* disable all endpoints except EP0 */
    FOR_EACH_IN_EP(i, ep)
        if(DIEPCTL(ep) & DEPCTL_epena)
            DIEPCTL(ep) =  DEPCTL_epdis | DEPCTL_snak;
        else
            DIEPCTL(ep) = 0;
    
    FOR_EACH_OUT_EP(i, ep)
        if(DOEPCTL(ep) & DEPCTL_epena)
            DOEPCTL(ep) =  DEPCTL_epdis | DEPCTL_snak;
        else
            DOEPCTL(ep) = 0;
    /* Setup EP0 OUT with the following parameters:
     * packet count = 1
     * setup packet count = 1
     * transfer size = 64
     * Setup EP0 IN/OUT with 64 byte maximum packet size and activate both. Enable transfer on EP0 OUT
     */
    
    DOEPTSIZ(0) = (1 << DEPTSIZ0_supcnt_bitp)
                | (1 << DEPTSIZ0_pkcnt_bitp)
                | 8;

    /* setup DMA */
    clean_dcache_range((void*)&ep0_setup_pkt, sizeof ep0_setup_pkt);  /* force write back */
    DOEPDMA(0) = (unsigned long)&ep0_setup_pkt; /* virtual address=physical address */

    /* Enable endpoint, clear nak */
    DOEPCTL(0) = DEPCTL_epena | DEPCTL_cnak | DEPCTL_usbactep
                | (DEPCTL_MPS_64 << DEPCTL_mps_bitp);

    /* 64 bytes packet size, active endpoint */
    DIEPCTL(0) = (DEPCTL_MPS_64 << DEPCTL_mps_bitp)
                | DEPCTL_usbactep;
}

static void core_dev_init(void)
{
    unsigned int num_in_ep = 0;
    unsigned int num_out_ep = 0;
    unsigned int i;
    /* Restart the phy clock */
    PCGCCTL = 0;
    /* Set phy speed : high speed */
    DCFG = (DCFG & ~bitm(DCFG, devspd)) | DCFG_devspd_hs_phy_hs;
    
    /* Check hardware capabilities */
    if(extract(GHWCFG2, arch) != GHWCFG2_ARCH_INTERNAL_DMA)
        panicf("usb: wrong architecture (%ld)", extract(GHWCFG2, arch));
    if(extract(GHWCFG2, hs_phy_type) != GHWCFG2_PHY_TYPE_UTMI)
        panicf("usb: wrong HS phy type (%ld)", extract(GHWCFG2, hs_phy_type));
    if(extract(GHWCFG2, fs_phy_type) != GHWCFG2_PHY_TYPE_UNSUPPORTED)
        panicf("usb: wrong FS phy type (%ld)", extract(GHWCFG2, fs_phy_type));
    if(extract(GHWCFG4, utmi_phy_data_width) != 0x2)
        panicf("usb: wrong utmi data width (%ld)", extract(GHWCFG4, utmi_phy_data_width));
    if(!(GHWCFG4 & GHWCFG4_ded_fifo_en)) /* it seems to be multiple tx fifo support */
        panicf("usb: no multiple tx fifo");

    #ifdef USE_CUSTOM_FIFO_LAYOUT
    if(!(GHWCFG2 & GHWCFG2_dyn_fifo))
        panicf("usb: no dynamic fifo");
    if(GRXFSIZ != DATA_FIFO_DEPTH)
        panicf("usb: wrong data fifo size");
    #endif /* USE_CUSTOM_FIFO_LAYOUT */

    /* do some logging */
    logf("hwcfg1: %08lx", GHWCFG1);
    logf("hwcfg2: %08lx", GHWCFG2);
    logf("hwcfg3: %08lx", GHWCFG3);
    logf("hwcfg4: %08lx", GHWCFG4);

    logf("%ld endpoints", extract(GHWCFG2, num_ep));
    num_in_ep = 0;
    num_out_ep = 0;
    for(i = 0; i < extract(GHWCFG2, num_ep); i++)
    {
        bool in = false, out = false;
        switch((GHWCFG1 >> GHWCFG1_epdir_bitp(i)) & GHWCFG1_epdir_bits)
        {
            case GHWCFG1_EPDIR_BIDIR: in = out = true; break;
            case GHWCFG1_EPDIR_IN: in = true; break;
            case GHWCFG1_EPDIR_OUT: out = true; break;
            default: panicf("usb: invalid epdir");
        }
        /* don't count EP0 which is special and always bidirectional */
        if(in && i != 0)
            num_in_ep++;
        if(out && i != 0)
            num_out_ep++;
        logf("  EP%d: IN=%s OUT=%s", i, in ? "yes" : "no", out ? "yes" : "no");
    }

    if(num_in_ep != extract(GHWCFG4, num_in_ep))
        panicf("usb: num in ep mismatch(%d,%lu)", num_in_ep, extract(GHWCFG4, num_in_ep));
    if(num_in_ep != NUM_IN_EP)
        panicf("usb: num in ep static mismatch(%u,%u)", num_in_ep, NUM_IN_EP);
    if(num_out_ep != NUM_OUT_EP)
        panicf("usb: num out ep static mismatch(%u,%u)", num_out_ep, NUM_OUT_EP);

    logf("%d in ep, %d out ep", num_in_ep, num_out_ep);
    
    logf("initial:");
    logf("  tot fifo sz: %lx", extract(GHWCFG3, dfifo_len));
    logf("  rx fifo: [%04x,+%4lx]", 0, GRXFSIZ);
    logf("  nptx fifo: [%04lx,+%4lx]", GET_FIFOSIZE_START_ADR(GNPTXFSIZ),
        GET_FIFOSIZE_DEPTH(GNPTXFSIZ));

    #ifdef USE_CUSTOM_FIFO_LAYOUT
    /* Setup FIFOs */
    /* Organize FIFO as follow:
     *             0           ->         rxfsize         : RX fifo
     *          rxfsize        ->   rxfsize + nptxfsize   : TX fifo for first IN ep
     *   rxfsize + nptxfsize   -> rxfsize + 2 * nptxfsize : TX fifo for second IN ep
     * rxfsize + 2 * nptxfsize -> rxfsize + 3 * nptxfsize : TX fifo for third IN ep
     * ...
     */

    unsigned short adr = 0;
    unsigned short depth = RX_FIFO_SIZE;
    GRXFSIZ = depth;
    adr += depth;
    depth = NPTX_FIFO_SIZE;
    GNPTXFSIZ = MAKE_FIFOSIZE_DATA(adr, depth);
    adr += depth;

    for(i = 1; i <= NUM_IN_EP; i++)
    {
        depth = EPTX_FIFO_SIZE;
        DIEPTXFSIZ(i) = MAKE_FIFOSIZE_DATA(adr, depth);
        adr += depth;
    }

    if(adr > DATA_FIFO_DEPTH)
        panicf("usb: total data fifo size exceeded");
    #endif /* USE_CUSTOM_FIFO_LAYOUT */
    
    for(i = 1; i <= NUM_IN_EP; i++)
    {
        logf("  dieptx fifo(%2u): [%04lx,+%4lx]", i,
            GET_FIFOSIZE_START_ADR(DIEPTXFSIZ(i)),
            GET_FIFOSIZE_DEPTH(DIEPTXFSIZ(i)));
    }

    /* Setup interrupt masks for endpoints */
    /* Setup interrupt masks */
    DOEPMSK = DOEPINT_setup | DOEPINT_xfercompl | DOEPINT_ahberr
                | DOEPINT_epdisabled;
    DIEPMSK = DIEPINT_xfercompl | DIEPINT_timeout
                | DIEPINT_epdisabled | DIEPINT_ahberr;
    DAINTMSK = 0xffffffff;

    reset_endpoints();

    /* fixme: threshold tweaking only takes place if we use multiple tx fifos it seems */
    /* only dump them for now, leave threshold disabled */
    /*
    logf("threshold control:");
    logf("  non_iso_thr_en: %d", (DTHRCTL & DTHRCTL_non_iso_thr_en) ? 1 : 0);
    logf("  iso_thr_en: %d", (DTHRCTL & DTHRCTL_iso_thr_en) ? 1 : 0);
    logf("  tx_thr_len: %lu", extract(DTHRCTL, tx_thr_len));
    logf("  rx_thr_en: %d", (DTHRCTL & DTHRCTL_rx_thr_en) ? 1 : 0);
    logf("  rx_thr_len: %lu", extract(DTHRCTL, rx_thr_len));
    */

    /* enable USB interrupts */
    enable_device_interrupts();
}

static void core_init(void)
{
    /* Disconnect */
    DCTL |= DCTL_sftdiscon;
    /* Select UTMI+ 16 */
    GUSBCFG |= GUSBCFG_phy_if;
    
    /* fixme: the current code is for internal DMA only, the clip+ architecture
     *        define the internal DMA model */
    /* Set burstlen and enable DMA*/
    GAHBCFG = (GAHBCFG_INT_DMA_BURST_INCR4 << GAHBCFG_hburstlen_bitp)
                | GAHBCFG_dma_enable;
    /* Disable HNP and SRP, not sure it's useful because we already forced dev mode */
    GUSBCFG &= ~(GUSBCFG_srpcap | GUSBCFG_hnpcapp);

    /* perform device model specific init */
    core_dev_init();
    
    /* Reconnect */
    DCTL &= ~DCTL_sftdiscon;
}

static void enable_global_interrupts(void)
{
    VIC_INT_ENABLE = INTERRUPT_USB;
    GAHBCFG |= GAHBCFG_glblintrmsk;
}

static void disable_global_interrupts(void)
{
    GAHBCFG &= ~GAHBCFG_glblintrmsk;
    VIC_INT_EN_CLEAR = INTERRUPT_USB;
}

void usb_drv_init(void)
{
    logf("usb_drv_init");
    /* Enable PHY and clocks (but leave pullups disabled) */
    as3525v2_connect();
    logf("usb: synopsis id: %lx", GSNPSID);
    /* Core init */
    core_init();
    /* Enable global interrupts */
    enable_global_interrupts();
}

void usb_drv_exit(void)
{
    logf("usb_drv_exit");

    disable_global_interrupts();
}

static void dump_regs(void)
{
    logf("DSTS: %lx", DSTS);
    logf("DOEPCTL0=%lx", DOEPCTL(0));
    logf("DOEPTSIZ=%lx", DOEPTSIZ(0));
    logf("DIEPCTL0=%lx", DIEPCTL(0));
    logf("DOEPMSK=%lx", DOEPMSK);
    logf("DIEPMSK=%lx", DIEPMSK);
    logf("DAINTMSK=%lx", DAINTMSK);
    logf("DAINT=%lx", DAINT);
    logf("GINTSTS=%lx", GINTSTS);
    logf("GINTMSK=%lx", GINTMSK);
    logf("DCTL=%lx", DCTL);
    logf("GAHBCFG=%lx", GAHBCFG);
    logf("GUSBCFG=%lx", GUSBCFG);
    logf("DCFG=%lx", DCFG);
    logf("DTHRCTL=%lx", DTHRCTL);
}

static bool handle_reset(void)
{
    logf("usb: bus reset");

    dump_regs();
    /* Clear the Remote Wakeup Signalling */
    DCTL &= ~DCTL_rmtwkupsig;

    /* Flush FIFOs */
    flush_tx_fifos(0x10);

    reset_endpoints();

    /* Reset Device Address */
    DCFG &= bitm(DCFG, devadr);

    usb_core_bus_reset();

    return true;
}

static bool handle_enum_done(void)
{
    logf("usb: enum done");

    /* read speed */
    switch(extract(DSTS, enumspd))
    {
        case DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ:
            logf("usb: HS");
            break;
        case DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ:
        case DSTS_ENUMSPD_FS_PHY_48MHZ:
            logf("usb: FS");
            break;
        case DSTS_ENUMSPD_LS_PHY_6MHZ:
            panicf("usb: LS is not supported");
    }

    /* fixme: change EP0 mps here */
    dump_regs();
    
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

/* interrupt service routine */
void INT_USB(void)
{
    /* some bits in GINTSTS can be set even though we didn't enable the interrupt source
     * so AND it with the actual mask */
    unsigned long sts = GINTSTS & GINTMSK;
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
    HANDLED_CASE(GINTMSK_usbreset, handle_reset)
    HANDLED_CASE(GINTMSK_enumdone, handle_enum_done)
    HANDLED_CASE(GINTMSK_inepintr, handle_in_ep_int)
    HANDLED_CASE(GINTMSK_outepintr, handle_out_ep_int)

    /* common part */
    UNHANDLED_CASE(GINTMSK_otgintr)
    UNHANDLED_CASE(GINTMSK_conidstschng)
    UNHANDLED_CASE(GINTMSK_disconnect)

    /* unlisted ones */
    if(sts & ~handled_one)
        goto Lunhandled;

    GINTSTS = GINTSTS;

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

