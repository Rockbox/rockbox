/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Michael Sparmann
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

#include "config.h"
#include "usb.h"
#include "usb_drv.h"

#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "panic.h"

#include "usb-s3c6400x.h"

#include "usb_ch9.h"
#include "usb_core.h"
#include <inttypes.h>
#include "power.h"

//#define LOGF_ENABLE
#include "logf.h"

#if CONFIG_CPU == AS3525v2
#define UNCACHED_ADDR AS3525_UNCACHED_ADDR
#define PHYSICAL_ADDR AS3525_PHYSICAL_ADDR
static inline void discard_dma_buffer_cache(void) {}
#else
#define UNCACHED_ADDR
#define PHYSICAL_ADDR
static inline void discard_dma_buffer_cache(void) { commit_discard_dcache(); }
#endif

/* store per endpoint, per direction, information */
struct ep_type
{
    unsigned int size; /* length of the data buffer */
    struct semaphore complete; /* wait object */
    int8_t status; /* completion status (0 for success) */
    bool active; /* true is endpoint has been requested (true for EP0) */
    bool done; /* transfer completed */
    bool busy; /* true is a transfer is pending */
};

static const uint8_t in_ep_list[] = {0, 1, 3, 5};
static const uint8_t out_ep_list[] = {0, 2, 4};

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
static struct ep_type endpoints[USB_NUM_ENDPOINTS][2];
/* setup packet for EP0 */

/* USB control requests may be up to 64 bytes in size.
   Even though we never use anything more than the 8 header bytes,
   we are required to accept request packets of up to 64 bytes size.
   Provide buffer space for these additional payload bytes so that
   e.g. write descriptor requests (which are rejected by us, but the
   payload is transferred anyway) do not cause memory corruption.
   Fixes FS#12310. -- Michael Sparmann (theseven) */
static union {
    struct usb_ctrlrequest header; /* 8 bytes */
    unsigned char payload[64];
} _ep0_setup_pkt USB_DEVBSS_ATTR;

static struct usb_ctrlrequest *ep0_setup_pkt = UNCACHED_ADDR(&_ep0_setup_pkt.header);

/* state of EP0 */
static enum ep0state ep0_state;

bool usb_drv_stalled(int endpoint, bool in)
{
    return DEPCTL(endpoint, !in) & DEPCTL_stall;
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
    if (stall)
        DEPCTL(endpoint, !in) |= DEPCTL_stall;
    else
        DEPCTL(endpoint, !in) &= ~DEPCTL_stall;
}

void usb_drv_set_address(int address)
{
    (void)address;
    /* Ignored intentionally, because the controller requires us to set the
       new address before sending the response for some reason. So we'll
       already set it when the control request arrives, before passing that
       into the USB core, which will then call this dummy function. */
}

static void ep_transfer(int ep, void *ptr, int len, bool out)
{
    /* disable interrupts to avoid any race */
    int oldlevel = disable_irq_save();

    struct ep_type *endpoint = &endpoints[ep][out ? DIR_OUT : DIR_IN];
    endpoint->busy   = true;
    endpoint->size   = len;
    endpoint->status = -1;

    if (out)
        DEPCTL(ep, out) &= ~DEPCTL_stall;

    int mps = usb_drv_port_speed() ? 512 : 64;
    int nb_packets = (len + mps - 1) / mps;
    if (nb_packets == 0)
        nb_packets = 1;

    DEPDMA(ep, out) = len ? (void*)PHYSICAL_ADDR(ptr) : NULL;
    DEPTSIZ(ep, out) = (nb_packets << DEPTSIZ_pkcnt_bitp) | len;
    if(out)
        discard_dcache_range(ptr, len);
    else
        commit_dcache_range(ptr, len);

    logf("pkt=%d dma=%lx", nb_packets, DEPDMA(ep, out));

//    if (!out) while (((GNPTXSTS & 0xffff) << 2) < MIN(mps, length));

    DEPCTL(ep, out) |= DEPCTL_epena | DEPCTL_cnak;

    restore_irq(oldlevel);
}

int usb_drv_send_nonblocking(int endpoint, void *ptr, int length)
{
    ep_transfer(EP_NUM(endpoint), ptr, length, false);
    return 0;
}

