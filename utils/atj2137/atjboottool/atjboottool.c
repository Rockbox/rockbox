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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/stat.h>
#include "misc.h"
#include "fwu.h"

bool g_debug = false;
char *g_out_prefix = NULL;
char *g_in_file = NULL;

/* [add]: string to add when there is no extension
 * [replace]: string to replace extension */
static void build_out_prefix(char *add, char *replace, bool slash)
{
    if(g_out_prefix)
        return;
    /** copy input filename with extra space */
    g_out_prefix = malloc(strlen(g_in_file) + strlen(add) + 16);
    strcpy(g_out_prefix, g_in_file);
    /** remove extension and add '/' */
    char *filename = strrchr(g_out_prefix, '/');
    // have p points to the beginning or after the last '/'
    filename = (filename == NULL) ? g_out_prefix : filename + 1;
    // extension ?
    char *dot = strrchr(filename, '.');
    if(dot)
    {
        *dot = 0; // cut at the dot
        strcat(dot, replace);
    }
    else
        strcat(filename, add); // add extra string

    if(slash)
    {
        strcat(filename, "/");
        /** make sure the directory exists */
        mkdir(g_out_prefix, S_IRWXU | S_IRGRP | S_IROTH);
    }
}

static int do_fwu(uint8_t *buf, size_t size, enum fwu_mode_t mode)
{
    int ret = fwu_decrypt(buf, &size, mode);
    if(ret != 0)
        return ret;

    build_out_prefix(".afi", ".afi", false);
    cprintf(GREY, "Descrambling to %s... ", g_out_prefix);
    FILE *f = fopen(g_out_prefix, "wb");
    if(f)
    {
        fwrite(buf, size, 1, f);
        fclose(f);
        cprintf(RED, "Ok\n");
        return 0;
    }
    else
    {
        color(RED);
        perror("Failed");
        return 1;
    }
}

/**
 * AFI
 *
 * part of this work comes from s1mp3/s1fwx
 **/

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

static uint32_t afi_checksum(void *ptr, int size)
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

static int do_afi(uint8_t *buf, int size)
{
    struct afi_t *afi = (void *)buf;

    if(size < (int)sizeof(struct afi_t))
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
    if((int)afi->hdr.afi_size == size)
        cprintf(RED, " Ok\n");
    else if((int)afi->hdr.afi_size < size)
        cprintf(RED, " Ok (file greater than archive)\n");
    else
    {
        cprintf(RED, " Error (file too small)\n");
        return 1;
    }

    cprintf_field("  Reserved: ", "%x %x %x\n", afi->hdr.res[0],
        afi->hdr.res[1], afi->hdr.res[2]);

    build_out_prefix(".fw", "", true);
    
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
        cprintf(RED, "%s\n", chk == entry->checksum ? "Ok" : "Mismatch");

        char *name = malloc(strlen(g_out_prefix) + strlen(filename) + 16);
        sprintf(name, "%s%s", g_out_prefix, filename);
        
        cprintf(GREY, "Unpacking to %s... ", name);
        FILE *f = fopen(name, "wb");
        if(f)
        {
            fwrite(buf + entry->offset, entry->size, 1, f);
            fclose(f);
            cprintf(RED, "Ok\n");
        }
        else
            cprintf(RED, "Failed: %m\n");
    }

    cprintf(BLUE, "Post Header\n");
    cprintf_field("  Checksum: ", "%x ", afi->post.checksum);
    uint32_t chk = afi_checksum(buf, sizeof(struct afi_t) - 4);
    cprintf(RED, "%s\n", chk == afi->post.checksum ? "Ok" : "Mismatch");

    return 0;
}

bool afi_check(uint8_t *buf, int size)
{
    struct afi_hdr_t *hdr = (void *)buf;

    if(size < (int)sizeof(struct afi_hdr_t))
        return false;
    return memcmp(hdr->sig, g_afi_signature, AFI_SIG_SIZE) == 0;
}

/**
 * FW
 **/

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

