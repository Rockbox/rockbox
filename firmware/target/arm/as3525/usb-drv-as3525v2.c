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
static int __in_ep_list_ep0[NUM_IN_EP + 1] = {0, IN_EP_LIST};
static int __out_ep_list_ep0[NUM_OUT_EP + 1] = {0, OUT_EP_LIST};

/* iterate through each in/out ep except EP0
 * 'counter' is the counter, 'ep' is the actual value */
#define FOR_EACH_EP(list, size, counter, ep) \
    for(counter = 0, ep = (list)[0]; \
        counter < (size); \
        counter++, ep = (list)[counter])
 
#define FOR_EACH_IN_EP_EX(include_ep0, counter, ep) \
    FOR_EACH_EP(include_ep0 ? __in_ep_list_ep0 : __in_ep_list, \
        include_ep0 ? NUM_IN_EP + 1: NUM_IN_EP, counter, ep)

#define FOR_EACH_OUT_EP_EX(include_ep0, counter, ep) \
    FOR_EACH_EP(include_ep0 ? __out_ep_list_ep0 : __out_ep_list, \
        include_ep0 ? NUM_OUT_EP + 1: NUM_OUT_EP, counter, ep)
 
#define FOR_EACH_IN_EP(counter, ep) \
    FOR_EACH_IN_EP_EX(false, counter, ep)

#define FOR_EACH_IN_EP_AND_EP0(counter, ep) \
    FOR_EACH_IN_EP_EX(true, counter, ep)

#define FOR_EACH_OUT_EP(counter, ep) \
    FOR_EACH_OUT_EP_EX(false, counter, ep)

#define FOR_EACH_OUT_EP_AND_EP0(counter, ep) \
    FOR_EACH_OUT_EP_EX(true, counter, ep)

/* store per endpoint, per direction, information */
struct usb_endpoint
{
    bool active; /* true is endpoint has been requested (true for EP0) */
    unsigned int len; /* length of the data buffer */
    bool wait; /* true if usb thread is blocked on completion */
    bool busy; /* true is a transfer is pending */
    int status; /* completion status (0 for success) */
    struct wakeup complete; /* wait object */
};

/* state of EP0 (to correctly schedule setup packet enqueing) */
enum ep0state
{
    /* Setup packet is enqueud, waiting for actual data */
    EP0_WAIT_SETUP = 0,
    /* Waiting for ack (either IN or OUT) */
    EP0_WAIT_ACK = 1,
    /* Ack complete, waiting for data (either IN or OUT)
     * This state is necessary because if both ack and data complete in the
     * same interrupt, we might process data completion before ack completion
     * so we need this bizarre state */
    EP0_WAIT_DATA = 2,
    /* Setup packet complete, waiting for ack and data */
    EP0_WAIT_DATA_ACK = 3,
};

/* endpoints[ep_num][DIR_IN/DIR_OUT] */
static struct usb_endpoint endpoints[USB_NUM_ENDPOINTS][2];
/* setup packet for EP0 */
static struct usb_ctrlrequest ep0_setup_pkt USB_DEVBSS_ATTR;
/* state of EP0 */
static enum ep0state ep0_state;

void usb_attach(void)
{
    logf("usb-drv: attach");
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
    logf("usb-drv: init as3525v2");
    /* 1) enable usb core clock */
    bitset32(&CGU_PERI, CGU_USB_CLOCK_ENABLE);
    usb_delay();
    /* 2) enable usb phy clock */
    /* PHY clock */
    CGU_USB = 1<<5 /* enable */
        | (CLK_DIV(AS3525_PLLA_FREQ, 60000000)) << 2
        | 1; /* source = PLLA */
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
    CCU_USB &= ~0x1000;
    usb_delay();
    CCU_USB &= ~0x300000;
    usb_delay();
    /* 12) reset usb core parameters (dev addr, speed, ...) */
    DCFG = 0;
    usb_delay();
}

