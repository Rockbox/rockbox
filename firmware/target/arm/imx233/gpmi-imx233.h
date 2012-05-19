/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#ifndef __GPMI_IMX233_H__
#define __GPMI_IMX233_H__

#include "system.h"

/* GPMI */

#define HW_GPMI_BASE        0x8000c000 

#define HW_GPMI_CTRL0       (*(volatile uint32_t *)(HW_GPMI_BASE + 0x0))
#define HW_GPMI_CTRL0__XFER_COUNT_BP    0
#define HW_GPMI_CTRL0__XFER_COUNT_BM    0xffff
#define HW_GPMI_CTRL0__ADDRESS_INCREMENT    (1 << 16)
#define HW_GPMI_CTRL0__CS_BP            20
#define HW_GPMI_CTRL0__CS_BM            (0x3 << 20)
#define HW_GPMI_CTRL0__LOCK_CS          (1 << 22)
#define HW_GPMI_CTRL0__WORD_LENGTH      (1 << 23)
#define HW_GPMI_CTRL0__WORD_LENGTH__16_BIT  (0 << 23)
#define HW_GPMI_CTRL0__WORD_LENGTH__8_BIT   (1 << 23)
#define HW_GPMI_CTRL0__COMMAND_MODE_BP  24
#define HW_GPMI_CTRL0__COMMAND_MODE_BM  (0x3 << 24)
#define HW_GPMI_CTRL0__COMMAND_MODE__WRITE  (0 << 24)
#define HW_GPMI_CTRL0__COMMAND_MODE__READ   (1 << 24)
#define HW_GPMI_CTRL0__COMMAND_MODE__READ_AND_COMPARE   (2 << 24)
#define HW_GPMI_CTRL0__COMMAND_MODE__WAIT_FOR_READY (3 << 24)
#define HW_GPMI_CTRL0__TIMEOUT_IRQ_EN   (1 << 27)
#define HW_GPMI_CTRL0__RUN              (1 << 29)

#define HW_GPMI_COMPARE     (*(volatile uint32_t *)(HW_GPMI_BASE + 0x10))
#define HW_GPMI_COMPARE__REFERENCE_BP   0
#define HW_GPMI_COMPARE__REFERENCE_BM   0xffff
#define HW_GPMI_COMPARE__MASK_BP        16
#define HW_GPMI_COMPARE__MASK_BM        0xffff0000

#define HW_GPMI_ECCCTRL     (*(volatile uint32_t *)(HW_GPMI_BASE + 0x20))
#define HW_GPMI_ECCCTRL__BUFFER_MASK_BP 0
#define HW_GPMI_ECCCTRL__BUFFER_MASK_BM 0x1ff
#define HW_GPMI_ECCCTRL__ENABLE_ECC     (1 << 12)
#define HW_GPMI_ECCCTRL__ECC_CMD_BP     13
#define HW_GPMI_ECCCTRL__ECC_CMD_BM     (0x3 << 13)
#define HW_GPMI_ECCCTRL__HANDLE_BP      16
#define HW_GPMI_ECCCTRL__HANDLE_BM      (0xffff << 16)

#define HW_GPMI_ECCCOUNT    (*(volatile uint32_t *)(HW_GPMI_BASE + 0x30))
#define HW_GPMI_ECCCOUNT__COUNT_BP  0
#define HW_GPMI_ECCCOUNT__COUNT_BM  0xffff

#define HW_GPMI_PAYLOAD     (*(volatile uint32_t *)(HW_GPMI_BASE + 0x40))

#define HW_GPMI_AUXILIARY   (*(volatile uint32_t *)(HW_GPMI_BASE + 0x50))

#define HW_GPMI_CTRL1       (*(volatile uint32_t *)(HW_GPMI_BASE + 0x60))
#define HW_GPMI_CTRL1__ATA_IRQRDY_POLARITY  (1 << 2)
#define HW_GPMI_CTRL1__DEV_RESET    (1 << 3)
#define HW_GPMI_CTRL1__ABORT_WAIT_FOR_READY(i)  (1 << (4 + (i)))
#define HW_GPMI_CTRL1__BURST_EN     (1 << 8)
#define HW_GPMI_CTRL1__TIMEOUT_IRQ  (1 << 9)
#define HW_GPMI_CTRL1__RDN_DELAY_BP 12
#define HW_GPMI_CTRL1__RDN_DELAY_BM (0xf << 12)
#define HW_GPMI_CTRL1__HALF_PERIOD  (1 << 16)
#define HW_GPMI_CTRL1__DLL_ENABLE   (1 << 17)
#define HW_GPMI_CTRL1__BCH_MODE     (1 << 18)
#define HW_GPMI_CTRL1__GANGED_RDYBUSY   (1 << 19)
#define HW_GPMI_CTRL1__CEx_SEL      (1 << (20 + (x)))

