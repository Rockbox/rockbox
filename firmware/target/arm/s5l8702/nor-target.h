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
 * IM3
 */
#define IM3_IDENT       "8702"
#define IM3_VERSION     "1.0"
#define IM3HDR_SZ       0x800
#define IM3INFO_SZ      (sizeof(struct Im3Info))
#define IM3INFOSIGN_SZ  (offsetof(struct Im3Info, info_sign))

#define SIGN_SZ     16

struct Im3Info
{
    uint8_t ident[4];
    uint8_t version[3];
    uint8_t enc_type;
    uint8_t entry[4];   /* LE */
    uint8_t data_sz[4]; /* LE */
    union {
        struct {
            uint8_t data_sign[SIGN_SZ];
            uint8_t _reserved[32];
        } enc12;
        struct {
            uint8_t sign_off[4]; /* LE */
            uint8_t cert_off[4]; /* LE */
            uint8_t cert_sz[4];  /* LE */
            uint8_t _reserved[36];
        } enc34;
    } u;
    uint8_t info_sign[SIGN_SZ];
} __attribute__ ((packed));

struct Im3Hdr
{
    struct Im3Info info;
    uint8_t _zero[IM3HDR_SZ - sizeof(struct Im3Info)];
} __attribute__ ((packed));

unsigned im3_nor_sz(struct Im3Info* hinfo);
void im3_sign(uint32_t keyidx, void* data, uint32_t size, void* sign);
void im3_crypt(enum hwkeyaes_direction direction,
                            struct Im3Info *hinfo, void *fw_addr);
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
