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

/* The "Scheme A" flash translation layer of rk27xx devices - the on-flash
 * format the original firmware of many rk2705/rk2706 players uses, read and
 * written compatibly so that the original firmware keeps working on the same
 * media. See ftl-scheme-a.c for how it works. */

#ifndef __FTL_SCHEME_A_H__
#define __FTL_SCHEME_A_H__

#include <stdbool.h>
#include <stdint.h>

/* The two volumes. SYS holds the original firmware and its resources; USER
 * is the music storage. */
#define FTL_A_VOL_SYS   0
#define FTL_A_VOL_USER  1

struct ftl_a_config
{
    /* Size of the SYS volume. The one layout value the FTL cannot read from
     * its own tables: it is recorded in the boot area's ID block. */
    uint32_t sys_sectors;
    /* Never write, not even the repairs a mount normally makes. */
    bool     read_only;
    /* A format flag accepted besides the standard 2 (0 = none), and whether
     * media carrying it may be written. See ftl_a_mount(). */
    uint8_t  alt_format_flag;
    bool     alt_format_writable;
};

/* Why a mount failed */
enum ftl_a_error
{
    FTL_A_OK = 0,
    FTL_A_ERR_LAYOUT,           /* the zone layout could not be recognised */
    FTL_A_ERR_NO_LOG_BLOCK,     /* a zone has no remap log: damaged */
    FTL_A_ERR_ZERO_CAPACITY,    /* the block counts never loaded */
    FTL_A_ERR_COUNTS_ERASED,    /* ... or were read from an erased page */
    FTL_A_ERR_SYS_TOO_BIG,      /* SYS larger than the chip */
    FTL_A_ERR_FORMAT_FLAG,      /* not formatted, or an unknown generation */
    FTL_A_ERR_NEEDS_REPAIR,     /* a log needs rewriting; not while read-only */
};

/* How sure the mount is of the layout it recognised */
enum ftl_a_confidence
{
    FTL_A_CONF_NONE = 0,
    FTL_A_CONF_CONFIRMED,       /* derived and cross-checked: writable */
    FTL_A_CONF_ASSUMED,         /* partly assumed: read-only */
};

struct ftl_a_status
{
    enum ftl_a_error      error;
    enum ftl_a_confidence confidence;
    bool     writable;
    uint8_t  reserve_base;      /* first reserve-pool entry of a zone table */
    uint8_t  sys_zone;          /* zone holding the boot area */
    uint8_t  sys_offset;        /* boot-area blocks at the start of it */
    uint8_t  base_fits;         /* reserve bases that fit the media; 1 = sure */
    uint8_t  format_flag;       /* as read from the media */
};

/* Mount the media. Returns FTL_A_OK or the reason it failed. Even a
 * successful mount may be read-only - see ftl_a_get_status(). */
enum ftl_a_error ftl_a_mount(const struct ftl_a_config *config);
void ftl_a_get_status(struct ftl_a_status *status);

/* Sectors on a volume; 0 when not mounted */
uint32_t ftl_a_capacity(int volume);

/* Return 0, or non-zero if the request was refused or a sector could not be
 * read reliably. Sectors are 512 bytes. */
int ftl_a_read(int volume, uint32_t sector, void *buf, uint32_t count);
int ftl_a_write(int volume, uint32_t sector, const void *buf, uint32_t count);

/* Write out everything still held in RAM */
void ftl_a_sync(void);

#endif /* __FTL_SCHEME_A_H__ */