#define HW_GPMI_TIMING0     (*(volatile uint32_t *)(HW_GPMI_BASE + 0x70))
#define HW_GPMI_TIMING0__DATA_SETUP_BP  0
#define HW_GPMI_TIMING0__DATA_SETUP_BM  0xff
#define HW_GPMI_TIMING0__DATA_HOLD_BP   8
#define HW_GPMI_TIMING0__DATA_HOLD_BM   0xff00
#define HW_GPMI_TIMING0__ADDRESS_SETUP_BP   16
#define HW_GPMI_TIMING0__ADDRESS_SETUP_BM   0xff0000

#define HW_GPMI_TIMING1     (*(volatile uint32_t *)(HW_GPMI_BASE + 0x80))
#define HW_GPMI_TIMING1__DEVICE_BUSY_TIMEOUT_BP 16
#define HW_GPMI_TIMING1__DEVICE_BUSY_TIMEOUT_BM 0xffff0000

#define HW_GPMI_DATA        (*(volatile uint32_t *)(HW_GPMI_BASE + 0xa0))

#define HW_GPMI_STAT        (*(volatile uint32_t *)(HW_GPMI_BASE + 0xb0))
#define HW_GPMI_STAT__DEVx_ERROR(x) (1 << (x))
#define HW_GPMI_STAT__FIFO_FULL     (1 << 4)
#define HW_GPMI_STAT__FIFO_EMPTY    (1 << 5)
#define HW_GPMI_STAT__INVALID_BUFFER_MASK   (1 << 6)
#define HW_GPMI_STAT__RDY_TIMEOUT_BP    8
#define HW_GPMI_STAT__RDY_TIMEOUT_BM    (0xf << 8)

#define HW_GPMI_DEBUG       (*(volatile uint32_t *)(HW_GPMI_BASE + 0xc0))

#define HW_GPMI_VERSION     (*(volatile uint32_t *)(HW_GPMI_BASE + 0xd0))

#define HW_GPMI_DEBUG2      (*(volatile uint32_t *)(HW_GPMI_BASE + 0xe0))

#define HW_GPMI_DEBUG3      (*(volatile uint32_t *)(HW_GPMI_BASE + 0xf0))

/* ECC8 */

#define HW_ECC8_BASE        0x80008000

#define HW_ECC8_CTRL        (*(volatile uint32_t *)(HW_ECC8_BASE + 0x0))
#define HW_ECC8_CTRL__COMPLETE_IRQ  (1 << 0)
#define HW_ECC8_CTRL__COMPLETE_IRQ_EN   (1 << 8)
#define HW_ECC8_CTRL__THROTTLE_BP   24
#define HW_ECC8_CTRL__THROTTLE_BM   (0xf << 24)
#define HW_ECC8_CTRL__AHBM_SFTRST   (1 << 29)

#define HW_ECC8_STATUS0     (*(volatile uint32_t *)(HW_ECC8_BASE + 0x10))
#define HW_ECC8_STATUS0__UNCORRECTABLE  (1 << 2)
#define HW_ECC8_STATUS0__CORRECTED      (1 << 3)
#define HW_ECC8_STATUS0__ALLONES        (1 << 4)
#define HW_ECC8_STATUS0__STATUS_AUX_BP  8
#define HW_ECC8_STATUS0__STATUS_AUX_BM  (0xf << 8)
#define HW_ECC8_STATUS0__COMPLETED_CE_BP    16
#define HW_ECC8_STATUS0__COMPLETED_CE_BM    (0xf << 16)
#define HW_ECC8_STATUS0__HANDLE_BP      20
#define HW_ECC8_STATUS0__HANDLE_BM      (0xfff << 20)

#define HW_ECC8_STATUS1     (*(volatile uint32_t *)(HW_ECC8_BASE + 0x20))
#define HW_ECC8_STATUS1__STATUS_PAYLOADx_BP (4 * (x))
#define HW_ECC8_STATUS1__STATUS_PAYLOADx_BM (0xf << (4 * (x)))