static void as3525v2_disconnect(void)
{
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
        panicf("usb-drv: hang of flush tx fifos (%x)", nums);
    /* wait 3 phy clocks */
    udelay(1);
}

static void prepare_setup_ep0(void)
{
    logf("usb-drv: prepare EP0");
    /* setup DMA */
    clean_dcache_range((void*)&ep0_setup_pkt, sizeof ep0_setup_pkt);  /* force write back */
    DOEPDMA(0) = (unsigned long)&ep0_setup_pkt; /* virtual address=physical address */

    /* Setup EP0 OUT with the following parameters:
     * packet count = 1
     * setup packet count = 1
     * transfer size = 8 (setup packet)
     */
    DOEPTSIZ(0) = (1 << DEPTSIZ0_supcnt_bitp)
                | (1 << DEPTSIZ0_pkcnt_bitp)
                | 8;

    /* Enable endpoint, clear nak */
    DOEPCTL(0) |= DEPCTL_epena | DEPCTL_cnak;

    if(!(DOEPCTL(0) & DEPCTL_epena))
        panicf("usb-drv: failed to enable EP0 !");
    ep0_state = EP0_WAIT_SETUP;
}

static void handle_ep0_complete(bool is_ack)
{
    switch(ep0_state)
    {
        case EP0_WAIT_SETUP:
            panicf("usb-drv: EP0 completion while waiting for SETUP");
        case EP0_WAIT_ACK:
            if(is_ack)
                /* everything is done, prepare next setup */
                prepare_setup_ep0();
            else
                panicf("usb-drv: EP0 data completion while waiting for ACK");
            break;
        case EP0_WAIT_DATA:
            if(is_ack)
                panicf("usb-drv: EP0 ACK while waiting for data completion");
            else
                /* everything is done, prepare next setup */
                prepare_setup_ep0();
            break;
        case EP0_WAIT_DATA_ACK:
            /* update state */
            if(is_ack)
                ep0_state = EP0_WAIT_DATA;
            else
                ep0_state = EP0_WAIT_ACK;
            break;
        default:
            panicf("usb-drv: invalid EP0 state");
    }
    logf("usb-drv: EP0 state updated to %d", ep0_state);
}

static void handle_ep0_setup(void)
{
    if(ep0_state != EP0_WAIT_SETUP)
    {
        logf("usb-drv: EP0 SETUP while in state %d", ep0_state);
        DCTL |= DCTL_sftdiscon;
        return;
    }
    /* determine is there is a data phase */
    if(ep0_setup_pkt.wLength == 0)
        /* no: wait for ack */
        ep0_state = EP0_WAIT_ACK;
    else
        /* yes: wait ack and data */
        ep0_state = EP0_WAIT_DATA_ACK;
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

    /* 64 bytes packet size, active endpoint */
    DOEPCTL(0) = (DEPCTL_MPS_64 << DEPCTL_mps_bitp) | DEPCTL_usbactep | DEPCTL_snak;
    DIEPCTL(0) = (DEPCTL_MPS_64 << DEPCTL_mps_bitp) | DEPCTL_usbactep | DEPCTL_snak;
    
    prepare_setup_ep0();
}

static void cancel_all_transfers(bool cancel_ep0)
{
    logf("usb-drv: cancel all transfers");
    int flags = disable_irq_save();
    unsigned i, ep;
    FOR_EACH_IN_EP_EX(cancel_ep0, i, ep)
    {
        endpoints[ep][DIR_IN].status = 1;
        endpoints[ep][DIR_IN].wait = false;
        endpoints[ep][DIR_IN].busy = false;
        wakeup_signal(&endpoints[ep][DIR_IN].complete);
        DIEPCTL(ep) = (DIEPCTL(ep) & ~DEPCTL_usbactep) | DEPCTL_epdis | DEPCTL_snak;
    }
    FOR_EACH_OUT_EP_EX(cancel_ep0, i, ep)
    {
        endpoints[ep][DIR_OUT].status = 1;
        endpoints[ep][DIR_OUT].wait = false;
        endpoints[ep][DIR_OUT].busy = false;
        wakeup_signal(&endpoints[ep][DIR_OUT].complete);
        DOEPCTL(ep) = (DOEPCTL(ep) & ~DEPCTL_usbactep) | DEPCTL_epdis | DEPCTL_snak;
    }
    
    restore_irq(flags);
}

