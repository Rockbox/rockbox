/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2009 Rafaël Carré
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
#include "as3525.h"
#include "clock-target.h"
#include "ascodec.h"
#include "as3514.h"
#include <stdbool.h>
#include "panic.h"
/*#define LOGF_ENABLE*/
#include "logf.h"


/* 4 input endpoints */
#define USB_IEP_CTRL(i)     *((volatile unsigned long*) USB_BASE + 0x0000 + (i*0x20))
#define USB_IEP_STS(i)      *((volatile unsigned long*) USB_BASE + 0x0004 + (i*0x20))
#define USB_IEP_TXFSIZE(i)  *((volatile unsigned long*) USB_BASE + 0x0008 + (i*0x20))
#define USB_IEP_MPS(i)      *((volatile unsigned long*) USB_BASE + 0x000C + (i*0x20))
#define USB_IEP_DESC_PTR(i) *((volatile unsigned long*) USB_BASE + 0x0014 + (i*0x20))
#define USB_IEP_STS_MASK(i) *((volatile unsigned long*) USB_BASE + 0x0018 + (i*0x20))

/* 4 output endpoints */
#define USB_OEP_CTRL(i)     *((volatile unsigned long*) USB_BASE + 0x0200 + (i*0x20))
#define USB_OEP_STS(i)      *((volatile unsigned long*) USB_BASE + 0x0204 + (i*0x20))
#define USB_OEP_RXFR(i)     *((volatile unsigned long*) USB_BASE + 0x0208 + (i*0x20))
#define USB_OEP_MPS(i)      *((volatile unsigned long*) USB_BASE + 0x020C + (i*0x20))
#define USB_OEP_SUP_PTR(i)  *((volatile unsigned long*) USB_BASE + 0x0210 + (i*0x20))
#define USB_OEP_DESC_PTR(i) *((volatile unsigned long*) USB_BASE + 0x0214 + (i*0x20))
#define USB_OEP_STS_MASK(i) *((volatile unsigned long*) USB_BASE + 0x0218 + (i*0x20))

#define USB_DEV_CFG             *((volatile unsigned long*) USB_BASE + 0x0400)
#define USB_DEV_CTRL            *((volatile unsigned long*) USB_BASE + 0x0404)
#define USB_DEV_STS             *((volatile unsigned long*) USB_BASE + 0x0408)
#define USB_DEV_INTR            *((volatile unsigned long*) USB_BASE + 0x040C)
#define USB_DEV_INTR_MASK       *((volatile unsigned long*) USB_BASE + 0x0410)
#define USB_DEV_EP_INTR         *((volatile unsigned long*) USB_BASE + 0x0414)
#define USB_DEV_EP_INTR_MASK    *((volatile unsigned long*) USB_BASE + 0x0418)

#define USB_PHY_EP0_INFO        *((volatile unsigned long*) USB_BASE + 0x0504)
#define USB_PHY_EP1_INFO        *((volatile unsigned long*) USB_BASE + 0x0508)
#define USB_PHY_EP2_INFO        *((volatile unsigned long*) USB_BASE + 0x050C)
#define USB_PHY_EP3_INFO        *((volatile unsigned long*) USB_BASE + 0x0510)
#define USB_PHY_EP4_INFO        *((volatile unsigned long*) USB_BASE + 0x0514)
#define USB_PHY_EP5_INFO        *((volatile unsigned long*) USB_BASE + 0x0518)

/* 4 channels */
#define USB_HOST_CH_SPLT(i)     *((volatile unsigned long*) USB_BASE + 0x1000 + (i*0x20))
#define USB_HOST_CH_STS(i)      *((volatile unsigned long*) USB_BASE + 0x1004 + (i*0x20))
#define USB_HOST_CH_TXFSIZE(i)  *((volatile unsigned long*) USB_BASE + 0x1008 + (i*0x20))
#define USB_HOST_CH_REQ(i)      *((volatile unsigned long*) USB_BASE + 0x100C + (i*0x20))
#define USB_HOST_CH_PER_INFO(i) *((volatile unsigned long*) USB_BASE + 0x1010 + (i*0x20))
#define USB_HOST_CH_DESC_PTR(i) *((volatile unsigned long*) USB_BASE + 0x1014 + (i*0x20))
#define USB_HOST_CH_STS_MASK(i) *((volatile unsigned long*) USB_BASE + 0x1018 + (i*0x20))

