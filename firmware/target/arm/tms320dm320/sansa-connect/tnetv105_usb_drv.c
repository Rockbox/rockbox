/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2021 by Tomasz Moń
 * Ported from Sansa Connect TNETV105 UDC Linux driver
 * Copyright (c) 2005,2006 Zermatt Systems, Inc.
 * Written by: Ben Bostwick
 * Linux driver was modeled strongly after the pxa usb driver.
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
#include "system.h"
#include "kernel.h"
#include "panic.h"
#include "logf.h"
#include "usb.h"
#include "usb_drv.h"
#include "usb_core.h"
#include <string.h>
#include "tnetv105_usb_drv.h"
#include "tnetv105_cppi.h"

#ifdef SANSA_CONNECT
#define SDRAM_SIZE 0x04000000

static void set_tnetv_reset(bool high)
{
    if (high)
    {
        IO_GIO_BITSET0 = (1 << 7);
    }
    else
    {
        IO_GIO_BITCLR0 = (1 << 7);
    }
}

static bool is_tnetv_reset_high(void)
{
    return (IO_GIO_BITSET0 & (1 << 7)) ? true : false;
}
#endif

static bool setup_is_set_address;

static cppi_info cppi;

static struct ep_runtime_t
{
    int max_packet_size;
    bool in_allocated;
    bool out_allocated;
    uint8_t *rx_buf;             /* OUT */
    int rx_remaining;
    int rx_size;
    uint8_t *tx_buf;             /* IN */
    int tx_remaining;
    int tx_size;
    volatile bool block;         /* flag indicating that transfer is blocking */
    struct semaphore complete;   /* semaphore for blocking transfers */
}
ep_runtime[USB_NUM_ENDPOINTS];

static const struct
{
    int type;
    int hs_max_packet_size;
    /* Not sure what xyoff[1] is for. Presumably it is double buffer, but how
     * the double buffering works is not so clear from the Sansa Connect Linux
     * kernel patch. As TNETV105 datasheet is not available, the values are
     * simply taken from the Linux patch as potential constraints are unknown.
     *
     * Linux kernel has 9 endpoints:
     *     * 0: ep0
     *     * 1: ep1in-bulk
     *     * 2: ep2out-bulk
     *     * 3: ep3in-int
     *     * 4: ep4in-int
     *     * 5: ep1out-bulk
     *     * 6: ep2in-bulk
     *     * 7: ep3out-int
     *     * 8: ep4out-int
     */
    uint16_t xyoff_in[2];
    uint16_t xyoff_out[2];
}
ep_const_data[USB_NUM_ENDPOINTS] =
{
    {
        .type = USB_ENDPOINT_XFER_CONTROL,
        .hs_max_packet_size = EP0_MAX_PACKET_SIZE,
        /* Do not set xyoff as it likely does not apply here.
         * Linux simply hardcodes the offsets when needed.
         */
    },
    {
        .type = USB_ENDPOINT_XFER_BULK,
        .hs_max_packet_size = EP1_MAX_PACKET_SIZE,
        .xyoff_in = {EP1_XBUFFER_ADDRESS, EP1_YBUFFER_ADDRESS},
        .xyoff_out = {EP5_XBUFFER_ADDRESS, EP5_YBUFFER_ADDRESS},
    },
    {
        .type = USB_ENDPOINT_XFER_BULK,
        .hs_max_packet_size = EP2_MAX_PACKET_SIZE,
        .xyoff_in = {EP6_XBUFFER_ADDRESS, EP6_YBUFFER_ADDRESS},
        .xyoff_out = {EP2_XBUFFER_ADDRESS, EP2_YBUFFER_ADDRESS},
    },
    {
        .type = USB_ENDPOINT_XFER_INT,
        .hs_max_packet_size = EP3_MAX_PACKET_SIZE,
        .xyoff_in = {EP3_XBUFFER_ADDRESS, EP3_YBUFFER_ADDRESS},
        .xyoff_out = {EP7_XBUFFER_ADDRESS, EP7_YBUFFER_ADDRESS},
    },
    {
        .type = USB_ENDPOINT_XFER_INT,
        .hs_max_packet_size = EP4_MAX_PACKET_SIZE,
        .xyoff_in = {EP4_XBUFFER_ADDRESS, EP4_YBUFFER_ADDRESS},
        .xyoff_out = {EP8_XBUFFER_ADDRESS, EP8_YBUFFER_ADDRESS},
    },
};

#define VLYNQ_CTL_RESET_MASK    0x0001
#define VLYNQ_CTL_CLKDIR_MASK   0x8000
#define VLYNQ_STS_LINK_MASK     0x0001

#define DM320_VLYNQ_CTRL_RESET         (1 << 0)
#define DM320_VLYNQ_CTRL_LOOP          (1 << 1)
#define DM320_VLYNQ_CTRL_ADR_OPT       (1 << 2)
#define DM320_VLYNQ_CTRL_INT_CFG       (1 << 7)
#define DM320_VLYNQ_CTRL_INT_VEC_MASK  (0x00001F00)
#define DM320_VLYNQ_CTRL_INT_EN        (1 << 13)
#define DM320_VLYNQ_CTRL_INT_LOC       (1 << 14)
#define DM320_VLYNQ_CTRL_CLKDIR        (1 << 15)
#define DM320_VLYNQ_CTRL_CLKDIV_MASK   (0x00070000)
#define DM320_VLYNQ_CTRL_PWR_MAN       (1 << 31)

#define DM320_VLYNQ_STAT_LINK          (1 << 0)
#define DM320_VLYNQ_STAT_MST_PEND      (1 << 1)
#define DM320_VLYNQ_STAT_SLV_PEND      (1 << 2)
#define DM320_VLYNQ_STAT_F0_NE         (1 << 3)
#define DM320_VLYNQ_STAT_F1_NE         (1 << 4)
#define DM320_VLYNQ_STAT_F2_NE         (1 << 5)
#define DM320_VLYNQ_STAT_F3_NE         (1 << 6)
#define DM320_VLYNQ_STAT_LOC_ERR       (1 << 7)
#define DM320_VLYNQ_STAT_REM_ERR       (1 << 8)
#define DM320_VLYNQ_STAT_FC_OUT        (1 << 9)
#define DM320_VLYNQ_STAT_FC_IN         (1 << 10)

#define MAX_PACKET(epn, speed) ((((speed) == USB_SPEED_HIGH) && (((epn) == 1) || ((epn) == 2))) ? USB_HIGH_SPEED_MAXPACKET : USB_FULL_SPEED_MAXPACKET)