int usb_drv_recv(int endpoint, void* ptr, int length)
{
    ep_transfer(EP_NUM(endpoint), ptr, length, true);
    return 0;
}

int usb_drv_port_speed(void)
{
    static const uint8_t speed[4] = {
        [DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ] = 1,
        [DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ] = 0,
        [DSTS_ENUMSPD_FS_PHY_48MHZ]          = 0,
        [DSTS_ENUMSPD_LS_PHY_6MHZ]           = 0,
    };

    unsigned enumspd = extract(DSTS, enumspd);

    if(enumspd == DSTS_ENUMSPD_LS_PHY_6MHZ)
        panicf("usb-drv: LS is not supported");

    return speed[enumspd & 3];
}

void usb_drv_set_test_mode(int mode)
{
    /* there is a perfect matching between usb test mode code
     * and the register field value */
    DCTL = (DCTL & ~bitm(DCTL, tstctl)) | (mode << DCTL_tstctl_bitp);
}

void usb_attach(void)
{
    usb_enable(true); // s5l only ?
    /* Nothing to do */
}

static void prepare_setup_ep0(void)
{
    DEPDMA(0, true) = (void*)PHYSICAL_ADDR(&_ep0_setup_pkt);
    DEPTSIZ(0, true) = (1 << DEPTSIZ0_supcnt_bitp)
                | (1 << DEPTSIZ0_pkcnt_bitp)
                | 8;
    DEPCTL(0, true) |= DEPCTL_epena | DEPCTL_cnak;

    ep0_state = EP0_WAIT_SETUP;
}

static size_t num_eps(bool out)
{
    return out ? sizeof(out_ep_list) : sizeof(in_ep_list);
}

static void reset_endpoints(void)
{
    for (int dir = 0; dir < 2; dir++)
    {
        bool out = dir == DIR_OUT;
        for (unsigned i = 0; i < num_eps(dir == DIR_OUT); i++)
        {
            int ep = ((dir == DIR_IN) ? in_ep_list : out_ep_list)[i];
            struct ep_type *endpoint = &endpoints[ep][out];
            endpoint->active = false;
            endpoint->busy   = false;
            endpoint->status = -1;
            endpoint->done   = true;
            semaphore_release(&endpoint->complete);

            if (i != 0)
                DEPCTL(ep, out) = DEPCTL_setd0pid;
        }
        DEPCTL(0, out) = /*(DEPCTL_MPS_64 << DEPCTL_mps_bitp) | */ DEPCTL_usbactep;
    }

    /* Setup next chain for IN eps */
    for (unsigned i = 0; i < num_eps(false); i++)
    {
        int ep = in_ep_list[i];
        int next_ep = in_ep_list[(i + 1) % num_eps(false)];
        DEPCTL(ep, false) |= next_ep << DEPCTL_nextep_bitp;
    }

    prepare_setup_ep0();
}

static void cancel_all_transfers(bool cancel_ep0)
{
    int flags = disable_irq_save();

    for (int dir = 0; dir < 2; dir++)
        for (unsigned i = !!cancel_ep0; i < num_eps(dir == DIR_OUT); i++)
        {
            int ep = ((dir == DIR_IN) ? in_ep_list : out_ep_list)[i];
            struct ep_type *endpoint = &endpoints[ep][dir == DIR_OUT];
            endpoint->status = -1;
            endpoint->busy   = false;
            endpoint->done   = false;
            semaphore_release(&endpoint->complete);
            DEPCTL(ep, dir) = (DEPCTL(ep, dir) & ~DEPCTL_usbactep);
        }

    restore_irq(flags);
}

