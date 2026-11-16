/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#include <strings.h>
#include <getopt.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/stat.h>
#include <zlib.h>
#include "misc.h"
#include "fwu.h"
#include "afi.h"
#include "fw.h"

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

static void *read_file(const char *name, size_t *size)
{
    FILE *f = fopen(name, "rb");
    if(f == NULL)
    {
        perror(name);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    void *buf = malloc(*size);
    if(buf == NULL || fread(buf, *size, 1, f) != 1)
    {
        perror(name);
        free(buf);
        buf = NULL;
    }
    fclose(f);
    return buf;
}

/* encrypt an AFI as a FWU with the keys of an original FWU */
static int do_fwu_encrypt(const char *orig_name, uint8_t *afi, size_t afi_size)
{
    if(g_out_prefix == NULL)
    {
        cprintf(GREY, "--fwu-encrypt needs an output file (-o)\n");
        return 1;
    }
    size_t orig_size;
    uint8_t *orig = read_file(orig_name, &orig_size);
    if(orig == NULL)
    {
        return 1;
    }
    uint8_t *out;
    size_t out_size;
    int ret = fwu_encrypt(orig, orig_size, afi, afi_size, &out, &out_size);
    free(orig);
    if(ret != 0)
    {
        return ret;
    }
    cprintf(GREY, "Writing %zu bytes to %s... ", out_size, g_out_prefix);
    FILE *f = fopen(g_out_prefix, "wb");
    if(f == NULL || fwrite(out, out_size, 1, f) != 1)
    {
        color(RED);
        perror("Failed");
        ret = 1;
    }
    else
    {
        cprintf(RED, "Ok\n");
    }
    if(f)
    {
        fclose(f);
    }
    free(out);
    return ret;
}

/* The Rockbox bootloader as a stand-in for WELCOME.BIN: BREC loads
 * SHUTOFF.BIN to physical 0x80000, then WELCOME.BIN to 0 and calls its start.
 * The file is the original WELCOME with its first 16 bytes a jump to the
 * bootloader: the OF reads data out of WELCOME.BIN by offset (CONFIG.APP,
 * STANDBY.APP, SETTING.APP: 28 bytes at 0x4ea10), so all of it stays where it
 * is. SHUTOFF follows at 0x80000, so loading the file leaves it as it is,
 * then a header (magic, WELCOME's size, its first 16 bytes) and the
 * bootloader, which puts the 16 bytes back to run WELCOME (atj213x
 * crt0.S). */
#define WELCOME_MAGIC       0x4d434c57      /* "WLCM" */
#define WELCOME_HDR         0x20
#define WELCOME_ENTRY_SIZE  16
#define SHUTOFF_ADDR        0x80000
/* BREC's buffer at 1 MiB */
#define WELCOME_FILE_MAX    0x100000

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = v;
    p[1] = v >> 8;
    p[2] = v >> 16;
    p[3] = v >> 24;
}

/* bootloader.e150 as the build makes it (tools/scramble -add=e150): a
 * big-endian checksum (the model number 128 plus every byte) and "e150"
 * before the binary. The header is checked and dropped; a plain
 * bootloader.bin is taken as it is. */
#define E150_MODEL_NUMBER   128

static int strip_e150_header(uint8_t **boot, size_t *size)
{
    if(*size < 8 || memcmp(*boot + 4, "e150", 4) != 0)
    {
        return 0;
    }
    uint32_t sum = E150_MODEL_NUMBER;
    for(size_t i = 8; i < *size; i++)
    {
        sum += (*boot)[i];
    }
    uint32_t want = ((*boot)[0] << 24) | ((*boot)[1] << 16) |
                    ((*boot)[2] << 8) | (*boot)[3];
    if(sum != want)
    {
        cprintf(GREY, "Bad bootloader checksum: %08x, header says %08x\n",
                sum, want);
        return -1;
    }
    *size -= 8;
    memmove(*boot, *boot + 8, *size);
    return 0;
}

