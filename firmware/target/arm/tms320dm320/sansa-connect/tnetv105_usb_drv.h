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
 * Copyright (c) 2005 Zermatt Systems, Inc.
 * Written by: Ben Bostwick
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

#ifndef TNETV105_USB_DRV_H
#define TNETV105_USB_DRV_H

#include <stdint.h>

#define DM320_AHB_PADDR        0x00060000
#define DM320_VLYNQ_PADDR      0x70000000

/* TNETV105 Memory Map */
#define VLYNQ_BASE                 (0x70000000)

#define TNETV_BASE                 (VLYNQ_BASE)
#define TNETV_V2USB_BASE           (TNETV_BASE + 0x00000200)
#define TNETV_WDOG_BASE            (TNETV_BASE + 0x00000280)
#define TNETV_USB_HOST_BASE        (TNETV_BASE + 0x00010000)
#define TNETV_USB_DEVICE_BASE      (TNETV_BASE + 0x00020000)

#define TNETV_V2USB_REG(x)         (TNETV_V2USB_BASE + (x))

#define TNETV_V2USB_RESET          (TNETV_V2USB_REG(0x00))
#define TNETV_V2USB_CLK_PERF       (TNETV_V2USB_REG(0x04))
#define TNETV_V2USB_CLK_MODE       (TNETV_V2USB_REG(0x08))
#define TNETV_V2USB_CLK_CFG        (TNETV_V2USB_REG(0x0C))
#define TNETV_V2USB_CLK_WKUP       (TNETV_V2USB_REG(0x10))
#define TNETV_V2USB_CLK_PWR        (TNETV_V2USB_REG(0x14))

#define TNETV_V2USB_PID_VID        (TNETV_V2USB_REG(0x28))

#define TNETV_V2USB_GPIO_DOUT     (TNETV_V2USB_REG(0x40))
#define TNETV_V2USB_GPIO_DIN      (TNETV_V2USB_REG(0x44))
#define TNETV_V2USB_GPIO_DIR      (TNETV_V2USB_REG(0x48))
#define TNETV_V2USB_GPIO_FS       (TNETV_V2USB_REG(0x4C))
#define TNETV_V2USB_GPIO_INTF     (TNETV_V2USB_REG(0x50))
#define TNETV_V2USB_GPIO_EOI      (TNETV_V2USB_REG(0x54))

#define TNETV_USB_DEVICE_REG(x)    (TNETV_USB_DEVICE_BASE + (x))

#define TNETV_USB_REV            (TNETV_USB_DEVICE_REG(0x00))
#define TNETV_USB_TX_CTL         (TNETV_USB_DEVICE_REG(0x04))
#define TNETV_USB_TX_TEARDOWN    (TNETV_USB_DEVICE_REG(0x08))
#define TNETV_USB_RX_CTL         (TNETV_USB_DEVICE_REG(0x14))
#define TNETV_USB_RX_TEARDOWN    (TNETV_USB_DEVICE_REG(0x18))
#define TNETV_USB_TX_ENDIAN_CTL  (TNETV_USB_DEVICE_REG(0x40))
#define TNETV_USB_RX_ENDIAN_CTL  (TNETV_USB_DEVICE_REG(0x44))

#define TNETV_USB_RX_FREE_BUF_CNT(ch) (TNETV_USB_DEVICE_REG(0x140 + ((ch) * 4)))

#define TNETV_USB_TX_INT_STATUS  (TNETV_USB_DEVICE_REG(0x170))
#define TNETV_USB_TX_INT_EN      (TNETV_USB_DEVICE_REG(0x178))
#define TNETV_USB_TX_INT_DIS     (TNETV_USB_DEVICE_REG(0x17C))
#define TNETV_USB_VBUS_INT       (TNETV_USB_DEVICE_REG(0x180))
#define TNETV_USB_VBUS_EOI       (TNETV_USB_DEVICE_REG(0x184))
#define TNETV_USB_RX_INT_STATUS  (TNETV_USB_DEVICE_REG(0x190))
#define TNETV_USB_RX_INT_EN      (TNETV_USB_DEVICE_REG(0x198))
#define TNETV_USB_RX_INT_DIS     (TNETV_USB_DEVICE_REG(0x19C))

#define TNETV_USB_RESET_CMPL    (TNETV_USB_DEVICE_REG(0x1A0))
#define TNETV_CPPI_STATE        (TNETV_USB_DEVICE_REG(0x1A4))

