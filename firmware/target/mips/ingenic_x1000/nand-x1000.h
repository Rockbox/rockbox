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

#ifndef __NAND_X1000_H__
#define __NAND_X1000_H__

/* NOTE: this is a very minimal API designed only to support a bootloader.
 * Not suitable for general data storage. It doesn't have proper support for
 * partial page writes, access to spare area, etc, which are all necessary
 * for an effective flash translation layer.
 *
 * There's no ECC support. This can be added if necessary, but it's unlikely
 * the boot area on any X1000 device uses software ECC as Ingenic's SPL simply
 * doesn't have much room for more code (theirs programmed to work on multiple
 * hardware configurations, so it's bigger than ours).
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nand-x1000-err.h"

/* Chip supports quad I/O for page read/write */
#define NANDCHIP_FLAG_QUAD      0x01

/* Set/clear the BRWD bit when enabling/disabling write protection */
#define NANDCHIP_FLAG_USE_BRWD  0x02

typedef struct nand_chip_data {
    /* Chip manufacturer / device ID */
    uint8_t  mf_id;
    uint8_t  dev_id;

    /* Width of row/column addresses in bytes */
    uint8_t  rowaddr_width;
    uint8_t  coladdr_width;

    /* SFC dev conf and clock frequency to use for this device */
    uint32_t dev_conf;
    uint32_t clock_freq;

    /* Page size in bytes = 1 << log2_page_size */
    uint32_t log2_page_size;

    /* Block size in number of pages = 1 << log2_block_size */
    uint32_t log2_block_size;

    /* Chip flags */
    uint32_t flags;
} nand_chip_data;

/* Open or close the NAND driver. The NAND driver takes control of the SFC,
 * so that driver must be in the closed state before opening the NAND driver.
 */
extern int nand_open(void);
extern void nand_close(void);

/* Identify the NAND chip. This must be done after opening the driver and
 * prior to any data access, in order to set the chip parameters. */
extern int nand_identify(int* mf_id, int* dev_id);

/* Return the chip data for the identified NAND chip.
 * Returns NULL if the chip is not identified. */
const nand_chip_data* nand_get_chip_data(void);

/* Controls the chip's write protect features. The driver also keeps track of
 * this flag and refuses to perform write or erase operations unless you have
 * enabled writes. Writes should be disabled again when you finish writing. */
extern int nand_enable_writes(bool en);

/* Reading and writing operates on whole pages at a time. If the address or
 * size is not aligned to a multiple of the page size, no data will be read
 * or written and an error code is returned. */
extern int nand_read(uint32_t addr, uint32_t size, uint8_t* buf);
extern int nand_write(uint32_t addr, uint32_t size, const uint8_t* buf);

/* Ereas eoperates on whole blocks. Like the page read/write operations,
 * the address and size must be aligned to a multiple of the block size.
 * If not, no blocks are erased and an error code is returned. */
extern int nand_erase(uint32_t addr, uint32_t size);

#endif /* __NAND_X1000_H__ */