static void core_dev_init(void)
{
    unsigned int i, ep;
    /* Restart the phy clock */
    PCGCCTL = 0;
    /* Set phy speed : high speed */
    DCFG = (DCFG & ~bitm(DCFG, devspd)) | DCFG_devspd_hs_phy_hs;
    
    /* Check hardware capabilities */
    if(extract(GHWCFG2, arch) != GHWCFG2_ARCH_INTERNAL_DMA)
        panicf("usb-drv: wrong architecture (%ld)", extract(GHWCFG2, arch));
    if(extract(GHWCFG2, hs_phy_type) != GHWCFG2_PHY_TYPE_UTMI)
        panicf("usb-drv: wrong HS phy type (%ld)", extract(GHWCFG2, hs_phy_type));
    if(extract(GHWCFG2, fs_phy_type) != GHWCFG2_PHY_TYPE_UNSUPPORTED)
        panicf("usb-drv: wrong FS phy type (%ld)", extract(GHWCFG2, fs_phy_type));
    if(extract(GHWCFG4, utmi_phy_data_width) != 0x2)
        panicf("usb-drv: wrong utmi data width (%ld)", extract(GHWCFG4, utmi_phy_data_width));
    if(!(GHWCFG4 & GHWCFG4_ded_fifo_en)) /* it seems to be multiple tx fifo support */
        panicf("usb-drv: no multiple tx fifo");

    #ifdef USE_CUSTOM_FIFO_LAYOUT
    if(!(GHWCFG2 & GHWCFG2_dyn_fifo))
        panicf("usb-drv: no dynamic fifo");
    if(GRXFSIZ != DATA_FIFO_DEPTH)
        panicf("usb-drv: wrong data fifo size");
    #endif /* USE_CUSTOM_FIFO_LAYOUT */

    /* do some logging */
    /*
    logf("hwcfg1: %08lx", GHWCFG1);
    logf("hwcfg2: %08lx", GHWCFG2);
    logf("hwcfg3: %08lx", GHWCFG3);
    logf("hwcfg4: %08lx", GHWCFG4);
    */

    if(USB_NUM_ENDPOINTS != extract(GHWCFG2, num_ep))
        panicf("usb-drv: wrong endpoint number");

    FOR_EACH_IN_EP_AND_EP0(i, ep)
    {
        int type = (GHWCFG1 >> GHWCFG1_epdir_bitp(ep)) & GHWCFG1_epdir_bits;
        if(type != GHWCFG1_EPDIR_BIDIR && type != GHWCFG1_EPDIR_IN)
            panicf("usb-drv: EP%d is no IN or BIDIR", ep);
    }
    FOR_EACH_OUT_EP_AND_EP0(i, ep)
    {
        int type = (GHWCFG1 >> GHWCFG1_epdir_bitp(ep)) & GHWCFG1_epdir_bits;
        if(type != GHWCFG1_EPDIR_BIDIR && type != GHWCFG1_EPDIR_OUT)
            panicf("usb-drv: EP%d is no OUT or BIDIR", ep);
    }

    /* Setup interrupt masks for endpoints */
    /* Setup interrupt masks */
    DOEPMSK = DOEPINT_setup | DOEPINT_xfercompl | DOEPINT_ahberr;
    DIEPMSK = DIEPINT_xfercompl | DIEPINT_timeout | DIEPINT_ahberr;
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
    unsigned i, ep;
    logf("usb_drv_init");
    /* Boost cpu */
    cpu_boost(1);
    /* Enable PHY and clocks (but leave pullups disabled) */
    as3525v2_connect();
    logf("usb-drv: synopsis id: %lx", GSNPSID);
    /* Core init */
    core_init();
    FOR_EACH_IN_EP_AND_EP0(i, ep)
        wakeup_init(&endpoints[ep][DIR_IN].complete);
    FOR_EACH_OUT_EP_AND_EP0(i, ep)
        wakeup_init(&endpoints[ep][DIR_OUT].complete);
    /* Enable global interrupts */
    enable_global_interrupts();
}

