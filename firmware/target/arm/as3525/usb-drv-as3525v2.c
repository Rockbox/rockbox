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
//#define LOGF_ENABLE
#include "logf.h"
#include "usb-drv-as3525v2.h"
#include "usb_core.h"

static const uint8_t in_ep_list[] = {0, 3, 5};
static const uint8_t out_ep_list[] = {0, 2, 4};

/* store per endpoint, per direction, information */
struct usb_endpoint
{
    unsigned int len; /* length of the data buffer */
    struct semaphore complete; /* wait object */
    int8_t status; /* completion status (0 for success) */
    bool active; /* true is endpoint has been requested (true for EP0) */
    bool done; /* transfer completed */
    bool busy; /* true is a transfer is pending */
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

static struct usb_ctrlrequest *ep0_setup_pkt = AS3525_UNCACHED_ADDR(&_ep0_setup_pkt.header);

/* state of EP0 */
static enum ep0state ep0_state;

void usb_attach(void)
{
    /* Nothing to do */
}

static void prepare_setup_ep0(void)
{
    DEPDMA(0, true) = (void*)AS3525_PHYSICAL_ADDR(&_ep0_setup_pkt);
    DEPTSIZ(0, true) = (1 << DEPTSIZ0_supcnt_bitp)
                | (1 << DEPTSIZ0_pkcnt_bitp)
                | 8;
    DEPCTL(0, true) |= DEPCTL_epena | DEPCTL_cnak;

    ep0_state = EP0_WAIT_SETUP;
}

static void reset_endpoints(void)
{
    for (int dir = 0; dir < 2; dir++)
        for (unsigned i = 0; i < sizeof((dir == DIR_IN) ? in_ep_list : out_ep_list); i++)
        {
            int ep = ((dir == DIR_IN) ? in_ep_list : out_ep_list)[i];
            endpoints[ep][dir == DIR_OUT] = (struct usb_endpoint) {
                .active = false,
                .busy   = false,
                .status = -1,
                .done   = false,
            };
            semaphore_release(&endpoints[ep][dir == DIR_OUT].complete);

            if (i != 0)
                DEPCTL(ep, dir == DIR_OUT) = (DEPCTL(ep, dir == DIR_OUT) & DEPCTL_epena)
                    ? DEPCTL_snak
                    : 0;
            else
                DEPCTL(0, dir == DIR_OUT)  = (DEPCTL_MPS_64 << DEPCTL_mps_bitp) | DEPCTL_usbactep | DEPCTL_snak;
        }

    /* Setup next chain for IN eps */
    for (unsigned i = 0; i < sizeof(in_ep_list); i++)
    {
        int ep = in_ep_list[i];
        int next_ep = in_ep_list[(i + 1) % sizeof(in_ep_list)];
        DEPCTL(ep, false) = (DEPCTL(ep, false) & ~bitm(DEPCTL, nextep)) | (next_ep << DEPCTL_nextep_bitp);
    }

    prepare_setup_ep0();
}

static void cancel_all_transfers(bool cancel_ep0)
{
    int flags = disable_irq_save();

    for (int dir = 0; dir < 2; dir++)
        for (unsigned i = !!cancel_ep0; i < sizeof((dir == DIR_IN) ? in_ep_list : out_ep_list); i++)
        {
            int ep = ((dir == DIR_IN) ? in_ep_list : out_ep_list)[i];
            endpoints[ep][dir == DIR_OUT] = (struct usb_endpoint) {
                .status = -1,
                .busy   = false,
                .done   = false,
            };
            semaphore_release(&endpoints[ep][dir == DIR_OUT].complete);
            DEPCTL(ep, dir) = (DEPCTL(ep, dir) & ~DEPCTL_usbactep) | DEPCTL_snak;
        }

    restore_irq(flags);
}

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
        for (unsigned i = 0; i < sizeof((dir == DIR_IN) ? in_ep_list : out_ep_list); i++)
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
    VIC_INT_EN_CLEAR = INTERRUPT_USB;

    DCTL = DCTL_pwronprgdone | DCTL_sftdiscon;
    sleep(HZ/20);

    CGU_USB = 0;
    bitclr32(&CGU_PERI, CGU_USB_CLOCK_ENABLE);
}

