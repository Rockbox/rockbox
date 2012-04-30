/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2010 Tobias Diedrich
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

#include "system.h"
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
#include "usb_ch9.h"
#include "usb_core.h"
#include "string.h"

#include "usb-drv-as3525.h"

static struct usb_endpoint endpoints[USB_NUM_EPS][2];
static int got_set_configuration = 0;
static int usb_enum_timeout = -1;
static bool initialized = false;

/*
 * dma/setup descriptors and buffers should avoid sharing
 * a cacheline with other data.
 * dmadescs may share with each other, since we only access them uncached.
 */
static struct usb_dev_dma_desc dmadescs[USB_NUM_EPS][2] __attribute__((aligned(32)));
/* reuse unused EP2 OUT descriptor here */
static struct usb_dev_setup_buf *setup_desc = (void*)&dmadescs[2][1];

#if AS3525_MCLK_SEL != AS3525_CLK_PLLB
static inline void usb_enable_pll(void)
{
    CGU_COUNTB = CGU_LOCK_CNT;
    CGU_PLLB = AS3525_PLLB_SETTING;
    CGU_PLLBSUP = 0;                       /* enable PLLB */
    while(!(CGU_INTCTRL & CGU_PLLB_LOCK)); /* wait until PLLB is locked */
}

static inline void usb_disable_pll(void)
{
    CGU_PLLBSUP = CGU_PLL_POWERDOWN;
}
#else
static inline void usb_enable_pll(void)
{
}

static inline void usb_disable_pll(void)
{
}
#endif /* AS3525_MCLK_SEL != AS3525_CLK_PLLB */

void usb_attach(void)
{
    logf("usb-drv: attach");
    /* Nothing to do */
}

static void usb_tick(void);

static void usb_phy_on(void)
{
    /* PHY clock */
    CGU_USB = 1<<5 /* enable */
        | (CLK_DIV(AS3525_PLLB_FREQ, 48000000) / 2) << 2
        | 2; /* source = PLLB */

    /* UVDD on */
    ascodec_write(AS3515_USB_UTIL, ascodec_read(AS3515_USB_UTIL) | (1<<4));
    mdelay(100);

    /* reset */
    CCU_SRC = CCU_SRC_USB_AHB_EN|CCU_SRC_USB_PHY_EN;
    CCU_SRL = CCU_SRL_MAGIC_NUMBER;
    mdelay(1);
    CCU_SRC = CCU_SRC_USB_AHB_EN;
    mdelay(1);
    CCU_SRC = CCU_SRL = 0;

    USB_GPIO_CSR = USB_GPIO_TX_ENABLE_N
                 | USB_GPIO_TX_BIT_STUFF_EN
                 | USB_GPIO_XO_ON
                 | USB_GPIO_CLK_SEL10; /* 0x06180000; */
}

static void usb_phy_suspend(void)
{
    USB_GPIO_CSR |= USB_GPIO_ASESSVLD_EXT |
                    USB_GPIO_BSESSVLD_EXT |
                    USB_GPIO_VBUS_VLD_EXT;
    mdelay(3);
    USB_GPIO_CSR |= USB_GPIO_VBUS_VLD_EXT_SEL;
    mdelay(10);
}

static void usb_phy_resume(void)
{
    USB_GPIO_CSR &= ~(USB_GPIO_ASESSVLD_EXT |
                      USB_GPIO_BSESSVLD_EXT |
                      USB_GPIO_VBUS_VLD_EXT);
    mdelay(3);
    USB_GPIO_CSR &= ~USB_GPIO_VBUS_VLD_EXT_SEL;
    mdelay(10);
}

static void setup_desc_init(struct usb_dev_setup_buf *desc)
{
    struct usb_dev_setup_buf *uc_desc = AS3525_UNCACHED_ADDR(desc);

    uc_desc->status = USB_DMA_DESC_BS_HST_RDY;
    uc_desc->resv   = 0xffffffff;
    uc_desc->data1  = 0xffffffff;
    uc_desc->data2  = 0xffffffff;
}