/* BCH */

#define HW_BCH_BASE         0x8000a000

#define HW_BCH_CTRL         (*(volatile uint32_t *)(HW_BCH_BASE + 0x0))
#define HW_BCH_CTRL__COMPLETE_IRQ   (1 << 0)
#define HW_BCH_CTRL__COMPLETE_IRQ_EN    (1 << 8)
#define HW_BCH_CTRL__M2M_ENABLE     (1 << 16)
#define HW_BCH_CTRL__M2M_ENCODE     (1 << 17)
#define HW_BCH_CTRL__M2M_LAYOUT_BP  18
#define HW_BCH_CTRL__M2M_LAYOUT_BM  (0x3 << 18)

#define HW_BCH_STATUS0      (*(volatile uint32_t *)(HW_BCH_BASE + 0x10))
#define HW_BCH_STATUS0__UNCORRECTABLE   (1 << 2)
#define HW_BCH_STATUS0__CORRECTED       (1 << 3)
#define HW_BCH_STATUS0__ALLONES         (1 << 4)
#define HW_BCH_STATUS0__STATUS_BLK0_BP  8
#define HW_BCH_STATUS0__STATUS_BLK0_BM  (0xf << 8)
#define HW_BCH_STATUS0__COMPLETED_CE_BP 16
#define HW_BCH_STATUS0__COMPLETED_CE_BM (0xf << 16)
#define HW_BCH_STATUS0__HANDLE_BP       20
#define HW_BCH_STATUS0__HANDLE_BM       (0xfff << 20)

#define HW_BCH_MODE         (*(volatile uint32_t *)(HW_BCH_BASE + 0x20))
#define HW_BCH_MODE__ERASE_THRESHOLD_BP 0
#define HW_BCH_MODE__ERASE_THRESHOLD_BM 0xff

#define HW_BCH_ENCODEPTR    (*(volatile uint32_t *)(HW_BCH_BASE + 0x30))

#define HW_BCH_DATAPTR      (*(volatile uint32_t *)(HW_BCH_BASE + 0x40))

#define HW_BCH_METAPTR      (*(volatile uint32_t *)(HW_BCH_BASE + 0x50))

#define HW_BCH_LAYOUTSELECT (*(volatile uint32_t *)(HW_BCH_BASE + 0x60))
#define HW_BCH_LAYOUTSELECT__CSx_SELECT_BP(x)   (2 * (x))
#define HW_BCH_LAYOUTSELECT__CSx_SELECT_BM(x)   (0x3 << (2 * (x)))

#define HW_BCH_FLASHxLAYOUT0(x) (*(volatile uint32_t *)(HW_BCH_BASE + 0x80 + (x) * 0x20))
#define HW_BCH_FLASHxLAYOUT0__DATA0_SIZE_BP 0
#define HW_BCH_FLASHxLAYOUT0__DATA0_SIZE_BM 0xfff
#define HW_BCH_FLASHxLAYOUT0__ECC0_BP       12
#define HW_BCH_FLASHxLAYOUT0__ECC0_BM       0xf000
#define HW_BCH_FLASHxLAYOUT0__META_SIZE_BP  16
#define HW_BCH_FLASHxLAYOUT0__META_SIZE_BM  0xff0000
#define HW_BCH_FLASHxLAYOUT0__NBLOCKS_BP    24
#define HW_BCH_FLASHxLAYOUT0__NBLOCKS_BM    0xff000000

#define HW_BCH_FLASHxLAYOUT1(x) (*(volatile uint32_t *)(HW_BCH_BASE + 0x90 + (x) * 0x20))
#define HW_BCH_FLASHxLAYOUT1__DATAN_SIZE_BP 0
#define HW_BCH_FLASHxLAYOUT1__DATAN_SIZE_BM 0xfff
#define HW_BCH_FLASHxLAYOUT1__ECCN_BP       12
#define HW_BCH_FLASHxLAYOUT1__ECCN_BM       0xf000
#define HW_BCH_FLASHxLAYOUT1__PAGE_SIZE_BP  16
#define HW_BCH_FLASHxLAYOUT1__PAGE_SIZE_BM  0xffff0000

#endif /* __GPMI_IMX233_H__ */
