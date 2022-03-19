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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "kernel.h"

#define NAND_SUCCESS            0
#define NAND_ERR_UNKNOWN_CHIP   (-1)
#define NAND_ERR_PROGRAM_FAIL   (-2)
#define NAND_ERR_ERASE_FAIL     (-3)
#define NAND_ERR_UNALIGNED      (-4)

/* keep max page size in sync with the NAND chip table in the .c file */
#define NAND_DRV_SCRATCHSIZE 32
#define NAND_DRV_MAXPAGESIZE 2112

/* Quad I/O support bit */
#define NAND_CHIPFLAG_QUAD          0x0001
/* Chip requires QE bit set to enable quad I/O mode */
#define NAND_CHIPFLAG_HAS_QE_BIT    0x0002

/* Types to distinguish between block & page addresses in the API.
 *
 *                BIT 31                            log2_ppb bits
 *                +-------------------------------+---------------+
 *  nand_page_t = | block nr                      | page nr       |
 *                +-------------------------------+---------------+
 *                                                            BIT 0
 *
 * The page address is split into block and page numbers. Page numbers occupy
 * the lower log2_ppb bits, and the block number occupies the upper bits.
 *
 * Block addresses are structured the same as page addresses, but with a page
 * number of 0. So block number N has address N << log2_ppb.
 */
typedef uint32_t nand_block_t;
typedef uint32_t nand_page_t;

typedef struct nand_chip {
    /* Manufacturer and device ID bytes */
    uint8_t mf_id;
    uint8_t dev_id;

    /* Row/column address width */
    uint8_t row_cycles;
    uint8_t col_cycles;

    /* Base2 logarithm of the number of pages per block */
    unsigned log2_ppb;

    /* Size of a page's main / oob areas, in bytes. */
    unsigned page_size;
    unsigned oob_size;

    /* Total number of blocks in the chip */
    unsigned nr_blocks;

    /* Bad block marker offset within the 1st page of a bad block */
    unsigned bbm_pos;

    /* Clock frequency to use */
    uint32_t clock_freq;

    /* Value of sfc_dev_conf */
    uint32_t dev_conf;

    /* Chip specific flags */
    uint32_t flags;
} nand_chip;

typedef struct nand_drv {
    /* NAND access lock. Needs to be held during any operations. */
    struct mutex mutex;

    /* Reference count for open/close operations */
    unsigned refcount;

    /* Scratch and page buffers. Both need to be cacheline-aligned and are
     * provided externally by the caller prior to nand_open().
     *
     * - The scratch buffer is NAND_DRV_SCRATCHSIZE bytes long and is used
     *   for small data transfers associated with commands. It must not be
     *   disturbed while any NAND operation is in progress.
     *
     * - The page buffer is used by certain functions like nand_read_bytes(),
     *   but it's main purpose is to provide a common temporary buffer for
     *   driver users to perform I/O with. Must be fpage_size bytes long.
     */
    uint8_t* scratch_buf;
    uint8_t* page_buf;

    /* Pointer to the chip data. */
    const nand_chip* chip;

    /* Pages per block = 1 << chip->log2_ppb */
    unsigned ppb;

    /* Full page size = chip->page_size + chip->oob_size */
    unsigned fpage_size;

    /* Probed mf_id / dev_id for debugging, in case identification fails. */
    uint8_t mf_id;
    uint8_t dev_id;

    /* SFC commands used for I/O, these are set based on chip data */
    uint32_t cmd_page_read;
    uint32_t cmd_read_cache;
    uint32_t cmd_program_load;
    uint32_t cmd_program_execute;
    uint32_t cmd_block_erase;
} nand_drv;

extern const nand_chip supported_nand_chips[];
extern const size_t nr_supported_nand_chips;

/* Return the static NAND driver instance.
 *
 * ALL normal Rockbox code should use this instance. The SPL does not
 * use it, because it needs to manually place buffers in external RAM.
 */
extern nand_drv* nand_init(void);

static inline void nand_lock(nand_drv* drv)
{
    mutex_lock(&drv->mutex);
}

static inline void nand_unlock(nand_drv* drv)
{
    mutex_unlock(&drv->mutex);
}

/* Open or close the NAND driver
 *
 * The NAND driver is reference counted, and opening / closing it will
 * increment and decrement the reference count. The hardware is only
 * controlled when the reference count rises above or falls to 0, else
 * these functions are no-ops which always succeed.
 *
 * These functions require the lock to be held.
 */
extern int nand_open(nand_drv* drv);
extern void nand_close(nand_drv* drv);

/* Read / program / erase operations. Buffer needs to be cache-aligned for DMA.
 * Read and program operate on full page data, ie. including OOB data areas.
 *
 * NOTE: ECC is not implemented. If it ever needs to be, these functions will
 * probably use ECC transparently. All code should be written to expect this.
 */
extern int nand_block_erase(nand_drv* drv, nand_block_t block);
extern int nand_page_program(nand_drv* drv, nand_page_t page, const void* buffer);
extern int nand_page_read(nand_drv* drv, nand_page_t page, void* buffer);

/* Wrappers to read/write bytes. For simple access to the main data area only.
 * The write address / length must align to a block boundary. Reads do not have
 * any alignment requirement. OOB data is never read, and is written as 0xff.
 */
extern int nand_read_bytes(nand_drv* drv, uint32_t byte_addr, uint32_t byte_len, void* buffer);
extern int nand_write_bytes(nand_drv* drv, uint32_t byte_addr, uint32_t byte_len, const void* buffer);

#endif /* __NAND_X1000_H__ */
