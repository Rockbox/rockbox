/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Marcin Bukat
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

/* Minimal polled driver for the X1000 DWC2 OTG core: EP0 only, internal DMA
 * mode (as Rockbox uses it), no interrupts. EP0 OUT
 * is always armed, NAKing data until usb_drv_recv(), so that the core can
 * take SETUP packets. The PHY and core setup follow Rockbox
 * (firmware/target/mips/ingenic_x1000/usb-x1000.c, usb-designware.c). */

#include "usb_drv.h"
#include "config.h"
#include "memory.h"
#include "target.h"
#include "x1000.h"

#define EP0_MPS         64
/* FIFO sizes in 32-bit words */
#define RX_FIFO_SIZE    256
#define TX0_FIFO_SIZE   64

static void wait_ahb_idle(void)
{
    while(!(REG_DWC_GRSTCTL & DWC_GRSTCTL_AHBIDL)) {}
}

static void flush_fifos(void)
{
    wait_ahb_idle();
    REG_DWC_GRSTCTL = DWC_GRSTCTL_TXFFLSH | DWC_GRSTCTL_TXFNUM(0x10);
    while(REG_DWC_GRSTCTL & DWC_GRSTCTL_TXFFLSH) {}
    REG_DWC_GRSTCTL = DWC_GRSTCTL_RXFFLSH;
    while(REG_DWC_GRSTCTL & DWC_GRSTCTL_RXFFLSH) {}
}

/* DMA buffers, used through their uncached alias */
static uint8_t ep0_out_buf[EP0_MPS] __attribute__((aligned(32)));
static uint8_t ep0_in_buf[EP0_MPS] __attribute__((aligned(32)));

/* Arm EP0 OUT for one packet: SETUPs always get in, data is NAKed until
 * usb_drv_recv() clears NAK */
static void ep0_out_arm(void)
{
    REG_DWC_DOEPDMA0 = PHYSADDR(ep0_out_buf);
    REG_DWC_DOEPTSIZ0 = DWC_EPTSIZ_STUPCNT(1) | DWC_EPTSIZ_PKTCNT(1) |
                        DWC_EPTSIZ_XFERSIZE(EP0_MPS);
    REG_DWC_DOEPCTL0 |= DWC_EPCTL_EPENA | DWC_EPCTL_SNAK;
}

static void bus_reset(void)
{
    REG_DWC_DCFG &= ~DWC_DCFG_DAD_MASK;
    REG_DWC_DIEPCTL0 &= ~DWC_EPCTL_STALL;
    REG_DWC_DOEPCTL0 &= ~DWC_EPCTL_STALL;
    flush_fifos();
    REG_DWC_DIEPINT0 = 0xffffffff;
    REG_DWC_DOEPINT0 = 0xffffffff;
    ep0_out_arm();
}

/* A new SETUP or a bus reset aborts the current control transfer */
static bool ep0_aborted(void)
{
    return (REG_DWC_DOEPINT0 & DWC_EPINT_STUP) ||
           (REG_DWC_GINTSTS & DWC_GINTSTS_USBRST);
}