#define TNETV_USB_STATUS       (TNETV_USB_DEVICE_REG(0x200))
#define TNETV_USB_CTRL         (TNETV_USB_DEVICE_REG(0x204))
#define TNETV_USB_IF_STATUS    (TNETV_USB_DEVICE_REG(0x210))
#define TNETV_USB_IF_ERR       (TNETV_USB_DEVICE_REG(0x214))
#define TNETV_USB_IF_SM        (TNETV_USB_DEVICE_REG(0x218))

#define TNETV_USB_EP0_CFG      (TNETV_USB_DEVICE_REG(0x220))
#define TNETV_USB_EP0_CNT      (TNETV_USB_DEVICE_REG(0x224))

#define TNETV_USB_EPx_CFG(x)      (TNETV_USB_DEVICE_REG(0x220 + (0x10 * (x))))
#define TNETV_USB_EPx_IN_CNT(x)   (TNETV_USB_DEVICE_REG(0x224 + (0x10 * (x))))
#define TNETV_USB_EPx_OUT_CNT(x)  (TNETV_USB_DEVICE_REG(0x228 + (0x10 * (x))))
#define TNETV_USB_EPx_ADR(x)      (TNETV_USB_DEVICE_REG(0x22C + (0x10 * (x))))

/* USB CPPI Config registers (0x300 - 0x30C) */
#define TNETV_USB_RNDIS_MODE      (TNETV_USB_DEVICE_REG(0x300))
#define TNETV_USB_CELL_DMA_EN     (TNETV_USB_DEVICE_REG(0x30C))

#define TNETV_USB_RAW_INT         (TNETV_USB_DEVICE_REG(0x310))
#define TNETV_USB_RAW_EOI         (TNETV_USB_DEVICE_REG(0x314))

/* USB DMA setup RAM (0x800 - 0x8FF) */
#define TNETV_DMA_BASE               (TNETV_USB_DEVICE_BASE + 0x800)
#define TNETV_DMA_TX_STATE(ch, wd)   ((uint32_t *) ((TNETV_DMA_BASE) + ((ch) * 0x40) + ((wd) * 4)))
#define TNETV_DMA_TX_CMPL(ch)        ((TNETV_DMA_BASE) + ((ch) * 0x40) + 0x1C)

#define TNETV_CPPI_TX_WORD_HDP       0

#define TNETV_DMA_RX_STATE(ch, wd)   ((uint32_t *) ((TNETV_DMA_BASE) + ((ch) * 0x40) + 0x20 + ((wd) * 4)))
#define TNETV_DMA_RX_CMPL(ch)        ((TNETV_DMA_BASE) + ((ch) * 0x40) + 0x3C)

#define TNETV_CPPI_RX_WORD_HDP       1

#define TNETV_DMA_NUM_CHANNELS       3

#define TNETV_DMA_TX_NUM_WORDS       6
#define TNETV_DMA_RX_NUM_WORDS       7


/* USB Buffer RAM (0x1000 - 0x1A00) */
#define TNETV_EP_DATA_ADDR(x)   ((uint32_t *) ((TNETV_USB_DEVICE_BASE) + 0x1000 + (x)))

#define TNETV_EP_DATA_SIZE      (0xA00)

#define TNETV_V2USB_RESET_DEV       (1 << 0)

#define TNETV_USB_CELL_DMA_EN_RX    (1 << 0)
#define TNETV_USB_CELL_DMA_EN_TX    (1 << 1)

#define TNETV_V2USB_CLK_WKUP_VBUS   (1 << 12)

#define DM320_VLYNQ_INTPND_PHY  ((DM320_AHB_PADDR) + 0x0314)


/* macro to convert from a linux pointer to a physical address
 * to be sent over the VLYNQ bus.  The dm320 vlynq rx registers are
 * set up so the base address is the physical address of RAM
 */
#define __dma_to_vlynq_phys(addr)    ((((uint32_t) (addr)) - 0x01000000))
#define __vlynq_phys_to_dma(addr)    ((((uint32_t) (addr)) + 0x01000000))

//----------------------------------------------------------------------

#define USB_FULL_SPEED_MAXPACKET  64
#define USB_HIGH_SPEED_MAXPACKET  512