/* WELCOME with the jump, SHUTOFF, header, bootloader; *out is malloc()ed */
static int make_welcome(const uint8_t *boot, size_t boot_size,
                        const uint8_t *welcome, size_t welcome_size,
                        const uint8_t *shutoff, size_t shutoff_size,
                        uint8_t **out, size_t *out_size)
{
    size_t hdr = (SHUTOFF_ADDR + shutoff_size + 0xff) & ~0xff;
    size_t boot_off = hdr + WELCOME_HDR;
    size_t size = boot_off + boot_size;
    if(welcome_size > SHUTOFF_ADDR || welcome_size < WELCOME_ENTRY_SIZE ||
       size > WELCOME_FILE_MAX)
    {
        cprintf(GREY, "Does not fit: WELCOME %zu (at most %u), file %zu (at "
                "most %u)\n", welcome_size, SHUTOFF_ADDR, size,
                WELCOME_FILE_MAX);
        return 1;
    }
    uint8_t *buf = calloc(1, size);
    if(buf == NULL)
    {
        return 1;
    }
    memcpy(buf, welcome, welcome_size);
    memcpy(buf + SHUTOFF_ADDR, shutoff, shutoff_size);
    put32(buf + hdr, WELCOME_MAGIC);
    put32(buf + hdr + 4, welcome_size);
    memcpy(buf + hdr + 8, welcome, WELCOME_ENTRY_SIZE);
    memcpy(buf + boot_off, boot, boot_size);
    /* lui t0, hi; ori t0, t0, lo; jr t0; nop - BREC loads us to 0xa0000000 */
    uint32_t entry = 0xa0000000 + boot_off;
    put32(buf, 0x3c080000 | (entry >> 16));
    put32(buf + 4, 0x35080000 | (entry & 0xffff));
    put32(buf + 8, 0x01000008);
    put32(buf + 12, 0);
    *out = buf;
    *out_size = size;
    return 0;
}

static int write_file(const char *name, const uint8_t *buf, size_t size)
{
    cprintf(GREY, "Writing %zu bytes to %s... ", size, name);
    FILE *f = fopen(name, "wb");
    int ret = 0;
    if(f == NULL || fwrite(buf, size, 1, f) != 1)
    {
        color(RED);
        perror("Failed");
        ret = 1;
    }
    else
    {
        cprintf(RED, "Ok\n");
    }
    if(f)
    {
        fclose(f);
    }
    return ret;
}

/* afi_unpack() callback: keep where the LFI is */
static uint8_t *g_lfi;
static size_t g_lfi_size;

static int find_lfi_cb(const char *name, uint8_t *buf, size_t size)
{
    if(strcasecmp(name, "FWIMAGE.FW") == 0)
    {
        g_lfi = buf;
        g_lfi_size = size;
    }
    return 0;
}

/* buf holds the string str (without its terminator) */
static bool contains(const uint8_t *buf, size_t size, const char *str)
{
    size_t len = strlen(str);
    for(size_t i = 0; i + len <= size; i++)
    {
        if(memcmp(buf + i, str, len) == 0)
        {
            return true;
        }
    }
    return false;
}

/* The iriver E150 install: an official update file (E150.HEX, FWU) with
 * WELCOME.BIN, which BREC calls before it loads the OF, replaced by the Rockbox
 * bootloader carrying the original (make_welcome()). MBRC, BREC, the OF
 * kernel and everything else stay as they are: the bootloader runs WELCOME,
 * then returns to BREC to boot the OF or starts Rockbox. Writes <out>. */