#define VLYNQ_INTR_USB20      (1 << 0)
#define VLYNQ_INTR_CPPI       (1 << 1)

static inline void set_vlynq_clock(bool enable)
{
    if (enable)
    {
        IO_CLK_MOD2 |= (1 << 13);
    }
    else
    {
        IO_CLK_MOD2 &= ~(1 << 13);
    }
}

static inline void set_vlynq_irq(bool enabled)
{
    if (enabled)
    {
        /* Enable VLYNQ interrupt */
        IO_INTC_EINT1 |= (1 << 0);
    }
    else
    {
        IO_INTC_EINT1 &= ~(1 << 0);
    }
}

static int tnetv_hw_reset(void)
{
    int timeout;

    /* hold down the reset pin on the USB chip */
    set_tnetv_reset(false);

    /* Turn on VLYNQ clock. */
    set_vlynq_clock(true);

    /* now reset the VLYNQ module */
    VL_CTRL |= (VLYNQ_CTL_CLKDIR_MASK | DM320_VLYNQ_CTRL_PWR_MAN);
    VL_CTRL |= VLYNQ_CTL_RESET_MASK;

    mdelay(10);

    /* pull up the reset pin */
    set_tnetv_reset(true);

    /* take the VLYNQ out of reset */
    VL_CTRL &= ~VLYNQ_CTL_RESET_MASK;

    timeout = 0;
    while (!(VL_STAT & VLYNQ_STS_LINK_MASK) && timeout++ < 50);
    {
        mdelay(40);
    }

    if (!(VL_STAT & VLYNQ_STS_LINK_MASK))
    {
        logf("ERROR: VLYNQ not initialized!\n");
        return -1;
    }

    /* set up vlynq local map */
    VL_TXMAP = DM320_VLYNQ_PADDR;
    VL_RXMAPOF1 = CONFIG_SDRAM_START;
    VL_RXMAPSZ1 = SDRAM_SIZE;

    /* set up vlynq remote map for tnetv105 */
    VL_TXMAP_R = 0x00000000;
    VL_RXMAPOF1_R = 0x0C000000;
    VL_RXMAPSZ1_R = 0x00030000;

    /* clear TNETV gpio state */
    tnetv_usb_reg_write(TNETV_V2USB_GPIO_FS, 0);

    /* set USB_CHARGE_EN pin (gpio 1) - output, disable pullup */
    tnetv_usb_reg_write(TNETV_V2USB_GPIO_DOUT, 0);
    tnetv_usb_reg_write(TNETV_V2USB_GPIO_DIR, 0xffff);

    return 0;
}

static int tnetv_xcvr_on(void)
{
    return tnetv_hw_reset();
}

static void tnetv_xcvr_off(void)
{
    /* turn off vlynq module clock */
    set_vlynq_clock(false);

    /* hold down the reset pin on the USB chip */
    set_tnetv_reset(false);
}

/* Copy data from the usb data memory. The memory reads should be done 32 bits at a time.
 * We do not assume that the dst data is aligned.
 */
static void tnetv_copy_from_data_mem(void *dst, const volatile uint32_t *sp, int size)
{
    uint8_t *dp = (uint8_t *) dst;
    uint32_t value;

    while (size >= 4)
    {
        value = *sp++;
        dp[0] = value;
        dp[1] = value >> 8;
        dp[2] = value >> 16;
        dp[3] = value >> 24;
        dp += 4;
        size -= 4;
    }

    if (size)
    {
        value = sp[0];
        switch (size)
        {
            case 3:
                dp[2] = value >> 16;
            case 2:
                dp[1] = value >> 8;
            case 1:
                dp[0] = value;
                break;
        }
    }
}

/* Copy data into the usb data memory. The memory writes must be done 32 bits at a time.
 * We do not assume that the src data is aligned.
*/
static void tnetv_copy_to_data_mem(volatile uint32_t *dp, const void *src, int size)
{
    const uint8_t *sp = (const uint8_t *) src;
    uint32_t value;

    while (size >= 4)
    {
        value = sp[0] | (sp[1] << 8) | (sp[2] << 16) | (sp[3] << 24);
        *dp++ = value;
        sp += 4;
        size -= 4;
    }

    switch (size)
    {
        case 3:
            value = sp[0] | (sp[1] << 8) | (sp[2] << 16);
            *dp = value;
            break;
        case 2:
            value = sp[0] | (sp[1] << 8);
            *dp = value;
            break;
        case 1:
            value = sp[0];
            *dp = value;
            break;
    }
}

