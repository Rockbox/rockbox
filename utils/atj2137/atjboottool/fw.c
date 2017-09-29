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
#include "fw.h"
#include "afi.h"

#define FW_SIG_SIZE 4

#define FW_ENTRIES  240

struct fw_entry_t
{
    char name[8];
    char ext[3];
    uint8_t attr;
    uint8_t res[2];
    uint16_t version;
    uint32_t block_offset; // offset shift by 9
    uint32_t size;
    uint32_t unk;
    uint32_t checksum;
} __attribute__((packed));

struct fw_hdr_t
{
    uint8_t sig[FW_SIG_SIZE];
    uint32_t res[4];
    uint8_t year[2];
    uint8_t month;
    uint8_t day;
    uint16_t usb_vid;
    uint16_t usb_pid;
    uint32_t checksum;
    char productor[16];
    char str2[16];
    char str3[32];
    char dev_name[32];
    uint8_t res2[8 * 16];
    char usb_name1[8];
    char usb_name2[8];
    char res3[4 * 16 + 1];
    char mtp_name1[33];
    char mtp_name2[33];
    char mtp_ver[33];
    uint16_t mtp_vid;
    uint16_t mtp_pid;
    char fw_ver[64];
    uint32_t res4[2];

    struct fw_entry_t entry[FW_ENTRIES];
} __attribute__((packed));

/* the s1fwx source code has a layout but it does not make any sense for firmwares
 * found in ATJ2127 for example. In doubt just don't do anything */
struct fw_hdr_f0_t
{
    uint8_t sig[FW_SIG_SIZE];
    uint8_t res[12];
    uint32_t checksum;
    uint8_t res2[492];

    struct fw_entry_t entry[FW_ENTRIES];
} __attribute__((packed));

const uint8_t g_fw_signature_f2[FW_SIG_SIZE] =
{
    0x55, 0xaa, 0xf2, 0x0f
};

const uint8_t g_fw_signature_f0[FW_SIG_SIZE] =
{
    0x55, 0xaa, 0xf0, 0x0f
};

static void build_filename_fw(char buf[16], struct fw_entry_t *ent)
{
    int pos = 0;
    for(int i = 0; i < 8 && ent->name[i] != ' '; i++)
        buf[pos++] = ent->name[i];
    buf[pos++] = '.';
    for(int i = 0; i < 3 && ent->ext[i] != ' '; i++)
        buf[pos++] = ent->ext[i];
    buf[pos] = 0;
}

int fw_unpack(uint8_t *buf, size_t size, fw_extract_callback_t unpack_cb)
{
    struct fw_hdr_t *hdr = (void *)buf;

    if(size < sizeof(struct fw_hdr_t))
    {
        cprintf(GREY, "File too small\n");
        return 1;
    }
    cprintf(BLUE, "Header\n");
    cprintf(GREEN, "  Signature:");
    for(int i = 0; i < FW_SIG_SIZE; i++)
        cprintf(YELLOW, " %02x", hdr->sig[i]);
    int variant = 0;
    if(memcmp(hdr->sig, g_fw_signature_f2, FW_SIG_SIZE) == 0)
    {
        variant = 0xf2;
        cprintf(RED, " Ok (f2 variant)\n");
    }
    else if(memcmp(hdr->sig, g_fw_signature_f0, FW_SIG_SIZE) == 0)
    {
        variant = 0xf0;
        cprintf(RED, " Ok (f0 variant)\n");
    }
    else
    {
        cprintf(RED, " Mismatch\n");
        return 1;
    }

    /* both variants have the same header size, only the fields differ */
    if(variant == 0xf2)
    {
        cprintf_field("  USB VID: ", "0x%x\n", hdr->usb_vid);
        cprintf_field("  USB PID: ", "0x%x\n", hdr->usb_pid);
        cprintf_field("  Date: ", "%x/%x/%02x%02x\n", hdr->day, hdr->month, hdr->year[0], hdr->year[1]);
        cprintf_field("  Checksum: ", "%x\n", hdr->checksum);
        cprintf_field("  Productor: ", "%.16s\n", hdr->productor);
        cprintf_field("  String 2: ", "%.16s\n", hdr->str2);
        cprintf_field("  String 3: ", "%.32s\n", hdr->str3);
        cprintf_field("  Device Name: ", "%.32s\n", hdr->dev_name);
        cprintf(GREEN, "  Unknown:\n");
        for(int i = 0; i < 8; i++)
        {
            cprintf(YELLOW, "    ");
            for(int j = 0; j < 16; j++)
                cprintf(YELLOW, "%02x ", hdr->res2[i * 16 + j]);
            cprintf(YELLOW, "\n");
        }
        cprintf_field("  USB Name 1: ", "%.8s\n", hdr->usb_name1);
        cprintf_field("  USB Name 2: ", "%.8s\n", hdr->usb_name2);
        cprintf_field("  MTP Name 1: ", "%.32s\n", hdr->mtp_name1);
        cprintf_field("  MTP Name 2: ", "%.32s\n", hdr->mtp_name2);
        cprintf_field("  MTP Version: ", "%.32s\n", hdr->mtp_ver);

        cprintf_field("  MTP VID: ", "0x%x\n", hdr->mtp_vid);
        cprintf_field("  MTP PID: ", "0x%x\n", hdr->mtp_pid);
        cprintf_field("  FW Version: ", "%.64s\n", hdr->fw_ver);
    }
    else
    {
        /* struct fw_hdr_f0_t *hdr_f0 = (void *)hdr; */
        cprintf(GREEN, "  Header not dumped because format is unclear.\n");
    }

    cprintf(BLUE, "Entries\n");
    for(int i = 0; i < FW_ENTRIES; i++)
    {
        if(hdr->entry[i].name[0] == 0)
            continue;
        struct fw_entry_t *entry = &hdr->entry[i];
        char filename[16];
        build_filename_fw(filename, entry);
        cprintf(RED, "  %s\n", filename);
        cprintf_field("    Attr: ", "%02x\n", entry->attr);
        cprintf_field("    Offset: ", "0x%x\n", entry->block_offset << 9);
        cprintf_field("    Size: ", "0x%x\n", entry->size);
        cprintf_field("    Unknown: ", "%x\n", entry->unk);
        cprintf_field("    Checksum: ", "0x%x ", entry->checksum);
        uint32_t chk = afi_checksum(buf + (entry->block_offset << 9), entry->size);
        if(chk != entry->checksum)
        {
            cprintf(RED, "Mismatch\n");
            return 1;
        }
        else
            cprintf(RED, "Ok\n");
        int ret = unpack_cb(filename, buf + (entry->block_offset << 9), entry->size);
        if(ret != 0)
            return ret;
    }

    return 0;
}

bool fw_check(uint8_t *buf, size_t size)
{
    struct fw_hdr_t *hdr = (void *)buf;

    if(size < sizeof(struct fw_hdr_t))
        return false;
    if(memcmp(hdr->sig, g_fw_signature_f2, FW_SIG_SIZE) == 0)
        return true;
    if(memcmp(hdr->sig, g_fw_signature_f0, FW_SIG_SIZE) == 0)
        return true;
    return false;
}
