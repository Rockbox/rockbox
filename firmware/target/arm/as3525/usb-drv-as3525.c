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

#if defined(USE_ROCKBOX_USB)

#define USB_NUM_EPS         4

typedef struct {
    volatile unsigned long offset[4096];
} __regbase;

/*
 * This generates better code.
 * Stripped object size with __regbase construct:  5192
 * Stripped object size with *((volatile int)(x)): 5228
 */
#define USB_REG(x)    ((__regbase *)(USB_BASE))->offset[(x)>>2]

/* 4 input endpoints */
#define USB_IEP_CTRL(i)         USB_REG(0x0000 + i*0x20)
#define USB_IEP_STS(i)          USB_REG(0x0004 + i*0x20)
/* txfsize: bits 0-15 */
#define USB_IEP_TXFSIZE(i)      USB_REG(0x0008 + i*0x20)
/* mps: bits 0-10 (max 2047) */
#define USB_IEP_MPS(i)          USB_REG(0x000C + i*0x20)
#define USB_IEP_DESC_PTR(i)     USB_REG(0x0014 + i*0x20)
#define USB_IEP_STS_MASK(i)     USB_REG(0x0018 + i*0x20)

/* 4 output endpoints */
#define USB_OEP_CTRL(i)         USB_REG(0x0200 + i*0x20)
#define USB_OEP_STS(i)          USB_REG(0x0204 + i*0x20)
/* 'rx packet frame number' */
#define USB_OEP_RXFR(i)         USB_REG(0x0208 + i*0x20)
/* mps: bits 0-10 (max 2047), bits 23-31 are fifo size */
#define USB_OEP_MPS(i)          USB_REG(0x020C + i*0x20)
#define USB_OEP_SUP_PTR(i)      USB_REG(0x0210 + i*0x20)
#define USB_OEP_DESC_PTR(i)     USB_REG(0x0214 + i*0x20)
#define USB_OEP_STS_MASK(i)     USB_REG(0x0218 + i*0x20)

/* more general macro */
/* d: true => IN, false => OUT */
#define USB_EP_CTRL(i,d)        USB_REG(0x0000 + i*0x20 + (!d)*0x0200)
#define USB_EP_STS(i,d)         USB_REG(0x0004 + i*0x20 + (!d)*0x0200)
#define USB_EP_TXFSIZE(i,d)     USB_REG(0x0008 + i*0x20 + (!d)*0x0200)
#define USB_EP_MPS(i,d)         USB_REG(0x000C + i*0x20 + (!d)*0x0200)
#define USB_EP_DESC_PTR(i,d)    USB_REG(0x0014 + i*0x20 + (!d)*0x0200)
#define USB_EP_STS_MASK(i,d)    USB_REG(0x0018 + i*0x20 + (!d)*0x0200)

#define USB_DEV_CFG             USB_REG(0x0400)
#define USB_DEV_CTRL            USB_REG(0x0404)
#define USB_DEV_STS             USB_REG(0x0408)
#define USB_DEV_INTR            USB_REG(0x040C)
#define USB_DEV_INTR_MASK       USB_REG(0x0410)
#define USB_DEV_EP_INTR         USB_REG(0x0414)
#define USB_DEV_EP_INTR_MASK    USB_REG(0x0418)

/* NOTE: Not written to in OF, most lied in host mode? */
#define USB_PHY_EP0_INFO        USB_REG(0x0504)
#define USB_PHY_EP1_INFO        USB_REG(0x0508)
#define USB_PHY_EP2_INFO        USB_REG(0x050C)
#define USB_PHY_EP3_INFO        USB_REG(0x0510)
#define USB_PHY_EP4_INFO        USB_REG(0x0514)
#define USB_PHY_EP5_INFO        USB_REG(0x0518)

/* 4 channels */
#define USB_HOST_CH_SPLT(i)     USB_REG(0x1000 + i*0x20)
#define USB_HOST_CH_STS(i)      USB_REG(0x1004 + i*0x20)
#define USB_HOST_CH_TXFSIZE(i)  USB_REG(0x1008 + i*0x20)
#define USB_HOST_CH_REQ(i)      USB_REG(0x100C + i*0x20)
#define USB_HOST_CH_PER_INFO(i) USB_REG(0x1010 + i*0x20)
#define USB_HOST_CH_DESC_PTR(i) USB_REG(0x1014 + i*0x20)
#define USB_HOST_CH_STS_MASK(i) USB_REG(0x1018 + i*0x20)

