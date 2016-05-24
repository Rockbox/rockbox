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

/** STMP3700 and over have OCOTP registers
 * where STMP3600 has laser fuses. */

#if IMX233_SUBTARGET >= 3700
#include "regs/ocotp.h"

#define IMX233_NUM_OCOTP_CUST   4
#define IMX233_NUM_OCOTP_CRYPTO 4
#define IMX233_NUM_OCOTP_HWCAP  6
#define IMX233_NUM_OCOTP_OPS    4
#define IMX233_NUM_OCOTP_UN     3
#define IMX233_NUM_OCOTP_ROM    8

static inline void imx233_ocotp_open_banks(bool open)
{
    if(open)
    {
        BF_CLR(OCOTP_CTRL, ERROR);
        BF_SET(OCOTP_CTRL, RD_BANK_OPEN);
        while(BF_RD(OCOTP_CTRL, BUSY));
    }
    else
        BF_CLR(OCOTP_CTRL, RD_BANK_OPEN);
}

static inline uint32_t imx233_ocotp_read(volatile uint32_t *reg)
{
    imx233_ocotp_open_banks(true);
    uint32_t val = *reg;
    imx233_ocotp_open_banks(false);
    return val;
}
#else
#include "regs/rtc.h"

#define IMX233_NUM_OCOTP_LASERFUSE  12

static inline uint32_t imx233_ocotp_read(volatile uint32_t *reg)
{
    BF_WR(RTC_UNLOCK, KEY_V(VAL));
    uint32_t val = *reg;
    BF_WR(RTC_UNLOCK, KEY(0));
    return val;
}
#endif

#endif /* OCOTP_IMX233_H */
