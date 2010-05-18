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
#ifndef __USB_DRV_AS3525v2_H__
#define __USB_DRV_AS3525v2_H__

#include "as3525v2.h"

#define USB_DEVICE                  (USB_BASE + 0x0800)   /** USB Device base address */

/**
 * Core Global Registers
 */
#define USB_GOTGCTL     (*(volatile unsigned long *)(USB_BASE + 0x000)) /** OTG Control and Status Register */
#define USB_GOTGINT     (*(volatile unsigned long *)(USB_BASE + 0x004)) /** OTG Interrupt Register */
#define USB_GAHBCFG     (*(volatile unsigned long *)(USB_BASE + 0x008)) /** Core AHB Configuration Register */
#define USB_GUSBCFG     (*(volatile unsigned long *)(USB_BASE + 0x00C)) /** Core USB Configuration Register */
#define USB_GRSTCTL     (*(volatile unsigned long *)(USB_BASE + 0x010)) /** Core Reset Register */
#define USB_GINTSTS     (*(volatile unsigned long *)(USB_BASE + 0x014)) /** Core Interrupt Register */
#define USB_GINTMSK     (*(volatile unsigned long *)(USB_BASE + 0x018)) /** Core Interrupt Mask Register */
#define USB_GRXSTSR     (*(volatile unsigned long *)(USB_BASE + 0x01C)) /** Receive Status Debug Read Register (Read Only) */
#define USB_GRXSTSP     (*(volatile unsigned long *)(USB_BASE + 0x020)) /** Receive Status Read /Pop Register (Read Only) */
#define USB_GRXFSIZ     (*(volatile unsigned long *)(USB_BASE + 0x024)) /** Receive FIFO Size Register */
#define USB_GNPTXFSIZ   (*(volatile unsigned long *)(USB_BASE + 0x028)) /** Periodic Transmit FIFO Size Register */
#define USB_GNPTXSTS    (*(volatile unsigned long *)(USB_BASE + 0x02C)) /** Non-Periodic Transmit FIFO/Queue Status Register */
#define USB_GI2CCTL     (*(volatile unsigned long *)(USB_BASE + 0x030)) /** I2C Access Register */
#define USB_GPVNDCTL    (*(volatile unsigned long *)(USB_BASE + 0x034)) /** PHY Vendor Control Register */
#define USB_GGPIO       (*(volatile unsigned long *)(USB_BASE + 0x038)) /** General Purpose Input/Output Register */
#define USB_GUID        (*(volatile unsigned long *)(USB_BASE + 0x03C)) /** User ID Register */
#define USB_GSNPSID     (*(volatile unsigned long *)(USB_BASE + 0x040)) /** Synopsys ID Register */
#define USB_GHWCFG1     (*(volatile unsigned long *)(USB_BASE + 0x044)) /** User HW Config1 Register */
#define USB_GHWCFG2     (*(volatile unsigned long *)(USB_BASE + 0x048)) /** User HW Config2 Register */
#define USB_GHWCFG3     (*(volatile unsigned long *)(USB_BASE + 0x04C)) /** User HW Config3 Register */
#define USB_GHWCFG4     (*(volatile unsigned long *)(USB_BASE + 0x050)) /** User HW Config4 Register */

#define USB_GRSTCTL_csftrst                 (1 << 0) /** Core soft reset */
#define USB_GRSTCTL_hsftrst                 (1 << 1) /** Hclk soft reset */
#define USB_GRSTCTL_ahbidle                 (1 << 31) /** AHB idle state*/

#define USB_GHWCFG1_IN_EP(ep)               ((USB_GHWCFG1 >> ((ep) *2)) & 0x1) /** 1 if EP(ep) has in cap */
#define USB_GHWCFG1_OUT_EP(ep)              ((USB_GHWCFG1 >> ((ep) *2 + 1)) & 0x1)/** 1 if EP(ep) has out cap */

#define USB_GHWCFG3_DFIFO_LEN               (USB_GHWCFG3 >> 16) /** Total fifo size */

#define USB_GHWCFG4_NUM_IN_EP               ((USB_GHWCFG4 >> 26) & 0xf) /** Number of IN endpoints */

#define USB_GHWCFG2_NUM_EP                  ((USB_GHWCFG2 >> 10) & 0xf) /** Number of endpoints */

#define USB_GUSBCFG_ulpi_utmi_sel           (1 << 4) /** select ulpi:1 or utmi:0 */
#define USB_GUSBCFG_phy_if                  (1 << 3) /** select utmi bus width ? */
#define USB_GUSBCFG_SRP_cap                 0x100
#define USB_GUSBCFG_HNP_cap                 0x200

#define USB_GAHBCFG_hburstlen_bit_pos       1
#define USB_GAHBCFG_INT_DMA_BURST_INCR      1 /** note: the linux patch has several other value, this is one picked for internal dma */
#define USB_GAHBCFG_dma_enable              (1 << 5)

/**
 * Device Registers Base Addresses
 */
#define USB_DCFG        (*(volatile unsigned long *)(USB_DEVICE + 0x00)) /** Device Configuration Register */
#define USB_DCTL        (*(volatile unsigned long *)(USB_DEVICE + 0x04)) /** Device Control Register */
#define USB_DSTS        (*(volatile unsigned long *)(USB_DEVICE + 0x08)) /** Device Status Register */
#define USB_DIEPMSK     (*(volatile unsigned long *)(USB_DEVICE + 0x10)) /** Device IN Endpoint Common Interrupt Mask Register */
#define USB_DOEPMSK     (*(volatile unsigned long *)(USB_DEVICE + 0x14)) /** Device OUT Endpoint Common Interrupt Mask Register */
#define USB_DAINT       (*(volatile unsigned long *)(USB_DEVICE + 0x18)) /** Device All Endpoints Interrupt Register */
#define USB_DAINTMSK    (*(volatile unsigned long *)(USB_DEVICE + 0x1C)) /** Device Endpoints Interrupt Mask Register */
#define USB_DTKNQR1     (*(volatile unsigned long *)(USB_DEVICE + 0x20)) /** Device IN Token Sequence Learning Queue Read Register 1 */
#define USB_DTKNQR2     (*(volatile unsigned long *)(USB_DEVICE + 0x24)) /** Device IN Token Sequence Learning Queue Register 2 */
#define USB_DTKNQP      (*(volatile unsigned long *)(USB_DEVICE + 0x28)) /** Device IN Token Queue Pop register */

#define USB_PCGCCTL     (*(volatile unsigned long *)(USB_BASE + 0xE00)) /** Power and Clock Gating Control Register */

#endif /* __USB_DRV_AS3525v2_H__ */