#define USB_HOST_CFG            USB_REG(0x1400)
#define USB_HOST_CTRL           USB_REG(0x1404)
#define USB_HOST_INTR           USB_REG(0x140C)
#define USB_HOST_INTR_MASK      USB_REG(0x1410)
#define USB_HOST_CH_INTR        USB_REG(0x1414)
#define USB_HOST_CH_INTR_MASK   USB_REG(0x1418)
#define USB_HOST_FRAME_INT      USB_REG(0x141C)
#define USB_HOST_FRAME_REM      USB_REG(0x1420)
#define USB_HOST_FRAME_NUM      USB_REG(0x1424)

#define USB_HOST_PORT0_CTRL_STS USB_REG(0x1500)

#define USB_OTG_CSR             USB_REG(0x2000)
#define USB_I2C_CSR             USB_REG(0x2004)
#define USB_GPIO_CSR            USB_REG(0x2008)
#define USB_SNPSID_CSR          USB_REG(0x200C)
#define USB_USERID_CSR          USB_REG(0x2010)
#define USB_USER_CONF1          USB_REG(0x2014)
#define USB_USER_CONF2          USB_REG(0x2018)
#define USB_USER_CONF3          USB_REG(0x201C)
#define USB_USER_CONF4          USB_REG(0x2020)
/* USER_CONF5 seems to the same as USBt least on read */
#define USB_USER_CONF5          USB_REG(0x2024)

#define USB_CSR_NUM_MASK        0x0000000f
#define USB_CSR_DIR_MASK        0x00000010
#define USB_CSR_DIR_IN            0x00000010
#define USB_CSR_DIR_OUT           0x00000000
#define USB_CSR_TYPE_MASK       0x00000060
#define USB_CSR_TYPE_CTL          0x00000000
#define USB_CSR_TYPE_ISO          0x00000020
#define USB_CSR_TYPE_BULK         0x00000040
#define USB_CSR_TYPE_INT          0x00000060
#define USB_CSR_CFG_MASK        0x00000780
#define USB_CSR_INTF_MASK       0x00007800
#define USB_CSR_ALT_MASK        0x00078000
#define USB_CSR_MAXPKT_MASK     0x3ff80000
#define USB_CSR_ISOMULT_MASK    0xc0000000

/* write bits 31..16 */
#define USB_GPIO_IDDIG_SEL        (1<<30)
#define USB_GPIO_FS_DATA_EXT      (1<<29)
#define USB_GPIO_FS_SE0_EXT       (1<<28)
#define USB_GPIO_FS_XCVR_OWNER    (1<<27)
#define USB_GPIO_TX_ENABLE_N      (1<<26)
#define USB_GPIO_TX_BIT_STUFF_EN  (1<<25)
#define USB_GPIO_BSESSVLD_EXT     (1<<24)
#define USB_GPIO_ASESSVLD_EXT     (1<<23)
#define USB_GPIO_VBUS_VLD_EXT     (1<<22)
#define USB_GPIO_VBUS_VLD_EXT_SEL (1<<21)
#define USB_GPIO_XO_ON            (1<<20)
#define USB_GPIO_CLK_SEL11        (3<<18)
#define USB_GPIO_CLK_SEL10        (2<<18)
#define USB_GPIO_CLK_SEL01        (1<<18)
#define USB_GPIO_CLK_SEL00        (0<<18)
#define USB_GPIO_XO_EXT_CLK_ENBN  (1<<17)
#define USB_GPIO_XO_REFCLK_ENB    (1<<16)
/* readronly bits 15..0 */
#define USB_GPIO_PHY_VBUSDRV      (1<< 1)
#define USB_GPIO_HS_INTR          (1<< 0)

