/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

#include "x1000/sfc.h"
#include <stdint.h>
#include <stdbool.h>

/* SPI transfer mode. SFC_TMODE_X_Y_Z means:
 *
 * - X lines for command phase
 * - Y lines for address+dummy phase
 * - Z lines for data phase
 */
#define SFC_TMODE_1_1_1 0
#define SFC_TMODE_1_1_2 1
#define SFC_TMODE_1_2_2 2
#define SFC_TMODE_2_2_2 3
#define SFC_TMODE_1_1_4 4
#define SFC_TMODE_1_4_4 5
#define SFC_TMODE_4_4_4 6

/* Phase format
 *  _____________________
 * / SFC_PFMT_ADDR_FIRST \
 * +-----+-------+-------+------+
 * | cmd | addr  | dummy | data |
 * +-----+-------+-------+------+
 *  ______________________
 * / SFC_PFMT_DUMMY_FIRST \
 * +-----+-------+-------+------+
 * | cmd | dummy | addr  | data |
 * +-----+-------+-------+------+
 */
#define SFC_PFMT_ADDR_FIRST  0
#define SFC_PFMT_DUMMY_FIRST 1

/* Direction of transfer flag */
#define SFC_READ    0
#define SFC_WRITE   (1 << 31)

/** \brief Macro to generate an SFC command for use with sfc_exec()
 * \param cmd       Command number (up to 16 bits)
 * \param tmode     SPI transfer mode
 * \param awidth    Number of address bytes
 * \param dwidth    Number of dummy cycles (1 cycle = 1 bit)
 * \param pfmt      Phase format (address first or dummy first)
 * \param data_en   1 to enable data phase, 0 to omit it
 */
#define SFC_CMD(cmd, tmode, awidth, dwidth, pfmt, data_en) \
    jz_orf(SFC_TRAN_CONF, COMMAND(cmd), CMD_EN(1), \
           MODE(tmode), ADDR_WIDTH(awidth), DUMMY_BITS(dwidth), \
           PHASE_FMT(pfmt), DATA_EN(data_en))

/* Open/close SFC hardware */
extern void sfc_open(void);
extern void sfc_close(void);

/* Enable IRQ mode, instead of busy waiting for operations to complete.
 * Needs to be called separately after sfc_open(), because the SPL has to
 * use busy waiting, but we cannot #ifdef it for the SPL due to limitations
 * of the build system. */
extern void sfc_irq_begin(void);
extern void sfc_irq_end(void);

/* Change the SFC clock frequency */
extern void sfc_set_clock(uint32_t freq);

/* Set the device configuration register */
static inline void sfc_set_dev_conf(uint32_t conf)
{
    REG_SFC_DEV_CONF = conf;
}

/* Control the state of the write protect pin */
static inline void sfc_set_wp_enable(bool en)
{
    jz_writef(SFC_GLB, WP_EN(en ? 1 : 0));
}

/** \brief Execute a command
 * \param cmd   Command encoded by `SFC_CMD` macro.
 * \param addr  Address up to 32 bits; pass 0 if the command doesn't need it
 * \param data  Buffer for data transfer commands, must be cache-aligned
 * \param size  Number of data bytes / direction of transfer flag
 * \returns SFC status code: 0 on success and < 0 on failure.
 *
 * - Non-data commands must pass `data = NULL` and `size = 0` in order to
 *   get correct results.
 *
 * - Data commands must specify a direction of transfer using the high bit
 *   of the `size` argument by OR'ing in `SFC_READ` or `SFC_WRITE`.
 */
extern void sfc_exec(uint32_t cmd, uint32_t addr, void* data, uint32_t size);

/* NOTE: the above will need to be changed if we need better performance
 * The hardware can do multiple commands in a sequence, including polling,
 * and emit an interrupt only at the end.
 *
 * Also, some chips need more than 4 address bytes even though the block
 * and page numbers would still fit in 32 bits; the current API cannot
 * handle this.
 */

#endif /* __SFC_X1000_H__ */