static int do_e150_patch(const char *boot_name, uint8_t *fwu, size_t fwu_size)
{
    if(g_out_prefix == NULL)
    {
        cprintf(GREY, "--e150-patch needs an output file (-o)\n");
        return 1;
    }
    int ret = 1;
    size_t boot_size, lfi_size, afi_size, out_size, welcome_size;
    size_t shutoff_size, image_size;
    uint8_t *boot = NULL, *orig = NULL, *lfi = NULL, *afi = NULL, *out = NULL;
    uint8_t *image = NULL;
    const uint8_t *welcome, *shutoff;
    static const char marker[] = "Rockbox boot loader";

    boot = read_file(boot_name, &boot_size);
    orig = malloc(fwu_size);
    if(boot == NULL || orig == NULL || strip_e150_header(&boot, &boot_size))
    {
        goto end;
    }
    /* fwu_encrypt() wants the original, fwu_decrypt() works in place */
    memcpy(orig, fwu, fwu_size);
    size_t plain_size = fwu_size;
    if(fwu_decrypt(fwu, &plain_size, FWU_AUTO) != 0 ||
       afi_unpack(fwu, plain_size, find_lfi_cb) != 0 || g_lfi == NULL)
    {
        cprintf(GREY, "No LFI (FWIMAGE.FW) in %s\n", g_in_file);
        goto end;
    }
    if(fw_find(g_lfi, g_lfi_size, "WELCOME.BIN", &welcome, &welcome_size) != 0)
    {
        goto end;
    }
    if(contains(welcome, welcome_size, marker))
    {
        cprintf(GREY, "WELCOME.BIN in %s is already a Rockbox bootloader\n",
                g_in_file);
        goto end;
    }
    if(fw_find(g_lfi, g_lfi_size, "SHUTOFF.BIN", &shutoff, &shutoff_size) != 0)
    {
        goto end;
    }

    if(make_welcome(boot, boot_size, welcome, welcome_size, shutoff,
                    shutoff_size, &image, &image_size) ||
       fw_replace(g_lfi, g_lfi_size, "WELCOME.BIN", image, image_size, &lfi,
                  &lfi_size) ||
       afi_replace(fwu, plain_size, "FWIMAGE.FW", lfi, lfi_size, &afi,
                   &afi_size) ||
       fwu_encrypt(orig, fwu_size, afi, afi_size, &out, &out_size))
    {
        goto end;
    }
    ret = write_file(g_out_prefix, out, out_size);
end:
    free(boot);
    free(orig);
    free(image);
    free(lfi);
    free(afi);
    free(out);
    return ret;
}

#define MAX_AFI_REPLACE 8
#define MAX_LFI_REPLACE 8

/* replace files in an LFI image, each "NAME=file" */
static int do_lfi_replace(char **specs, int nr, uint8_t *buf, size_t size)
{
    if(g_out_prefix == NULL)
    {
        cprintf(GREY, "--lfi-replace needs an output file (-o)\n");
        return 1;
    }
    uint8_t *cur = buf;
    size_t cur_size = size;
    int ret = 0;
    for(int i = 0; i < nr && ret == 0; i++)
    {
        char *eq = strchr(specs[i], '=');
        if(eq == NULL)
        {
            cprintf(GREY, "--lfi-replace wants NAME=file, not %s\n", specs[i]);
            ret = 1;
            break;
        }
        *eq = 0;
        size_t data_size;
        uint8_t *data = read_file(eq + 1, &data_size);
        if(data == NULL)
        {
            ret = 1;
            break;
        }
        uint8_t *next;
        size_t next_size;
        ret = fw_replace(cur, cur_size, specs[i], data, data_size, &next, &next_size);
        free(data);
        if(ret == 0)
        {
            cprintf(GREY, "Replaced %s with %s (%zu bytes)\n", specs[i], eq + 1, data_size);
            if(cur != buf)
            {
                free(cur);
            }
            cur = next;
            cur_size = next_size;
        }
    }
    if(ret == 0)
    {
        cprintf(GREY, "Writing %zu bytes to %s... ", cur_size, g_out_prefix);
        FILE *f = fopen(g_out_prefix, "wb");
        if(f == NULL || fwrite(cur, cur_size, 1, f) != 1)
        {
            color(RED);
            perror("Failed");
            ret = 1;
        }
        else
        {
            cprintf(RED, "Ok\n");
        }
        if(f)
        {
            fclose(f);
        }
    }
    if(cur != buf)
    {
        free(cur);
    }
    return ret;
}

