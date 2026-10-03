/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2009 by Michael Sparmann
 *
 * Interface modelled on
 * firmware/target/arm/s5l8700/ipodnano2g/ftl-target.h.
 *
 * Copyright (C) 2026 by Andrew Rice
 * Copyright (C) 2026 by Robin Tschirschnitz
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

#ifndef __FTL_TARGET_H__
#define __FTL_TARGET_H__

#include <stdint.h>

/* Mount Apple's FTL. nand_init() must have succeeded. Returns 0, or a
 * negative number saying which stage failed. The mount is read-only, so
 * writes fail and ftl_sync() does nothing, unless the firmware is built with
 * N4G_FTL_WRITE and the context, pool and log counts agree on the medium. */
int ftl_init(void);

/* Read count logical sectors of NAND_PAGE_SIZE bytes. Returns 0 or a
 * negative number. */
int ftl_read(uint32_t sector, uint32_t count, void *buffer);

/* Write count logical sectors of NAND_PAGE_SIZE bytes. The data is on the
 * medium when this returns, but the block map that finds it is not: call
 * ftl_sync() to commit that. Returns 0 or a negative number. */
int ftl_write(uint32_t sector, uint32_t count, const void *buffer);

/* Commit the block map, so the writes above survive a remount. Returns 0 or
 * a negative number; after a failure the FTL refuses further writes. */
int ftl_sync(void);

/* Number of logical sectors, 0 if the FTL is not mounted */
uint32_t ftl_num_sectors(void);

#ifdef BOOTLOADER
/* Return one unremapped physical block belonging to an FTL free
 * superblock. Diagnostic only; does not change the free pool. */
int ftl_get_free_block(uint32_t *sb, uint32_t *bank, uint32_t *block,
                       uint32_t *freecount);
/* True only while block is an unremapped physical member of the live FTL
 * free pool on bank. */
int ftl_is_free_block(uint32_t bank, uint32_t block);

/* Read-only: how the mounted VFL treats physical block 0. poolstart is the
 * first reserved block. in_ring is set when the context ring lists block 0.
 * vblock0_remapped is set when virtual block 0 lives in the spare pool, so
 * the FTL does not use physical block 0. Returns 0, or -1 when no context
 * is mounted. */
int ftl_boot_block_reserved(uint32_t bank, uint16_t *poolstart,
                            int *in_ring, int *vblock0_remapped);
#endif

/* Read path counters for the bootloader's diagnostics: multi-page reads
 * that failed, pages the ECC could not correct, retries that failed, and
 * pages whose spare did not name the logical page (returned as 0xff) */
struct ftl_read_dbg {
    uint32_t multi_fail, ecc_fail, page_fail, stale;
    int multi_rc, page_rc;
    uint32_t lpn, meta0;
    /* how the mount went: context loaded clean, or rebuilt by restore */
    uint32_t restored, nlogs, nholes, cxtusn;
    /* 0 when the context and the medium agree, so writes could be enabled.
     * Otherwise the check that failed: 1 spare superblock count, 2 table
     * sizes, 3 free + logs over the pool, 4 a superblock used twice, 5 a
     * restore left too many holes. */
    uint32_t writecheck;
};
extern struct ftl_read_dbg ftl_read_dbg;

/* 1 when writes and ftl_sync() are allowed, 0 when the mount stayed
 * read-only or the FTL is not open. */
int ftl_is_writable(void);

#if defined(BOOTLOADER) && defined(N4G_BL_FTL_WRITE)
/* Turn writes on after the user has confirmed. When the mount was rebuilt,
 * erase the hole blocks and the free pool, then commit. Returns 0, or a
 * negative writecheck or commit error. The mount stays read-only on failure. */
int ftl_arm_writes(void);
#endif

/* Not implemented on the Nano 4G. Returns -1. */
int ftl_format(void);

#endif /* __FTL_TARGET_H__ */
