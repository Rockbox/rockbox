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

static void ep_transfer(int ep, void *ptr, int length, bool out);
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

#if CONFIG_CPU == AS3525v2 /* FIXME FIXME FIXME */
# include "as3525/usb-drv-as3525v2.c"
#else

static struct ep_type endpoints[USB_NUM_ENDPOINTS];

/* USB control requests may be up to 64 bytes in size.
   Even though we never use anything more than the 8 header bytes,
   we are required to accept request packets of up to 64 bytes size.
   Provide buffer space for these additional payload bytes so that
   e.g. write descriptor requests (which are rejected by us, but the
   payload is transferred anyway) do not cause memory corruption.
   Fixes FS#12310. -- Michael Sparmann (theseven) */
static union
{
    struct usb_ctrlrequest header; /* 8 bytes */
    unsigned char payload[64];
} ctrlreq USB_DEVBSS_ATTR;

static void reset_endpoints(int reinit)
{
    unsigned int i;
    for (i = 0; i < sizeof(endpoints)/sizeof(struct ep_type); i++)
    {
        if (reinit) endpoints[i].active = false;
        endpoints[i].busy = false;
        endpoints[i].status = -1;
        endpoints[i].done = true;
        semaphore_release(&endpoints[i].complete);
    }

    DEPCTL(0, false) = DEPCTL_usbactep | (1 << DEPCTL_nextep_bitp);
    DEPCTL(0, true) = DEPCTL_usbactep;
    DEPTSIZ(0, true) = (1 << DEPTSIZ_pkcnt_bitp) | (1 << DEPTSIZ0_supcnt_bitp) | 64;

    DEPDMA(0, true) = &ctrlreq;
    DEPCTL(0, true) |= DEPCTL_epena | DEPCTL_cnak;
    /* HACK: Enable all endpoints here, because we have no other chance to do it */
    if (reinit)
    {
        /* The size is getting set to zero, because we don't know
           whether we are Full Speed or High Speed at this stage */
        DEPCTL(1, false) = DEPCTL_usbactep | DEPCTL_setd0pid | (3 << DEPCTL_nextep_bitp);
        DEPCTL(2, true) = DEPCTL_usbactep | DEPCTL_setd0pid;
        DEPCTL(3, false) = DEPCTL_usbactep | DEPCTL_setd0pid | (0 << DEPCTL_nextep_bitp);
        DEPCTL(4, true) = DEPCTL_usbactep | DEPCTL_setd0pid;
    }
    else
    {
        DEPCTL(1, false) = DEPCTL(1, false) | DEPCTL_usbactep | DEPCTL_setd0pid;
        DEPCTL(2, true) = DEPCTL(2, true) | DEPCTL_usbactep | DEPCTL_setd0pid;
        DEPCTL(3, false) = DEPCTL(3, false) | DEPCTL_usbactep | DEPCTL_setd0pid;
        DEPCTL(4, true) = DEPCTL(4, true) | DEPCTL_usbactep | DEPCTL_setd0pid;
    }
    DAINTMSK = 0xFFFFFFFF;  /* Enable interrupts on all EPs */
}

int usb_drv_request_endpoint(int type, int dir)
{
    bool out = dir == USB_DIR_OUT;
    for(size_t ep = out ? 2 : 1; ep < USB_NUM_ENDPOINTS; ep += 2)
        if (!endpoints[ep].active)
        {
            endpoints[ep].active = true;
            DEPCTL(ep, out) = (DEPCTL(ep, out) & ~(DEPCTL_eptype_bits << DEPCTL_eptype_bitp)) |
                (type << DEPCTL_eptype_bitp);
            return ep | dir;
        }

    return -1;
}

void usb_drv_release_endpoint(int ep)
{
    ep = ep & 0x7f;

    if (ep < 1 || ep > USB_NUM_ENDPOINTS)
        return;

    endpoints[ep].active = false;
}

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

    GRXFSIZ = 512;
    GNPTXFSIZ = MAKE_FIFOSIZE_DATA(512);

    GAHBCFG = SYNOPSYSOTG_AHBCFG;
    GUSBCFG = (1 << 12) | (1 << 10) | GUSBCFG_phy_if; /* OTG: 16bit PHY and some reserved bits */

    DCFG = DCFG_nzstsouthshk;  /* Address 0 */
    DCTL = DCTL_pwronprgdone;  /* Soft Reconnect */
    DIEPMSK = DIEPINT_timeout | DEPINT_ahberr | DEPINT_xfercompl;
    DOEPMSK = DOEPINT_setup | DEPINT_ahberr | DEPINT_xfercompl;
    DAINTMSK = 0xFFFFFFFF;  /* Enable interrupts on all endpoints */
    GINTMSK = GINTMSK_outepintr | GINTMSK_inepintr | GINTMSK_usbreset | GINTMSK_enumdone;

    reset_endpoints(1);
}

