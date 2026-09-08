/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
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

#ifndef __SFC_X1000_H__
#define __SFC_X1000_H__

/* Register definitions first: sfc-ingenic.h is written against these names.
 * The shared contract -- the constants and declarations that
 * ingenic/nand-ingenic.c depends on -- lives there. */
#include "x1000/sfc.h"
#include "sfc-ingenic.h"

/** \brief Macro to generate an SFC command for use with sfc_exec()
 * \param cmd       Command number (up to 16 bits)
 * \param tmode     SPI transfer mode
 * \param awidth    Number of address bytes
 * \param dwidth    Number of dummy cycles (1 cycle = 1 bit)
 * \param pfmt      Phase format (address first or dummy first)
 * \param data_en   1 to enable data phase, 0 to omit it
 *
 * Per-target: the X1600 keeps the transfer mode elsewhere. Same signature
 * either way -- see the note in sfc-ingenic.h. */
#define SFC_CMD(cmd, tmode, awidth, dwidth, pfmt, data_en) \
    jz_orf(SFC_TRAN_CONF, COMMAND(cmd), CMD_EN(1), \
           MODE(tmode), ADDR_WIDTH(awidth), DUMMY_BITS(dwidth), \
           PHASE_FMT(pfmt), DATA_EN(data_en))

#endif /* __SFC_X1000_H__ */