static void tnetv_init_endpoints(void)
{
    UsbEp0CtrlType ep0Cfg;
    UsbEp0ByteCntType ep0Cnt;
    UsbEpCfgCtrlType epCfg;
    UsbEpStartAddrType epStartAddr;
    int ch, wd, epn;

    ep0Cnt.val = 0;
    ep0Cnt.f.out_ybuf_nak = 1;
    ep0Cnt.f.out_xbuf_nak = 1;
    ep0Cnt.f.in_ybuf_nak = 1;
    ep0Cnt.f.in_xbuf_nak = 1;
    tnetv_usb_reg_write(TNETV_USB_EP0_CNT, ep0Cnt.val);

    /* Setup endpoint zero */
    ep0Cfg.val = 0;
    ep0Cfg.f.buf_size = EP0_BUF_SIZE_64;  /* must be 64 bytes for USB 2.0 */
    ep0Cfg.f.dbl_buf = 0;
    ep0Cfg.f.in_en = 1;
    ep0Cfg.f.in_int_en = 1;
    ep0Cfg.f.out_en = 1;
    ep0Cfg.f.out_int_en = 1;
    tnetv_usb_reg_write(TNETV_USB_EP0_CFG, ep0Cfg.val);

    /* disable cell dma */
    tnetv_usb_reg_write(TNETV_USB_CELL_DMA_EN, 0);

    /* turn off dma engines */
    tnetv_usb_reg_write(TNETV_USB_TX_CTL, 0);
    tnetv_usb_reg_write(TNETV_USB_RX_CTL, 0);

    /* clear out DMA registers */
    for (ch = 0; ch < TNETV_DMA_NUM_CHANNELS; ch++)
    {
        for (wd = 0; wd < TNETV_DMA_TX_NUM_WORDS; wd++)
        {
            tnetv_usb_reg_write(TNETV_DMA_TX_STATE(ch, wd), 0);
        }

        for (wd = 0; wd < TNETV_DMA_RX_NUM_WORDS; wd++)
        {
            tnetv_usb_reg_write(TNETV_DMA_RX_STATE(ch, wd), 0);
        }

        /* flush the free buf count */
        while (tnetv_usb_reg_read(TNETV_USB_RX_FREE_BUF_CNT(ch)) != 0)
        {
            tnetv_usb_reg_write(TNETV_USB_RX_FREE_BUF_CNT(ch), 0xFFFF);
        }
    }

    for (epn = 1; epn < USB_NUM_ENDPOINTS; epn++)
    {
        tnetv_usb_reg_write(TNETV_USB_EPx_ADR(epn),0);
        tnetv_usb_reg_write(TNETV_USB_EPx_CFG(epn), 0);
        tnetv_usb_reg_write(TNETV_USB_EPx_IN_CNT(epn), 0x80008000);
        tnetv_usb_reg_write(TNETV_USB_EPx_OUT_CNT(epn), 0x80008000);
    }

    /* Setup the other endpoints */
    for (epn = 1; epn < USB_NUM_ENDPOINTS; epn++)
    {
        epCfg.val = tnetv_usb_reg_read(TNETV_USB_EPx_CFG(epn));
        epStartAddr.val = tnetv_usb_reg_read(TNETV_USB_EPx_ADR(epn));

        /* Linux kernel enables dbl buf for both IN and OUT.
         * For IN this is problematic when tnetv_cppi_send() is called
         * to send single ZLP, it will actually send two ZLPs.
         * Disable the dbl buf here as datasheet is not available and
         * this results in working mass storage on Windows 10.
         */
        epCfg.f.in_dbl_buf = 0;
        epCfg.f.in_toggle_rst = 1;
        epCfg.f.in_ack_int = 0;
        epCfg.f.in_stall = 0;
        epCfg.f.in_nak_int = 0;
        epCfg.f.out_dbl_buf = 0;
        epCfg.f.out_toggle_rst = 1;
        epCfg.f.out_ack_int = 0;
        epCfg.f.out_stall = 0;
        epCfg.f.out_nak_int = 0;

        /* buf_size is specified "in increments of 8 bytes" */
        epCfg.f.in_buf_size = ep_const_data[epn].hs_max_packet_size >> 3;
        epCfg.f.out_buf_size = ep_const_data[epn].hs_max_packet_size >> 3;

        epStartAddr.f.xBuffStartAddrIn = ep_const_data[epn].xyoff_in[0] >> 4;
        epStartAddr.f.yBuffStartAddrIn = ep_const_data[epn].xyoff_in[1] >> 4;
        epStartAddr.f.xBuffStartAddrOut = ep_const_data[epn].xyoff_out[0] >> 4;
        epStartAddr.f.yBuffStartAddrOut = ep_const_data[epn].xyoff_out[1] >> 4;

        /* allocate memory for DMA */
        tnetv_cppi_init_rcb(&cppi, (epn - 1));
        /* set up DMA queue */
        tnetv_cppi_init_tcb(&cppi, (epn - 1));

        /* now write out the config to the TNETV (write enable bits last) */
        tnetv_usb_reg_write(TNETV_USB_EPx_ADR(epn), epStartAddr.val);
        tnetv_usb_reg_write(TNETV_USB_EPx_CFG(epn), epCfg.val);
        tnetv_usb_reg_write(TNETV_USB_EPx_IN_CNT(epn), 0x80008000);
        tnetv_usb_reg_write(TNETV_USB_EPx_OUT_CNT(epn), 0x80008000);
    }

    /* turn on dma engines */
    tnetv_usb_reg_write(TNETV_USB_TX_CTL, 1);
    tnetv_usb_reg_write(TNETV_USB_RX_CTL, 1);

    /* enable cell dma */
    tnetv_usb_reg_write(TNETV_USB_CELL_DMA_EN, (TNETV_USB_CELL_DMA_EN_RX | TNETV_USB_CELL_DMA_EN_TX));
}

static void tnetv_udc_enable_interrupts(void)
{
    UsbCtrlType usb_ctl;
    uint8_t tx_int_en, rx_int_en;
    int ep, chan;

    /* set up the system interrupts */
    usb_ctl.val = tnetv_usb_reg_read(TNETV_USB_CTRL);
    usb_ctl.f.vbus_int_en = 1;
    usb_ctl.f.reset_int_en = 1;
    usb_ctl.f.suspend_int_en = 1;
    usb_ctl.f.resume_int_en = 1;
    usb_ctl.f.ep0_in_int_en = 1;
    usb_ctl.f.ep0_out_int_en = 1;
    usb_ctl.f.setup_int_en = 1;
    usb_ctl.f.setupow_int_en = 1;
    tnetv_usb_reg_write(TNETV_USB_CTRL, usb_ctl.val);

    /* Enable the DMA endpoint interrupts */
    tx_int_en = 0;
    rx_int_en = 0;

    for (ep = 1; ep < USB_NUM_ENDPOINTS; ep++)
    {
        chan = ep - 1;
        rx_int_en |= (1 << chan); /* OUT */
        tx_int_en |= (1 << chan); /* IN */
    }

    /* enable rx interrupts */
    tnetv_usb_reg_write(TNETV_USB_RX_INT_EN, rx_int_en);
    /* enable tx interrupts */
    tnetv_usb_reg_write(TNETV_USB_TX_INT_EN, tx_int_en);

    set_vlynq_irq(true);
}

static void tnetv_udc_disable_interrupts(void)
{
    UsbCtrlType usb_ctl;

    /* disable interrupts from linux */
    set_vlynq_irq(false);

    /* Disable Endpoint Interrupts */
    tnetv_usb_reg_write(TNETV_USB_RX_INT_DIS, 0x3);
    tnetv_usb_reg_write(TNETV_USB_TX_INT_DIS, 0x3);

    /* Disable USB system interrupts */
    usb_ctl.val = tnetv_usb_reg_read(TNETV_USB_CTRL);
    usb_ctl.f.vbus_int_en = 0;
    usb_ctl.f.reset_int_en = 0;
    usb_ctl.f.suspend_int_en = 0;
    usb_ctl.f.resume_int_en = 0;
    usb_ctl.f.ep0_in_int_en = 0;
    usb_ctl.f.ep0_out_int_en = 0;
    usb_ctl.f.setup_int_en = 0;
    usb_ctl.f.setupow_int_en = 0;
    tnetv_usb_reg_write(TNETV_USB_CTRL, usb_ctl.val);
}

static void tnetv_ep_halt(int epn, bool in)
{
    if (in)
    {
        tnetv_usb_reg_write(TNETV_USB_EPx_IN_CNT(epn), 0x80008000);
    }
    else
    {
        tnetv_usb_reg_write(TNETV_USB_EPx_OUT_CNT(epn), 0x80008000);
    }
}