void usb_drv_exit(void)
{
    logf("usb_drv_exit");

    disable_global_interrupts();
    as3525v2_disconnect();
    cpu_boost(0);
}

static void handle_ep_int(int ep, bool dir_in)
{
    struct usb_endpoint *endpoint = &endpoints[ep][dir_in];
    if(dir_in)
    {
        if(DIEPINT(ep) & DIEPINT_ahberr)
            panicf("usb-drv: ahb error on EP%d IN", ep);
        if(DIEPINT(ep) & DIEPINT_xfercompl)
        {
            logf("usb-drv: xfer complete on EP%d IN", ep);
            if(endpoint->busy)
            {
                endpoint->busy = false;
                endpoint->status = 0;
                /* works even for PE0 */
                int transfered = endpoint->len - (DIEPTSIZ(ep) & DEPTSIZ_xfersize_bits);
                logf("len=%d reg=%ld xfer=%d", endpoint->len,
                    (DIEPTSIZ(ep) & DEPTSIZ_xfersize_bits),
                    transfered);
                invalidate_dcache_range((void *)DIEPDMA(ep), transfered);
                DIEPCTL(ep) |= DEPCTL_snak;
                /* handle EP0 state if necessary,
                 * this is a ack if length is 0 */
                if(ep == 0)
                    handle_ep0_complete(endpoint->len == 0);
                usb_core_transfer_complete(ep, USB_DIR_IN, 0, transfered);
                wakeup_signal(&endpoint->complete);
            }
        }
        if(DIEPINT(ep) & DIEPINT_timeout)
        {
            panicf("usb-drv: timeout on EP%d IN", ep);
            if(endpoint->busy)
            {
                endpoint->busy = false;
                endpoint->status = 1;
                /* for safety, act as if no bytes as been transfered */
                endpoint->len = 0;
                DIEPCTL(ep) |= DEPCTL_snak;
                usb_core_transfer_complete(ep, USB_DIR_IN, 1, 0);
                wakeup_signal(&endpoint->complete);
            }
        }
        /* clear interrupts */
        DIEPINT(ep) = DIEPINT(ep);
    }
    else
    {
        if(DOEPINT(ep) & DOEPINT_ahberr)
            panicf("usb-drv: ahb error on EP%d OUT", ep);
        if(DOEPINT(ep) & DOEPINT_xfercompl)
        {
            logf("usb-drv: xfer complete on EP%d OUT", ep);
            if(endpoint->busy)
            {
                endpoint->busy = false;
                endpoint->status = 0;
                /* works even for EP0 */
                int transfered = endpoint->len - (DOEPTSIZ(ep) & DEPTSIZ_xfersize_bits);
                logf("len=%d reg=%ld xfer=%d", endpoint->len,
                    (DOEPTSIZ(ep) & DEPTSIZ_xfersize_bits),
                    transfered);
                invalidate_dcache_range((void *)DOEPDMA(ep), transfered);
                /* handle EP0 state if necessary,
                 * this is a ack if length is 0 */
                if(ep == 0)
                    handle_ep0_complete(endpoint->len == 0);
                else
                    DOEPCTL(ep) |= DEPCTL_snak;
                usb_core_transfer_complete(ep, USB_DIR_OUT, 0, transfered);
                wakeup_signal(&endpoint->complete);
            }
        }
        if(DOEPINT(ep) & DOEPINT_setup)
        {
            logf("usb-drv: setup on EP%d OUT", ep);
            if(ep != 0)
                panicf("usb-drv: setup not on EP0, this is impossible");
            if((DOEPTSIZ(ep) & DEPTSIZ_xfersize_bits) != 0)
            {
                logf("usb-drv: ignore spurious setup (xfersize=%ld)", DOEPTSIZ(ep) & DEPTSIZ_xfersize_bits);
            }
            else
            {
                DOEPCTL(ep) |= DEPCTL_snak;
                logf("DOEPCTL0=%lx", DOEPCTL(ep));
                logf("DOEPTSIZE0=%lx", DOEPTSIZ(ep));
                if(DOEPDMA(ep) != 8 + (unsigned long)&ep0_setup_pkt)
                    panicf("usb-drv: EP0 wrong DMA adr (%lx vs %lx)", (unsigned long)&ep0_setup_pkt, DOEPDMA(ep));
                /* handle the set address here because of a bug in the usb core */
                invalidate_dcache_range((void*)&ep0_setup_pkt, sizeof ep0_setup_pkt);
                /* handle EP0 state */
                handle_ep0_setup();
                logf("  rt=%x r=%x", ep0_setup_pkt.bRequestType, ep0_setup_pkt.bRequest);
                if(ep0_setup_pkt.bRequestType == USB_TYPE_STANDARD &&
                        ep0_setup_pkt.bRequest == USB_REQ_SET_ADDRESS)
                    usb_drv_set_address(ep0_setup_pkt.wValue);
                usb_core_control_request(&ep0_setup_pkt);
            }
        }
        /* clear interrupts */
        DOEPINT(ep) = DOEPINT(ep);
    }
}