#if CONFIG_CPU == AS3525v2
void usb_drv_init(void)
{
    for (int i = 0; i < USB_NUM_ENDPOINTS; i++)
        for (int dir = 0; dir < 2; dir++)
            semaphore_init(&endpoints[i][dir].complete, 1, 0);

    bitset32(&CGU_PERI, CGU_USB_CLOCK_ENABLE);
    CCU_USB = (CCU_USB & ~(3<<24)) | (1 << 24); /* ?? */
    /* PHY clock */
    CGU_USB = 1<<5  /* enable */
        | 0 << 2
        | 0; /* source = ? (24MHz crystal?) */

    PCGCCTL = 0;
    DCTL = DCTL_pwronprgdone | DCTL_sftdiscon;

    GRSTCTL = GRSTCTL_csftrst;
    while (GRSTCTL & GRSTCTL_csftrst);  /* Wait for OTG to ack reset */
    while (!(GRSTCTL & GRSTCTL_ahbidle));  /* Wait for OTG AHB master idle */

    GRXFSIZ = 512;
    GNPTXFSIZ = MAKE_FIFOSIZE_DATA(512);

    /* FIXME: the current code is for internal DMA only, the clip+ architecture
     *        defines the internal DMA model */
    GAHBCFG = (GAHBCFG_INT_DMA_BURST_INCR << GAHBCFG_hburstlen_bitp)
                | GAHBCFG_dma_enable | GAHBCFG_glblintrmsk;

    /* Select UTMI+ 16 */
    GUSBCFG = GUSBCFG_force_device_mode | GUSBCFG_phy_if | 7 << GUSBCFG_toutcal_bitp;

    /* Do something that is probably CCU related but undocumented*/
    CCU_USB |= 0x1000;
    CCU_USB &= ~0x300000;

    DCFG = DCFG_nzstsouthshk | DCFG_devspd_hs_phy_hs; /* Address 0, high speed */
    DCTL = DCTL_pwronprgdone;

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

    if(USB_NUM_ENDPOINTS != extract(GHWCFG2, num_ep))
        panicf("usb-drv: wrong endpoint number");

    for (int dir = 0; dir < 2; dir++)
        for (unsigned i = 0; i < num_eps(dir == DIR_OUT); i++)
        {
            int ep = ((dir == DIR_IN) ? in_ep_list : out_ep_list)[i];
            int type = (GHWCFG1 >> GHWCFG1_epdir_bitp(ep)) & GHWCFG1_epdir_bits;
            int flag = (dir == DIR_IN) ? GHWCFG1_EPDIR_IN : GHWCFG1_EPDIR_OUT;
            if(type != GHWCFG1_EPDIR_BIDIR && type != flag)
                panicf("usb-drv: EP%d not in correct direction", ep);
        }

    DOEPMSK = DEPINT_xfercompl | DEPINT_ahberr | DOEPINT_setup;
    DIEPMSK = DEPINT_xfercompl | DEPINT_ahberr | DIEPINT_timeout;
    DAINTMSK = 0xffffffff;

    reset_endpoints();

    GINTMSK = GINTMSK_usbreset
            | GINTMSK_enumdone
            | GINTMSK_inepintr
            | GINTMSK_outepintr
            | GINTMSK_disconnect;

    VIC_INT_ENABLE = INTERRUPT_USB;
}

void usb_drv_exit(void)
{
    DCTL = DCTL_pwronprgdone | DCTL_sftdiscon;

    VIC_INT_EN_CLEAR = INTERRUPT_USB;

    sleep(HZ/20);

    CGU_USB = 0;
    bitclr32(&CGU_PERI, CGU_USB_CLOCK_ENABLE);
}
#elif CONFIG_CPU == S5L8701 || CONFIG_CPU == S5L8702
static void usb_reset(void)
{
    DCTL = DCTL_pwronprgdone | DCTL_sftdiscon;

    OPHYPWR = 0;  /* PHY: Power up */
    udelay(10);
    OPHYUNK1 = 1;
    OPHYUNK2 = 0xE3F;
    ORSTCON = 1;  /* PHY: Assert Software Reset */
    udelay(10);
    ORSTCON = 0;  /* PHY: Deassert Software Reset */
    OPHYUNK3 = 0x600;
    OPHYCLK = SYNOPSYSOTG_CLOCK;
    udelay(400);

    GRSTCTL = GRSTCTL_csftrst;  /* OTG: Assert Software Reset */
    while (GRSTCTL & GRSTCTL_csftrst);  /* Wait for OTG to ack reset */
    while (!(GRSTCTL & GRSTCTL_ahbidle));  /* Wait for OTG AHB master idle */

    GRXFSIZ = 1024;
    GNPTXFSIZ = (256 << 16) | 1024;

    GAHBCFG = SYNOPSYSOTG_AHBCFG;
    GUSBCFG = (1 << 12) | (1 << 10) | GUSBCFG_phy_if; /* OTG: 16bit PHY and some reserved bits */

    DCFG = DCFG_nzstsouthshk;  /* Address 0 */
    DCTL = DCTL_pwronprgdone;  /* Soft Reconnect */
    DIEPMSK = DIEPINT_timeout | DEPINT_ahberr | DEPINT_xfercompl;
    DOEPMSK = DOEPINT_setup | DEPINT_ahberr | DEPINT_xfercompl;
    DAINTMSK = 0xFFFFFFFF;  /* Enable interrupts on all endpoints */
    GINTMSK = GINTMSK_outepintr | GINTMSK_inepintr | GINTMSK_usbreset | GINTMSK_enumdone;

    reset_endpoints();
}