static void dma_desc_init(int ep, int dir)
{
    struct usb_dev_dma_desc *desc = &dmadescs[ep][dir];
    struct usb_dev_dma_desc *uc_desc = AS3525_UNCACHED_ADDR(desc);

    endpoints[ep][dir].uc_desc = uc_desc;

    uc_desc->status    = USB_DMA_DESC_BS_DMA_DONE | \
                         USB_DMA_DESC_LAST | \
                         USB_DMA_DESC_ZERO_LEN;
    uc_desc->resv      = 0xffffffff;
    uc_desc->data_ptr  = 0;
    uc_desc->next_desc = 0;
}

static void reset_endpoints(int init)
{
    int i;

    /*
     * OUT EP 2 is an alias for OUT EP 0 on this HW!
     *
     * Resonates with "3 bidirectional- plus 1 in-endpoints in device mode"
     * from the datasheet, but why ep2 and not ep3?
     *
     * Reserve it here so we will skip over it in request_endpoint().
     */
    endpoints[2][1].state |= EP_STATE_ALLOCATED;

    for(i = 0; i < USB_NUM_EPS; i++) {
        /*
         * MPS sizes depending on speed:
         * LS: 8 (control), no bulk available
         * FS: 64 (control), 64 (bulk)
         * HS: 64 (control), 512 (bulk)
         *
         * We don't need to handle LS since there is no low-speed only
         * host AFAIK.
         */
        int mps = i == 0 ? 64 : (usb_drv_port_speed() ? 512 : 64);

        if (init) {
            if (endpoints[i][0].state & EP_STATE_BUSY) {
                if (endpoints[i][0].state & EP_STATE_ASYNC) {
                    endpoints[i][0].rc = -1;
                    semaphore_release(&endpoints[i][0].complete);
                } else {
                    usb_core_transfer_complete(i, USB_DIR_IN, -1, 0);
                }
            }
            endpoints[i][0].state = 0;
            semaphore_wait(&endpoints[i][0].complete, TIMEOUT_NOBLOCK);

            if (i != 2) { /* Skip the OUT EP0 alias */
                if (endpoints[i][1].state & EP_STATE_BUSY)
                    usb_core_transfer_complete(i, USB_DIR_OUT, -1, 0);
                endpoints[i][1].state = 0;
                semaphore_wait(&endpoints[i][1].complete, TIMEOUT_NOBLOCK);
                USB_OEP_SUP_PTR(i)    = 0;
            }
        }

        dma_desc_init(i, 0);
        USB_IEP_CTRL    (i) = USB_EP_CTRL_FLUSH|USB_EP_CTRL_SNAK;
        USB_IEP_MPS     (i) = mps; /* in bytes */
        /* We don't care about the 'IN token received' event */
        USB_IEP_STS_MASK(i) = USB_EP_STAT_IN; /* OF: 0x840 */
        USB_IEP_TXFSIZE (i) = mps/2; /* in dwords => mps*2 bytes */
        USB_IEP_STS     (i) = 0xffffffff; /* clear status */
        USB_IEP_DESC_PTR(i) = 0;

        if (i != 2) { /* Skip the OUT EP0 alias */
            dma_desc_init(i, 1);
            USB_OEP_CTRL    (i) = USB_EP_CTRL_FLUSH|USB_EP_CTRL_SNAK;
            USB_OEP_MPS     (i) = (mps/2 << 16) | mps;
            USB_OEP_STS_MASK(i) = USB_EP_STAT_BNA; /* OF: 0x1800 */
            USB_OEP_RXFR    (i) = 0;      /* Always 0 in OF trace? */
            USB_OEP_STS     (i) = 0xffffffff; /* clear status */
            USB_OEP_DESC_PTR(i) = 0;
        }
    }

    setup_desc_init(setup_desc);
    USB_OEP_SUP_PTR(0)    = AS3525_PHYSICAL_ADDR((int)setup_desc);
}

