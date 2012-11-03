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
#include "misc.h"
#include <sys/stat.h>
#include <openssl/md5.h>
#include "nvp.h"

bool g_debug = false;
static char *g_out_prefix = NULL;
static FILE *g_in_file = NULL;
bool g_force = false;
static int g_nvp_node = -1;

#define let_the_force_flow(x) do { if(!g_force) return x; } while(0)
#define continue_the_force(x) if(x) let_the_force_flow(x)

#define check_field(v_exp, v_have, str_ok, str_bad) \
    if((v_exp) != (v_have)) \
    { cprintf(RED, str_bad); let_the_force_flow(__LINE__); } \
    else { cprintf(RED, str_ok); }

#define errorf(...) do { cprintf(GREY, __VA_ARGS__); return __LINE__; } while(0)

static void print_hex(void *p, int size, int unit)
{
    uint8_t *p8 = p;
    uint16_t *p16 = p;
    uint32_t *p32 = p;
    for(int i = 0; i < size; i += unit, p8++, p16++, p32++)
    {
        if(i != 0 && (i % 16) == 0)
            printf("\n");
        if(unit == 1)
            printf(" %02x", *p8);
        else if(unit == 2)
            printf(" %04x", *p16);
        else
            printf(" %08x", *p32);
    }
}

#define SECTOR              512u
#define EMMC_MINIBOOT_START 0
#define EMMC_MINIBOOT_SIZE  (8 * SECTOR)
#define EMMC_UBOOT_START    (8 * SECTOR)
#define EMMC_FU_LINUX_START (512 * SECTOR)
#define EMMC_LINUX_START    (66048 * SECTOR)
#define EMMC_NVP_START      ((512 + 32768) * SECTOR)
#define EMMC_NVP_SIZE       (30720 * SECTOR)

#define print_entry(begin, end, ...) \
    do{ cprintf(YELLOW, "  %08x  %08x  ", begin, end); cprintf(GREEN, __VA_ARGS__); } while(0)

static int read(uint32_t offset, uint32_t size, void *buf)
{
    if(fseek(g_in_file, offset, SEEK_SET))
        errorf("Cannot seek in file: %m\n");
    if(fread(buf, size, 1, g_in_file) != 1)
        errorf("Cannot read in file: %m\n");
    return 0;
}

static int nvp_read(uint32_t offset, uint32_t size, void *buf)
{
    if(offset + size > EMMC_NVP_SIZE)
        errorf("nvp read out of nvp area\n");
    return read(offset + EMMC_NVP_START, size, buf);
}

// returns size or 0
static uint32_t do_image(uint32_t start, const char *name)
{
    uint32_t size;
    int ret = read(start, sizeof(size), &size);
    if(ret) return ret;
    /* actual uboot size contains 4 bytes for the size, 4 for the crc pad and
     * must be ronded to the next sector */
    size = ROUND_UP(size + 8, SECTOR);

    print_entry(start, start + size, name);

    /* Check U-Boot crc (must be 0) */
    uint32_t crc_buffer[SECTOR / 4];
    uint32_t crc = 0;
    uint32_t pos = start + 4;
    uint32_t rem_size = size - 4;
    while(rem_size)
    {
        ret = read(pos, SECTOR, crc_buffer);
        if(ret) return ret;
        uint32_t sz = MIN(rem_size, SECTOR);
        for(unsigned i = 0; i < sz / 4; i++)
            crc ^= crc_buffer[i];
        pos += sz;
        rem_size -= sz;
    }

    if(crc == 0)
    {
        cprintf(RED, " (CRC Ok)\n");
        return size;
    }
    else
    {
        cprintf(RED, " (CRC Mismatch)\n");
        return 0;
    }
}

static int do_emmc(void)
{
    cprintf(BLUE, "eMMC map\n");
    cprintf(RED, "    begin      end    comment\n");

    print_entry(EMMC_MINIBOOT_START, EMMC_MINIBOOT_START + EMMC_MINIBOOT_SIZE, "eMMC Mini Boot\n");

    uint32_t uboot_size = do_image(EMMC_UBOOT_START, "U-Boot");
    if(!uboot_size)
        return 1;

    uint32_t fulinux_start = EMMC_UBOOT_START + uboot_size;
    uint32_t fulinux_size = do_image(fulinux_start, "FU Linux");
    if(!fulinux_size)
        return 1;

    uint32_t fu_initrd_size = do_image(EMMC_FU_LINUX_START, "FU initrd");
    if(!fu_initrd_size)
        return 1;

    print_entry(EMMC_NVP_START, EMMC_NVP_START + EMMC_NVP_SIZE, "NVP\n");

    uint32_t linux_size = do_image(EMMC_LINUX_START, "Linux");
    if(!linux_size)
        return 1;
    
    int ret = nvp_info();
    continue_the_force(ret);

    return 0;
}

static int do_nvp_extract(void)
{
    if(!g_out_prefix)
    {
        cprintf(GREY, "You must specify an output prefix to extract a NVP node\n");
        return 1;
    }
    if(!nvp_is_valid_node(g_nvp_node))
    {
        cprintf(GREY, "Invalid NVP node %d\n", g_nvp_node);
        return 3;
    }
    
    FILE *f = fopen(g_out_prefix, "wb");
    if(!f)
    {
        cprintf(GREY, "Cannot open output file: %m\n");
        return 2;
    }

    int size = nvp_get_node_size(g_nvp_node);
    void *buffer = malloc(size);
    int ret = nvp_read_node(g_nvp_node, 0, buffer, size);
    if(ret < 0)
        cprintf(GREY, "NVP read error: %d\n", ret);
    else
    {
        cprintf(YELLOW, "%d ", ret);
        cprintf(GREEN, "bytes written\n");
        fwrite(buffer, 1, ret, f);
    }
    free(buffer);
    fclose(f);
    return 0;
}

static void usage(void)
{
    printf("Usage: emmctool [options] img\n");
    printf("Options:\n");
    printf("  -o <prefix>\t\tSet output prefix\n");
    printf("  -f/--force\t\tForce to continue on errors\n");
    printf("  -?/--help\t\tDisplay this message\n");
    printf("  -d/--debug\t\tDisplay debug messages\n");
    printf("  -c/--no-color\t\tDisable color output\n");
    printf("  -e/--nvp-ex <node>\tExtract a NVP node\n");
    exit(1);
}

int main(int argc, char **argv)
{
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {"no-color", no_argument, 0, 'c'},
            {"force", no_argument, 0, 'f'},
            {"nvp-ex", required_argument, 0, 'e'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?dcfo:e:", long_options, NULL);
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
            case 'f':
                g_force = true;
                break;
            case '?':
                usage();
                break;
            case 'o':
                g_out_prefix = optarg;
                break;
            case 'e':
                g_nvp_node = strtoul(optarg, NULL, 0);
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

    g_in_file = fopen(argv[optind], "rb");
    if(g_in_file == NULL)
    {
        perror("Cannot open boot file");
        return 1;
    }

    int ret = nvp_init(EMMC_NVP_SIZE, &nvp_read, g_debug);
    if(ret) return ret;
    ret = do_emmc();
    if(ret == 0 && g_nvp_node >= 0)
        ret = do_nvp_extract();

    fclose(g_in_file);

    color(OFF);

    return ret;
}