/* replace files in an AFI archive, each "NAME=file" */
static int do_afi_replace(char **specs, int nr, uint8_t *buf, size_t size)
{
    if(g_out_prefix == NULL)
    {
        cprintf(GREY, "--afi-replace needs an output file (-o)\n");
        return 1;
    }
    uint8_t *cur = buf;
    size_t cur_size = size;
    int ret = 0;
    for(int i = 0; i < nr && ret == 0; i++)
    {
        char *eq = strchr(specs[i], '=');
        if(eq == NULL)
        {
            cprintf(GREY, "--afi-replace wants NAME=file, not %s\n", specs[i]);
            ret = 1;
            break;
        }
        *eq = 0;
        size_t data_size;
        uint8_t *data = read_file(eq + 1, &data_size);
        if(data == NULL)
        {
            ret = 1;
            break;
        }
        uint8_t *next;
        size_t next_size;
        ret = afi_replace(cur, cur_size, specs[i], data, data_size, &next, &next_size);
        free(data);
        if(ret == 0)
        {
            cprintf(GREY, "Replaced %s with %s (%zu bytes)\n", specs[i], eq + 1, data_size);
            if(cur != buf)
            {
                free(cur);
            }
            cur = next;
            cur_size = next_size;
        }
    }
    if(ret == 0)
    {
        cprintf(GREY, "Writing %zu bytes to %s... ", cur_size, g_out_prefix);
        FILE *f = fopen(g_out_prefix, "wb");
        if(f == NULL || fwrite(cur, cur_size, 1, f) != 1)
        {
            color(RED);
            perror("Failed");
            ret = 1;
        }
        else
        {
            cprintf(RED, "Ok\n");
        }
        if(f)
        {
            fclose(f);
        }
    }
    if(cur != buf)
    {
        free(cur);
    }
    return ret;
}

