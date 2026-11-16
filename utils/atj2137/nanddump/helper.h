/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Marcin Bukat
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
#ifndef NANDDUMP_HELPER_H
#define NANDDUMP_HELPER_H

/* helper is loaded to SDRAM (uncached) and called through hwstub exec,
 * host fills parameter block, calls entry and reads back results */
#define HELPER_BASE     0xa0080000
#define HELPER_PARAMS   HELPER_BASE
#define HELPER_ENTRY    (HELPER_BASE + 0x40)
#define HELPER_MAX_SIZE 0x8000

#define HELPER_BUF      0xa0100000
#define HELPER_BUF_SIZE 0x400000

#define HELPER_MAGIC    0x444e4141 /* 'AAND' */

#define OP_INIT         1   /* clocks, reset, timing, controller setup */
#define OP_READID       2   /* id of params.ce -> buf[0..3] */
#define OP_READ         3   /* raw read npages starting at page */

#define ST_OK           0
#define ST_BADMAGIC     1
#define ST_BADOP        2
#define ST_TIMEOUT_FSM  3
#define ST_TIMEOUT_XFER 4
#define ST_TIMEOUT_DMA  5
#define ST_BADPARAM     6
#define ST_TIMEOUT_RB   7

#ifndef __ASSEMBLER__
#include <stdint.h>

struct helper_params_t
{
    uint32_t magic;
    uint32_t op;
    uint32_t ce;
    uint32_t page;       /* first page (row address) */
    uint32_t npages;
    uint32_t page_bytes; /* bytes read per page (main + spare) */
    uint32_t chunk;      /* bytes per DMA transfer */
    uint32_t nandclk;    /* CMU_NANDCLK value for OP_INIT */
    uint32_t fifotim;    /* NAND_FIFOTIM value for OP_INIT */
    uint32_t status;     /* out: ST_* */
    uint32_t done;       /* out: pages read */
    uint32_t dbg;        /* out: NAND_STATUS on error */
    uint32_t eccmode;    /* NAND_ECCMODE value used for reads */
    uint32_t load_delay; /* us to wait after 30h before polling */
    uint32_t reserved[2];
};
#endif

#endif