static void handle_ep_ints(void)
{
    logf("usb-drv: ep int");
    /* we must read it */
    unsigned long daint = DAINT;
    unsigned i, ep;

    FOR_EACH_IN_EP_AND_EP0(i, ep)
        if(daint & DAINT_IN_EP(ep))
            handle_ep_int(ep, true);
    FOR_EACH_OUT_EP_AND_EP0(i, ep)
        if(daint & DAINT_OUT_EP(ep))
            handle_ep_int(ep, false);

    /* write back to clear status */
    DAINT = daint;
}

/* interrupt service routine */
void INT_USB(void)
{
    /* some bits in GINTSTS can be set even though we didn't enable the interrupt source
     * so AND it with the actual mask */
    unsigned long sts = GINTSTS & GINTMSK;

    if(sts & GINTMSK_usbreset)
    {
        logf("usb-drv: bus reset");

        /* Clear the Remote Wakeup Signalling */
        //DCTL &= ~DCTL_rmtwkupsig;

        /* Flush FIFOs */
        flush_tx_fifos(0x10);

        reset_endpoints();

        /* Reset Device Address */
        DCFG &= ~bitm(DCFG, devadr);

        usb_core_bus_reset();
    }

    if(sts & GINTMSK_enumdone)
    {
        logf("usb-drv: enum done");

        /* read speed */
        if(usb_drv_port_speed())
            logf("usb-drv: HS");
        else
            logf("usb-drv: FS");

        /* fixme: change EP0 mps here */
        prepare_setup_ep0();
    }

    if(sts & (GINTMSK_outepintr | GINTMSK_inepintr))
    {
        handle_ep_ints();
    }

    if(sts & GINTMSK_disconnect)
    {
        panicf("usb-drv: disconnect");
        cancel_all_transfers(true);
        usb_enable(false);        
    }

    GINTSTS = GINTSTS;
}

int usb_drv_port_speed(void)
{
    switch(extract(DSTS, enumspd))
    {
        case DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ:
            return 1;
        case DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ:
        case DSTS_ENUMSPD_FS_PHY_48MHZ:
            return 0;
            break;
        case DSTS_ENUMSPD_LS_PHY_6MHZ:
            panicf("usb-drv: LS is not supported");
            return 0;
        default:
            panicf("usb-drv: wtf is this speed ?");
            return 0;
    }
}