/* Reset the TNETV usb2.0 controller and configure it to run in function mode */
static void tnetv_usb_reset(void)
{
    uint32_t timeout = 0;
    int wd;
    int ch;

    /* configure function clock */
    tnetv_usb_reg_write(TNETV_V2USB_CLK_CFG, 0x80);

    /* Reset the USB 2.0 function module */
    tnetv_usb_reg_write(TNETV_V2USB_RESET, 0x01);

    /* now poll the module ready register until the 2.0 controller finishes resetting */
    while (!(tnetv_usb_reg_read(TNETV_USB_RESET_CMPL) & 0x1) && (timeout < 1000000))
    {
        timeout++;
    }

    if (!(tnetv_usb_reg_read(TNETV_USB_RESET_CMPL) & 0x1))
    {
        logf("tnetv105_udc: VLYNQ USB module reset failed!\n");
        return;
    }

    /* turn off external clock */
    tnetv_usb_reg_write(TNETV_V2USB_CLK_PERF, 0);

    /* clear out USB data memory */
    for (wd = 0; wd < TNETV_EP_DATA_SIZE; wd += 4)
    {
        tnetv_usb_reg_write(TNETV_EP_DATA_ADDR(wd), 0);
    }

    /* clear out DMA memory */
    for (ch = 0; ch < TNETV_DMA_NUM_CHANNELS; ch++)
    {
        for (wd = 0; wd < TNETV_DMA_TX_NUM_WORDS; wd++)
        {
            tnetv_usb_reg_write(TNETV_DMA_TX_STATE(ch, wd), 0);
        }

        for (wd = 0; wd < TNETV_DMA_RX_NUM_WORDS; wd++)
        {

            tnetv_usb_reg_write(TNETV_DMA_RX_STATE(ch, wd), 0);
        }
    }

    /* point VLYNQ interrupts at the pending register */
    VL_INTPTR = DM320_VLYNQ_INTPND_PHY;

    /* point VLYNQ remote interrupts at the pending register */
    VL_INTPTR_R = 0;

    /* clear out interrupt register */
    VL_INTST |= 0xFFFFFFFF;

    /* enable interrupts on remote device */
    VL_CTRL_R |= (DM320_VLYNQ_CTRL_INT_EN);
    VL_INTVEC30_R = 0x8180;

    /* enable VLYNQ interrupts & set interrupts to trigger VLYNQ int */
    VL_CTRL |= (DM320_VLYNQ_CTRL_INT_LOC | DM320_VLYNQ_CTRL_INT_CFG);
}

static int tnetv_ep_start_xmit(int epn, void *buf, int size)
{
    UsbEp0ByteCntType ep0Cnt;

    if (epn == 0)
    {
        /* Write the Control Data packet to the EP0 IN memory area */
        tnetv_copy_to_data_mem(TNETV_EP_DATA_ADDR(EP0_INPKT_ADDRESS), buf, size);

        /* start xmitting */
        ep0Cnt.val = tnetv_usb_reg_read(TNETV_USB_EP0_CNT);
        ep0Cnt.f.in_xbuf_cnt = size;
        ep0Cnt.f.in_xbuf_nak = 0;
        tnetv_usb_reg_write(TNETV_USB_EP0_CNT, ep0Cnt.val);
    }
    else
    {
        dma_addr_t buffer = (dma_addr_t)buf;
        int send_zlp = 0;
        if (size == 0)
        {
            /* Any address in SDRAM will do, contents do not matter */
            buffer = CONFIG_SDRAM_START;
            size = 1;
            send_zlp = 1;
        }
        else
        {
            commit_discard_dcache_range(buf, size);
        }
        if ((buffer >= CONFIG_SDRAM_START) && (buffer + size < CONFIG_SDRAM_START + SDRAM_SIZE))
        {
            if (tnetv_cppi_send(&cppi, (epn - 1), buffer, size, send_zlp))
            {
                panicf("tnetv_cppi_send() failed");
            }
        }
        else
        {
            panicf("USB xmit buf outside SDRAM %p", buf);
        }
    }

    return 0;
}

static void tnetv_gadget_req_nuke(int epn, bool in)
{
    struct ep_runtime_t *ep = &ep_runtime[epn];
    uint32_t old_rx_int = 0;
    uint32_t old_tx_int = 0;
    int ch;
    int flags;

    /* don't nuke control ep */
    if (epn == 0)
    {
        return;
    }

    flags = disable_irq_save();

    /* save and disable interrupts before nuking request */
    old_rx_int = tnetv_usb_reg_read(TNETV_USB_RX_INT_EN);
    old_tx_int = tnetv_usb_reg_read(TNETV_USB_TX_INT_EN);
    tnetv_usb_reg_write(TNETV_USB_RX_INT_DIS, 0x3);
    tnetv_usb_reg_write(TNETV_USB_TX_INT_DIS, 0x3);

    ch = epn - 1;

    if (in)
    {
        tnetv_cppi_flush_tx_queue(&cppi, ch);

        tnetv_usb_reg_write(TNETV_USB_EPx_IN_CNT(epn), 0x80008000);
        if (ep->tx_remaining > 0)
        {
            usb_core_transfer_complete(epn, USB_DIR_IN, -1, 0);
        }
        ep->tx_buf = NULL;
        ep->tx_remaining = 0;
        ep->tx_size = 0;

        if (ep->block)
        {
            semaphore_release(&ep->complete);
            ep->block = false;
        }
    }
    else
    {
        tnetv_cppi_flush_rx_queue(&cppi, ch);

        tnetv_usb_reg_write(TNETV_USB_EPx_OUT_CNT(epn), 0x80008000);
        if (ep->rx_remaining > 0)
        {
            usb_core_transfer_complete(epn, USB_DIR_OUT, -1, 0);
        }
        ep->rx_buf = NULL;
        ep->rx_remaining = 0;
        ep->rx_size = 0;
    }

    /* reenable any interrupts */
    tnetv_usb_reg_write(TNETV_USB_RX_INT_EN, old_rx_int);
    tnetv_usb_reg_write(TNETV_USB_TX_INT_EN, old_tx_int);

    restore_irq(flags);
}