/* Device Control Register and bit fields */
#define USB_DEV_CTRL_REMOTE_WAKEUP      0x00000001  // set remote wake-up signal
#define USB_DEV_CTRL_RESERVED0          0x00000002  // reserved, ro, read as 0
#define USB_DEV_CTRL_RDE                0x00000004  // receive dma enable
#define USB_DEV_CTRL_TDE                0x00000008  // transmit dma enable
#define USB_DEV_CTRL_DESC_UPDATE        0x00000010  // update desc after dma
#define USB_DEV_CTRL_BE                 0x00000020  // big endian when set (ro)
#define USB_DEV_CTRL_BUFFER_FULL        0x00000040
#define USB_DEV_CTRL_THRES_ENABLE       0x00000080  // threshold enable
#define USB_DEV_CTRL_BURST_ENABLE       0x00000100  // ahb burst enable
#define USB_DEV_CTRL_MODE               0x00000200  // 0=slave, 1=dma
#define USB_DEV_CTRL_SOFT_DISCONN       0x00000400  // soft disconnect
#define USB_DEV_CTRL_SCALEDOWN          0x00000800  // for simulation speedup
#define USB_DEV_CTRL_DEVNAK             0x00001000  // set nak on all OUT EPs
#define USB_DEV_CTRL_APCSR_DONE           0x00002000  // set to signal CSR update
#define USB_DEV_CTRL_MASK_BURST_LEN     0x000f0000  // mask for burst length
#define USB_DEV_CTRL_MASK_THRESHOLD_LEN 0xff000000  // mask for threshold length

/* settings of burst length for maskBurstLen_c field */
/* amd 5536 datasheet: (BLEN+1) dwords */
#define USB_DEV_CTRL_BLEN_1DWORD        0x00000000
#define USB_DEV_CTRL_BLEN_2DWORDS       0x00010000
#define USB_DEV_CTRL_BLEN_4DWORDS       0x00020000
#define USB_DEV_CTRL_BLEN_8DWORDS       0x00030000
#define USB_DEV_CTRL_BLEN_16DWORDS      0x00040000
#define USB_DEV_CTRL_BLEN_32DWORDS      0x00050000
#define USB_DEV_CTRL_BLEN_64DWORDS      0x00060000
#define USB_DEV_CTRL_BLEN_128DWORDS     0x00070000
#define USB_DEV_CTRL_BLEN_256DWORDS     0x00080000
#define USB_DEV_CTRL_BLEN_512DWORDS     0x00090000

/* settings of threshold length for maskThresholdLen_c field */
/* amd 5536 datasheet: (TLEN+1) dwords */
#define USB_DEV_CTRL_TLEN_1DWORD        0x00000000
#define USB_DEV_CTRL_TLEN_HALFMAXSIZE   0x01000000
#define USB_DEV_CTRL_TLEN_4THMAXSIZE    0x02000000
#define USB_DEV_CTRL_TLEN_8THMAXSIZE    0x03000000

#define USB_DEV_CFG_HS                  0x00000000
#define USB_DEV_CFG_FS                  0x00000001 /* 30 or 60MHz */
#define USB_DEV_CFG_LS                  0x00000002
#define USB_DEV_CFG_FS_48               0x00000003 /* 48MHz */
#define USB_DEV_CFG_REMOTE_WAKEUP       0x00000004
#define USB_DEV_CFG_SELF_POWERED        0x00000008
#define USB_DEV_CFG_SYNC_FRAME          0x00000010
#define USB_DEV_CFG_PI_16BIT            0x00000000
#define USB_DEV_CFG_PI_8BIT             0x00000020
#define USB_DEV_CFG_UNI_DIR             0x00000000
#define USB_DEV_CFG_BI_DIR              0x00000040
#define USB_DEV_CFG_STAT_ACK            0x00000000
#define USB_DEV_CFG_STAT_STALL          0x00000080
#define USB_DEV_CFG_PHY_ERR_DETECT      0x00000200 /* monitor phy for errors */
#define USB_DEV_CFG_HALT_STAT           0x00010000 /* ENDPOINT_HALT supported */
    /* 0: ACK, 1: STALL */
#define USB_DEV_CFG_CSR_PRG             0x00020000
#define USB_DEV_CFG_SET_DESC            0x00040000 /* SET_DESCRIPTOR supported */
    /* 0: STALL, 1: pass on setup packet */
#define USB_DEV_CFG_DMA_RESET           0x20000000
#define USB_DEV_CFG_HNPSFEN             0x40000000
#define USB_DEV_CFG_SOFT_RESET          0x80000000

/* Device Status Register and bit fields */
#define USB_DEV_STS_MASK_CFG            0x0000000f
#define USB_DEV_STS_MASK_IF             0x000000f0
#define USB_DEV_STS_MASK_ALT_SET        0x00000f00
#define USB_DEV_STS_SUSPEND_STAT        0x00001000
#define USB_DEV_STS_MASK_SPD            0x00006000 /* Enumerated Speed */
#define USB_DEV_STS_SPD_HS                0x00000000
#define USB_DEV_STS_SPD_FS                0x00002000
#define USB_DEV_STS_SPD_LS                0x00004000
#define USB_DEV_STS_RXF_EMPTY           0x00008000
#define USB_DEV_STS_PHY_ERROR           0x00010000
#define USB_DEV_STS_SESSVLD             0x00020000 /* session valid (vbus>1.2V) */
#define USB_DEV_STS_MASK_FRM_NUM        0xfffc0000 /* SOF frame number */


