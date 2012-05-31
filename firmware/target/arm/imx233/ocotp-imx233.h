/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#ifndef OCOTP_IMX233_H
#define OCOTP_IMX233_H

#include "config.h"
#include "system.h"

#define HW_OCOTP_BASE           0x8002c000

#define HW_OCOTP_CTRL           (*(volatile uint32_t *)(HW_OCOTP_BASE + 0x0))
#define HW_OCOTP_CTRL__RD_BANK_OPEN (1 << 12)
#define HW_OCOTP_CTRL__ERROR        (1 << 9)
#define HW_OCOTP_CTRL__BUSY         (1 << 8)

#define HW_OCOTP_CUSTx(x)       (*(volatile uint32_t *)(HW_OCOTP_BASE + 0x20 + 0x10 * (x)))

#define HW_OCOTP_CRYPTOx(x)     (*(volatile uint32_t *)(HW_OCOTP_BASE + 0x60 + 0x10 * (x)))

#define HW_OCOTP_HWCAPx(x)      (*(volatile uint32_t *)(HW_OCOTP_BASE + 0xa0 + 0x10 * (x)))

#define HW_OCOTP_SWCAP          (*(volatile uint32_t *)(HW_OCOTP_BASE + 0x100))

#define HW_OCOTP_CUSTCAP        (*(volatile uint32_t *)(HW_OCOTP_BASE + 0x110))

#define HW_OCOTP_OPSx(x)        (*(volatile uint32_t *)(HW_OCOTP_BASE + 0x130 + 0x10 * (x)))

#define HW_OCOTP_UNx(x)         (*(volatile uint32_t *)(HW_OCOTP_BASE + 0x170 + 0x10 * (x)))

#define HW_OCOTP_ROMx(x)        (*(volatile uint32_t *)(HW_OCOTP_BASE + 0x1a0 + 0x10 * (x)))

#define IMX233_NUM_OCOTP_CUST   4
#define IMX233_NUM_OCOTP_CRYPTO 4
#define IMX233_NUM_OCOTP_HWCAP  6
#define IMX233_NUM_OCOTP_OPS    4
#define IMX233_NUM_OCOTP_UN     3
#define IMX233_NUM_OCOTP_ROM    8

static void imx233_ocotp_open_banks(bool open)
{
    if(open)
    {
        __REG_CLR(HW_OCOTP_CTRL) = HW_OCOTP_CTRL__ERROR;
        __REG_SET(HW_OCOTP_CTRL) = HW_OCOTP_CTRL__RD_BANK_OPEN;
        while(HW_OCOTP_CTRL & HW_OCOTP_CTRL__BUSY);
    }
    else
        __REG_CLR(HW_OCOTP_CTRL) = HW_OCOTP_CTRL__RD_BANK_OPEN;
}

static uint32_t imx233_ocotp_read(volatile uint32_t *reg)
{
    imx233_ocotp_open_banks(true);
    uint32_t val = *reg;
    imx233_ocotp_open_banks(false);
    return val;
}

#endif /* OCOTP_IMX233_H */