static int tnetv_gadget_ep_enable(int epn, bool in)
{
    UsbEpCfgCtrlType epCfg;
    int flags;
    enum usb_device_speed speed;

    if (epn == 0 || epn >= USB_NUM_ENDPOINTS)
    {
        return 0;
    }

    flags = disable_irq_save();

    /* set the maxpacket for this endpoint based on the current speed */
    speed = usb_drv_port_speed() ? USB_SPEED_HIGH : USB_SPEED_FULL;
    ep_runtime[epn].max_packet_size = MAX_PACKET(epn, speed);

    /* Enable the endpoint */
    epCfg.val = tnetv_usb_reg_read(TNETV_USB_EPx_CFG(epn));
    if (in)
    {
        epCfg.f.in_en = 1;
        epCfg.f.in_stall = 0;
        epCfg.f.in_toggle_rst = 1;
        epCfg.f.in_buf_size = ep_runtime[epn].max_packet_size >> 3;
        tnetv_usb_reg_write(TNETV_USB_EPx_IN_CNT(epn), 0x80008000);
    }
    else
    {
        epCfg.f.out_en = 1;
        epCfg.f.out_stall = 0;
        epCfg.f.out_toggle_rst = 1;
        epCfg.f.out_buf_size = ep_runtime[epn].max_packet_size >> 3;
        tnetv_usb_reg_write(TNETV_USB_EPx_OUT_CNT(epn), 0x80008000);
    }
    tnetv_usb_reg_write(TNETV_USB_EPx_CFG(epn), epCfg.val);

    restore_irq(flags);

    return 0;
}

static int tnetv_gadget_ep_disable(int epn, bool in)
{
    UsbEpCfgCtrlType epCfg;
    int flags;

    if (epn == 0 || epn >= USB_NUM_ENDPOINTS)
    {
        return 0;
    }

    flags = disable_irq_save();

    /* Disable the endpoint */
    epCfg.val = tnetv_usb_reg_read(TNETV_USB_EPx_CFG(epn));
    if (in)
    {
        epCfg.f.in_en = 0;
    }
    else
    {
        epCfg.f.out_en = 0;
    }
    tnetv_usb_reg_write(TNETV_USB_EPx_CFG(epn), epCfg.val);

    /* Turn off the endpoint and unready it */
    tnetv_ep_halt(epn, in);

    restore_irq(flags);

    /* Clear out all the pending requests */
    tnetv_gadget_req_nuke(epn, in);

    return 0;
}

/* TNETV udc goo
 * Power up and enable the udc. This includes resetting the hardware, turn on the appropriate clocks
 * and initializing things so that the first setup packet can be received.
 */
static void tnetv_udc_enable(void)
{
    /* Enable M48XI crystal resonator */
    IO_CLK_LPCTL1 &= ~(0x01);

    /* Set GIO33 as CLKOUT1B */
    IO_GIO_FSEL3 |= 0x0003;

    if (tnetv_xcvr_on())
        return;

    tnetv_usb_reset();

    /* BEN - RNDIS mode is assuming zlps after packets that are multiples of buffer endpoints
     *              zlps are not required by the spec and many controllers don't send them.
     * set DMA to RNDIS mode (packet concatenation, less interrupts)
     * tnetv_usb_reg_write(TNETV_USB_RNDIS_MODE, 0xFF);
     */
    tnetv_usb_reg_write(TNETV_USB_RNDIS_MODE, 0);

    tnetv_init_endpoints();

    tnetv_udc_enable_interrupts();
}

static void tnetv_udc_disable(void)
{
    tnetv_udc_disable_interrupts();

    tnetv_hw_reset();

    tnetv_xcvr_off();

    /* Set GIO33 as normal output, drive it low */
    IO_GIO_FSEL3 &= ~(0x0003);
    IO_GIO_BITCLR2 = (1 << 1);

    /* Disable M48XI crystal resonator */
    IO_CLK_LPCTL1 |= 0x01;
}

static void tnetv_udc_handle_reset(void)
{
    UsbCtrlType usbCtrl;

    /* disable USB interrupts */
    tnetv_udc_disable_interrupts();

    usbCtrl.val = tnetv_usb_reg_read(TNETV_USB_CTRL);
    usbCtrl.f.func_addr = 0;
    tnetv_usb_reg_write(TNETV_USB_CTRL, usbCtrl.val);

    /* Reset endpoints */
    tnetv_init_endpoints();

    /* Re-enable interrupts */
    tnetv_udc_enable_interrupts();
}

static void ep_write(int epn)
{
    struct ep_runtime_t *ep = &ep_runtime[epn];
    int tx_size;
    if (epn == 0)
    {
        tx_size = MIN(ep->max_packet_size, ep->tx_remaining);
    }
    else
    {
        /* DMA takes care of splitting the buffer into packets,
         * but only up to CPPI_MAX_FRAG. After the data is sent
         * a single interrupt is generated. There appears to be
         * splitting code in the tnetv_cppi_send() function but
         * it is somewhat suspicious (it doesn't seem like it
         * will work with requests larger than 2*CPPI_MAX_FRAG).
         * Also, if tnetv_cppi_send() does the splitting, we will
         * get an interrupt after CPPI_MAX_FRAG but before the
         * full request is sent.
         *
         * CPPI_MAX_FRAG is multiple of both 64 and 512 so we
         * don't have to worry about this split prematurely ending
         * the transfer.
         */
        tx_size = MIN(CPPI_MAX_FRAG, ep->tx_remaining);
    }
    tnetv_ep_start_xmit(epn, ep->tx_buf, tx_size);
    ep->tx_remaining -= tx_size;
    ep->tx_buf += tx_size;
}

static void in_interrupt(int epn)
{
    struct ep_runtime_t *ep = &ep_runtime[epn];

    if (ep->tx_remaining <= 0)
    {
        usb_core_transfer_complete(epn, USB_DIR_IN, 0, ep->tx_size);
        /* release semaphore for blocking transfer */
        if (ep->block)
        {
            semaphore_release(&ep->complete);
            ep->tx_buf = NULL;
            ep->tx_size = 0;
            ep->tx_remaining = 0;
            ep->block = false;
        }
    }
    else if (ep->tx_buf)
    {
        ep_write(epn);
    }
}

static void ep_read(int epn)
{
    if (epn == 0)
    {
        UsbEp0ByteCntType ep0Cnt;
        ep0Cnt.val = tnetv_usb_reg_read(TNETV_USB_EP0_CNT);
        ep0Cnt.f.out_xbuf_nak = 0;
        tnetv_usb_reg_write(TNETV_USB_EP0_CNT, ep0Cnt.val);
    }
    else
    {
        struct ep_runtime_t *ep = &ep_runtime[epn];
        tnetv_cppi_rx_queue_add(&cppi, (epn - 1), 0, ep->rx_remaining);
    }
}