/* Device Intr Register and bit fields */
#define USB_DEV_INTR_SET_CONFIG         0x00000001  /* set configuration cmd rcvd */
#define USB_DEV_INTR_SET_INTERFACE      0x00000002  /* set interface command rcvd */
#define USB_DEV_INTR_EARLY_SUSPEND      0x00000004  /* idle on usb for 3ms */
#define USB_DEV_INTR_USB_RESET          0x00000008  /* usb bus reset req */
#define USB_DEV_INTR_USB_SUSPEND        0x00000010  /* usb bus suspend req */
#define USB_DEV_INTR_SOF                0x00000020  /* SOF seen on bus */
#define USB_DEV_INTR_ENUM_DONE          0x00000040  /* usb speed enum done */
#define USB_DEV_INTR_SVC                0x00000080  /* USB_DEV_STS changed */

/* EP Control Register Fields */
#define USB_EP_CTRL_STALL               0x00000001
#define USB_EP_CTRL_FLUSH               0x00000002     /* EP In data fifo Flush */
#define USB_EP_CTRL_SNOOP_MODE          0x00000004     // snoop mode for out endpoint
#define USB_EP_CTRL_PD                  0x00000008     /* EP Poll Demand */
#define USB_EP_CTRL_EPTYPE_MASK         0x00000030     // bit 5-4: endpoint types
#define USB_EP_TYPE_CONTROL               0x00000000   // control endpoint
#define USB_EP_TYPE_ISO                   0x00000010   // isochronous endpoint
#define USB_EP_TYPE_BULK                  0x00000020   // bulk endpoint
#define USB_EP_TYPE_INTERRUPT             0x00000030   // interrupt endpoint
#define USB_EP_CTRL_NAK                 0x00000040     /* EP NAK Status */
#define USB_EP_CTRL_SNAK                0x00000080     /* EP Set NAK Bit */
#define USB_EP_CTRL_CNAK                0x00000100     /* EP Clr NAK Bit */
#define USB_EP_CTRL_ACT                 0x00000400     /* EP Clr NAK Bit */

/* bit fields in EP Status Register */
#define USB_EP_STAT_OUT_RCVD            0x00000010     /* OUT token received */
#define USB_EP_STAT_SETUP_RCVD          0x00000020     /* SETUP token received */
#define USB_EP_STAT_IN                  0x00000040     /* IN token received? */
#define USB_EP_STAT_BNA                 0x00000080     /* Buffer Not Available */
#define USB_EP_STAT_BUFF_ERROR          0x00000100
#define USB_EP_STAT_HERR                0x00000200     /* Host Error */
#define USB_EP_STAT_AHB_ERROR           0x00000200
#define USB_EP_STAT_TDC                 0x00000400     /* Transmit DMA Complete */

/*-------------------------*/
/* DMA Related Definitions */
/*-------------------------*/

/* dma status */
#define USB_DMA_DESC_BUFF_STS           0x80000000    /* Buffer Status */
#define USB_DMA_DESC_BS_HST_RDY         0x80000000    /* Host Ready */
#define USB_DMA_DESC_BS_DMA_DONE        0x00000000    /* DMA Done */
#define USB_DMA_DESC_ZERO_LEN           0x40000000    /* zero length packet */
#define USB_DMA_DESC_EARY_INTR          0x20000000    /* early interrupt */
#define USB_DMA_DESC_RXTX_STS           0x10000000
#define USB_DMA_DESC_RTS_SUCC           0x00000000    /* Success */
#define USB_DMA_DESC_RTS_BUFERR         0x10000000    /* Buffer Error */
#define USB_DMA_DESC_LAST               0x08000000
#define USB_DMA_DESC_MASK_FRAM_NUM      0x07ff0000    // bits 26-16: frame number for iso
#define USB_DMA_DESC_RXTX_BYTES         0x0000FFFF

