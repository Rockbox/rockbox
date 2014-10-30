/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#include "usb_drv.h"
#include "config.h"
#include "memory.h"
#include "target.h"

#define REG(x)  (*(volatile uint32_t *)(x))

#define USB_DEV     (USB_BASE + 0x800)
#define USB_DEV_IEP (USB_BASE + 0x900)
#define USB_DEV_OEP (USB_BASE + 0xb00)

/* Global OTG Interrupt Status */
#define GOTGINT     REG(USB_BASE + 0x4)
/* Global AHB Configuration Register */
#define GAHBCFG     REG(USB_BASE + 0x8)
#define GAHBCFG_GINT    (1 << 0)
#define GAHBCFG_TXFELVL (1 << 7)
/* Global Reset Register */
#define GRSTCTL     REG(USB_BASE + 0x10)
#define GRSTCTL_AHBIDL  (1 << 31)
#define GRSTCTL_TXFNUM_BP   6
#define GRSTCTL_TXFNUM_BM   (0x1f << 6)
#define GRSTCTL_TXFFLSH (1 << 5)
#define GRSTCTL_RXFFLSH (1 << 4)
#define GRSTCTL_INTKNQFLSH  (1 << 3)
#define GRSTCTL_CSRST   (1 << 0)
/* Interrupt Status Register */
#define GINTSTS     REG(USB_BASE + 0x14)
#define GINTSTS_RXFLVL  (1 << 4)
#define GINTSTS_USBRST  (1 << 12)
#define GINTSTS_ENUMDNE (1 << 13)
#define GINTSTS_IEPINT  (1 << 18)
#define GINTSTS_OEPINT  (1 << 19)
/* Interrupt Mask Register */
#define GINTMSK     REG(USB_BASE + 0x18)
/* Receive Status Read and Pop Register */
#define GRXSTSR     REG(USB_BASE + 0x1c)
#define GRXSTSP     REG(USB_BASE + 0x20)
#define GRXSTSR_EPNUM_BP    0
#define GRXSTSR_EPNUM_BM    (0xf << 0)
#define GRXSTSR_BCNT_BP     4
#define GRXSTSR_BCNT_BM     (0x7ff << 4)
#define GRXSTSR_PKTSTS_BP   17
#define GRXSTSR_PKTSTS_BM   (0xf << 17)
#define GRXSTSR_PKTSTS_OUT      2 /* data packet */
#define GRXSTSR_PKTSTS_SETUP    6 /* data packet */
/* Nonperiodic transmit FIFO/queue status register */
#define GNPTXSTS    REG(USB_BASE + 0x2c)
#define GNPTXSTS_NPTXFSAV_BP    0
#define GNPTXSTS_NPTXFSAV_BM    (0xffff << 0)
#define GNPTXSTS_NPTQXSAV_BP    16
#define GNPTXSTS_NPTQXSAV_BM    (0xff << 16)
/* Device Configuration Register */
#define DCFG        REG(USB_DEV)
#define DCFG_DSPS_BP    0
#define DCFG_DSPS_BM    (3 << 0)
#define DCFG_DAD_BM     (0x7f << 4)
#define DCFG_DAD_BP     4
/* Device Control Register */
#define DCTL        REG(USB_DEV + 4)
#define DCTL_RWUSIG     (1 << 0)
#define DCTL_SDIS       (1 << 1)
#define DCTL_CGINAK     (1 << 8)
/* Device Status */
#define DSTS        REG(USB_DEV + 8)
#define DSTS_ENUMSPD_BP 1
#define DSTS_ENUMSPD_BM (3 << 1)
#define DSTS_ENUMSPD_HS 0
#define DSTS_ENUMSPD_FS 3
/* Device OUT Endpoint Interrupt Register */
#define DIEPMSK     REG(USB_DEV + 0x10)
/* Device OUT Endpoint Interrupt Register */
#define DOEPMSK     REG(USB_DEV + 0x14)
/* Device All Endpoint Interrupt Register */
#define DAINT       REG(USB_DEV + 0x18)
#define DAINT_IEPINT(n) (1 << (n))
#define DAINT_OEPINT(n) (1 << ((n) + 16))
/* Device All Endpoint Interrupt Mask Register */
#define DAINTMSK    REG(USB_DEV + 0x1c)
/* Device Output Endpoint Control Register */
#define DOEPCTL(n)  REG(USB_DEV_OEP + 0x0 + (n) * 0x20)
#define DOEPCTL0    DOEPCTL(0)
#define DOEPCTL0_MPSIZ_BP   0
#define DOEPCTL0_MPSIZ_BM   (3 << 0)
#define DOEPCTL0_MPSIZ_64B  0
#define DOEPCTL0_MPSIZ_8B   3
#define DOEPCTL_USBAEP  (1 << 15)
#define DOEPCTL_STALL   (1 << 21)
#define DOEPCTL_CNAK    (1 << 26)
#define DOEPCTL_SNAK    (1 << 27)
#define DOEPCTL_EPDIS   (1 << 30)
#define DOEPCTL_EPENA   (1 << 31)
/* Device Output Endpoint Interrupt Register */
#define DOEPINT(n)  REG(USB_DEV_OEP + 0x8 + (n) * 0x20)
#define DOEPINT_XFRC    (1 << 0)
#define DOEPINT_STUP    (1 << 3)
/* Device Output Endpoint Size Register */
#define DOEPSIZ(n)  REG(USB_DEV_OEP + 0x10 + (n) * 0x20)
#define DOEPSIZ0    DOEPSIZ(0)
#define DOEPSIZ0_XFRSIZ_BP  0
#define DOEPSIZ0_XFRSIZ_BM  (0x7ffff << 0)
#define DOEPSIZ0_PKTCNT_BP  19
#define DOEPSIZ0_PKTCNT_BM  (0x3ff << 19)
#define DOEPSIZ0_STUPCNT_BM (3 << 29)
#define DOEPSIZ0_STUPCNT_BP 29
/* Device Output Endpoint Control Register */
#define DIEPCTL(n)  REG(USB_DEV_IEP + 0x0 + (n) * 0x20)
#define DIEPCTL0    DIEPCTL(0)
#define DIEPCTL0_MPSIZ_BP   0
#define DIEPCTL0_MPSIZ_BM   (3 << 0)
#define DIEPCTL0_MPSIZ_64B  0
#define DIEPCTL0_MPSIZ_8B   3
#define DIEPCTL_USBAEP  (1 << 15)
#define DIEPCTL_STALL   (1 << 21)
#define DIEPCTL_CNAK    (1 << 26)
#define DIEPCTL_SNAK    (1 << 27)
#define DIEPCTL_EPDIS   (1 << 30)
#define DIEPCTL_EPENA   (1 << 31)
/* Device Input Endpoint Interrupt Register */
#define DIEPINT(n)  REG(USB_DEV_IEP + 0x8 + (n) * 0x20)
#define DIEPINT_XFRC    (1 << 0)
/* Device Output Endpoint Size Register */
#define DIEPSIZ(n)  REG(USB_DEV_IEP + 0x10 + (n) * 0x20)
#define DIEPSIZ0    DIEPSIZ(0)
#define DIEPSIZ0_XFRSIZ_BP  0
#define DIEPSIZ0_XFRSIZ_BM  (0x7f << 0)
#define DIEPSIZ0_PKTCNT_BP  19
#define DIEPSIZ0_PKTCNT_BM  (3 << 19)
#define DIEPSIZ_XFRSIZ_BP   0
#define DIEPSIZ_XFRSIZ_BM   (0x7ffff << 0)
#define DIEPSIZ_PKTCNT_BP   19
#define DIEPSIZ_PKTCNT_BM   (0x3ff << 19)
/* Device IN endpoint transmit FIFO status register */
#define DTXFSTS(n)  REG(USB_DEV_IEP + 0x18 + (n) * 0x20)
/* Power */
#define PCGCTL      REG(USB_BASE + 0xe00)
/* Device Endpoint DFIFO */
#define DFIFO(n)    REG(USB_BASE + (n) * 0x1000)

