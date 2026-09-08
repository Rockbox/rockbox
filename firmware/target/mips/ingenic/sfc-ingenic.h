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

/* Include through the target SFC header for register definitions.
 * SFC_CMD() stays per-target: X1600 moved the transfer-mode field. */

#ifndef __SFC_INGENIC_H__
#define __SFC_INGENIC_H__

#ifndef REG_SFC_DEV_CONF
# error "include sfc-x1000.h / sfc-x1600.h, not sfc-ingenic.h directly"
#endif

#include <stdint.h>
#include <stdbool.h>

/* SPI transfer mode. SFC_TMODE_X_Y_Z means:
 *
 * - X lines for command phase
 * - Y lines for address+dummy phase
 * - Z lines for data phase
 *
 * Bit-for-bit the same on both parts; only the register differs. */
#define SFC_TMODE_1_1_1 0
#define SFC_TMODE_1_1_2 1
#define SFC_TMODE_1_2_2 2
#define SFC_TMODE_2_2_2 3
#define SFC_TMODE_RESERVED 4    /* X1000 10.7.3.6 MODE / X1600 PM 5.7.3.9 */
#define SFC_TMODE_1_1_4 5
#define SFC_TMODE_1_4_4 6
#define SFC_TMODE_4_4_4 7

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

/* Open/close SFC hardware */
extern void sfc_open(void);
extern void sfc_close(void);

/* Enable IRQ mode, instead of busy waiting for operations to complete.
 * Needs to be called separately after sfc_open(), because the SPL has to
 * use busy waiting, but we cannot #ifdef it for the SPL due to limitations
 * of the build system. */
extern void sfc_irq_begin(void);
extern void sfc_irq_end(void);

/* Change the SFC clock frequency; returns zero on success, negative on error. */
extern int sfc_set_clock(uint32_t freq);

/* Set the device configuration register */
static inline void sfc_set_dev_conf(uint32_t conf)
{
    REG_SFC_DEV_CONF = conf;
}

/** \brief Execute a command
 * \param cmd   Command encoded by the target's `SFC_CMD` macro.
 * \param addr  Address up to 32 bits; pass 0 if the command doesn't need it
 * \param data  Buffer for data transfer commands, must be cache-aligned
 * \param size  Number of data bytes / direction of transfer flag
 * \return Zero on completion, negative on a controller error.
 *
 * - Non-data commands must pass `data = NULL` and `size = 0` in order to
 *   get correct results.
 *
 * - Data commands must specify a direction of transfer using the high bit
 *   of the `size` argument by OR'ing in `SFC_READ` or `SFC_WRITE`.
 */
extern int sfc_exec(uint32_t cmd, uint32_t addr, void* data, uint32_t size);

/* NOTE: the above will need to be changed if we need better performance.
 * The hardware can do multiple commands in a sequence, including polling,
 * and emit an interrupt only at the end.
 */

/* Route the SFC's six pins to the controller. Implemented per target -- the
 * X1000 uses PA26-31, the X1600 PC17-22, which on the X1000's map is the
 * display. Called by the shared SPL storage backend, which cannot rely on
 * pinctrl and must not silently depend on the BootROM having done it. */
extern void sfc_configure_pins(void);

#endif /* __SFC_INGENIC_H__ */