void usb_drv_init(void)
{
    logf("usb_drv_init() !!!!\n");

    if (!initialized)
    {
        int i;
        for (i = 0; i < USB_NUM_EPS; i++)
        {
            semaphore_init(&endpoints[i][0].complete, 1, 0);
            semaphore_init(&endpoints[i][1].complete, 1, 0);
        }

        initialized = true;
    }

    usb_enable_pll();

    /* we have external power, so boost cpu */
    cpu_boost(1);

    /* length regulator: normal operation */
    ascodec_write(AS3514_CVDD_DCDC3, ascodec_read(AS3514_CVDD_DCDC3) | 1<<2);

    /* AHB part */
    bitset32(&CGU_PERI, CGU_USB_CLOCK_ENABLE);

    /* reset AHB */
    CCU_SRC = CCU_SRC_USB_AHB_EN;
    CCU_SRL = CCU_SRL_MAGIC_NUMBER;
    mdelay(1);
    CCU_SRC = CCU_SRL = 0;

    USB_GPIO_CSR = USB_GPIO_TX_ENABLE_N
                 | USB_GPIO_TX_BIT_STUFF_EN
                 | USB_GPIO_XO_ON
                 | USB_GPIO_CLK_SEL10; /* 0x06180000; */

    /* bug workaround according to linux patch */
    USB_DEV_CFG = (USB_DEV_CFG & ~3) | 1; /* full speed */

    /* enable soft disconnect */
    USB_DEV_CTRL |= USB_DEV_CTRL_SOFT_DISCONN;

    usb_phy_on();
    usb_phy_suspend();
    USB_DEV_CTRL |= USB_DEV_CTRL_SOFT_DISCONN;

    /* We don't care about SVC or SOF events */
    /* Right now we don't handle suspend, so mask those too */
    USB_DEV_INTR_MASK = USB_DEV_INTR_SVC |
                        USB_DEV_INTR_SOF |
                        USB_DEV_INTR_USB_SUSPEND |
                        USB_DEV_INTR_EARLY_SUSPEND;

    USB_DEV_CFG = USB_DEV_CFG_STAT_ACK      |
                  USB_DEV_CFG_UNI_DIR       |
                  USB_DEV_CFG_PI_16BIT      |
                  USB_DEV_CFG_HS            |
                  USB_DEV_CFG_SELF_POWERED  |
                  USB_DEV_CFG_CSR_PRG       |
                  USB_DEV_CFG_PHY_ERR_DETECT;

    USB_DEV_CTRL = USB_DEV_CTRL_DESC_UPDATE  |
                   USB_DEV_CTRL_THRES_ENABLE |
                   USB_DEV_CTRL_BURST_ENABLE |
                   USB_DEV_CTRL_BLEN_8DWORDS |
                   USB_DEV_CTRL_TLEN_8THMAXSIZE;

    USB_DEV_EP_INTR_MASK &= ~((1<<0) | (1<<16));    /* ep 0 */

    reset_endpoints(1);

    /* clear pending interrupts */
    USB_DEV_EP_INTR = 0xffffffff;
    USB_DEV_INTR    = 0xffffffff;

    VIC_INT_ENABLE = INTERRUPT_USB;

    usb_phy_resume();
    USB_DEV_CTRL &= ~USB_DEV_CTRL_SOFT_DISCONN;

    USB_GPIO_CSR = USB_GPIO_TX_ENABLE_N
                 | USB_GPIO_TX_BIT_STUFF_EN
                 | USB_GPIO_XO_ON
                 | USB_GPIO_HS_INTR
                 | USB_GPIO_CLK_SEL10; /* 0x06180000; */

    tick_add_task(usb_tick);

    usb_enum_timeout = HZ; /* one second timeout for enumeration */
}