static void out_interrupt(int epn)
{
    struct ep_runtime_t *ep = &ep_runtime[epn];
    int is_short;
    int rcv_len;

    if (epn == 0)
    {
        UsbEp0ByteCntType ep0Cnt;

        /* get the length of the received data */
        ep0Cnt.val = tnetv_usb_reg_read(TNETV_USB_EP0_CNT);
        rcv_len = ep0Cnt.f.out_xbuf_cnt;

        if (rcv_len > ep->rx_remaining)
        {
            rcv_len = ep->rx_remaining;
        }

        tnetv_copy_from_data_mem(ep->rx_buf, TNETV_EP_DATA_ADDR(EP0_OUTPKT_ADDRESS), rcv_len);
        ep->rx_buf += rcv_len;
        ep->rx_remaining -= rcv_len;

        /* See if we are done */
        is_short = rcv_len && (rcv_len < ep->max_packet_size);
        if (is_short || (ep->rx_remaining == 0))
        {
            usb_core_transfer_complete(epn, USB_DIR_OUT, 0, ep->rx_size - ep->rx_remaining);
            ep->rx_remaining = 0;
            ep->rx_size = 0;
            ep->rx_buf = 0;
            return;
        }

        /* make sure nak is cleared only if we expect more data */
        ep0Cnt.f.out_xbuf_nak = 0;
        tnetv_usb_reg_write(TNETV_USB_EP0_CNT, ep0Cnt.val);
        ep_read(epn);
    }
    else if (ep->rx_remaining > 0)
    {
        int ret, bytes_rcvd;

        /* copy the data from the DMA buffers */
        bytes_rcvd = ep->rx_remaining;
        ret = tnetv_cppi_rx_int_recv(&cppi, (epn - 1), &bytes_rcvd, ep->rx_buf, ep->max_packet_size);
        if (ret == 0 || ret == -EAGAIN)
        {
            ep->rx_buf += bytes_rcvd;
            ep->rx_remaining -= bytes_rcvd;
        }

        /* complete the request if we got a short packet or an error
         * make sure we don't complete a request with zero bytes.
         */
        if ((ret == 0) && (ep->rx_remaining != ep->rx_size))
        {
            usb_core_transfer_complete(epn, USB_DIR_OUT, 0, ep->rx_size - ep->rx_remaining);
            ep->rx_remaining = 0;
            ep->rx_size = 0;
            ep->rx_buf = 0;
        }
    }
}

static bool tnetv_handle_cppi(void)
{
    int ret;
    int ch;
    uint32_t tx_intstatus;
    uint32_t rx_intstatus;
    uint32_t status;
    int rcv_sched = 0;

    rx_intstatus = tnetv_usb_reg_read(TNETV_USB_RX_INT_STATUS);
    tx_intstatus = tnetv_usb_reg_read(TNETV_USB_TX_INT_STATUS);

    /* handle any transmit interrupts */
    status = tx_intstatus;
    for (ch = 0; ch < CPPI_NUM_CHANNELS && status; ch++)
    {
        if (status & 0x1)
        {
            ret = tnetv_cppi_tx_int(&cppi, ch);
            if (ret >= 0)
            {
                in_interrupt(ch + 1);
            }
        }

        status = status >> 1;
    }

    rcv_sched = 0;
    status = rx_intstatus;
    for (ch = 0; ch < CPPI_NUM_CHANNELS; ch++)
    {
        if (status & 0x1 || tnetv_cppi_rx_int_recv_check(&cppi, ch))
        {
            ret = tnetv_cppi_rx_int(&cppi, ch);
            if (ret < 0)
            {
                /* only an error if interrupt bit is set */
                logf("CPPI Rx: failed to ACK int!\n");
            }
            else
            {
                if (tnetv_cppi_rx_int_recv_check(&cppi, ch))
                {
                    out_interrupt(ch + 1);
                }
            }
        }

        if (tnetv_cppi_rx_int_recv_check(&cppi, ch))
        {
            rcv_sched = 1;
        }

        status = status >> 1;
    }

    rx_intstatus = tnetv_usb_reg_read(TNETV_USB_RX_INT_STATUS);
    tx_intstatus = tnetv_usb_reg_read(TNETV_USB_TX_INT_STATUS);

    if (rx_intstatus || tx_intstatus || rcv_sched)
    {
        /* Request calling again after short delay
         * Needed when for example when OUT endpoint has pending data
         * but the USB task did not call usb_drv_recv_nonblocking() yet.
         */
        return true;
    }
    return false;
}

static int cppi_timeout_cb(struct timeout *tmo)
{
    (void)tmo;
    int flags = disable_irq_save();
    bool requeue = tnetv_handle_cppi();
    restore_irq(flags);
    return requeue ? 1 : 0;
}