int usb_drv_request_endpoint(int type, int dir)
{
    (void) type;
    (void) dir;
    logf("usb-drv: request endpoint (type=%d,dir=%s)", type, dir == USB_DIR_IN ? "IN" : "OUT");
    return -1;
}

void usb_drv_release_endpoint(int ep)
{
    //logf("usb-drv: release EP%d %s", EP_NUM(ep), EP_DIR(ep) == DIR_IN ? "IN" : "OUT");
    endpoints[EP_NUM(ep)][EP_DIR(ep)].active = false;
}

void usb_drv_cancel_all_transfers()
{
    cancel_all_transfers(false);
}

static int usb_drv_transfer(int ep, void *ptr, int len, bool dir_in, bool blocking)
{
    ep = EP_NUM(ep);
    logf("usb-drv: xfer EP%d, len=%d, dir_in=%d, blocking=%d", ep,
        len, dir_in, blocking);
    
    volatile unsigned long *epctl = dir_in ? &DIEPCTL(ep) : &DOEPCTL(ep);
    volatile unsigned long *eptsiz = dir_in ? &DIEPTSIZ(ep) : &DOEPTSIZ(ep);
    volatile unsigned long *epdma = dir_in ? &DIEPDMA(ep) : &DOEPDMA(ep);
    struct usb_endpoint *endpoint = &endpoints[ep][dir_in];
    #define DEPCTL  *epctl
    #define DEPTSIZ *eptsiz
    #define DEPDMA  *epdma

    if(DEPCTL & DEPCTL_stall)
    {
        logf("usb-drv: cannot receive/send on a stalled endpoint");
        return -1;
    }

    if(endpoint->busy)
        logf("usb-drv: EP%d %s is already busy", ep, dir_in ? "IN" : "OUT");
    
    endpoint->busy = true;
    endpoint->len = len;
    endpoint->wait = blocking;
    DEPCTL |= DEPCTL_usbactep;

    int mps = 64;
    int nb_packets = (len + mps - 1) / mps;

    if(len == 0)
        DEPTSIZ = 1 << DEPTSIZ_pkcnt_bitp;
    else
        DEPTSIZ = (nb_packets << DEPTSIZ_pkcnt_bitp) | len;
    clean_dcache_range(ptr, len);
    DEPDMA = (unsigned long)ptr;
    DEPCTL |= DEPCTL_epena | DEPCTL_cnak;

    /* fixme: check if endpoint was really enabled ? */
    if((DEPCTL & DEPCTL_epena) == 0)
        panicf("usb-drv: couldn't start xfer on EP%d %s", ep, dir_in ? "IN" : "OUT");
    
    if(blocking)
        wakeup_wait(&endpoint->complete, TIMEOUT_BLOCK);
    if(endpoint->status != 0)
        return -1;
    return 0;

    #undef DEPCTL
    #undef DEPTSIZ
    #undef DEPDMA
}

int usb_drv_recv(int ep, void *ptr, int len)
{
    return usb_drv_transfer(ep, ptr, len, false, false);
}

int usb_drv_send(int ep, void *ptr, int len)
{
    return usb_drv_transfer(ep, ptr, len, true, true);
}

int usb_drv_send_nonblocking(int ep, void *ptr, int len)
{
    return usb_drv_transfer(ep, ptr, len, true, false);
}


void usb_drv_set_test_mode(int mode)
{
    (void) mode;
}

void usb_drv_set_address(int address)
{
    /* ignore it if addres is already set */
    if(extract(DCFG, devadr) == 0)
    {
        logf("usb-drv: set address %x", address);
        DCFG = (DCFG & ~bitm(DCFG, devadr)) | (address << DCFG_devadr_bitp);
    }
}

void usb_drv_stall(int ep, bool stall, bool in)
{
    (void) ep;
    (void) stall;
    (void) in;
    logf("usb-drv: %sstall EP%d %s", stall ? "" : "un", ep, in ? "IN" : "OUT");
}

bool usb_drv_stalled(int ep, bool in)
{
    (void) ep;
    (void) in;
    return true;
}

