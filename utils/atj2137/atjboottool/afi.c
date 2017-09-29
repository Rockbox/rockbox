/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 Amaury Pouly
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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "misc.h"
#include "afi.h"

/** part of this work comes from s1mp3/s1fwx */

#define AFI_ENTRIES     126
#define AFI_SIG_SIZE    4

struct afi_hdr_t
{
    uint8_t sig[AFI_SIG_SIZE];
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t ver_id[2];
    uint8_t ext_ver_id[2];
    uint8_t year[2];
    uint8_t month;
    uint8_t day;
    uint32_t afi_size;
    uint32_t res[3];
} __attribute__((packed));

struct afi_entry_t
{
    char name[8];
    char ext[3];
    char type;
    uint32_t addr;
    uint32_t offset;
    uint32_t size;
    char desc[4];
    uint32_t checksum;
} __attribute__((packed));

struct afi_post_hdr_t
{
    uint8_t res[28];
    uint32_t checksum;
} __attribute__((packed));

struct afi_t
{
    struct afi_hdr_t hdr;
    struct afi_entry_t entry[AFI_ENTRIES];
    struct afi_post_hdr_t post;
};

#define AFI_ENTRY_BREC  'B'
#define AFI_ENTRY_FWSC  'F'
#define AFI_ENTRY_ADFUS 'A'
#define AFI_ENTRY_FW    'I'

#define AFI_ENTRY_DLADR_BREC  0x00000006    // 'B'
#define AFI_ENTRY_DLADR_FWSC  0x00020008    // 'F'
#define AFI_ENTRY_DLADR_ADFUS 0x000C0008    // 'A'
#define AFI_ENTRY_DLADR_ADFU  0x00000000    // 'U'
#define AFI_ENTRY_DLADR_FW    0x00000011    // 'I'

const uint8_t g_afi_signature[AFI_SIG_SIZE] =
{
    'A', 'F', 'I', 0
};

uint32_t afi_checksum(void *ptr, size_t size)
{
    uint32_t crc = 0;
    uint32_t *cp = ptr;
    for(; size >= 4; size -= 4)
        crc += *cp++;
    if(size == 1)
        crc += *(uint8_t *)cp;
    else if(size == 2)
        crc += *(uint16_t *)cp;
    else if(size == 3)
        crc += *(uint16_t *)cp + ((*(uint8_t *)(cp + 2)) << 16);
    return crc;
}

static void build_filename(char buf[16], struct afi_entry_t *ent)
{
    int pos = 0;
    for(int i = 0; i < 8 && ent->name[i] != ' '; i++)
        buf[pos++] = ent->name[i];
    buf[pos++] = '.';
    for(int i = 0; i < 3 && ent->ext[i] != ' '; i++)
        buf[pos++] = ent->ext[i];
    buf[pos] = 0;
}

int afi_unpack(uint8_t *buf, size_t size, afi_extract_callback_t unpack_cb)
{
    struct afi_t *afi = (void *)buf;

    if(size < sizeof(struct afi_t))
    {
        cprintf(GREY, "File too small\n");
        return 1;
    }
    cprintf(BLUE, "Header\n");
    cprintf(GREEN, "  Signature:");
    for(int i = 0; i < AFI_SIG_SIZE; i++)
        cprintf(YELLOW, " %02x", afi->hdr.sig[i]);
    if(memcmp(afi->hdr.sig, g_afi_signature, AFI_SIG_SIZE) == 0)
        cprintf(RED, " Ok\n");
    else
    {
        cprintf(RED, " Mismatch\n");
        return 1;
    }

    cprintf_field("  Vendor ID: ", "0x%x\n", afi->hdr.vendor_id);
    cprintf_field("  Product ID: ", "0x%x\n", afi->hdr.product_id);
    cprintf_field("  Version: ", "%x.%x\n", afi->hdr.ver_id[0], afi->hdr.ver_id[1]);
    cprintf_field("  Ext Version: ", "%x.%x\n", afi->hdr.ext_ver_id[0],
        afi->hdr.ext_ver_id[1]);
    cprintf_field("  Date: ", "%02x/%02x/%02x%02x\n", afi->hdr.day, afi->hdr.month,
        afi->hdr.year[0], afi->hdr.year[1]);

    cprintf_field("  AFI size: ", "%d ", afi->hdr.afi_size);
    if(afi->hdr.afi_size == size)
        cprintf(RED, " Ok\n");
    else if(afi->hdr.afi_size < size)
        cprintf(RED, " Ok (file greater than archive)\n");
    else
    {
        cprintf(RED, " Error (file too small)\n");
        return 1;
    }

    cprintf_field("  Reserved: ", "%x %x %x\n", afi->hdr.res[0],
        afi->hdr.res[1], afi->hdr.res[2]);

    cprintf(BLUE, "Entries\n");
    for(int i = 0; i < AFI_ENTRIES; i++)
    {
        if(afi->entry[i].name[0] == 0)
            continue;
        struct afi_entry_t *entry = &afi->entry[i];
        char filename[16];
        build_filename(filename, entry);
        cprintf(RED, "  %s\n", filename);
        cprintf_field("    Type: ", "%02x", entry->type);
        if(isprint(entry->type))
            cprintf(RED, " %c", entry->type);
        printf("\n");
        cprintf_field("    Addr: ", "0x%x\n", entry->addr);
        cprintf_field("    Offset: ", "0x%x\n", entry->offset);
        cprintf_field("    Size: ", "0x%x\n", entry->size);
        cprintf_field("    Desc: ", "%.4s\n", entry->desc);
        cprintf_field("    Checksum: ", "0x%x ", entry->checksum);
        uint32_t chk = afi_checksum(buf + entry->offset, entry->size);
        if(chk != entry->checksum)
        {
            cprintf(RED, "Mismatch\n");
            return 1;
        }
        else
            cprintf(RED, "Ok\n");
        int ret = unpack_cb(filename, buf + entry->offset, entry->size);
        if(ret != 0)
            return ret;
    }

    cprintf(BLUE, "Post Header\n");
    cprintf_field("  Checksum: ", "%x ", afi->post.checksum);
    uint32_t chk = afi_checksum(buf, sizeof(struct afi_t) - 4);
    if(chk != afi->post.checksum)
    {
        cprintf(RED, "Mismatch\n");
        return 1;
    }
    else
        cprintf(RED, "Ok\n");

    return 0;
}

bool afi_check(uint8_t *buf, size_t size)
{
    struct afi_hdr_t *hdr = (void *)buf;

    if(size < sizeof(struct afi_hdr_t))
        return false;
    return memcmp(hdr->sig, g_afi_signature, AFI_SIG_SIZE) == 0;
}