/* Same as usb_dw_target_enable_clocks() in Rockbox */
static void phy_init(void)
{
    REG_CPM_CLKGR &= ~CPM_CLKGR_OTG;
    /* PHY clock from EXCLK (24 MHz) */
    REG_CPM_USBCDR = (REG_CPM_USBCDR & ~(CPM_USBCDR_CLKSRC_MASK |
                        CPM_USBCDR_CLKDIV_MASK | CPM_USBCDR_STOP |
                        CPM_USBCDR_PHY_GATE)) | CPM_USBCDR_CE;
    while(REG_CPM_USBCDR & CPM_USBCDR_BUSY) {}
    REG_CPM_USBCDR &= ~CPM_USBCDR_CE;

    /* PHY soft reset */
    REG_CPM_SRBC |= CPM_SRBC_OTG_SR;
    target_udelay(10);
    REG_CPM_SRBC &= ~CPM_SRBC_OTG_SR;

    /* ungate the PHY clock and exit suspend */
    REG_CPM_OPCR &= ~CPM_OPCR_GATE_USBPHY_CLK;
    REG_CPM_OPCR |= CPM_OPCR_SPENDN0;
    target_udelay(45);

    REG_CPM_USBVBFIL = 0;
    REG_CPM_USBRDT = CPM_USBRDT_VBFIL_LD_EN | CPM_USBRDT_RDT(0x96);
    REG_CPM_USBPCR = CPM_USBPCR_OTG_DISABLE | CPM_USBPCR_COMMONONN |
                     CPM_USBPCR_VBUSVLDEXT | CPM_USBPCR_VBUSVLDEXTSEL |
                     CPM_USBPCR_SQRXTUNE(7) | CPM_USBPCR_TXPREEMPHTUNE |
                     CPM_USBPCR_TXHSXVTUNE(1) | CPM_USBPCR_TXVREFTUNE(7);
    REG_CPM_USBPCR1 = CPM_USBPCR1_BVLD_REG | CPM_USBPCR1_REFCLK_SEL_CLKCORE |
                      CPM_USBPCR1_REFCLK_DIV_24MHZ | CPM_USBPCR1_WORD_IF_16BIT;

    /* power on reset */
    REG_CPM_USBPCR |= CPM_USBPCR_POR;
    target_mdelay(1);
    REG_CPM_USBPCR &= ~CPM_USBPCR_POR;
    target_mdelay(1);
}

void usb_drv_init(void)
{
    /* drop off the bus so that the host enumerates us again */
    REG_DWC_DCTL |= DWC_DCTL_SDIS;
    REG_DWC_GINTMSK = 0;
    /* stop the boot ROM's DMA, otherwise the AHB never goes idle */
    REG_DWC_GAHBCFG = 0;

    phy_init();
    REG_DWC_PCGCCTL = 0;

    /* 16-bit UTMI+, turnaround is 4 AHB clocks + 1 PHY clock. Like Rockbox,
     * do not force device mode, the PHY is set up as device only. */
    uint32_t gusbcfg = DWC_GUSBCFG_PHYIF16 | DWC_GUSBCFG_TRDT(5);
    REG_DWC_GUSBCFG = gusbcfg;

    /* core reset */
    target_udelay(100);
    wait_ahb_idle();
    REG_DWC_GRSTCTL = DWC_GRSTCTL_CSRST;
    while(REG_DWC_GRSTCTL & DWC_GRSTCTL_CSRST) {}
    wait_ahb_idle();

    REG_DWC_GUSBCFG = gusbcfg;
    REG_DWC_GRXFSIZ = RX_FIFO_SIZE;
    REG_DWC_TX0FSIZ = (TX0_FIFO_SIZE << 16) | RX_FIFO_SIZE;
    flush_fifos();

    REG_DWC_GAHBCFG = DWC_GAHBCFG_DMAEN | DWC_GAHBCFG_HBSTLEN_INCR16;
    REG_DWC_DCFG = DWC_DCFG_NZLSOHSK | DWC_DCFG_DSPD_HS;
    REG_DWC_DCTL = DWC_DCTL_SDIS;
    REG_DWC_GINTSTS = 0xffffffff;
    REG_DWC_DIEPINT0 = 0xffffffff;
    REG_DWC_DOEPINT0 = 0xffffffff;

    /* give the host time to notice the disconnect, then reconnect */
    target_mdelay(3);
    REG_DWC_DCTL &= ~DWC_DCTL_SDIS;
}

void usb_drv_exit(void)
{
    REG_DWC_DCTL |= DWC_DCTL_SDIS;
    /* see usb_drv_jz4760b.c: hubs may need more than T_DDIS (2 us) */
    target_mdelay(1);
}