#define FIELD_MAKE(reg,field,val) \
    (((val) << reg##_##field##_BP) & reg##_##field##_BM)
#define FIELD_SET(reg,field,val) \
    reg = (reg & ~(reg##_##field##_BM)) | (((val) << reg##_##field##_BP) & reg##_##field##_BM)
#define FIELD_GET(reg,field) \
    ((reg & reg##_##field##_BM) >> reg##_##field##_BP)
#define FIELD_GETX(val, reg,field) \
    ((val & reg##_##field##_BM) >> reg##_##field##_BP)

extern void target_enable_usb_irq(void);
extern void target_clear_usb_irq(void);
extern void target_disable_usb_irq(void);

void usb_drv_set_address(int address)
{
    FIELD_SET(DCFG, DAD, address);
}

int usb_drv_send(int endpoint, void* ptr, int length)
{
    if(endpoint != 0)
        return -1;
    int mps =  64;
    unsigned pktcnt = (length + mps - 1) / mps;
    if(pktcnt == 0)
        pktcnt = 1;
    uint32_t *p = ptr;
    while(pktcnt-- > 0)
    {
        unsigned xfr = MIN(mps, length);
        length -= xfr;
        while(FIELD_GET(GNPTXSTS, NPTQXSAV) == 0 || FIELD_GET(GNPTXSTS, NPTXFSAV) * 4 < xfr) {}
        DIEPSIZ(0) = FIELD_MAKE(DIEPSIZ0, PKTCNT, 1) |
            FIELD_MAKE(DIEPSIZ0, XFRSIZ, xfr);
        DIEPCTL(0) |= DIEPCTL_EPENA | DIEPCTL_CNAK;
        xfr = (xfr + 3) / 4;
        while(xfr-- > 0)
            DFIFO(0) = *p++;
        while(DIEPCTL(0) & DIEPCTL_EPENA) {}
    }
    return 0;
}

int usb_drv_recv(int endpoint, void* ptr, int length)
{
    if(endpoint != 0)
        return -1;
    int mps = 64;
    unsigned pktcnt = (length + mps - 1) / mps;
    if(pktcnt == 0)
        pktcnt = 1;
    uint32_t *p = ptr;
    DOEPSIZ(0) = FIELD_MAKE(DOEPSIZ0, STUPCNT, 1) | 
        FIELD_MAKE(DOEPSIZ0, PKTCNT, pktcnt) | FIELD_MAKE(DOEPSIZ0, XFRSIZ, length);
    DOEPCTL(0) |= DOEPCTL_EPENA | DOEPCTL_CNAK;
    return -1;
}


int usb_drv_port_speed(void)
{
    return FIELD_GET(DSTS, ENUMSPD) == DSTS_ENUMSPD_HS ? 0 : 1;
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
    if(in)
    {
        if(stall)
            DIEPCTL(endpoint) |= DIEPCTL_STALL;
        else
            DIEPCTL(endpoint) &= ~DIEPCTL_STALL;
    }
    else
    {
        if(stall)
            DOEPCTL(endpoint) |= DOEPCTL_STALL;
        else
            DOEPCTL(endpoint) &= ~DOEPCTL_STALL;
    }
}

static volatile int has_setup = 0;
static volatile uint32_t setup[2];

void usb_drv_irq(void)
{
    uint32_t gintsts = GINTSTS;
    if(gintsts & GINTSTS_USBRST)
    {
        DCTL &= ~DCTL_RWUSIG;
    }
    if(gintsts & GINTSTS_ENUMDNE)
    {
        /* prepare EP0 for setup or packet */
        FIELD_SET(DIEPCTL0, MPSIZ, DOEPCTL0_MPSIZ_64B); // apparently OUT max packet size is the one in the IN register
        DOEPSIZ(0) = FIELD_MAKE(DOEPSIZ0, STUPCNT, 1) |
            FIELD_MAKE(DOEPSIZ0, PKTCNT, 1) | FIELD_MAKE(DOEPSIZ0, XFRSIZ, 64);
        DOEPCTL(0) |= DOEPCTL_EPENA;
        DCTL |= DCTL_CGINAK;
    }
    if(gintsts & GINTSTS_RXFLVL)
    {
        uint32_t grxstsp = GRXSTSP;
        /* extract packet status */
        unsigned epnum = FIELD_GETX(grxstsp, GRXSTSR, EPNUM);
        unsigned bcnt = FIELD_GETX(grxstsp, GRXSTSR, BCNT);
        unsigned pktsts = FIELD_GETX(grxstsp, GRXSTSR, PKTSTS);
        if(pktsts == GRXSTSR_PKTSTS_SETUP && bcnt == 8)
        {
            setup[0] = DFIFO(0);
            setup[1] = DFIFO(0);
            has_setup = 1;
            DOEPSIZ(0) = FIELD_MAKE(DOEPSIZ0, STUPCNT, 1) |
                FIELD_MAKE(DOEPSIZ0, PKTCNT, 1) | FIELD_MAKE(DOEPSIZ0, XFRSIZ, 64);
        }
        else
        {
            DOEPCTL(epnum) = DOEPCTL_EPDIS | DOEPCTL_SNAK; // disable EP0
            /* assume interrupt is only enabled at the right time and we are not
            * writing past the buffer or things like this */
            bcnt = (bcnt + 3) / 4;
            while(bcnt-- > 0)
                (void)DFIFO(epnum);
        }
    }
    if(gintsts & GINTSTS_OEPINT)
    {
        uint32_t daint = DAINT;
        if(daint & DAINT_OEPINT(0))
        {
            DOEPINT(0) = DOEPINT(0);
        }
        DAINT = daint;
    }
    if(gintsts & GINTSTS_IEPINT)
    {
        uint32_t daint = DAINT;
        if(daint & DAINT_IEPINT(0))
        {
            DIEPINT(0) = DIEPINT(0);
        }
        DAINT = daint;
    }
    GINTSTS = gintsts;
}

void usb_drv_init(void)
{
    target_disable_usb_irq();
    /* disconnect */
    DCTL |= DCTL_SDIS;
    target_mdelay(1);
    /* reset core */
    while(!(GRSTCTL & GRSTCTL_AHBIDL)) {}
    GRSTCTL |= GRSTCTL_CSRST;
    while(GRSTCTL & GRSTCTL_CSRST) {}
    /* power up */
    PCGCTL = 0;
    /* init */
    GAHBCFG |= GAHBCFG_TXFELVL; // interrupt on empty fifo
    FIELD_SET(DCFG, DSPS, 0); // high speed
    FIELD_SET(DCFG, DAD, 0); // address is 0
    GRSTCTL |= GRSTCTL_TXFFLSH | FIELD_MAKE(GRSTCTL, TXFNUM, 16); // flush all tx fifos
    while(GRSTCTL & GRSTCTL_TXFFLSH) {}
    GRSTCTL |= GRSTCTL_RXFFLSH; // flush rx fifo
    while(GRSTCTL & GRSTCTL_RXFFLSH) {}
    GRSTCTL |= GRSTCTL_INTKNQFLSH; // flush token learning queue
    /* setup endpoints */
    DIEPCTL(0) = DIEPCTL_EPDIS | DIEPCTL_SNAK; // disable EP0
    DIEPSIZ(0) = 0;
    DIEPINT(0) = 0xff; // clear all interrupts
    DOEPCTL(0) = DOEPCTL_EPDIS | DOEPCTL_SNAK | FIELD_MAKE(DOEPCTL0, MPSIZ, DOEPCTL0_MPSIZ_64B);
    DOEPSIZ(0) = 0;
    DOEPINT(0) = 0xff; // clear all interrupts
    DIEPMSK = DIEPINT_XFRC; // only keep transfer complete interrupt
    DOEPMSK = DOEPINT_STUP | DOEPINT_XFRC; // transfer complete and setup
    DAINT = 0xffffffff;
    DAINTMSK = DAINT_IEPINT(0) | DAINT_OEPINT(0); // only care about EP0
    GINTSTS = 0xffffffff;
    GOTGINT = 0xffffffff;
    GINTMSK = GINTSTS_ENUMDNE | GINTSTS_USBRST | GINTSTS_RXFLVL;
    /* enable irq */
    GAHBCFG |= GAHBCFG_GINT;
    target_enable_usb_irq();
    /* reconnect */
    DCTL &= ~DCTL_SDIS;
}

void usb_drv_exit(void)
{
    /* disconnect */
    DCTL |= DCTL_SDIS;
}

int usb_drv_recv_setup(struct usb_ctrlrequest *req)
{
    /* wait for irq */
    while(!has_setup) {}
    memcpy(req, (void *)setup, 8);
    has_setup = 0;
    return 0;
}