static void handle_ep_int(int ep, bool out)
{
    unsigned long sts = DEPINT(ep, out);
    logf("%s(%d %s): sts = 0x%lx", __func__, ep, out?"OUT":"IN", sts);

    if(sts & DEPINT_ahberr)
        panicf("usb-drv: ahb error on EP%d %s", ep, out ? "OUT" : "IN");

    if(sts & DEPINT_xfercompl)
    {
        struct usb_endpoint *endpoint = &endpoints[ep][out ? DIR_OUT : DIR_IN];
        if(endpoint->busy)
        {
            endpoint->busy = false;
            endpoint->status = 0;
            /* works even for EP0 */
            int size = (DEPTSIZ(ep, out) & DEPTSIZ_xfersize_bits);
            int transfered = endpoint->len - size;
            if(ep == 0)
            {
                bool is_ack = endpoint->len == 0;
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
                endpoint->len = size;
            usb_core_transfer_complete(ep, out ? USB_DIR_OUT : USB_DIR_IN, 0, transfered);
            endpoint->done = true;
            semaphore_release(&endpoint->complete);
        }
    }

    if(!out && (sts & DIEPINT_timeout))
        panicf("usb-drv: timeout on EP%d IN", ep);

    if(out && (sts & DOEPINT_setup))
    {
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

void INT_USB(void)
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

static unsigned long usb_drv_mps_by_type(int type)
{
    static const uint16_t mps[4][2] = {
        /*         type                 fs      hs   */
        [USB_ENDPOINT_XFER_CONTROL] = { 64,     64   },
        [USB_ENDPOINT_XFER_ISOC]    = { 1023,   1024 },
        [USB_ENDPOINT_XFER_BULK]    = { 64,     512  },
        [USB_ENDPOINT_XFER_INT]     = { 64,     1024 },
    };
    return mps[type & 3][usb_drv_port_speed() & 1];
}

int usb_drv_request_endpoint(int type, int dir)
{
    for (unsigned i = 1; i < sizeof((dir == USB_DIR_IN) ? in_ep_list : out_ep_list); i++)
    {
        int ep = ((dir == USB_DIR_IN) ? in_ep_list : out_ep_list)[i];
        bool *active = &endpoints[ep][(dir == USB_DIR_IN) ? DIR_IN : DIR_OUT].active;
        if(*active)
            continue;
        *active = true;
        DEPCTL(ep, dir != USB_DIR_IN) = (DEPCTL(ep, dir != USB_DIR_IN) & ~(bitm(DEPCTL, eptype) | bitm(DEPCTL, mps)))
            | DEPCTL_setd0pid | (type << DEPCTL_eptype_bitp)
            | (usb_drv_mps_by_type(type) << DEPCTL_mps_bitp) | DEPCTL_usbactep | DEPCTL_snak;
        return ep | dir;
    }

    return -1;
}

void usb_drv_release_endpoint(int ep)
{
    endpoints[EP_NUM(ep)][EP_DIR(ep)].active = false;
}

void usb_drv_cancel_all_transfers()
{
    cancel_all_transfers(false);
}

static void usb_drv_transfer(int ep, void *ptr, int len, bool dir_in)
{
    /* disable interrupts to avoid any race */
    int oldlevel = disable_irq_save();

    endpoints[ep][dir_in] = (struct usb_endpoint) {
        .busy   = true,
        .len    = len,
        .status = -1,
    };

    DEPCTL(ep, !dir_in) = (DEPCTL(ep, !dir_in) & ~DEPCTL_stall) | DEPCTL_usbactep;

    int type = (DEPCTL(ep, !dir_in) >> DEPCTL_eptype_bitp) & DEPCTL_eptype_bits;
    int mps = usb_drv_mps_by_type(type);
    int nb_packets = (len + mps - 1) / mps;
    if (nb_packets == 0)
        nb_packets = 1;

    DEPDMA(ep, !dir_in) = len
        ? (void*)AS3525_PHYSICAL_ADDR(ptr)
        : (void*)0x10000000;
    DEPTSIZ(ep, !dir_in) = (nb_packets << DEPTSIZ_pkcnt_bitp) | len;
    if(dir_in)
        clean_dcache_range(ptr, len);
    else
        dump_dcache_range(ptr, len);

    logf("pkt=%d dma=%lx", nb_packets, DEPDMA(ep, !dir_in));

    DEPCTL(ep, !dir_in) |= DEPCTL_epena | DEPCTL_cnak;

    restore_irq(oldlevel);
}

int usb_drv_recv(int ep, void *ptr, int len)
{
    usb_drv_transfer(EP_NUM(ep), ptr, len, false);
    return 0;
}

int usb_drv_send(int ep, void *ptr, int len)
{
    ep = EP_NUM(ep);
    struct usb_endpoint *endpoint = &endpoints[ep][1];
    endpoint->done = false;
    usb_drv_transfer(ep, ptr, len, true);
    while (endpoint->busy && !endpoint->done)
        semaphore_wait(&endpoint->complete, TIMEOUT_BLOCK);
    return endpoint->status;
}

int usb_drv_send_nonblocking(int ep, void *ptr, int len)
{
    usb_drv_transfer(EP_NUM(ep), ptr, len, true);
    return 0;
}


void usb_drv_set_test_mode(int mode)
{
    /* there is a perfect matching between usb test mode code
     * and the register field value */
    DCTL = (DCTL & ~bitm(DCTL, tstctl)) | (mode << DCTL_tstctl_bitp);
}

void usb_drv_set_address(int address)
{
    (void) address;
}

void usb_drv_stall(int ep, bool stall, bool in)
{
    if (stall)
        DEPCTL(ep, !in) |=  DEPCTL_stall;
    else
        DEPCTL(ep, !in) &= ~DEPCTL_stall;
}

bool usb_drv_stalled(int ep, bool in)
{
    return DEPCTL(ep, !in) & DEPCTL_stall;
}