#define USB_HOST_CFG            *((volatile unsigned long*) USB_BASE + 0x1400)
#define USB_HOST_CTRL           *((volatile unsigned long*) USB_BASE + 0x1404)
#define USB_HOST_INTR           *((volatile unsigned long*) USB_BASE + 0x140C)
#define USB_HOST_INTR_MASK      *((volatile unsigned long*) USB_BASE + 0x1410)
#define USB_HOST_CH_INTR        *((volatile unsigned long*) USB_BASE + 0x1414)
#define USB_HOST_CH_INTR_MASK   *((volatile unsigned long*) USB_BASE + 0x1418)
#define USB_HOST_FRAME_INT      *((volatile unsigned long*) USB_BASE + 0x141C)
#define USB_HOST_FRAME_REM      *((volatile unsigned long*) USB_BASE + 0x1420)
#define USB_HOST_FRAME_NUM      *((volatile unsigned long*) USB_BASE + 0x1424)

#define USB_HOST_PORT0_CTRL_STS *((volatile unsigned long*) USB_BASE + 0x1500)

#define USB_OTG_CSR             *((volatile unsigned long*) USB_BASE + 0x2000)
#define USB_I2C_CSR             *((volatile unsigned long*) USB_BASE + 0x2004)
#define USB_GPIO_CSR            *((volatile unsigned long*) USB_BASE + 0x2008)
#define USB_SNPSID_CSR          *((volatile unsigned long*) USB_BASE + 0x200C)
#define USB_USERID_CSR          *((volatile unsigned long*) USB_BASE + 0x2010)
#define USB_USER_CONF1          *((volatile unsigned long*) USB_BASE + 0x2014)
#define USB_USER_CONF2          *((volatile unsigned long*) USB_BASE + 0x2018)
#define USB_USER_CONF3          *((volatile unsigned long*) USB_BASE + 0x201C)
#define USB_USER_CONF4          *((volatile unsigned long*) USB_BASE + 0x2020)
#define USB_USER_CONF5          *((volatile unsigned long*) USB_BASE + 0x2024)

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

void usb_drv_init(void)
{
    int i;
    for(i = 0; i < USB_NUM_ENDPOINTS * 2; i++)
        endpoints[i].busy = false;

    ascodec_write(AS3514_CVDD_DCDC3, ascodec_read(AS3514_CVDD_DCDC3) | 1<<2);
    ascodec_write(AS3514_USB_UTIL, ascodec_read(AS3514_USB_UTIL) & ~(1<<4));

    /* PHY part */
    CGU_USB = 1<<5 /* enable */
        | (CLK_DIV(AS3525_PLLA_FREQ, 48000000) / 2) << 2
        | 1; /* source = PLLA */

    /* AHB part */
    CGU_PERI |= CGU_USB_CLOCK_ENABLE;

    USB_GPIO_CSR = 0x6180000;
    USB_DEV_CFG = (USB_DEV_CFG & ~3) | 1; /* full speed */
    USB_DEV_CTRL |= 0x400; /* soft disconnect */

    /* UVDD */
    ascodec_write(AS3514_USB_UTIL, ascodec_read(AS3514_USB_UTIL) | (1<<4));
    sleep(10); //msleep(100)

    USB_GPIO_CSR = 0x6180000;

    USB_GPIO_CSR |= 0x1C00000;
    sleep(1); //msleep(3)
    USB_GPIO_CSR |= 0x200000;
    sleep(1); //msleep(10)

    USB_DEV_CTRL |= 0x400; /* soft disconnect */

    USB_GPIO_CSR &= ~0x1C00000;
    sleep(1); //msleep(3)
    USB_GPIO_CSR &= ~0x200000;
    sleep(1); //msleep(10)
    USB_DEV_CTRL &= ~0x400; /* clear soft disconnect */

#if defined(SANSA_CLIP)
    GPIOA_DIR |= (1<<6);
    GPIOA_PIN(6) = (1<<6);
    GPIOA_DIR &= ~(1<<6);   /* restore direction for usb_detect() */
#elif defined(SANSA_FUZE) || defined(SANSA_E200V2)
    GPIOA_DIR |= (1<<3);
    GPIOA_PIN(3) = (1<<3);
    GPIOA_DIR &= ~(1<<3);   /* restore direction for usb_detect() */
#elif defined(SANSA_C200V2)
    GPIOA_DIR |= (1<<1);
    GPIOA_PIN(1) = (1<<1);
    GPIOA_DIR &= ~(1<<1);   /* restore direction for usb_detect() */
#endif

#if 0 /* linux */
    USB_DEV_CFG |= (1<<17)  /* csr programming */
                |  (1<<3)   /* self powered */
                |  (1<<2);  /* remote wakeup */

    USB_DEV_CFG &= ~3; /* high speed */
#endif

    USB_IEP_CTRL(0) &= (3 << 4); /* control endpoint */
    USB_IEP_DESC_PTR(0) = 0;

    USB_OEP_CTRL(0) &= (3 << 4); /* control endpoint */
    USB_OEP_DESC_PTR(0) = 0;

    USB_DEV_INTR_MASK &= ~0xff;   /* unmask all flags */

    USB_DEV_EP_INTR_MASK &= ~((1<<0) | (1<<16));    /* ep 0 */

    VIC_INT_ENABLE = INTERRUPT_USB;

    USB_IEP_CTRL(0) |= (1<<7); /* set NAK */
    USB_OEP_CTRL(0) |= (1<<7); /* set NAK */
}