void usb_drv_init(void)
{
    for (unsigned i = 0; i < sizeof(endpoints)/(2*sizeof(struct ep_type)); i++)
        for (unsigned dir = 0; dir < 2; dir++)
            semaphore_init(&endpoints[i][dir].complete, 1, 0);

    /* Enable USB clock */
#if CONFIG_CPU==S5L8701
    PWRCON &= ~0x4000;
    PWRCONEXT &= ~0x800;
    INTMSK |= INTMSK_USB_OTG;
#elif CONFIG_CPU==S5L8702
    PWRCON(0) &= ~0x4;
    PWRCON(1) &= ~0x8;
    VIC0INTENABLE |= 1 << 19;
#endif
    PCGCCTL = 0;

    /* reset the beast */
    usb_reset();
}

void usb_drv_exit(void)
{
    DCTL = DCTL_pwronprgdone | DCTL_sftdiscon;

    OPHYPWR = 0xF;  /* PHY: Power down */
    udelay(10);
    ORSTCON = 7;  /* Put the PHY into reset (needed to get current down) */
    udelay(10);
    PCGCCTL = 1;  /* Shut down PHY clock */
    
#if CONFIG_CPU==S5L8701
    PWRCON |= 0x4000;
    PWRCONEXT |= 0x800;
#elif CONFIG_CPU==S5L8702
    PWRCON(0) |= 0x4;
    PWRCON(1) |= 0x8;
#endif
}
#endif

static void handle_ep_int(int ep, bool out)
{
    unsigned long sts = DEPINT(ep, out);
    struct ep_type *endpoint = &endpoints[ep][out ? DIR_OUT : DIR_IN];

    logf("%s(%d %s): sts = 0x%lx", __func__, ep, out?"OUT":"IN", sts);

    if(sts & DEPINT_ahberr)
        panicf("usb-drv: ahb error on EP%d %s", ep, out ? "OUT" : "IN");

    if(sts & DEPINT_xfercompl)
    {
        discard_dma_buffer_cache();
        if(endpoint->busy)
        {
            endpoint->busy = false;
            endpoint->status = 0;
            /* works even for EP0 */
            int size = (DEPTSIZ(ep, out) & DEPTSIZ_xfersize_bits);
            int transfered = endpoint->size - size;
            if(ep == 0)
            {
                bool is_ack = endpoint->size == 0;
                switch(ep0_state)
                {
                case EP0_WAIT_SETUP:
                    panicf("usb-drv: EP0 completion while waiting for SETUP");
                case EP0_WAIT_DATA_ACK:
                    ep0_state = is_ack ? EP0_WAIT_DATA : EP0_WAIT_ACK;
                    break;
                case EP0_WAIT_ACK:
                case EP0_WAIT_DATA:
                    if((!is_ack && ep0_state == EP0_WAIT_ACK) || (is_ack && ep0_state == EP0_WAIT_DATA))
                        panicf("usb-drv: bad EP0 state");

                    prepare_setup_ep0();
                    break;
                }
            }
            if (!out)
                endpoint->size = size;
            usb_core_transfer_complete(ep, out ? USB_DIR_OUT : USB_DIR_IN, 0, transfered);
            endpoint->done = true;
            semaphore_release(&endpoint->complete);
        }
    }

    if(!out && (sts & DIEPINT_timeout)) {
        if (endpoint->busy)
        {
            endpoint->busy = false;
            endpoint->status = 1;
            endpoint->done = true;
            semaphore_release(&endpoint->complete);
        }
    }

    if(out && (sts & DOEPINT_setup))
    {
        discard_dma_buffer_cache();
        if(ep != 0)
            panicf("usb-drv: setup not on EP0, this is impossible");
        if((DEPTSIZ(ep, true) & DEPTSIZ_xfersize_bits) != 0)
        {
            logf("usb-drv: ignore spurious setup (xfersize=%ld)", DOEPTSIZ(ep) & DEPTSIZ_xfersize_bits);
            prepare_setup_ep0();
        }
        else
        {
            if(ep0_state == EP0_WAIT_SETUP)
            {
                bool data_phase = ep0_setup_pkt->wLength != 0;
                ep0_state = data_phase ? EP0_WAIT_DATA_ACK : EP0_WAIT_ACK;
            }

            logf("  rt=%x r=%x", ep0_setup_pkt->bRequestType, ep0_setup_pkt->bRequest);

            if(ep0_setup_pkt->bRequestType == USB_TYPE_STANDARD &&
               ep0_setup_pkt->bRequest     == USB_REQ_SET_ADDRESS)
                DCFG = (DCFG & ~bitm(DCFG, devadr)) | (ep0_setup_pkt->wValue << DCFG_devadr_bitp);

            usb_core_control_request(ep0_setup_pkt);
        }
    }

    DEPINT(ep, out) = sts;
}