void VLYNQ(void) __attribute__ ((section(".icode")));
void VLYNQ(void)
{
    UsbStatusType sysIntrStatus;
    UsbStatusType sysIntClear;
    UsbCtrlType usbCtrl;
    volatile uint32_t *reg;
    uint32_t vlynq_intr;

    /* Clear interrupt */
    IO_INTC_IRQ1 = (1 << 0);

    /* clear out VLYNQ interrupt register */
    vlynq_intr = VL_INTST;

    if (vlynq_intr & VLYNQ_INTR_USB20)
    {
        VL_INTST = VLYNQ_INTR_USB20;

        /* Examine system interrupt status */
        sysIntrStatus.val = tnetv_usb_reg_read(TNETV_USB_STATUS);

        if (sysIntrStatus.f.reset)
        {
            sysIntClear.val = 0;
            sysIntClear.f.reset = 1;
            tnetv_usb_reg_write(TNETV_USB_STATUS, sysIntClear.val);

            tnetv_udc_handle_reset();
            usb_core_bus_reset();
        }

        if (sysIntrStatus.f.suspend)
        {
            sysIntClear.val = 0;
            sysIntClear.f.suspend = 1;
            tnetv_usb_reg_write(TNETV_USB_STATUS, sysIntClear.val);
        }

        if (sysIntrStatus.f.resume)
        {
            sysIntClear.val = 0;
            sysIntClear.f.resume = 1;
            tnetv_usb_reg_write(TNETV_USB_STATUS, sysIntClear.val);
        }

        if (sysIntrStatus.f.vbus)
        {
            sysIntClear.val = 0;
            sysIntClear.f.vbus = 1;
            tnetv_usb_reg_write(TNETV_USB_STATUS, sysIntClear.val);

            if (*((uint32_t *) TNETV_USB_IF_STATUS) & 0x40)
            {
                /* write out connect bit */
                reg = (volatile uint32_t *) TNETV_USB_CTRL;
                *reg |= 0x80;

                /* write to wakeup bit in clock config */
                reg = (volatile uint32_t *) TNETV_V2USB_CLK_WKUP;
                *reg |= TNETV_V2USB_CLK_WKUP_VBUS;
            }
            else
            {
                /* clear out connect bit */
                reg = (volatile uint32_t *) TNETV_USB_CTRL;
                *reg &= ~0x80;
            }
        }

        if (sysIntrStatus.f.setup_ow)
        {
            sysIntrStatus.f.setup_ow = 0;
            sysIntClear.val = 0;
            sysIntClear.f.setup_ow = 1;
            tnetv_usb_reg_write(TNETV_USB_STATUS, sysIntClear.val);
        }
        if (sysIntrStatus.f.setup)
        {
            UsbEp0ByteCntType ep0Cnt;
            static struct usb_ctrlrequest setup;

            sysIntrStatus.f.setup = 0;

            /* Copy setup packet into buffer */
            tnetv_copy_from_data_mem(&setup, TNETV_EP_DATA_ADDR(EP0_OUTPKT_ADDRESS), sizeof(setup));

            /* Determine next stage of the control message */
            if (setup.bRequestType & USB_DIR_IN)
            {
                /* This is a control-read. Switch directions to send the response.
                 * set the dir bit before clearing the interrupt
                 */
                usbCtrl.val = tnetv_usb_reg_read(TNETV_USB_CTRL);
                usbCtrl.f.dir = 1;
                tnetv_usb_reg_write(TNETV_USB_CTRL, usbCtrl.val);
            }
            else
            {
                /* This is a control-write. Remain using USB_DIR_OUT to receive the rest of the data.
                 * set the NAK bits according to supplement doc
                 */
                ep0Cnt.val = 0;
                ep0Cnt.f.in_xbuf_nak = 1;
                ep0Cnt.f.out_xbuf_nak = 1;
                tnetv_usb_reg_write(TNETV_USB_EP0_CNT, ep0Cnt.val);

                /* clear the dir bit before clearing the interrupt */
                usbCtrl.val = tnetv_usb_reg_read(TNETV_USB_CTRL);
                usbCtrl.f.dir = 0;
                tnetv_usb_reg_write(TNETV_USB_CTRL, usbCtrl.val);
            }

            /* Clear interrupt */
            sysIntClear.val = 0;
            sysIntClear.f.setup = 1;
            tnetv_usb_reg_write(TNETV_USB_STATUS, sysIntClear.val);

            if (((setup.bRequestType & USB_RECIP_MASK) == USB_RECIP_DEVICE) &&
                (setup.bRequest == USB_REQ_SET_ADDRESS))
            {
                /* Rockbox USB core works according to USB specification, i.e.
                 * it first acknowledges the control transfer and then sets
                 * the address. However, Linux TNETV105 driver first sets the
                 * address and then acknowledges the transfer. At first,
                 * it seemed that Linux driver was wrong, but it seems that
                 * TNETV105 simply requires such order. It might be documented
                 * in the datasheet and thus there is no comment in the Linux
                 * driver about this.
                 */
                setup_is_set_address = true;
            }
            else
            {
                setup_is_set_address = false;
            }

            /* Process control packet */
            usb_core_legacy_control_request(&setup);
        }

        if (sysIntrStatus.f.ep0_in_ack)
        {
            sysIntClear.val = 0;
            sysIntClear.f.ep0_in_ack = 1;
            tnetv_usb_reg_write(TNETV_USB_STATUS, sysIntClear.val);

            in_interrupt(0);
        }

        if (sysIntrStatus.f.ep0_out_ack)
        {
            sysIntClear.val = 0;
            sysIntClear.f.ep0_out_ack = 1;
            tnetv_usb_reg_write(TNETV_USB_STATUS, sysIntClear.val);

            out_interrupt(0);
        }
    }

    if (vlynq_intr & VLYNQ_INTR_CPPI)
    {
        static struct timeout cppi_timeout;

        VL_INTST = VLYNQ_INTR_CPPI;

        if (tnetv_handle_cppi())
        {
            timeout_register(&cppi_timeout, cppi_timeout_cb, 1, 0);
        }
    }
}

void usb_charging_maxcurrent_change(int maxcurrent)
{
    uint32_t wreg;

    if (!is_tnetv_reset_high())
    {
        /* TNETV105 is in reset, it is not getting more than 100 mA */
        return;
    }

    wreg = tnetv_usb_reg_read(TNETV_V2USB_GPIO_DOUT);
    if (maxcurrent < 500)
    {
        /* set tnetv into low power mode */
        tnetv_usb_reg_write(TNETV_V2USB_GPIO_DOUT, (wreg & ~0x2));
    }
    else
    {
        /* set tnetv into high power mode */
        tnetv_usb_reg_write(TNETV_V2USB_GPIO_DOUT, (wreg | 0x2));
    }
}

void usb_drv_init(void)
{
    int epn;
    memset(ep_runtime, 0, sizeof(ep_runtime));
    ep_runtime[0].max_packet_size = EP0_MAX_PACKET_SIZE;
    ep_runtime[0].in_allocated = true;
    ep_runtime[0].out_allocated = true;
    for (epn = 0; epn < USB_NUM_ENDPOINTS; epn++)
    {
        semaphore_init(&ep_runtime[epn].complete, 1, 0);
    }
    tnetv_cppi_init(&cppi);
    tnetv_udc_enable();
}

void usb_drv_exit(void)
{
    tnetv_udc_disable();
    tnetv_cppi_cleanup(&cppi);
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
    int epn = EP_NUM(endpoint);

    if (epn == 0)
    {
        UsbEp0CtrlType ep0Ctrl;
        ep0Ctrl.val = tnetv_usb_reg_read(TNETV_USB_EP0_CFG);
        if (in)
        {
            ep0Ctrl.f.in_stall = stall ? 1 : 0;
        }
        else
        {
            ep0Ctrl.f.out_stall = stall ? 1 : 0;
        }
        tnetv_usb_reg_write(TNETV_USB_EP0_CFG, ep0Ctrl.val);
    }
    else
    {
        UsbEpCfgCtrlType epCfg;
        epCfg.val = tnetv_usb_reg_read(TNETV_USB_EPx_CFG(epn));
        if (in)
        {
            epCfg.f.in_stall = stall ? 1 : 0;
        }
        else
        {
            epCfg.f.out_stall = stall ? 1 : 0;
        }
        tnetv_usb_reg_write(TNETV_USB_EPx_CFG(epn), epCfg.val);
    }
}

