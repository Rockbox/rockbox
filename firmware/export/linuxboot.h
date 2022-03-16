/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2022 Aidan MacDonald
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

#ifndef __LINUXBOOT_H__
#define __LINUXBOOT_H__

#include "system.h"
#include <stddef.h>
#include <sys/types.h>

/*
 * From u-boot's include/image.h
 * SPDX-License-Identifier: GPL-2.0+
 * (C) Copyright 2008 Semihalf
 * (C) Copyright 2000-2005
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#define IH_MAGIC 0x27051956
#define IH_NMLEN 32

enum
{
    IH_ARCH_INVALID,
    IH_ARCH_ALPHA,
    IH_ARCH_ARM,
    IH_ARCH_I386,
    IH_ARCH_IA64,
    IH_ARCH_MIPS,
    IH_ARCH_MIPS64,
    IH_ARCH_PPC,
    IH_ARCH_S390,
    IH_ARCH_SH,
    IH_ARCH_SPARC,
    IH_ARCH_SPARC64,
    IH_ARCH_M68K,
    /* NOTE: Other archs not relevant and omitted here, can add if needed */
    IH_ARCH_COUNT,
};

enum
{
    IH_TYPE_INVALID = 0,
    IN_TYPE_STANDALONE,
    IH_TYPE_KERNEL,
    IH_TYPE_RAMDISK,
    IH_TYPE_MULTI,
    IH_TYPE_FIRMWARE,
    IH_TYPE_SCRIPT,
    IH_TYPE_FILESYSTEM,
    IH_TYPE_FLATDT,
    /* NOTE: Other types not relevant and omitted here, can add if needed */
    IH_TYPE_COUNT,
};

enum
{
    IH_COMP_NONE = 0,
    IH_COMP_GZIP,
    IH_COMP_BZIP2,
    IH_COMP_LZMA,
    IH_COMP_LZO,
    IH_COMP_LZ4,
    IH_COMP_ZSTD,
    IH_COMP_COUNT,
};

/* Legacy U-Boot image header as produced by mkimage(1).
 *
 * WARNING: all fields are big-endian so you usually do not want to
 * access them directly, use the accessor functions instead.
 */
struct uimage_header
{
    uint32_t ih_magic;
    uint32_t ih_hcrc;
    uint32_t ih_time;
    uint32_t ih_size;
    uint32_t ih_load;
    uint32_t ih_ep;
    uint32_t ih_dcrc;
    uint8_t  ih_os;
    uint8_t  ih_arch;
    uint8_t  ih_type;
    uint8_t  ih_comp;
    uint8_t  ih_name[IH_NMLEN];
};

#define _uimage_get32_f(name) \
    static inline uint32_t uimage_get_##name(const struct uimage_header* uh) \
    { return betoh32(uh->ih_##name); }
_uimage_get32_f(magic)
_uimage_get32_f(hcrc)
_uimage_get32_f(time)
_uimage_get32_f(size)
_uimage_get32_f(load)
_uimage_get32_f(ep)
_uimage_get32_f(dcrc)
#undef _uimage_get32_f

#define _uimage_set32_f(name) \
    static inline void uimage_set_##name(struct uimage_header* uh, uint32_t val) \
    { uh->ih_##name = htobe32(val); }
_uimage_set32_f(magic)
_uimage_set32_f(hcrc)
_uimage_set32_f(time)
_uimage_set32_f(size)
_uimage_set32_f(load)
_uimage_set32_f(ep)
_uimage_set32_f(dcrc)
#undef _uimage_set32_f

#define _uimage_get8_f(name) \
    static inline uint8_t uimage_get_##name(const struct uimage_header* uh) \
    { return uh->ih_##name; }
_uimage_get8_f(os)
_uimage_get8_f(arch)
_uimage_get8_f(type)
_uimage_get8_f(comp)
#undef _uimage_get8_f

#define _uimage_set8_f(name) \
    static inline void uimage_set_##name(struct uimage_header* uh, uint8_t val) \
    { uh->ih_##name = val; }
_uimage_set8_f(os)
_uimage_set8_f(arch)
_uimage_set8_f(type)
_uimage_set8_f(comp)
#undef _uimage_set8_f

/*
 * uImage utilities
 */

/** Reader callback for use with `uimage_load`
 *
 * \param buf   Buffer to write into
 * \param size  Number of bytes to read
 * \param ctx   State argument
 * \return Number of bytes actually read, or -1 on error.
 */
typedef ssize_t(*uimage_reader)(void* buf, size_t size, void* ctx);

/** Calculate U-Boot style CRC */
uint32_t uimage_crc(uint32_t crc, const void* data, size_t size);

/** Calculate CRC of a uImage header */
uint32_t uimage_calc_hcrc(const struct uimage_header* uh);

/** Load and decompress a uImage
 *
 * \param uh        Returned header struct (will be filled with read data)
 * \param out_size  Returned size of the decompressed data
 * \param reader    Data reader function
 * \param rctx      Context argument for the reader function
 * \return Buflib handle containing the decompressed data, or negative on error
 *
 * This function will read a uImage, verify checksums and decompress the image
 * data into a non-moveable buflib allocation. The length of the compressed
 * data will be taken from the uImage header.
 */
int uimage_load(struct uimage_header* uh, size_t* out_size,
                uimage_reader reader, void* rctx);

/** File reader for use with `uimage_load`
 *
 * \param ctx   File descriptor, casted to `void*`
 */
ssize_t uimage_fd_reader(void* buf, size_t size, void* ctx);

/* helper for patching broken self-extracting kernels on MIPS */
uint32_t mips_linux_stub_get_entry(void** code_start, size_t code_size);

#endif /* __LINUXBOOT_H__ */