static void handle_ep_int(bool out)
{
    static const uint8_t eps[2][3] = { /* IN */ {0, 1, 3}, /* OUT */ {0, 2, 4}};
    for (int i = 0; i < 3; i++)
    {
        int ep = eps[!!out][i];
        uint32_t epints = DEPINT(ep, out);
        if (!epints)
            continue;

        if (epints & DEPINT_xfercompl)
        {
            commit_discard_dcache();
            int bytes = endpoints[ep].size - (DEPTSIZ(ep, out) & (DEPTSIZ_xfersize_bits < DEPTSIZ_xfersize_bitp));
            if (endpoints[ep].busy)
            {
                endpoints[ep].busy = false;
                endpoints[ep].status = 0;
                endpoints[ep].done = true;
                usb_core_transfer_complete(ep, out ? USB_DIR_OUT : USB_DIR_IN, 0, bytes);
                semaphore_release(&endpoints[ep].complete);
            }
        }

        if (epints & DEPINT_ahberr)
            panicf("USB: AHB error on EP%d (dir %d)", ep, out);

        if (!out && (epints & DIEPINT_timeout))
        {
            if (endpoints[ep].busy)
            {
                endpoints[ep].busy = false;
                endpoints[ep].status = 1;
                endpoints[ep].done = true;
                semaphore_release(&endpoints[ep].complete);
            }
        }

        if (out && (epints & DOEPINT_setup))
        {
            commit_discard_dcache();
            if (ep != 0)
                panicf("USB: SETUP done on OUT EP%d!?", ep);

            /* Set the new address here, before passing the packet to the core.
               See usb_drv_set_address() for details. */
            if (ctrlreq.header.bRequest == USB_REQ_SET_ADDRESS)
                DCFG = (DCFG & ~(DCFG_devadr_bits << DCFG_devadr_bitp))
                             | (ctrlreq.header.wValue << DCFG_devadr_bitp);

            usb_core_control_request(&ctrlreq.header);
        }

        /* Make sure EP0 OUT is set up to accept the next request */
        if (out && ep == 0)
        {
            DEPTSIZ(0, true) = (1 << DEPTSIZ0_supcnt_bitp) | (1 << DEPTSIZ0_pkcnt_bitp) | 64;
            DEPDMA(0, true) = &ctrlreq;
            DEPCTL(0, true) |= DEPCTL_epena | DEPCTL_cnak;
        }
        DEPINT(ep, out) = epints;
    }
}

/* IRQ handler */
void INT_USB_FUNC(void)
{
    uint32_t ints = GINTSTS;
    if (ints & GINTMSK_usbreset)
    {
        DCFG = DCFG_nzstsouthshk;  /* Address 0 */
        reset_endpoints(1);
        usb_core_bus_reset();
    }

    if (ints & GINTMSK_enumdone)  /* enumeration done, we now know the speed */
    {
        /* Set up the maximum packet sizes accordingly */
        uint32_t maxpacket = (usb_drv_port_speed() ? 512 : 64) << DEPCTL_mps_bitp;
        DEPCTL(1, false) = (DEPCTL(1, false) & ~(DEPCTL_mps_bits << DEPCTL_mps_bitp)) | maxpacket;
        DEPCTL(2, true) = (DEPCTL(2, true) & ~(DEPCTL_mps_bits << DEPCTL_mps_bitp)) | maxpacket;
        DEPCTL(3, false) = (DEPCTL(3, false) & ~(DEPCTL_mps_bits << DEPCTL_mps_bitp)) | maxpacket;
        DEPCTL(4, true) = (DEPCTL(4, true) & ~(DEPCTL_mps_bits << DEPCTL_mps_bitp)) | maxpacket;
    }

    if (ints & GINTMSK_inepintr)
        handle_ep_int(false);

    if (ints & GINTMSK_outepintr)
        handle_ep_int(true);

    GINTSTS = ints;
}

static void ep_transfer(int ep, void *ptr, int length, bool out)
{
    endpoints[ep].busy = true;
    endpoints[ep].size = length;
    if (out)
        DEPCTL(ep, out) &= ~DEPCTL_stall;
    int blocksize = usb_drv_port_speed() ? 512 : 64;
    int packets = (length + blocksize - 1) / blocksize;
    if (packets == 0)
        packets = 1;

    DEPTSIZ(ep, out) = length | (packets << DEPTSIZ0_pkcnt_bitp);
    DEPDMA(ep, out) = length ? ptr : NULL;
    commit_dcache();
    DEPCTL(ep, out) |= DEPCTL_epena | DEPCTL_cnak;
}

int usb_drv_send(int endpoint, void *ptr, int length)
{
    endpoint = EP_NUM(endpoint);
    endpoints[endpoint].done = false;
    ep_transfer(endpoint, ptr, length, false);
    while (!endpoints[endpoint].done && endpoints[endpoint].busy)
        semaphore_wait(&endpoints[endpoint].complete, TIMEOUT_BLOCK);
    return endpoints[endpoint].status;
}

void usb_drv_cancel_all_transfers(void)
{
    int flags = disable_irq_save();
    reset_endpoints(0);
    restore_irq(flags);
}

void usb_drv_init(void)
{
    for (unsigned i = 0; i < sizeof(endpoints)/sizeof(struct ep_type); i++)
        semaphore_init(&endpoints[i].complete, 1, 0);

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

void usb_attach(void)
{
    usb_enable(true);
}
#endif // CONFIG_CPU == AS3525v2 /* FIXME FIXME FIXME */
