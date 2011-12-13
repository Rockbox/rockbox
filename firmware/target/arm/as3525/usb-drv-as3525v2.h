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
#include "usb-s3c6400x.h"

/**
 * Registers not present in usb-s3c6400
 */
#define BASE_REG(offset) (*(volatile uint32_t*)(OTGBASE + offset))

/** User HW Config1 Register */
#define GHWCFG1                 BASE_REG(0x044)
#define GHWCFG1_epdir_bitp(ep)  (2 * (ep))
#define GHWCFG1_epdir_bits      0x3
#define GHWCFG1_EPDIR_BIDIR     0
#define GHWCFG1_EPDIR_IN        1
#define GHWCFG1_EPDIR_OUT       2

/** User HW Config2 Register */
#define GHWCFG2                     BASE_REG(0x048)
#define GHWCFG2_arch_bitp           3 /** Architecture */
#define GHWCFG2_arch_bits           0x3
#define GHWCFG2_hs_phy_type_bitp    6 /** High speed PHY type */
#define GHWCFG2_hs_phy_type_bits    0x3
#define GHWCFG2_fs_phy_type_bitp    8 /** Full speed PHY type */
#define GHWCFG2_fs_phy_type_bits    0x3
#define GHWCFG2_num_ep_bitp         10 /** Number of endpoints */
#define GHWCFG2_num_ep_bits         0xf
#define GHWCFG2_dyn_fifo            (1 << 19) /** Dynamic FIFO */
/* For GHWCFG2_HS_PHY_TYPE and GHWCFG2_FS_PHY_TYPE */
#define GHWCFG2_PHY_TYPE_UNSUPPORTED        0
#define GHWCFG2_PHY_TYPE_UTMI               1
#define GHWCFG2_ARCH_INTERNAL_DMA           2

/** User HW Config3 Register */
#define GHWCFG3                 BASE_REG(0x04C)
#define GHWCFG3_dfifo_len_bitp  16 /** Total fifo size */
#define GHWCFG3_dfifo_len_bits  0xffff

/** User HW Config4 Register */
#define GHWCFG4                             BASE_REG(0x050)
#define GHWCFG4_utmi_phy_data_width_bitp    14 /** UTMI+ data bus width */
#define GHWCFG4_utmi_phy_data_width_bits    0x3
#define GHWCFG4_ded_fifo_en                 (1 << 25) /** Dedicated Tx FIFOs */
#define GHWCFG4_num_in_ep_bitp              26 /** Number of IN endpoints */
#define GHWCFG4_num_in_ep_bits              0xf

/** Device IN Endpoint (ep) Transmit FIFO Status Register */
#define DTXFSTS(ep) (*((const void* volatile*)(OTGBASE + 0x918 + 0x20 * (x))))

/** Device OUT Endpoint (ep) Frame number Register */
#define DOEPFN(ep)  (*((const void* volatile*)(OTGBASE + 0xB04 + 0x20 * (x))))

#endif /* __USB_DRV_AS3525v2_H__ */