/* setup descriptor */
#define SETUP_MASK_CONFIG_STAT          0x0fff0000
#define SETUP_MASK_CONFIG_NUM           0x0f000000
#define SETUP_MASK_IF_NUM               0x00f00000
#define SETUP_MASK_ALT_SETNUM           0x000f0000

#define EP_STATE_ALLOCATED              0x00000001
#define EP_STATE_BUSY                   0x00000002
#define EP_STATE_ASYNC                  0x00000004

struct usb_dev_dma_desc {
    int   status;
    int   resv;
    void  *data_ptr;
    void  *next_desc;
};

struct usb_dev_setup_buf {
    int   status;
    int   resv;
    int   data1; /* first 4 bytes of data */
    int   data2; /* last 4 bytes of data */
};

struct usb_endpoint
{
    void *buf;
    unsigned int len;
    volatile unsigned int state;
    int rc;
    struct wakeup complete;
    struct usb_dev_dma_desc *uc_desc;
};

static struct usb_endpoint endpoints[USB_NUM_EPS][2];

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
    usb_enable(true);
}

/* delay is in milliseconds */
static inline void usb_delay(int delay)
{
    while(delay--)
        udelay(1000);
}

static void usb_phy_on(void)
{
    /* PHY clock */
    CGU_USB = 1<<5 /* enable */
        | (CLK_DIV(AS3525_PLLB_FREQ, 48000000) / 2) << 2
        | 2; /* source = PLLB */

    /* UVDD on */
    ascodec_write(AS3515_USB_UTIL, ascodec_read(AS3515_USB_UTIL) | (1<<4));
    usb_delay(100);

    /* reset */
    CCU_SRC = CCU_SRC_USB_AHB_EN|CCU_SRC_USB_PHY_EN;
    CCU_SRL = CCU_SRL_MAGIC_NUMBER;
    usb_delay(1);
    CCU_SRC = CCU_SRC_USB_AHB_EN;
    usb_delay(1);
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
    usb_delay(3);
    USB_GPIO_CSR |= USB_GPIO_VBUS_VLD_EXT_SEL;
    usb_delay(10);
}

static void usb_phy_resume(void)
{
    USB_GPIO_CSR &= ~(USB_GPIO_ASESSVLD_EXT |
                      USB_GPIO_BSESSVLD_EXT |
                      USB_GPIO_VBUS_VLD_EXT);
    usb_delay(3);
    USB_GPIO_CSR &= ~USB_GPIO_VBUS_VLD_EXT_SEL;
    usb_delay(10);
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
         * LS: 8 (control), no bulk available 
         * FS: 64 (control), 64 (bulk)
         * HS: 64 (control), 512 (bulk)
         * TODO: switch depending on speed.
         */
        int mps = i == 0 ? 64 : 512;

        if (init) {
            endpoints[i][0].state = 0;
            wakeup_init(&endpoints[i][0].complete);

            if (i != 2) { /* Skip the OUT EP0 alias */
                endpoints[i][1].state = 0;
                wakeup_init(&endpoints[i][1].complete);
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
            USB_OEP_MPS     (i) = (mps/2 << 23) | mps;
            USB_OEP_STS_MASK(i) = 0x0000; /* OF: 0x1800 */
            USB_OEP_RXFR    (i) = 0;      /* Always 0 in OF trace? */
            USB_OEP_STS     (i) = 0xffffffff; /* clear status */
            USB_OEP_DESC_PTR(i) = 0;
        }
    }

    setup_desc_init(setup_desc);
    USB_OEP_SUP_PTR(0)    = (int)setup_desc;
}

void usb_drv_init(void)
{
    logf("usb_drv_init() !!!!\n");

    usb_enable_pll();

    /* length regulator: normal operation */
    ascodec_write(AS3514_CVDD_DCDC3, ascodec_read(AS3514_CVDD_DCDC3) | 1<<2);

    /* AHB part */
    CGU_PERI |= CGU_USB_CLOCK_ENABLE;

    /* reset AHB */
    CCU_SRC = CCU_SRC_USB_AHB_EN;
    CCU_SRL = CCU_SRL_MAGIC_NUMBER;
    usb_delay(1);
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

    USB_DEV_CTRL = USB_DEV_CTRL_BLEN_1DWORD  |
                   USB_DEV_CTRL_DESC_UPDATE  |
                   USB_DEV_CTRL_THRES_ENABLE |
                   USB_DEV_CTRL_RDE          |
                   0x04000000;

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

}

