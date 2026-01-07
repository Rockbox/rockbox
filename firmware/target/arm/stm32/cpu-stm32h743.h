/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#ifndef __CPU_STM32H743_H__
#define __CPU_STM32H743_H__

#include "config.h"

#define CACHE_SIZE              (16 * 1024)
#define CACHEALIGN_BITS         5

#define DCACHE_SIZE             CACHE_SIZE
#define DCACHE_WAYS             0x4
#define DCACHE_SETS             0x80
#define DCACHE_LINESIZE         (1 << CACHEALIGN_BITS)

#define STM32_LSI_FREQ          32000
#define STM32_HSI_FREQ          64000000
#define STM32_CSI_FREQ          4000000
#define STM32_HSI48_FREQ        48000000

#if defined(HAVE_USBSTACK)
# if !defined(STM32H743_USBOTG_INSTANCE)
#  error "STM32H743_USBOTG_INSTANCE is undefined!"
# endif
# if !defined(STM32H743_USBOTG_PHY)
#  error "STM32H743_USBOTG_PHY is undefined!"
# endif
# if !defined(STM32H743_USBOTG_CLKSEL)
#  error "STM32H743_USBOTG_CLKSEL is undefined!"
# endif
# if (STM32H743_USBOTG_INSTANCE == STM32H743_USBOTG_INSTANCE_USB1)
#  define OTGBASE 0x40040000
# elif (STM32H743_USBOTG_INSTANCE == STM32H743_USBOTG_INSTANCE_USB2)
#  define OTGBASE 0x40080000
# endif
# if (STM32H743_USBOTG_PHY == STM32H743_USBOTG_PHY_ULPI_FS)
#  define USB_DW_DCFG_SPEED 1
# elif (STM32H743_USBOTG_PHY == STM32H743_USBOTG_PHY_INT_FS)
#  define USB_DW_DCFG_SPEED 3
# endif
# define USB_DW_TURNAROUND 5
/*
 * The hardware supports up to 9 endpoints, but since FIFO RAM
 * is limited, we can't support more than 5 IN EPs with a max
 * packet size of 512 bytes (usb-designware allocates the same
 * amount of RAM to each IN endpoint).
 */
# define USB_NUM_ENDPOINTS 6
#endif

#define STM32_ITCM_BASE         0x00000000
#define STM32_ITCM_SIZE         (64 * 1024)

#define STM32_DTCM_BASE         0x20000000
#define STM32_DTCM_SIZE         (128 * 1024)

#define STM32_SRAM_AXI_BASE     0x24000000
#define STM32_SRAM_AXI_SIZE     (512 * 1024)

#define STM32_SRAM1_BASE        0x30000000
#define STM32_SRAM1_SIZE        (128 * 1024)

#define STM32_SRAM2_BASE        0x30020000
#define STM32_SRAM2_SIZE        (128 * 1024)

#define STM32_SRAM3_BASE        0x30040000
#define STM32_SRAM3_SIZE        (128 * 1024)

#define STM32_SRAM4_BASE        0x38000000
#define STM32_SRAM4_SIZE        (64 * 1024)

#define STM32_SRAM_BKP_BASE     0x38800000
#define STM32_SRAM_BKP_SIZE     (4 * 1024)

#define STM32_FLASH_BANK1_BASE  0x08000000
#define STM32_FLASH_BANK1_SIZE  (1024 * 1024)

#define STM32_FLASH_BANK2_BASE  0x08100000
#define STM32_FLASH_BANK2_SIZE  (1024 * 1024)

/* FIXME: this changes depending on target settings */
#define STM32_SDRAM1_BASE       0x70000000

#endif /* __CPU_STM32H743_H__ */