void usb_drv_exit(void)
{
    tick_remove_task(usb_tick);
    USB_DEV_CTRL |= (1<<10); /* soft disconnect */
    usb_phy_suspend();
    /*
     * mask all interrupts _before_ writing to VIC_INT_EN_CLEAR,
     * or else the core might latch the interrupt while
     * the write ot VIC_INT_EN_CLEAR is in the pipeline and
     * so cause a fake spurious interrupt.
     */
    USB_DEV_EP_INTR_MASK = 0xffffffff;
    USB_DEV_INTR_MASK    = 0xffffffff;
    VIC_INT_EN_CLEAR = INTERRUPT_USB;
    CGU_USB &= ~(1<<5);
    bitclr32(&CGU_PERI, CGU_USB_CLOCK_ENABLE);
    /* Disable UVDD generating LDO */
    ascodec_write(AS3515_USB_UTIL, ascodec_read(AS3515_USB_UTIL) & ~(1<<4));
    usb_disable_pll();
    cpu_boost(0);
    initialized = false;
    logf("usb_drv_exit() !!!!\n");
}

int usb_drv_port_speed(void)
{
    return (USB_DEV_STS & USB_DEV_STS_MASK_SPD) ? 0 : 1;
}

int usb_drv_request_endpoint(int type, int dir)
{
    int d = dir == USB_DIR_IN ? 0 : 1;
    int i = 1; /* skip the control EP */

    for(; i < USB_NUM_EPS; i++) {
        if (endpoints[i][d].state & EP_STATE_ALLOCATED)
            continue;

        endpoints[i][d].state |= EP_STATE_ALLOCATED;

        if (dir == USB_DIR_IN) {
            USB_IEP_CTRL(i) = USB_EP_CTRL_FLUSH |
                              USB_EP_CTRL_SNAK  |
                              USB_EP_CTRL_ACT   |
                              (type << 4);
            USB_DEV_EP_INTR_MASK &= ~(1<<i);
        } else {
            USB_OEP_CTRL(i) = USB_EP_CTRL_FLUSH |
                              USB_EP_CTRL_SNAK  |
                              USB_EP_CTRL_ACT   |
                              (type << 4);
            USB_DEV_EP_INTR_MASK &= ~(1<<(16+i));
        }
        /* logf("usb_drv_request_endpoint(%d, %d): returning %02x\n", type, dir, i | dir); */
        return i | dir;
    }

    logf("usb_drv_request_endpoint(%d, %d): no free endpoint found\n", type, dir);
    return -1;
}

void usb_drv_release_endpoint(int ep)
{
    int i = ep & 0x7f;
    int d = ep & USB_DIR_IN ? 0 : 1;

    if (i >= USB_NUM_EPS)
        return;
    /*
     * Check for control EP and ignore it.
     * Unfortunately the usb core calls
     * usb_drv_release_endpoint() for ep=0..(USB_NUM_ENDPOINTS-1),
     * but doesn't request a new control EP after that...
     */
    if (i == 0 || /* Don't mask control EP */
        (i == 2 && d == 1)) /* See reset_endpoints(), EP2_OUT == EP0_OUT */
        return;

    if (!(endpoints[i][d].state & EP_STATE_ALLOCATED))
        return;

    /* logf("usb_drv_release_endpoint(%d, %d)\n", i, d); */
    endpoints[i][d].state = 0;
    USB_DEV_EP_INTR_MASK |= (1<<(16*d+i));
    USB_EP_CTRL(i, !d) = USB_EP_CTRL_FLUSH | USB_EP_CTRL_SNAK;
}

void usb_drv_cancel_all_transfers(void)
{
    logf("usb_drv_cancel_all_transfers()\n");
    return;

    int flags = disable_irq_save();
    reset_endpoints(0);
    restore_irq(flags);
}

