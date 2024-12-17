/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:
 *
 * Copyright © 2009 Michael Sparmann
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
#ifndef __NOR_TARGET_H__
#define __NOR_TARGET_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#include "config.h"
#include "crypto-s5l8702.h"


/* NOR memory map (Classic 6G):
 *
 *         1MB  ______________
 *             |              |
 *             |   flsh DIR   |
 *   1MB-0x200 |______________|
 *             |              |
 *             |    File 1    |
 *             |..............|
 *             |              |
 *             |    File 2    |
 *             |..............|
 *             |              |
 *             .              .
 *             .              .
 *             .              .
 *             |              |
 *             |..............|
 *             |              |
 *             |    File N    |
 *             |______________|
 *             |              |
 *             |              |
 *             |              |
 *             |              |                  IM3 (128KB)
 *             |    Unused    |     . . . . . .  ______________
 *             |              |   /             |              |
 *             |              |  /              |              |
 *             |              | /               |              |
 *       160KB |______________|/                |              |
 *             |              |                 |              |
 *             |              |                 |      EFI     |
 *             |   Original   |                 |    Volumen   |
 *             |   NOR Boot   |                 |              |
 *             |              |                 |              |
 *             |              |                 |              |
 *        32KB |______________|                 |              |
 *             |              |\          0x800 |______________|
 *             |              | \               |              |
 *             |              |  \              |  IM3 Header  |
 *             |______________|   \ . . . . . . |______________|
 *             |              |
 *             |    SysCfg    |
 *           0 |______________|
 */

#define SPI_PORT    0

#define NOR_SZ          0x100000
#define NORBOOT_OFF     0x8000
#define NORBOOT_MAXSZ   0x20000

void bootflash_init(int port);
void bootflash_read(int port, uint32_t addr, uint32_t size, void* buf);
void bootflash_write(int port, int offset, void* addr, int size);
int bootflash_compare(int port, int offset, void* addr, int size);
void bootflash_erase_blocks(int port, int first, int n);
void bootflash_close(int port);

/*
 * SysCfg
 */
#define SYSCFG_MAX_ENTRIES 9 // 9 on iPod Classic/6G

struct SysCfgHeader {
    uint32_t magic; // always 'SCfg'
    uint32_t size;
    uint32_t unknown1; // 0x00000200 on iPod classic
    uint32_t version; // maybe? 0x00010001 on iPod classic
    uint32_t unknown2; // 0x00000000 on iPod classic
    uint32_t num_entries;
}; // 0x18

struct SysCfgEntry {
    uint32_t tag;
    uint8_t data[0x10];
};

struct SysCfg {
    struct SysCfgHeader header;
    struct SysCfgEntry entries[SYSCFG_MAX_ENTRIES];
};

#define SYSCFG_MAGIC 0x53436667 // SCfg

#define SYSCFG_TAG_SRNM 0x53724e6d // SrNm
#define SYSCFG_TAG_FWID 0x46774964 // FwId
#define SYSCFG_TAG_HWID 0x48774964 // HwId
#define SYSCFG_TAG_HWVR 0x48775672 // HwVr
#define SYSCFG_TAG_CODC 0x436f6463 // Codc
#define SYSCFG_TAG_SWVR 0x53775672 // SwVr
#define SYSCFG_TAG_MLBN 0x4d4c424e // MLBN
#define SYSCFG_TAG_MODN 0x4d6f6423 // Mod#
#define SYSCFG_TAG_REGN 0x5265676e // Regn

ssize_t syscfg_read(struct SysCfg* syscfg);

/*
 * IM3
 */
unsigned im3_nor_sz(struct Im3Info* hinfo);
int im3_read(uint32_t offset, struct Im3Info *hinfo, void *fw_addr);
bool im3_write(int offset, void *im3_addr);


/*
 * flsh FS
 */
#define DIR_SZ      0x200
#define DIR_OFF     (NOR_SZ - DIR_SZ)
#define ENTRY_SZ    ((int)sizeof(image_t))
#define MAX_ENTRY   (DIR_SZ / ENTRY_SZ)

#define FILEHDR_SZ  0x200

/* from tools/ipod_fw.c */
typedef struct _image {
    char type[4];           /* '' */
    unsigned id;            /* */
    char pad1[4];           /* 0000 0000 */
    unsigned devOffset;     /* byte offset of start of image code */
    unsigned len;           /* length in bytes of image */
    unsigned addr;          /* load address */
    unsigned entryOffset;   /* execution start within image */
    unsigned chksum;        /* checksum for image */
    unsigned vers;          /* image version */
    unsigned loadAddr;      /* load address for image */
} image_t;

int flsh_get_info(int *used, int *unused);
unsigned flsh_get_unused(void);
int flsh_find_file(char *name, image_t *entry);
int flsh_load_file(image_t *entry, void *hdr, void *data);

#endif /* __NOR_TARGET_H__ */