/* WORD offsets into the data memory */
#define EP0_MAX_PACKET_SIZE    64     /* Control ep - 64 bytes */
#define EP1_MAX_PACKET_SIZE    512    /* Bulk ep - 512 bytes */
#define EP2_MAX_PACKET_SIZE    512    /* Bulk ep - 512 bytes */
#define EP3_MAX_PACKET_SIZE    64     /* Int ep - 64 bytes */
#define EP4_MAX_PACKET_SIZE    64     /* Int ep - 64 bytes */

/* BEN TODO: fix this crap */
#define EP0_OUTPKT_ADDRESS      0
#define EP0_INPKT_ADDRESS       (EP0_MAX_PACKET_SIZE)
#define EP1_XBUFFER_ADDRESS     (EP0_MAX_PACKET_SIZE << 1)
#define EP1_YBUFFER_ADDRESS     (EP1_XBUFFER_ADDRESS + EP1_MAX_PACKET_SIZE)
#define EP2_XBUFFER_ADDRESS     (EP1_XBUFFER_ADDRESS + (EP1_MAX_PACKET_SIZE << 1))
#define EP2_YBUFFER_ADDRESS     (EP2_XBUFFER_ADDRESS + EP2_MAX_PACKET_SIZE)
#define EP3_XBUFFER_ADDRESS     (EP2_XBUFFER_ADDRESS + (EP2_MAX_PACKET_SIZE << 1))
#define EP3_YBUFFER_ADDRESS     (EP3_XBUFFER_ADDRESS + EP3_MAX_PACKET_SIZE)
#define EP4_XBUFFER_ADDRESS     (EP3_XBUFFER_ADDRESS + (EP3_MAX_PACKET_SIZE << 1))
#define EP4_YBUFFER_ADDRESS     (EP4_XBUFFER_ADDRESS + EP4_MAX_PACKET_SIZE)
#define EP5_XBUFFER_ADDRESS     (EP4_XBUFFER_ADDRESS + (EP4_MAX_PACKET_SIZE << 1))
#define EP5_YBUFFER_ADDRESS     (EP5_XBUFFER_ADDRESS + EP1_MAX_PACKET_SIZE)
#define EP6_XBUFFER_ADDRESS     (EP5_XBUFFER_ADDRESS + (EP1_MAX_PACKET_SIZE << 1))
#define EP6_YBUFFER_ADDRESS     (EP6_XBUFFER_ADDRESS + EP2_MAX_PACKET_SIZE)
#define EP7_XBUFFER_ADDRESS     (EP6_XBUFFER_ADDRESS + (EP2_MAX_PACKET_SIZE << 1))
#define EP7_YBUFFER_ADDRESS     (EP7_XBUFFER_ADDRESS + EP3_MAX_PACKET_SIZE)
#define EP8_XBUFFER_ADDRESS     (EP7_XBUFFER_ADDRESS + (EP3_MAX_PACKET_SIZE << 1))
#define EP8_YBUFFER_ADDRESS     (EP8_XBUFFER_ADDRESS + EP4_MAX_PACKET_SIZE)

#define SETUP_PKT_DATA_SIZE     8

#define EP0_BUF_SIZE_8         0
#define EP0_BUF_SIZE_16        1
#define EP0_BUF_SIZE_32        2
#define EP0_BUF_SIZE_64        3

/* USB Status register */
typedef struct {
    uint32_t rsvd1           : 5;
    uint32_t ep0_out_ack     : 1;
    uint32_t rsvd2           : 1;
    uint32_t ep0_in_ack      : 1;
    uint32_t rsvd3           : 16;
    uint32_t setup_ow        : 1;
    uint32_t setup           : 1;
    uint32_t vbus            : 1;
    uint32_t resume          : 1;
    uint32_t suspend         : 1;
    uint32_t reset           : 1;
    uint32_t sof             : 1;
    uint32_t any_int         : 1;
} UsbStatusStruct;

typedef union {
    uint32_t val;
    UsbStatusStruct f;
} UsbStatusType;

