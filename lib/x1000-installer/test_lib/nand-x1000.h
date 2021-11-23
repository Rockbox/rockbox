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

/* Stripped down fake version of X1000 NAND API for testing purposes,
 * uses a normal file to store the data */

#ifndef __NAND_X1000_H__
#define __NAND_X1000_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define NAND_SUCCESS            0
#define NAND_ERR_UNKNOWN_CHIP   (-1)
#define NAND_ERR_PROGRAM_FAIL   (-2)
#define NAND_ERR_ERASE_FAIL     (-3)
#define NAND_ERR_UNALIGNED      (-4)
#define NAND_ERR_OTHER          (-5)
#define NAND_ERR_INJECTED       (-6)

/* keep max page size in sync with the NAND chip table in the .c file */
#define NAND_DRV_SCRATCHSIZE 32
#define NAND_DRV_MAXPAGESIZE 2112

typedef uint32_t nand_block_t;
typedef uint32_t nand_page_t;

enum nand_trace_type {
    NTT_READ,
    NTT_PROGRAM,
    NTT_ERASE,
};

enum nand_trace_exception {
    NTE_NONE,
    NTE_DOUBLE_PROGRAMMED,
    NTE_CLEARED,
};

struct nand_trace {
    enum nand_trace_type type;
    enum nand_trace_exception exception;
    nand_page_t addr;
};

typedef struct nand_chip {
    /* Base2 logarithm of the number of pages per block */
    unsigned log2_ppb;

    /* Size of a page's main / oob areas, in bytes. */
    unsigned page_size;
    unsigned oob_size;

    /* Total number of blocks in the chip */
    unsigned nr_blocks;
} nand_chip;

typedef struct nand_drv {
    /* Backing file */
    int fd;
    int metafd;
    int lock_count;

    unsigned refcount;
    uint8_t* scratch_buf;
    uint8_t* page_buf;
    const nand_chip* chip;
    unsigned ppb;
    unsigned fpage_size;
} nand_drv;

extern const char* nand_backing_file;
extern const char* nand_meta_file;

extern struct nand_trace* nand_trace;
extern size_t nand_trace_capacity;
extern size_t nand_trace_length;

extern void nand_trace_reset(size_t size);
extern void nand_inject_error(int rc);

extern nand_drv* nand_init(void);

extern void nand_lock(nand_drv* drv);
extern void nand_unlock(nand_drv* drv);

extern int nand_open(nand_drv* drv);
extern void nand_close(nand_drv* drv);
extern int nand_block_erase(nand_drv* drv, nand_block_t block);
extern int nand_page_program(nand_drv* drv, nand_page_t page, const void* buffer);
extern int nand_page_read(nand_drv* drv, nand_page_t page, void* buffer);

#endif /* __NAND_X1000_H__ */