int usb_drv_recv(int ep, void *ptr, int len)
{
    struct usb_dev_dma_desc *uc_desc = endpoints[ep][1].uc_desc;

    ep &= 0x7f;
    logf("usb_drv_recv(%d,%x,%d)\n", ep, (int)ptr, len);

    if (len > USB_DMA_DESC_RXTX_BYTES)
        panicf("usb_recv: len=%d > %d", len, USB_DMA_DESC_RXTX_BYTES);

    if ((int)ptr & 31) {
        logf("addr %08x not aligned!\n", (int)ptr);
    }

    endpoints[ep][1].state |= EP_STATE_BUSY;
    endpoints[ep][1].len = len;
    endpoints[ep][1].rc  = -1;

    /* remove data buffer from cache */
    discard_dcache_range(ptr, len);

    /* DMA setup */
    uc_desc->status    = USB_DMA_DESC_BS_HST_RDY |
                         USB_DMA_DESC_LAST |
                         len;
    if (len == 0) {
        uc_desc->status   |= USB_DMA_DESC_ZERO_LEN;
        uc_desc->data_ptr  = 0;
    } else {
        uc_desc->data_ptr  = AS3525_PHYSICAL_ADDR(ptr);
    }
    USB_OEP_DESC_PTR(ep) = AS3525_PHYSICAL_ADDR((int)&dmadescs[ep][1]);
    USB_OEP_STS(ep)      = USB_EP_STAT_OUT_RCVD; /* clear status */

    /* Make sure receive DMA is on */
    if (!(USB_DEV_CTRL & USB_DEV_CTRL_RDE)){
        USB_DEV_CTRL |= USB_DEV_CTRL_RDE;
        if (!(USB_DEV_CTRL & USB_DEV_CTRL_RDE))
            logf("failed to enable RDE!\n");
    }

    USB_OEP_CTRL(ep)    |= USB_EP_CTRL_CNAK; /* Go! */

    if (USB_OEP_CTRL(ep) & USB_EP_CTRL_NAK) {
        int i = 0;
        while (USB_OEP_CTRL(ep) & USB_EP_CTRL_NAK) {
            USB_OEP_CTRL(ep)    |= USB_EP_CTRL_CNAK; /* Go! */
            i++;
        }
        logf("ep%d CNAK needed %d retries CTRL=%x\n", ep, i, (int)USB_OEP_CTRL(ep));
    }

    return 0;
}

#if defined(LOGF_ENABLE)
static char hexbuf[1025];
static char hextab[16] = "0123456789abcdef";

char *make_hex(char *data, int len)
{
    int i;
    if (!((int)data & 0x40000000))
        data = AS3525_UNCACHED_ADDR(data); /* don't pollute the cache */

    if (len > 512)
        len = 512;

    for (i=0; i<len; i++) {
        hexbuf[2*i  ] = hextab[(unsigned char)data[i] >> 4 ];
        hexbuf[2*i+1] = hextab[(unsigned char)data[i] & 0xf];
    }
    hexbuf[2*i] = 0;

    return hexbuf;
}
#endif

static void ep_send(int ep, void *ptr, int len)
{
    struct usb_dev_dma_desc *uc_desc = endpoints[ep][0].uc_desc;

    endpoints[ep][0].state |= EP_STATE_BUSY;
    endpoints[ep][0].len = len;
    endpoints[ep][0].rc = -1;

    /*
     * I'm seeing a problem where Linux sends two SETUP requests,
     * but fails to read the response from the first one.
     * We then have the response we wanted to send still in our fifo,
     * so flush the fifo before sending on the control endpoint.
     */
    if (ep == 0)
        USB_IEP_CTRL(ep) |= USB_EP_CTRL_FLUSH;

    /* Make sure data is committed to memory */
    commit_dcache_range(ptr, len);

    logf("xx%s\n", make_hex(ptr, len));

    uc_desc->status    = USB_DMA_DESC_BS_HST_RDY |
                         USB_DMA_DESC_LAST |
                         len;
    if (len == 0)
        uc_desc->status |= USB_DMA_DESC_ZERO_LEN;

    uc_desc->data_ptr  = AS3525_PHYSICAL_ADDR(ptr);

    USB_IEP_DESC_PTR(ep) = AS3525_PHYSICAL_ADDR((int)&dmadescs[ep][0]);
    USB_IEP_STS(ep)      = 0xffffffff; /* clear status */
    /* start transfer */
    USB_IEP_CTRL(ep)     |= USB_EP_CTRL_CNAK | USB_EP_CTRL_PD;
    /* HW automatically sets NAK bit later */
}

