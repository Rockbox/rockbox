/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Amaury Pouly
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
#ifndef __SB_H__
#define __SB_H__

#include <stdint.h>
#include <stdbool.h>

#include "misc.h"

#define BLOCK_SIZE      16

/* All fields are in big-endian BCD */
struct sb_version_t
{
    uint16_t major;
    uint16_t pad0;
    uint16_t minor;
    uint16_t pad1;
    uint16_t revision;
    uint16_t pad2;
};

struct sb_header_t
{
    uint8_t sha1_header[20]; /* SHA-1 of the rest of the header */
    uint8_t signature[4]; /* Signature "STMP" */
    uint8_t major_ver; /* Should be 1 */
    uint8_t minor_ver; /* Should be 1 */
    uint16_t flags;
    uint32_t image_size; /* In blocks (=16bytes) */
    uint32_t first_boot_tag_off; /* Offset in blocks */
    uint32_t first_boot_sec_id; /* First bootable section ID */
    uint16_t nr_keys; /* Number of encryption keys */
    uint16_t key_dict_off; /* Offset to key dictionary (in blocks) */
    uint16_t header_size; /* In blocks */
    uint16_t nr_sections; /* Number of sections */
    uint16_t sec_hdr_size; /* Section header size (in blocks) */
    uint8_t rand_pad0[6]; /* Random padding */
    uint64_t timestamp; /* In microseconds since 2000/1/1 00:00:00 */
    struct sb_version_t product_ver;
    struct sb_version_t component_ver;
    uint16_t drive_tag; /* first tag to boot ? */
    uint8_t rand_pad1[6]; /* Random padding */
} __attribute__((packed));

struct sb_section_header_t
{
    uint32_t identifier;
    uint32_t offset; /* In blocks */
    uint32_t size; /* In blocks */
    uint32_t flags;
} __attribute__((packed));

struct sb_key_dictionary_entry_t
{
    uint8_t hdr_cbc_mac[16]; /* CBC-MAC of the header */
    uint8_t key[16]; /* Actual AES Key (encrypted by the global key) */
} __attribute__((packed));

#define IMAGE_MAJOR_VERSION     1
#define IMAGE_MINOR_VERSION     1

#define SECTION_BOOTABLE        (1 << 0)
#define SECTION_CLEARTEXT       (1 << 1)

#define SB_INST_NOP     0x0
#define SB_INST_TAG     0x1
#define SB_INST_LOAD    0x2
#define SB_INST_FILL    0x3
#define SB_INST_JUMP    0x4
#define SB_INST_CALL    0x5
#define SB_INST_MODE    0x6

/* flags */
#define SB_INST_LAST_TAG    1 /* for TAG */
#define SB_INST_LOAD_DCD    1 /* for LOAD */
#define SB_INST_FILL_BYTE   0 /* for FILL */
#define SB_INST_FILL_HWORD  1 /* for FILL */
#define SB_INST_FILL_WORD   2 /* for FILL */
#define SB_INST_HAB_EXEC    1 /* for JUMP/CALL */

struct sb_instruction_header_t
{
    uint8_t checksum;
    uint8_t opcode;
    uint16_t flags;
} __attribute__((packed));

struct sb_instruction_common_t
{
    struct sb_instruction_header_t hdr;
    uint32_t addr;
    uint32_t len;
    uint32_t data;
} __attribute__((packed));

struct sb_instruction_load_t
{
    struct sb_instruction_header_t hdr;
    uint32_t addr;
    uint32_t len;
    uint32_t crc;
} __attribute__((packed));

struct sb_instruction_fill_t
{
    struct sb_instruction_header_t hdr;
    uint32_t addr;
    uint32_t len;
    uint32_t pattern;
} __attribute__((packed));

struct sb_instruction_mode_t
{
    struct sb_instruction_header_t hdr;
    uint32_t zero1;
    uint32_t zero2;
    uint32_t mode;
} __attribute__((packed));

struct sb_instruction_call_t
{
    struct sb_instruction_header_t hdr;
    uint32_t addr;
    uint32_t zero;
    uint32_t arg;
} __attribute__((packed));

struct sb_instruction_tag_t
{
    struct sb_instruction_header_t hdr;
    uint32_t identifier; /* section identifier */
    uint32_t len; /* length of the section */
    uint32_t flags; /* section flags */
} __attribute__((packed));

/*******
 * API *
 *******/

#define SB_INST_DATA    0xff

struct sb_inst_t
{
    uint8_t inst; /* SB_INST_* */
    uint32_t size;
    uint32_t addr;
    // <union>
    void *data;
    uint32_t pattern;
    // </union>
    uint32_t argument; // for call, jump and mode
    /* for production use */
    uint32_t padding_size;
    uint8_t *padding;
};

struct sb_section_t
{
    uint32_t identifier;
    bool is_data;
    bool is_cleartext;
    uint32_t alignment;
    // data sections are handled as one or more SB_INST_DATA virtual instruction 
    int nr_insts;
    struct sb_inst_t *insts;
    /* for production use */
    uint32_t file_offset; /* in blocks */
    uint32_t sec_size; /* in blocks */
};

struct sb_file_t
{
    /* override real, otherwise it is randomly generated */
    bool override_real_key;
    uint8_t real_key[16];
    /* override crypto IV, use with caution ! Use NULL to generate it */
    bool override_crypto_iv;
    uint8_t crypto_iv[16];
    
    int nr_sections;
    uint16_t drive_tag;
    uint32_t first_boot_sec_id;
    uint16_t flags;
    uint8_t minor_version;
    struct sb_section_t *sections;
    struct sb_version_t product_ver;
    struct sb_version_t component_ver;
    /* for production use */
    uint32_t image_size; /* in blocks */
};

enum sb_error_t
{
    SB_SUCCESS = 0,
    SB_ERROR = -1,
    SB_OPEN_ERROR = -2,
    SB_READ_ERROR = -3,
    SB_WRITE_ERROR = -4,
    SB_FORMAT_ERROR = -5,
    SB_CHECKSUM_ERROR = -6,
    SB_NO_VALID_KEY = -7,
    SB_FIRST_CRYPTO_ERROR = -8,
    SB_LAST_CRYPTO_ERROR = SB_FIRST_CRYPTO_ERROR - CRYPTO_NUM_ERRORS,
};

enum sb_error_t sb_write_file(struct sb_file_t *sb, const char *filename);

typedef void (*sb_color_printf)(void *u, bool err, color_t c, const char *f, ...);
struct sb_file_t *sb_read_file(const char *filename, bool raw_mode, void *u,
    sb_color_printf printf, enum sb_error_t *err);
/* use size_t(-1) to use maximum size */
struct sb_file_t *sb_read_file_ex(const char *filename, size_t offset, size_t size, bool raw_mode, void *u,
    sb_color_printf printf, enum sb_error_t *err);
struct sb_file_t *sb_read_memory(void *buffer, size_t size, bool raw_mode, void *u,
    sb_color_printf printf, enum sb_error_t *err);

void sb_fill_section_name(char name[5], uint32_t identifier);
void sb_dump(struct sb_file_t *file, void *u, sb_color_printf printf);
void sb_free_instruction(struct sb_inst_t inst);
void sb_free_section(struct sb_section_t file);
void sb_free(struct sb_file_t *file);

#endif /* __SB_H__ */