void usb_drv_exit(void)
{
    USB_DEV_CTRL |= (1<<10); /* soft disconnect */
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
    CGU_PERI &= ~CGU_USB_CLOCK_ENABLE;
    /* Disable UVDD generating LDO */
    ascodec_write(AS3515_USB_UTIL, ascodec_read(AS3515_USB_UTIL) & ~(1<<4));
    usb_disable_pll();
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

    if ((int)ptr & 31) {
        logf("addr %08x not aligned!\n", (int)ptr);
    }

    endpoints[ep][1].state |= EP_STATE_BUSY;
    endpoints[ep][1].len = len;
    endpoints[ep][1].rc = -1;

    /* remove data buffer from cache */
    invalidate_dcache();
    /* DMA setup */
    uc_desc->status    = USB_DMA_DESC_BS_HST_RDY |
                         USB_DMA_DESC_LAST |
                         len;
    if (len == 0) {
        uc_desc->status   |= USB_DMA_DESC_ZERO_LEN;
        uc_desc->data_ptr  = 0;
    } else {
        uc_desc->data_ptr  = ptr;
    }
    USB_OEP_DESC_PTR(ep) = (int)&dmadescs[ep][1];
    USB_OEP_STS(ep)      = USB_EP_STAT_OUT_RCVD; /* clear status */
    USB_OEP_CTRL(ep)    |= USB_EP_CTRL_CNAK;

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

void ep_send(int ep, void *ptr, int len)
{
    struct usb_dev_dma_desc *uc_desc = endpoints[ep][0].uc_desc;

    endpoints[ep][0].state |= EP_STATE_BUSY;
    endpoints[ep][0].len = len;
    endpoints[ep][0].rc = -1;

    /* Make sure data is committed to memory */
    clean_dcache();

    logf("xx%s\n", make_hex(ptr, len));

    uc_desc->status    = USB_DMA_DESC_BS_HST_RDY |
                         USB_DMA_DESC_LAST |
                         len;
    if (len == 0)
        uc_desc->status |= USB_DMA_DESC_ZERO_LEN;

    uc_desc->data_ptr  = ptr;

    USB_IEP_DESC_PTR(ep) = (int)&dmadescs[ep][0];
    USB_IEP_STS(ep)      = 0xffffffff; /* clear status */
    /* start transfer */
    USB_IEP_CTRL(ep)     |= USB_EP_CTRL_CNAK | USB_EP_CTRL_PD;
    /* HW automatically sets NAK bit later */
}

int usb_drv_send(int ep, void *ptr, int len)
{
    logf("usb_drv_send(%d,%x,%d): ", ep, (int)ptr, len);

    ep &= 0x7f;
    ep_send(ep, ptr, len);
    while (endpoints[ep][0].state & EP_STATE_BUSY)
        wakeup_wait(&endpoints[ep][0].complete, TIMEOUT_BLOCK);

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
        logf("ep%d IN, status %x (BNA)\n", ep, ep_sts);
        panicf("ep%d IN 0x%x (BNA)", ep, ep_sts);
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
            wakeup_signal(&endpoints[ep][0].complete);
        }
        ep_sts &= ~USB_EP_STAT_TDC;
    }

    if (ep_sts) {
        logf("ep%d IN, hwstat %lx, epstat %x\n", ep, USB_IEP_STS(ep), endpoints[ep][0].state);
        panicf("ep%d IN 0x%x", ep, ep_sts);
    }

    /* HW automatically disables RDE, re-enable it */
    /* But this an IN ep, I wonder... */
    USB_DEV_CTRL |= USB_DEV_CTRL_RDE;
}