int usb_drv_send(int ep, void *ptr, int len)
{
    logf("usb_drv_send(%d,%x,%d): ", ep, (int)ptr, len);

    ep &= 0x7f;

    if (ep == 0 && got_set_configuration) {
        got_set_configuration = 0;
        if (len != 0)
            panicf("usb_drv_send: GSC, but len!=0");
        /* Tell the HW we handled the request */
        USB_DEV_CTRL |= USB_DEV_CTRL_APCSR_DONE;
        return 0;
    }

    ep_send(ep, ptr, len);
    if (semaphore_wait(&endpoints[ep][0].complete, HZ) == OBJ_WAIT_TIMEDOUT)
        logf("send timed out!\n");

    return endpoints[ep][0].rc;
}

int usb_drv_send_nonblocking(int ep, void *ptr, int len)
{
    logf("usb_drv_send_nonblocking(%d,%x,%d): ", ep, (int)ptr, len);
    ep &= 0x7f;
    endpoints[ep][0].state |= EP_STATE_ASYNC;
    ep_send(ep, ptr, len);
    return 0;
}

static void handle_in_ep(int ep)
{
    int ep_sts = USB_IEP_STS(ep) & ~USB_IEP_STS_MASK(ep);

    if (ep > 3)
        panicf("in_ep > 3?!");

    USB_IEP_STS(ep) = ep_sts; /* ack */

    if (ep_sts & USB_EP_STAT_BNA) { /* Buffer was not set up */
        int ctrl = USB_IEP_CTRL(ep);
        logf("ep%d IN, status %x ctrl %x (BNA)\n", ep, ep_sts, ctrl);
        panicf("ep%d IN 0x%x 0x%x (BNA)", ep, ep_sts, ctrl);
    }

    if (ep_sts & USB_EP_STAT_TDC) {
        endpoints[ep][0].state &= ~EP_STATE_BUSY;
        endpoints[ep][0].rc = 0;
        logf("EP%d %x %stx done len %x stat %08x\n",
             ep, ep_sts, endpoints[ep][0].state & EP_STATE_ASYNC ? "async " :"",
             endpoints[ep][0].len,
             endpoints[ep][0].uc_desc->status);
        if (endpoints[ep][0].state & EP_STATE_ASYNC) {
            endpoints[ep][0].state &= ~EP_STATE_ASYNC;
            usb_core_transfer_complete(ep, USB_DIR_IN, 0, endpoints[ep][0].len);
        } else {
            semaphore_release(&endpoints[ep][0].complete);
        }
        ep_sts &= ~USB_EP_STAT_TDC;
    }

    if (ep_sts) {
        logf("ep%d IN, hwstat %lx, epstat %x\n", ep, USB_IEP_STS(ep), endpoints[ep][0].state);
        panicf("ep%d IN 0x%x", ep, ep_sts);
    }
}