void INT_USB_FUNC(void)
{
    /* some bits in GINTSTS can be set even though we didn't enable the interrupt source
     * so AND it with the actual mask */
    unsigned long sts = GINTSTS & GINTMSK;
    logf("usb-drv: INT 0x%lx", sts);

    if(sts & GINTMSK_usbreset)
    {
        DCFG &= ~bitm(DCFG, devadr); /* Address 0 */
        reset_endpoints();
        usb_core_bus_reset();
    }

    if(sts & GINTMSK_enumdone)  /* enumeration done, we now know the speed */
    {
        /* Set up the maximum packet sizes accordingly */
        uint32_t maxpacket = (usb_drv_port_speed() ? 512 : 64) << DEPCTL_mps_bitp;
        for (int dir = 0; dir < 2; dir++)
        {
            bool out = dir == DIR_OUT;
            for (unsigned i = 1; i < num_eps(out); i++)
            {
                int ep = (out ? out_ep_list : in_ep_list)[i];
                DEPCTL(ep, out) &= ~(DEPCTL_mps_bits << DEPCTL_mps_bitp);
                DEPCTL(ep, out) |= maxpacket;
            }
        }
    }

    if(sts & (GINTMSK_outepintr | GINTMSK_inepintr))
    {
        unsigned long daint = DAINT;

        for (int i = 0; i < USB_NUM_ENDPOINTS; i++)
        {
            if (daint & DAINT_IN_EP(i))
                handle_ep_int(i, false);
            if (daint & DAINT_OUT_EP(i))
                handle_ep_int(i, true);
        }

        DAINT = daint;
    }

    if(sts & GINTMSK_disconnect)
        cancel_all_transfers(true);

    GINTSTS = sts;
}

int usb_drv_request_endpoint(int type, int dir)
{
    bool out = dir == USB_DIR_OUT;
    for (unsigned i = 1; i < num_eps(out); i++)
    {
        int ep = (out ? out_ep_list : in_ep_list)[i];
        bool *active = &endpoints[ep][out ? DIR_OUT : DIR_IN].active;
        if(*active)
            continue;
        *active = true;
        DEPCTL(ep, out) = (DEPCTL(ep, out) & ~(DEPCTL_eptype_bits << DEPCTL_eptype_bitp))
            | DEPCTL_setd0pid | (type << DEPCTL_eptype_bitp) | DEPCTL_usbactep;
        return ep | dir;
    }

    return -1;
}

void usb_drv_release_endpoint(int ep)
{
    if ((ep & 0x7f) == 0)
        return;
    endpoints[EP_NUM(ep)][EP_DIR(ep)].active = false;
}

void usb_drv_cancel_all_transfers()
{
    cancel_all_transfers(false);
}


int usb_drv_send(int ep, void *ptr, int len)
{
    ep = EP_NUM(ep);
    struct ep_type *endpoint = &endpoints[ep][1];
    endpoint->done = false;
    ep_transfer(ep, ptr, len, false);
    while (endpoint->busy && !endpoint->done)
        semaphore_wait(&endpoint->complete, TIMEOUT_BLOCK);
    return endpoint->status;
}
