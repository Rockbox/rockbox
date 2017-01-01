/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 Amaury Pouly
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
#ifndef __SB1_H__
#define __SB1_H__

#include <stdint.h>
#include <stdbool.h>

#include "misc.h"

#define SECTOR_SIZE      512

/* All fields are in big-endian BCD */
struct sb1_version_t
{
    uint16_t major;
    uint16_t pad0;
    uint16_t minor;
    uint16_t pad1;
    uint16_t revision;
    uint16_t pad2;
};

struct sb1_header_t
{
    uint32_t rom_version;
    uint32_t image_size;
    uint32_t header_size;
    uint32_t userdata_offset;
    uint32_t pad2;
    uint8_t signature[4]; /* Signature "STMP" */
    struct sb1_version_t product_ver;
    struct sb1_version_t component_ver;
    uint32_t drive_tag;
} __attribute__((packed));

struct sb1_cmd_header_t
{
    uint32_t cmd; // 31:21=cmd size, 20=critical, 19:6=size 5:4=datatype, 3:0=boot cmd
    uint32_t addr;
} __attribute__((packed));

#define SB1_CMD_MAX_LOAD_SIZE   0x1ff8
#define SB1_CMD_MAX_FILL_SIZE   0x3fff

#define SB1_CMD_SIZE(cmd)       ((cmd) >> 21)
#define SB1_CMD_CRITICAL(cmd)   !!(cmd & (1 << 20))
#define SB1_CMD_BYTES(cmd)      (((cmd) >> 6) & 0x3fff)
#define SB1_CMD_DATATYPE(cmd)   (((cmd) >> 4) & 0x3)
#define SB1_CMD_BOOT(cmd)       ((cmd) & 0xf)

#define SB1_MK_CMD(boot,data,bytes,crit,size) \
    ((boot) | (data) << 4 | (bytes) << 6 | (crit) << 20 | (size) << 21)

#define SB1_ADDR_SDRAM_CS(addr) ((addr) & 0x3)
#define SB1_ADDR_SDRAM_SZ(addr) ((addr) >> 16)

#define SB1_MK_ADDR_SDRAM(cs,sz)    ((cs) | (sz) << 16)

int sb1_sdram_size_by_index(int index); // returns - 1 on error
int sb1_sdram_index_by_size(int size); // returns -1 on error

#define SB1_INST_LOAD   0x1
#define SB1_INST_FILL   0x2
#define SB1_INST_JUMP   0x3
#define SB1_INST_CALL   0x4
#define SB1_INST_MODE   0x5
#define SB1_INST_SDRAM  0x6

#define SB1_DATATYPE_UINT32 0
#define SB1_DATATYPE_UINT16 1
#define SB1_DATATYPE_UINT8  2

/*******
 * API *
 *******/

struct sb1_inst_t
{
    uint8_t cmd;
    uint16_t size;
    // <union>
    struct
    {
        uint8_t chip_select;
        uint8_t size_index;
    }sdram;
    uint8_t mode;
    uint32_t addr;
    // </union>
    uint8_t datatype;
    uint8_t critical;
    // <union>
    void *data;
    uint32_t pattern;
    uint32_t argument;
    // </union>
};

struct sb1_file_t
{
    uint32_t rom_version;
    uint32_t pad2; // unknown meaning but copy it anyway !
    uint32_t drive_tag;
    struct sb1_version_t product_ver;
    struct sb1_version_t component_ver;
    int nr_insts;
    struct sb1_inst_t *insts;
    void *userdata;
    int userdata_size;
    struct crypto_key_t key;
};

enum sb1_error_t
{
    SB1_SUCCESS = 0,
    SB1_ERROR = -1,
    SB1_OPEN_ERROR = -2,
    SB1_READ_ERROR = -3,
    SB1_WRITE_ERROR = -4,
    SB1_FORMAT_ERROR = -5,
    SB1_CHECKSUM_ERROR = -6,
    SB1_NO_VALID_KEY = -7,
    SB1_CRYPTO_ERROR = -8,
};

enum sb1_error_t sb1_write_file(struct sb1_file_t *sb, const char *filename);

struct sb1_file_t *sb1_read_file(const char *filename, void *u,
    generic_printf_t printf, enum sb1_error_t *err);
/* use size_t(-1) to use maximum size */
struct sb1_file_t *sb1_read_file_ex(const char *filename, size_t offset, size_t size,
    void *u, generic_printf_t printf, enum sb1_error_t *err);
struct sb1_file_t *sb1_read_memory(void *buffer, size_t size, void *u,
    generic_printf_t printf, enum sb1_error_t *err);

/* do as little checks as possible, make sure the image is valid (advance use only).
 * Buffer should be of size SECTOR_SIZE at least. */
bool sb1_is_key_valid_fast(void *buffer, union xorcrypt_key_t key[2]);
bool sb1_brute_force(const char *filename, void *u, generic_printf_t printf,
    enum sb1_error_t *err, struct crypto_key_t *key);

void sb1_get_default_key(struct crypto_key_t *key);

void sb1_dump(struct sb1_file_t *file, void *u, generic_printf_t printf);
void sb1_free(struct sb1_file_t *file);

#endif /* __SB1_H__ */