static int unpack_afi_fw_cb(const char *filename, uint8_t *buf, size_t size)
{
    char *name = malloc(strlen(g_out_prefix) + strlen(filename) + 16);
    sprintf(name, "%s%s", g_out_prefix, filename);

    cprintf(GREY, "Unpacking to %s... ", name);
    FILE *f = fopen(name, "wb");
    if(f)
    {
        if (0 != memcmp(buf, "\x1f\x8b\x8\0\0\0\0\0\0\xb", 10))
            fwrite(buf, size, 1, f);
        else
        {
            uint8_t buf_out[8192];
            z_stream zs;
            int err = Z_OK;
            cprintf(GREEN, "inflating... ");
            memset(&zs, 0, sizeof(zs));
            zs.next_in = buf + 10;
            zs.avail_in = size - 10;
            inflateInit2(&zs, -MAX_WBITS); /* raw */
            while (err == Z_OK)
            {
                zs.next_out = buf_out;
                zs.avail_out = sizeof(buf_out);
                err = inflate(&zs, Z_NO_FLUSH);
                fwrite(buf_out, 1, sizeof(buf_out) - zs.avail_out, f);
            }
        }
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

static int do_afi(uint8_t *buf, size_t size)
{
    build_out_prefix(".fw", "", true);
    return afi_unpack(buf, size, &unpack_afi_fw_cb);
}

static int do_fw(uint8_t *buf, size_t size, bool big_endian)
{
    build_out_prefix(".unpack", "", true);
    return fw_unpack(buf, size, &unpack_afi_fw_cb, big_endian);
}
static void usage(void)
{
    printf("Usage: atjboottool [options] firmware\n");
    printf("Options:\n");
    printf("  -o <path>         Set output file or output prefix\n");
    printf("  -h/--help         Display this message\n");
    printf("  -d/--debug        Display debug messages\n");
    printf("  -c/--no-color     Disable color output\n");
    printf("  --fwu-decrypt     Decrypt a FWU firmware file (to an AFI archive)\n");
    printf("  --afi             Unpack a AFI archive file\n");
    printf("  --fw              Unpack a FW archive file\n");
    printf("  --fw251           Big-endian FW archive used on Flip80251\n");
    printf("  --atj2127         Force ATJ2127 decryption mode\n");
    printf("  --fwu-encrypt <fwu>\n");
    printf("                    Encrypt the input AFI as a FWU with the keys of <fwu>\n");
    printf("  --afi-replace NAME=file\n");
    printf("                    Replace a file in the input AFI (repeatable)\n");
    printf("  --lfi-replace NAME=file\n");
    printf("                    Replace a file in the input LFI (FWIMAGE.FW, repeatable)\n");
    printf("  --e150-patch <bootloader.e150|bootloader.bin>\n");
    printf("                    Make the input E150 update (E150.HEX) start the\n");
    printf("                    Rockbox bootloader in place of WELCOME.BIN\n");
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
    bool big_endian = false;
    enum fwu_mode_t fwu_mode = FWU_AUTO;
    const char *fwu_encrypt_orig = NULL;
    char *afi_replace_specs[MAX_AFI_REPLACE];
    int nr_afi_replace = 0;
    const char *e150_boot = NULL;
    char *lfi_replace_specs[MAX_LFI_REPLACE];
    int nr_lfi_replace = 0;

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, 'h'},
            {"debug", no_argument, 0, 'd'},
            {"no-color", no_argument, 0, 'c'},
            {"fwu-decrypt", no_argument, 0, 'u'},
            {"afi", no_argument, 0, 'a'},
            {"fw", no_argument, 0, 'w'},
            {"fw251", no_argument, 0, 'b'},
            {"atj2127", no_argument, 0, '2'},
            {"fwu-encrypt", required_argument, 0, 'p'},
            {"afi-replace", required_argument, 0, 'r'},
            {"lfi-replace", required_argument, 0, 'l'},
            {"e150-patch", required_argument, 0, 'e'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "hdco:a2b", long_options, NULL);
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
            case 'h':
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
            case 'b':
                try_fw = true;
                big_endian = true;
                break;
            case '2':
                fwu_mode = FWU_ATJ2127;
                break;
            case 'p':
                fwu_encrypt_orig = optarg;
                break;
            case 'e':
                e150_boot = optarg;
                break;
            case 'l':
                if(nr_lfi_replace == MAX_LFI_REPLACE)
                {
                    cprintf(GREY, "Too many --lfi-replace\n");
                    return 1;
                }
                lfi_replace_specs[nr_lfi_replace++] = optarg;
                break;
            case 'r':
                if(nr_afi_replace == MAX_AFI_REPLACE)
                {
                    cprintf(GREY, "Too many --afi-replace\n");
                    return 1;
                }
                afi_replace_specs[nr_afi_replace++] = optarg;
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
    if(e150_boot)
    {
        ret = do_e150_patch(e150_boot, buf, size);
    }
    else if(fwu_encrypt_orig)
    {
        ret = do_fwu_encrypt(fwu_encrypt_orig, buf, size);
    }
    else if(nr_lfi_replace > 0)
    {
        ret = do_lfi_replace(lfi_replace_specs, nr_lfi_replace, buf, size);
    }
    else if(nr_afi_replace > 0)
    {
        ret = do_afi_replace(afi_replace_specs, nr_afi_replace, buf, size);
    }
    else if(try_fwu || fwu_check(buf, size))
        ret = do_fwu(buf, size, fwu_mode);
    else if(try_afi || afi_check(buf, size))
        ret = do_afi(buf, size);
    else if(try_fw || fw_check(buf, size))
        ret = do_fw(buf, size, big_endian);
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