static int do_fw(uint8_t *buf, int size)
{
    struct fw_hdr_t *hdr = (void *)buf;

    if(size < (int)sizeof(struct fw_hdr_t))
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

    build_out_prefix(".unpack", "", true);

    cprintf(BLUE, "Entries\n");
    for(int i = 0; i < AFI_ENTRIES; i++)
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
        cprintf(RED, "%s\n", chk == entry->checksum ? "Ok" : "Mismatch");
        if(g_out_prefix)
        {
            char *name = malloc(strlen(g_out_prefix) + strlen(filename) + 16);
            sprintf(name, "%s%s", g_out_prefix, filename);
            
            cprintf(GREY, "Unpacking to %s... ", name);
            FILE *f = fopen(name, "wb");
            if(f)
            {
                fwrite(buf + (entry->block_offset << 9), entry->size, 1, f);
                fclose(f);
                cprintf(RED, "Ok\n");
            }
            else
            cprintf(RED, "Failed: %m\n");
        }
    }

    return 0;
}

static bool check_fw(uint8_t *buf, int size)
{
    struct fw_hdr_t *hdr = (void *)buf;

    if(size < (int)sizeof(struct fw_hdr_t))
        return false;
    if(memcmp(hdr->sig, g_fw_signature_f2, FW_SIG_SIZE) == 0)
        return true;
    if(memcmp(hdr->sig, g_fw_signature_f0, FW_SIG_SIZE) == 0)
        return true;
    return false;
}

static void usage(void)
{
    printf("Usage: atjboottool [options] firmware\n");
    printf("Options:\n");
    printf("  -o <prefix>\tSet output prefix\n");
    printf("  -f/--force\tForce to continue on errors\n");
    printf("  -?/--help\tDisplay this message\n");
    printf("  -d/--debug\tDisplay debug messages\n");
    printf("  -c/--no-color\tDisable color output\n");
    printf("  --fwu\tUnpack a FWU firmware file\n");
    printf("  --afi\tUnpack a AFI archive file\n");
    printf("  --fw\tUnpack a FW archive file\n");
    printf("  --atj2127\tForce ATJ2127 decryption mode\n");
    printf("The default is to try to guess the format.\n");
    printf("If several formats are specified, all are tried.\n");
    printf("If no output prefix is specified, a default one is picked.\n");
    exit(1);
}

int main(int argc, char **argv)
{
    bool try_fwu = false;
    bool try_afi = false;
    bool try_fw = false;
    enum fwu_mode_t fwu_mode = FWU_AUTO;

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {"no-color", no_argument, 0, 'c'},
            {"fwu", no_argument, 0, 'u'},
            {"afi", no_argument, 0, 'a'},
            {"fw", no_argument, 0, 'w'},
            {"atj2127", no_argument, 0, '2'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?dcfo:a1", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'c':
                enable_color(false);
                break;
            case 'd':
                g_debug = true;
                break;
                break;
            case '?':
                usage();
                break;
            case 'o':
                g_out_prefix = optarg;
                break;
            case 'a':
                try_afi = true;
                break;
            case 'u':
                try_fwu = true;
                break;
            case 'w':
                try_fw = true;
                break;
            case '2':
                fwu_mode = FWU_ATJ2127;
                break;
            default:
                abort();
        }
    }

    if(argc - optind != 1)
    {
        usage();
        return 1;
    }

    g_in_file = argv[optind];
    FILE *fin = fopen(g_in_file, "r");
    if(fin == NULL)
    {
        perror("Cannot open boot file");
        return 1;
    }
    fseek(fin, 0, SEEK_END);
    long size = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    void *buf = malloc(size);
    if(buf == NULL)
    {
        perror("Cannot allocate memory");
        return 1;
    }

    if(fread(buf, size, 1, fin) != 1)
    {
        perror("Cannot read file");
        return 1;
    }

    fclose(fin);

    int ret = -99;
    if(try_fwu || fwu_check(buf, size))
        ret = do_fwu(buf, size, fwu_mode);
    else if(try_afi || afi_check(buf, size))
        ret = do_afi(buf, size);
    else if(try_fw || check_fw(buf, size))
        ret = do_fw(buf, size);
    else
    {
        cprintf(GREY, "No valid format found\n");
        ret = 1;
    }

    if(ret != 0)
    {
        cprintf(GREY, "Error: %d", ret);
        printf("\n");
        ret = 2;
    }
    free(buf);

    color(OFF);

    return ret;
}