static void handle_out_ep(int ep)
{
    struct usb_ctrlrequest *req = (void*)AS3525_UNCACHED_ADDR(&setup_desc->data1);
    int ep_sts = USB_OEP_STS(ep) & ~USB_OEP_STS_MASK(ep);

    if (ep > 3)
        panicf("out_ep > 3!?");

    USB_OEP_STS(ep) = ep_sts; /* ACK */

    if (ep_sts & USB_EP_STAT_BNA) { /* Buffer was not set up */
        int ctrl = USB_OEP_CTRL(ep);
        logf("ep%d OUT, status %x ctrl %x (BNA)\n", ep, ep_sts, ctrl);
        panicf("ep%d OUT 0x%x 0x%x (BNA)", ep, ep_sts, ctrl);
        ep_sts &= ~USB_EP_STAT_BNA;
    }

    if (ep_sts & USB_EP_STAT_OUT_RCVD) {
        struct usb_dev_dma_desc *uc_desc = endpoints[ep][1].uc_desc;
        int dma_sts = uc_desc->status;
        int dma_len = dma_sts & 0xffff;

        if (!(dma_sts & USB_DMA_DESC_ZERO_LEN)) {
             logf("EP%d OUT token, st:%08x len:%d frm:%x data=%s epstate=%d\n",
                 ep, dma_sts & 0xf8000000, dma_len, (dma_sts >> 16) & 0x7ff,
                 make_hex(uc_desc->data_ptr, dma_len), endpoints[ep][1].state);
             /*
              * If parts of the just dmaed range are in cache, dump them now.
              */
             discard_dcache_range(uc_desc->data_ptr, dma_len);
        } else{
             logf("EP%d OUT token, st:%08x frm:%x (no data)\n", ep,
                 dma_sts & 0xf8000000, (dma_sts >> 16) & 0x7ff);
        }

        if (endpoints[ep][1].state & EP_STATE_BUSY) {
            endpoints[ep][1].state &= ~EP_STATE_BUSY;
            endpoints[ep][1].rc = 0;
            usb_core_transfer_complete(ep, USB_DIR_OUT, 0, dma_len);
        } else {
            logf("EP%d OUT, but no one was listening?\n", ep);
        }

        USB_OEP_CTRL(ep) |= USB_EP_CTRL_SNAK; /* make sure NAK is set */
        ep_sts &= ~USB_EP_STAT_OUT_RCVD;
    }

    if (ep_sts & USB_EP_STAT_SETUP_RCVD) {
        static struct usb_ctrlrequest req_copy;

        req_copy = *req;
        logf("t%ld:got SETUP packet: %s type=%d req=%d val=%d ind=%d len=%d\n",
             current_tick,
             make_hex((void*)req, 8),
             req->bRequestType,
             req->bRequest,
             req->wValue,
             req->wIndex,
             req->wLength);

        usb_core_control_request(&req_copy);
        setup_desc_init(setup_desc);

        ep_sts &= ~USB_EP_STAT_SETUP_RCVD;
    }

    if (ep_sts) {
        logf("ep%d OUT, status %x\n", ep, ep_sts);
        panicf("ep%d OUT 0x%x", ep, ep_sts);
    }
}

/*
 * This is a simplified version of the timer based RDE enable from
 * the Linux amd5536udc.c driver.
 * We need this because of the following hw issue:
 * The usb_storage buffer is 63KB, but Linux sends 120KB.
 * We get the first part, but upon re-enabling receive dma we
 * get a 'buffer not available' error from the hardware, since
 * we haven't gotten the next usb_drv_recv() from the stack yet.
 * It seems the NAK bit is ignored here and the HW tries to dma
 * the incoming data anyway.
 * In theory I think the BNA error should be recoverable, but
 * I haven't figured out how to do that yet and this approach seems
 * to work for now.
 */
static void usb_tick(void)
{
    static int rde_timer = 0;
    static int rde_fails = 0;

    if (usb_enum_timeout != -1) {
        /*
         * If the enum times out it's a charger, drop out of usb mode.
         */
        if (usb_enum_timeout-- <= 0)
            usb_remove_int();
    }

    if (USB_DEV_CTRL & USB_DEV_CTRL_RDE)
        return;

    if (!(USB_DEV_STS & USB_DEV_STS_RXF_EMPTY))
        rde_timer++;

    if (rde_timer < 2)
        return;

    logf("usb_tick: re-enabling RDE\n");
    USB_DEV_CTRL |= USB_DEV_CTRL_RDE;
    rde_timer = 0;
    if (USB_DEV_CTRL & USB_DEV_CTRL_RDE) {
        rde_fails = 0;
    } else {
        rde_fails++;
        if (rde_fails > 3)
            panicf("usb_tick: failed to set RDE");
    }
}