/* USB Function control register */
typedef struct {
    uint32_t dir             : 1;
    uint32_t hs_test_mode    : 3;
    uint32_t rsvd1           : 1;
    uint32_t wkup_en         : 1;
    uint32_t low_pwr_en      : 1;
    uint32_t connect         : 1;
    uint32_t rsvd2           : 4;
    uint32_t ep0_in_int_en   : 1;
    uint32_t ep0_out_int_en  : 1;
    uint32_t err_cnt_en      : 2;
    uint32_t func_addr       : 7;
    uint32_t speed           : 1;
    uint32_t setupow_int_en  : 1;
    uint32_t setup_int_en    : 1;
    uint32_t vbus_int_en     : 1;
    uint32_t resume_int_en   : 1;
    uint32_t suspend_int_en  : 1;
    uint32_t reset_int_en    : 1;
    uint32_t sof_int_en      : 1;
    uint32_t rsvd3           : 1;
} UsbCtrlStruct;

typedef union {
    uint32_t val;
    UsbCtrlStruct f;
} UsbCtrlType;

/* Endpoint 0 Control Register */
typedef struct {
    uint32_t buf_size        : 2;
    uint32_t in_int_en       : 1;
    uint32_t in_stall        : 1;
    uint32_t dbl_buf         : 1;
    uint32_t in_toggle       : 1;
    uint32_t in_nak_int_en   : 1;
    uint32_t in_en           : 1;
    uint32_t res3            : 10;
    uint32_t out_int_en      : 1;
    uint32_t out_stall       : 1;
    uint32_t res4            : 1;
    uint32_t out_toggle      : 1;
    uint32_t out_nak_int_en  : 1;
    uint32_t out_en          : 1;
    uint32_t res6            : 8;
} UsbEp0CtrlStruct;

typedef union {
    uint32_t val;
    UsbEp0CtrlStruct f;
} UsbEp0CtrlType;

/* Endpoint 0 current packet size register */
typedef struct {
    uint32_t in_xbuf_cnt     : 7;
    uint32_t in_xbuf_nak     : 1;
    uint32_t in_ybuf_cnt     : 7;
    uint32_t in_ybuf_nak     : 1;
    uint32_t out_xbuf_cnt    : 7;
    uint32_t out_xbuf_nak    : 1;
    uint32_t out_ybuf_cnt    : 7;
    uint32_t out_ybuf_nak    : 1;
} UsbEp0ByteCntStruct;

typedef union {
    uint32_t val;
    UsbEp0ByteCntStruct f;
} UsbEp0ByteCntType;

/* Endpoint n Configuration and Control register */
typedef struct {
    uint32_t res1           : 1;
    uint32_t in_toggle_rst  : 1;
    uint32_t in_ack_int     : 1;
    uint32_t in_stall       : 1;
    uint32_t in_dbl_buf     : 1;
    uint32_t in_toggle      : 1;
    uint32_t in_nak_int     : 1;
    uint32_t in_en          : 1;
    uint32_t res2           : 1;
    uint32_t out_toggle_rst : 1;
    uint32_t out_ack_int    : 1;
    uint32_t out_stall      : 1;
    uint32_t out_dbl_buf    : 1;
    uint32_t out_toggle     : 1;
    uint32_t out_nak_int    : 1;
    uint32_t out_en         : 1;
    uint32_t in_buf_size    : 8;
    uint32_t out_buf_size   : 8;
} UsbEpCfgCtrlStruct;

typedef union {
    uint32_t val;
    UsbEpCfgCtrlStruct f;
} UsbEpCfgCtrlType;

/* Endpoint n XY Buffer Start Address register */
typedef struct {
    uint8_t xBuffStartAddrIn;
    uint8_t yBuffStartAddrIn;
    uint8_t xBuffStartAddrOut;
    uint8_t yBuffStartAddrOut;
} UsbEpStartAddrStruct;

typedef union {
    uint32_t val;
    UsbEpStartAddrStruct f;
} UsbEpStartAddrType;

/* Endpoint n Packet Control register */
typedef struct {
    uint32_t xBufPacketCount  : 11;
    uint32_t res1             : 4;
    uint32_t xbuf_nak         : 1;
    uint32_t yBufPacketCount  : 11;
    uint32_t res2             : 4;
    uint32_t ybuf_nak         : 1;
} UsbEpByteCntStruct;

typedef union {
    uint32_t val;
    UsbEpByteCntStruct f;
} UsbEpByteCntType;

#define tnetv_usb_reg_read(x)  (*((volatile uint32_t *) (x)))
#define tnetv_usb_reg_write(x, val)  (*((volatile uint32_t *) (x)) = (uint32_t) (val))


#endif