bool usb_drv_stalled(int endpoint, bool in)
{
    int epn = EP_NUM(endpoint);
    if (epn == 0)
    {
        UsbEp0CtrlType ep0Ctrl;
        ep0Ctrl.val = tnetv_usb_reg_read(TNETV_USB_EP0_CFG);
        if (in)
        {
            return ep0Ctrl.f.in_stall;
        }
        else
        {
            return ep0Ctrl.f.out_stall;
        }
    }
    else
    {
        UsbEpCfgCtrlType epCfg;
        epCfg.val = tnetv_usb_reg_read(TNETV_USB_EPx_CFG(epn));
        if (in)
        {
            return epCfg.f.in_stall;
        }
        else
        {
            return epCfg.f.out_stall;
        }
    }
}

static int _usb_drv_send(int endpoint, void *ptr, int length, bool block)
{
    int epn = EP_NUM(endpoint);
    struct ep_runtime_t *ep;
    int flags;

    ep = &ep_runtime[epn];

    flags = disable_irq_save();
    ep->tx_buf = ptr;
    ep->tx_remaining = ep->tx_size = length;
    ep->block = block;
    ep_write(epn);
    restore_irq(flags);

    /* wait for transfer to end */
    if (block)
    {
        semaphore_wait(&ep->complete, TIMEOUT_BLOCK);
    }
    return 0;
}

int usb_drv_send(int endpoint, void* ptr, int length)
{
    if ((EP_NUM(endpoint) == 0) && (length == 0))
    {
        if (setup_is_set_address)
        {
            /* usb_drv_set_address() will call us later */
            return 0;
        }
        /* HACK: Do not wait for status stage ZLP
         * This seems to be the only way to get through SET ADDRESS
         * and retain ability to receive SETUP packets.
         */
        return _usb_drv_send(endpoint, ptr, length, false);
    }
    return _usb_drv_send(endpoint, ptr, length, false);
}

int usb_drv_send_nonblocking(int endpoint, void* ptr, int length)
{
    return _usb_drv_send(endpoint, ptr, length, false);
}

int usb_drv_recv_nonblocking(int endpoint, void* ptr, int length)
{
    int epn = EP_NUM(endpoint);
    struct ep_runtime_t *ep;
    int flags;

    ep = &ep_runtime[epn];

    flags = disable_irq_save();
    ep->rx_buf = ptr;
    ep->rx_remaining = ep->rx_size = length;
    ep_read(epn);
    restore_irq(flags);

    return 0;
}

void usb_drv_set_address(int address)
{
    UsbCtrlType usbCtrl;
    usbCtrl.val = tnetv_usb_reg_read(TNETV_USB_CTRL);
    usbCtrl.f.func_addr = address;
    tnetv_usb_reg_write(TNETV_USB_CTRL, usbCtrl.val);

    /* This seems to be the only working order */
    setup_is_set_address = false;
    usb_drv_send(EP_CONTROL, NULL, 0);
    usb_drv_cancel_all_transfers();
}

/* return port speed FS=0, HS=1 */
int usb_drv_port_speed(void)
{
    UsbCtrlType usbCtrl;
    usbCtrl.val = tnetv_usb_reg_read(TNETV_USB_CTRL);
    return usbCtrl.f.speed ? 1 : 0;
}

void usb_drv_cancel_all_transfers(void)
{
    int epn;
    if (setup_is_set_address)
    {
        return;
    }
    for (epn = 1; epn < USB_NUM_ENDPOINTS; epn++)
    {
        tnetv_gadget_req_nuke(epn, false);
        tnetv_gadget_req_nuke(epn, true);
    }
}

static const uint8_t TestPacket[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
    0xEE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xBF, 0xDF,
    0xEF, 0xF7, 0xFB, 0xFD, 0xFC, 0x7E, 0xBF, 0xDF,
    0xEF, 0xF7, 0xFB, 0xFD, 0x7E
};

void usb_drv_set_test_mode(int mode)
{
    UsbCtrlType usbCtrl;
    if (mode == 4)
    {
        volatile uint32_t *reg;
        UsbEp0ByteCntType ep0Cnt;
        UsbEp0CtrlType ep0Cfg;
        uint8_t *addr;
        size_t i;

        /* set up the xnak for ep0 */
        reg = (volatile uint32_t *) TNETV_USB_EP0_CNT;
        *reg &= ~0xFF;

        /* Setup endpoint zero */
        ep0Cfg.val = 0;
        ep0Cfg.f.buf_size = EP0_BUF_SIZE_64;  /* must be 64 bytes for USB 2.0 */
        ep0Cfg.f.dbl_buf = 0;
        ep0Cfg.f.in_en = 1;
        ep0Cfg.f.in_int_en = 0;
        ep0Cfg.f.out_en = 0;
        ep0Cfg.f.out_int_en = 0;
        tnetv_usb_reg_write(TNETV_USB_EP0_CFG, ep0Cfg.val);

        addr = (uint8_t *) TNETV_EP_DATA_ADDR(EP0_INPKT_ADDRESS);
        for (i = 0; i < sizeof(TestPacket); i++)
        {
            *addr++ = TestPacket[i];
        }

        /* start xmitting (only 53 bytes are used) */
        ep0Cnt.val = 0;
        ep0Cnt.f.in_xbuf_cnt = 53;
        ep0Cnt.f.in_xbuf_nak = 0;
        tnetv_usb_reg_write(TNETV_USB_EP0_CNT, ep0Cnt.val);
    }

    /* write the config */
    usbCtrl.val = tnetv_usb_reg_read(TNETV_USB_CTRL);
    usbCtrl.f.hs_test_mode = mode;
    usbCtrl.f.dir = 1;
    tnetv_usb_reg_write(TNETV_USB_CTRL, usbCtrl.val);
}

int usb_drv_request_endpoint(int type, int dir)
{
    int epn;
    for (epn = 1; epn < USB_NUM_ENDPOINTS; epn++)
    {
        if (type == ep_const_data[epn].type)
        {
            if ((dir == USB_DIR_IN) && (!ep_runtime[epn].in_allocated))
            {
                ep_runtime[epn].in_allocated = true;
                tnetv_gadget_ep_enable(epn, true);
                return epn | USB_DIR_IN;
            }
            if ((dir == USB_DIR_OUT) && (!ep_runtime[epn].out_allocated))
            {
                ep_runtime[epn].out_allocated = true;
                tnetv_gadget_ep_enable(epn, false);
                return epn | USB_DIR_OUT;
            }
        }
    }
    return -1;
}

void usb_drv_release_endpoint(int ep)
{
    int epn = EP_NUM(ep);
    if (EP_DIR(ep) == DIR_IN)
    {
        ep_runtime[epn].in_allocated = false;
        tnetv_gadget_ep_disable(epn, true);
    }
    else
    {
        ep_runtime[epn].out_allocated = false;
        tnetv_gadget_ep_disable(epn, false);
    }
}
