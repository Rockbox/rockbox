/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 *
 * Based on firmware/target/mips/ingenic_x1000/sfc-x1000.h,
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __SFC_X1600_H__
#define __SFC_X1600_H__

/* Register definitions first; the shared contract follows. */
#include "x1600/sfc.h"
#include "sfc-ingenic.h"

/* The X1600 transfer mode is in TRAN_CONF1 rather than TRAN_CONF.
 * sfc_exec() moves the encoded mode bits before issuing the command. The
 * clock divider is in SFCCDR; DMA and CDT modes are not implemented. */

/* Carry the transfer mode in bits 31:29 until sfc_exec() moves it to
 * TRAN_CONF1. */
#define SFC_CMD_TMODE_BP        29
#define SFC_CMD_TMODE_BM        0xe0000000
#define SFC_CMD_TMODE(v)        (((uint32_t)(v) & 0x7) << SFC_CMD_TMODE_BP)
#define SFC_CMD_GET_TMODE(c)    (((c) & SFC_CMD_TMODE_BM) >> SFC_CMD_TMODE_BP)

/** \brief Macro to generate an SFC command for use with sfc_exec()
 * \param cmd       Command number (up to 16 bits)
 * \param tmode     SPI transfer mode (SFC_TMODE_*)
 * \param awidth    Number of address bytes
 * \param dwidth    Number of dummy cycles (1 cycle = 1 bit)
 * \param pfmt      Phase format (address first or dummy first)
 * \param data_en   1 to enable data phase, 0 to omit it
 */
#define SFC_CMD(cmd, tmode, awidth, dwidth, pfmt, data_en)  \
    (jz_orf(SFC_TRAN_CONF, COMMAND(cmd), CMD_EN(1),         \
            ADDR_WIDTH(awidth), DUMMY_BITS(dwidth),         \
            PHASE_FMT(pfmt), DATA_EN(data_en))              \
     | SFC_CMD_TMODE(tmode))

/* Command descriptor table size. */
#define SFC_CDT_WORDS   256

#endif /* __SFC_X1600_H__ */
