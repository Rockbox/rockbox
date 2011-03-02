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
#ifndef __USB_DRV_AS3525_H__
#define __USB_DRV_AS3525_H__

#include "as3525.h"

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
#define USB_DEV_INTR_MYSTERY            0x00000200  /* Unknown, maybe Host Error */

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
    unsigned int len;
    volatile unsigned int state;
    int rc;
    struct semaphore complete;
    struct usb_dev_dma_desc *uc_desc;
};

#endif /* __USB_DRV_AS3525_H__ */