/* interrupt service routine */
void INT_USB_FUNC(void)
{
    int ep = USB_DEV_EP_INTR & ~USB_DEV_EP_INTR_MASK;
    int intr = USB_DEV_INTR & ~USB_DEV_INTR_MASK;

    /* ACK interrupt sources */
    USB_DEV_EP_INTR = ep;
    USB_DEV_INTR = intr;

    /* Handle endpoint interrupts */
    while (ep) {
        int onebit = 31-__builtin_clz(ep);

        if (onebit < 16) handle_in_ep(onebit);
        else handle_out_ep(onebit-16);

        ep &= ~(1 << onebit);
    }

    /* Handle general device interrupts */
    if (intr) {
        if (intr & USB_DEV_INTR_SET_INTERFACE) {/* SET_INTERFACE received */
            logf("set interface\n");
            panicf("set interface");
            intr &= ~USB_DEV_INTR_SET_INTERFACE;
        }
        if (intr & USB_DEV_INTR_SET_CONFIG) {/* SET_CONFIGURATION received */
            /*
             * This is handled in HW, we have to fake a request here
             * for usb_core.
             */
            static struct usb_ctrlrequest set_config = {
            bRequestType: USB_TYPE_STANDARD | USB_RECIP_DEVICE,
            bRequest: USB_REQ_SET_CONFIGURATION,
            wValue: 0,
            wIndex: 0,
            wLength: 0,
            };

            logf("set config\n");
            got_set_configuration = 1;

            set_config.wValue = USB_DEV_STS & USB_DEV_STS_MASK_CFG;
            usb_core_control_request(&set_config);
            intr &= ~USB_DEV_INTR_SET_CONFIG;
        }
        if (intr & USB_DEV_INTR_EARLY_SUSPEND) {/* idle >3ms detected */
            logf("usb idle\n");
            intr &= ~USB_DEV_INTR_EARLY_SUSPEND;
        }
        if (intr & USB_DEV_INTR_USB_RESET) {/* usb reset from host? */
            logf("usb reset\n");
            reset_endpoints(1);
            usb_core_bus_reset();
            intr &= ~USB_DEV_INTR_USB_RESET;
        }
        if (intr & USB_DEV_INTR_USB_SUSPEND) {/* suspend req from host? */
            logf("usb suspend\n");
            intr &= ~USB_DEV_INTR_USB_SUSPEND;
        }
        if (intr & USB_DEV_INTR_SOF) {/* sof received */
            logf("sof\n");
            intr &= ~USB_DEV_INTR_SOF;
        }
        if (intr & USB_DEV_INTR_SVC) {/* device status changed */
            logf("svc: %08x otg: %08x\n", (int)USB_DEV_STS, (int)USB_OTG_CSR);
            intr &= ~USB_DEV_INTR_SVC;
        }
        if (intr & USB_DEV_INTR_ENUM_DONE) {/* speed enumeration complete */
            int spd = USB_DEV_STS & USB_DEV_STS_MASK_SPD;  /* Enumerated Speed */
            usb_enum_timeout = -1;

            logf("speed enum complete: ");
            if (spd == USB_DEV_STS_SPD_HS) logf("hs\n");
            if (spd == USB_DEV_STS_SPD_FS) logf("fs\n");
            if (spd == USB_DEV_STS_SPD_LS) logf("ls\n");

            USB_DEV_CTRL |= USB_DEV_CTRL_APCSR_DONE;
            USB_IEP_CTRL(0) |= USB_EP_CTRL_ACT;
            USB_OEP_CTRL(0) |= USB_EP_CTRL_ACT;
            intr &= ~USB_DEV_INTR_ENUM_DONE;
        }
        if (intr & USB_DEV_INTR_MYSTERY) {
            logf("got mystery dev intr\n");
            USB_DEV_INTR_MASK |= USB_DEV_INTR_MYSTERY;
            intr &= ~USB_DEV_INTR_MYSTERY;
        }
        if (intr) {
            logf("usb devirq 0x%x", intr);
            panicf("usb devirq 0x%x", intr);
        }
    }
}

/* (not essential? , not implemented in usb-tcc.c) */
void usb_drv_set_test_mode(int mode)
{
    (void)mode;
}

/* handled internally by controller */
void usb_drv_set_address(int address)
{
    (void)address;
}

void usb_drv_stall(int ep, bool stall, bool in)
{
   if (stall) USB_EP_CTRL(ep, in) |= USB_EP_CTRL_STALL;
   else USB_EP_CTRL(ep, in) &= ~USB_EP_CTRL_STALL;
}

bool usb_drv_stalled(int ep, bool in)
{
    return USB_EP_CTRL(ep, in) & USB_EP_CTRL_STALL;
}