static void handle_out_ep(int ep)
{
    struct usb_ctrlrequest *req = (void*)AS3525_UNCACHED_ADDR(&setup_desc->data1);
    int ep_sts = USB_OEP_STS(ep) & ~USB_OEP_STS_MASK(ep);
    struct usb_dev_dma_desc *uc_desc = endpoints[ep][1].uc_desc;

    if (ep > 3)
        panicf("out_ep > 3!?");

    USB_OEP_STS(ep) = ep_sts; /* ACK */

    if (ep_sts & USB_EP_STAT_BNA) { /* Buffer was not set up */
        logf("ep%d OUT, status %x (BNA)\n", ep, ep_sts);
        panicf("ep%d OUT 0x%x (BNA)", ep, ep_sts);
    }

    if (ep_sts & USB_EP_STAT_OUT_RCVD) {
        int dma_sts = uc_desc->status;
        int dma_len = dma_sts & 0xffff;
#ifdef LOGF_ENABLE
        int dma_frm = (dma_sts >> 16) & 0x7ff;
        int dma_mst = dma_sts & 0xf8000000;
#endif

        if (!(dma_sts & USB_DMA_DESC_ZERO_LEN)) {
             logf("EP%d OUT token, st:%08x len:%d frm:%x data=%s epstate=%d\n", ep,
                 dma_mst, dma_len, dma_frm, make_hex(uc_desc->data_ptr, dma_len),
                 endpoints[ep][1].state);
             /*
              * If parts of the just dmaed range are in cache, dump them now.
              */
             dump_dcache_range(uc_desc->data_ptr, dma_len);
        } else{
             logf("EP%d OUT token, st:%08x frm:%x (no data)\n", ep,
                 dma_mst, dma_frm);
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
        logf("t%ld:got SETUP packet: type=%d req=%d val=%d ind=%d len=%d\n",
             current_tick,
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

    /* HW automatically disables RDE, re-enable it */
    /* THEORY: Because we only set up one DMA buffer... */
    USB_DEV_CTRL |= USB_DEV_CTRL_RDE;
}

/* interrupt service routine */
void INT_USB(void)
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

            set_config.wValue = USB_DEV_STS & USB_DEV_STS_MASK_CFG;
            usb_core_control_request(&set_config);

            /* Tell the HW we handled the request */
            USB_DEV_CTRL |= USB_DEV_CTRL_APCSR_DONE;
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

            logf("speed enum complete: ");
            if (spd == USB_DEV_STS_SPD_HS) logf("hs\n");
            if (spd == USB_DEV_STS_SPD_FS) logf("fs\n");
            if (spd == USB_DEV_STS_SPD_LS) logf("ls\n");

            USB_PHY_EP0_INFO = 0x00200000       |
                               USB_CSR_DIR_OUT  |
                               USB_CSR_TYPE_CTL;
            USB_PHY_EP1_INFO = 0x00200000       |
                               USB_CSR_DIR_IN   |
                               USB_CSR_TYPE_CTL;
            USB_PHY_EP2_INFO = 0x00200001       |
                               USB_CSR_DIR_IN   |
                               USB_CSR_TYPE_BULK;
            USB_PHY_EP3_INFO = 0x00200001       |
                               USB_CSR_DIR_IN   |
                               USB_CSR_TYPE_BULK;
            USB_DEV_CTRL |= USB_DEV_CTRL_APCSR_DONE;
            USB_IEP_CTRL(0) |= USB_EP_CTRL_ACT;
            USB_OEP_CTRL(0) |= USB_EP_CTRL_ACT;
            intr &= ~USB_DEV_INTR_ENUM_DONE;
        }
        if (intr)
            panicf("usb devirq 0x%x", intr);
    }

    if (!(USB_DEV_CTRL & USB_DEV_CTRL_RDE)){
        logf("re-enabling receive DMA\n");
        USB_DEV_CTRL |= USB_DEV_CTRL_RDE;
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

#else

void usb_attach(void)
{
}

void usb_drv_init(void)
{
}

void usb_drv_exit(void)
{
}

int usb_drv_port_speed(void)
{
    return 0;
}

int usb_drv_request_endpoint(int type, int dir)
{
    (void)type;
    (void)dir;

    return -1;
}

void usb_drv_release_endpoint(int ep)
{
    (void)ep;
}

void usb_drv_cancel_all_transfers(void)
{
}

void usb_drv_set_test_mode(int mode)
{
    (void)mode;
}

void usb_drv_set_address(int address)
{
    (void)address;
}

int usb_drv_recv(int ep, void *ptr, int len)
{
    (void)ep;
    (void)ptr;
    (void)len;

    return -1;
}

int usb_drv_send(int ep, void *ptr, int len)
{
    (void)ep;
    (void)ptr;
    (void)len;

    return -1;
}

int usb_drv_send_nonblocking(int ep, void *ptr, int len)
{
    (void)ep;
    (void)ptr;
    (void)len;

    return -1;
}

void usb_drv_stall(int ep, bool stall, bool in)
{
    (void)ep;
    (void)stall;
    (void)in;
}

bool usb_drv_stalled(int ep, bool in)
{
    (void)ep;
    (void)in;

    return 0;
}

#endif