int usb_drv_recv_setup(struct usb_ctrlrequest *req)
{
    while(1)
    {
        uint32_t gintsts = REG_DWC_GINTSTS;
        if(gintsts & DWC_GINTSTS_USBRST)
        {
            REG_DWC_GINTSTS = DWC_GINTSTS_USBRST;
            bus_reset();
            continue;
        }
        if(gintsts & DWC_GINTSTS_ENUMDNE)
        {
            /* DIEPCTL0.MPSIZ = 0 selects 64 bytes, at any speed */
            REG_DWC_GINTSTS = DWC_GINTSTS_ENUMDNE;
            REG_DWC_DIEPCTL0 &= ~3;
            ep0_out_arm();
            continue;
        }

        uint32_t epint = REG_DWC_DOEPINT0;
        if(epint & DWC_EPINT_STUP)
        {
            REG_DWC_DOEPINT0 = epint;
            memcpy(req, UNCACHEDADDR(ep0_out_buf), sizeof(*req));
            ep0_out_arm();
            /* The core needs the new address before the status stage, it
             * keeps using the old one until the status stage is done. hwstub
             * only calls usb_drv_set_address() after it. */
            if(req->bRequestType == (USB_DIR_OUT | USB_TYPE_STANDARD |
                                     USB_RECIP_DEVICE) &&
               req->bRequest == USB_REQ_SET_ADDRESS)
                usb_drv_set_address(req->wValue);
            return 0;
        }
        if(epint & DWC_EPINT_SETUPRECVD)
        {
            /* SETUP in memory, STUP follows when the setup stage is done */
            REG_DWC_DOEPINT0 = DWC_EPINT_SETUPRECVD | DWC_EPINT_XFRC;
        }
        else if(epint & DWC_EPINT_XFRC)
        {
            /* stray OUT packet (eg. a status stage we did not wait for) */
            REG_DWC_DOEPINT0 = epint;
            ep0_out_arm();
        }
    }
}

int usb_drv_port_speed(void)
{
    /* DSTS.ENUMSPD = 0: high speed */
    return ((REG32(OTG_BASE + 0x808) >> 1) & 3) == 0 ? 1 : 0;
}

void usb_drv_set_address(int address)
{
    REG_DWC_DCFG = (REG_DWC_DCFG & ~DWC_DCFG_DAD_MASK) | DWC_DCFG_DAD(address);
}

int usb_drv_send(int endpoint, void *ptr, int length)
{
    (void) endpoint;
    /* one packet at a time, a ZLP when length is 0 */
    do
    {
        int cnt = MIN(length, EP0_MPS);
        memcpy(UNCACHEDADDR(ep0_in_buf), ptr, cnt);
        REG_DWC_DIEPDMA0 = PHYSADDR(ep0_in_buf);
        REG_DWC_DIEPTSIZ0 = DWC_EPTSIZ_PKTCNT(1) | DWC_EPTSIZ_XFERSIZE(cnt);
        REG_DWC_DIEPCTL0 |= DWC_EPCTL_EPENA | DWC_EPCTL_CNAK;
        while(!(REG_DWC_DIEPINT0 & DWC_EPINT_XFRC))
            if(ep0_aborted())
                return -1;
        REG_DWC_DIEPINT0 = DWC_EPINT_XFRC;
        ptr = (uint8_t *)ptr + cnt;
        length -= cnt;
    } while(length > 0);
    return 0;
}

int usb_drv_recv(int endpoint, void* ptr, int length)
{
    (void) endpoint;
    int total = 0;
    /* one packet at a time until a short one; length 0 takes the status ZLP */
    while(1)
    {
        /* EP0 OUT is already armed, let the data in */
        REG_DWC_DOEPCTL0 |= DWC_EPCTL_CNAK;
        while(!(REG_DWC_DOEPINT0 & DWC_EPINT_XFRC))
            if(ep0_aborted())
                return -1;
        REG_DWC_DOEPINT0 = DWC_EPINT_XFRC;
        int cnt = EP0_MPS - (REG_DWC_DOEPTSIZ0 & DWC_EPTSIZ0_XFERSIZE_MASK);
        int n = MIN(cnt, length - total);
        memcpy((uint8_t *)ptr + total, UNCACHEDADDR(ep0_out_buf), n);
        total += n;
        ep0_out_arm();
        if(cnt < EP0_MPS || total >= length)
            return total;
    }
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
    (void) endpoint;
    (void) in;
    if(!stall)
        return; /* EP0 unstalls on the next SETUP */
    REG_DWC_DIEPCTL0 |= DWC_EPCTL_STALL;
    REG_DWC_DOEPCTL0 |= DWC_EPCTL_STALL;
}