void usb_drv_exit(void)
{
    USB_DEV_CTRL |= (1<<10); /* soft disconnect */
    VIC_INT_EN_CLEAR = INTERRUPT_USB;
    CGU_USB &= ~(1<<5);
    CGU_PERI &= ~CGU_USB_CLOCK_ENABLE;
    ascodec_write(AS3514_USB_UTIL, ascodec_read(AS3514_USB_UTIL) & ~(1<<4));
}

int usb_drv_port_speed(void)
{
    return (USB_DEV_CFG & 3) ? 0 : 1;
}

int usb_drv_request_endpoint(int type, int dir)
{
    (void) type;
    int i = dir == USB_DIR_IN ? 0 : 1;

    for(; i < USB_NUM_ENDPOINTS * 2; i += 2)
        if(!endpoints[i].busy)
        {
            endpoints[i].busy = true;
            i >>= 1;
            USB_DEV_EP_INTR_MASK &= ~((1<<i) | (1<<(16+i)));
            return i | dir;
        }

    return -1;
}

void usb_drv_release_endpoint(int ep)
{
    int i = (ep & 0x7f) * 2;
    if(ep & USB_DIR_OUT)
        i++;
    endpoints[i].busy = false;
    USB_DEV_EP_INTR_MASK |= (1<<ep) | (1<<(16+ep));
}

void usb_drv_cancel_all_transfers(void)
{
}

int usb_drv_recv(int ep, void *ptr, int len)
{
    (void)ep;(void)ptr;(void)len;
    if(ep >= 2)
        return -1;

    return -1;
}

int usb_drv_send(int ep, void *ptr, int len)
{
    (void)ep;(void)ptr;(void)len;
    if(ep >= 2)
        return -1;

    return -1;
}

int usb_drv_send_nonblocking(int ep, void *ptr, int len)
{
    /* TODO */
    return usb_drv_send(ep, ptr, len);
}

/* interrupt service routine */
void INT_USB(void)
{
    panicf("USB interrupt !");
}

/* (not essential? , not implemented in usb-tcc.c) */
void usb_drv_set_test_mode(int mode)
{
    (void)mode;
}

void usb_drv_set_address(int address)
{
    (void)address;
}

void usb_drv_stall(int ep, bool stall, bool in)
{
    (void)ep;(void)stall;(void)in;
}

bool usb_drv_stalled(int ep, bool in)
{
    (void)ep;(void)in;
    return true;
}

