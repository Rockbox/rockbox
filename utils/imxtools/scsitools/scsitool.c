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
#include <stddef.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "rbscsi.h"
#include "misc.h"
#include "stmp_scsi.h"

bool g_debug = false;
bool g_force = false;
stmp_device_t g_dev_fd;

#define let_the_force_flow(x) do { if(!g_force) return x; } while(0)
#define continue_the_force(x) if(x) let_the_force_flow(x)

#define check_field(v_exp, v_have, str_ok, str_bad) \
    if((v_exp) != (v_have)) \
    { cprintf(RED, str_bad); let_the_force_flow(__LINE__); } \
    else { cprintf(RED, str_ok); }

#define errorf(...) do { cprintf(GREY, __VA_ARGS__); return __LINE__; } while(0)

void *buffer_alloc(int sz)
{
    return malloc(sz);
}

static void print_hex(void *_buffer, int buffer_size)
{
    uint8_t *buffer = _buffer;
    for(int i = 0; i < buffer_size; i += 16)
    {
        for(int j = 0; j < 16; j++)
        {
            if(i + j < buffer_size)
                cprintf(YELLOW, " %02x", buffer[i + j]);
            else
                cprintf(YELLOW, "   ");
        }
        printf(" ");
        for(int j = 0; j < 16; j++)
        {
            if(i + j < buffer_size)
                cprintf(RED, "%c", isprint(buffer[i + j]) ? buffer[i + j] : '.');
            else
                cprintf(RED, " ");
        }
        printf("\n");
    }
    if(buffer_size == 0)
        printf("\n");
}

static const char *get_size_suffix(unsigned long long size)
{
    int order = 0;
    while(size >= 1024)
    {
        size /= 1024;
        order++;
    }
    static const char *suffix[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    return suffix[order];
}

static float get_size_natural(unsigned long long size)
{
    float res = size;
    while(res >= 1024)
        res /= 1024;
    return res;
}

static void print_ver(struct scsi_stmp_logical_drive_info_version_t *ver)
{
    cprintf(YELLOW, "%x.%x.%x\n", ver->major, ver->minor, ver->revision);
}

static int do_info(void)
{
    cprintf(BLUE, "Information\n");

    uint8_t dev_type;
    char vendor[9];
    char product[17];
    int ret = stmp_scsi_inquiry(g_dev_fd, &dev_type, vendor, product);
    if(ret == 0)
    {
        cprintf_field("  Vendor: ", "%s\n", vendor);
        cprintf_field("  Product: ", "%s\n", product);
    }
    else if(g_debug)
        cprintf(GREY, "Cannot get inquiry data: %d\n", ret);

    struct scsi_stmp_protocol_version_t ver;
    ret = stmp_get_protocol_version(g_dev_fd, &ver);
    if(ret == 0)
        cprintf_field("  Protocol: ", "%x.%x\n", ver.major, ver.minor);
    else if(g_debug)
        cprintf(GREY, "Cannot get protocol version: %d\n", ret);

    cprintf(BLUE, "Device\n");

    uint8_t *serial;
    int serial_len;
    if(!stmp_get_device_serial(g_dev_fd, &serial, &serial_len))
    {
        cprintf_field("  Serial Number:", " ");
        print_hex(serial, serial_len);
        free(serial);
    }

    uint16_t chip_rev;
    ret = stmp_get_chip_major_rev_id(g_dev_fd, &chip_rev);
    if(ret)
        cprintf(GREY, "Cannot get chip major revision id: %d\n", ret);
    else
        cprintf_field("  Chip Major Rev ID: ", "%x\n", chip_rev);

    uint16_t rom_rev;
    ret = stmp_get_rom_rev_id(g_dev_fd, &rom_rev);
    if(ret)
        cprintf(GREY, "Cannot get rom revision id: %d\n", ret);
    else
        cprintf_field("  ROM Rev ID: ", "%x\n", rom_rev);

    cprintf(BLUE, "Logical Media\n");
    struct stmp_logical_media_info_t info;
    ret = stmp_get_logical_media_info(g_dev_fd, &info);
    if(!ret)
    {
        if(info.has.nr_drives)
            cprintf_field("  Number of drives:", " %u\n", info.nr_drives);
        if(info.has.size)
        {
            cprintf_field("  Media size:", " %llu ", (unsigned long long)info.size);
            cprintf(RED, "(%.3f %s)\n", get_size_natural(info.size), get_size_suffix(info.size));
        }
        if(info.has.alloc_size)
        {
            cprintf_field("  Allocation unit size:", " %lu ", (unsigned long)info.alloc_size);
            cprintf(RED, "(%.3f %s)\n", get_size_natural(info.alloc_size), get_size_suffix(info.alloc_size));
        }
        if(info.has.initialised)
            cprintf_field("  Initialised:", " %u\n", info.initialised);
        if(info.has.state)
            cprintf_field("  State:", " %u\n", info.state);
        if(info.has.write_protected)
            cprintf_field("  Write protected:", " %u\n", info.write_protected);
        if(info.has.type)
        {
            cprintf_field("  Type:", " %u ", info.type);
            cprintf(RED, "(%s)\n", stmp_get_logical_media_type_string(info.type));
        }
        if(info.has.serial)
        {
            cprintf_field("  Serial:", " ");
            print_hex(info.serial, info.serial_len);
            free(info.serial);
        }
        if(info.has.system)
            cprintf_field("  System:", " %u\n", info.system);
        if(info.has.present)
            cprintf_field("  Present:", " %u\n", info.present);
        if(info.has.page_size)
        {
            cprintf_field("  Page size:", " %lu ", (unsigned long)info.page_size);
            cprintf(RED, "(%.3f %s)\n", get_size_natural(info.page_size), get_size_suffix(info.page_size));
        }
        if(info.has.vendor)
        {
            cprintf_field("  Vendor:", " %u ", info.vendor);
            cprintf(RED, "(%s)\n", stmp_get_logical_media_vendor_string(info.vendor));
        }
        if(info.has.nand_id)
        {
            cprintf_field("  Nand ID:", " ");
            print_hex(info.nand_id, sizeof(info.nand_id));
        }
        if(info.has.nr_devices)
            cprintf_field("  Number of devices:", " %lu\n", (unsigned long)info.nr_devices);
    }
    else
        cprintf(GREY, "Cannot get media info: %d\n", ret);

    struct stmp_logical_media_table_t *table;
    ret = stmp_get_logical_media_table(g_dev_fd, &table);
    if(!ret)
    {
        cprintf(BLUE, "Logical Media Table\n");
        for(int i = 0; i < table->header.count; i++)
        {
            cprintf(RED, "  Drive ");
            cprintf_field("No: ", "%2x", table->entry[i].drive_no);
            cprintf_field(" Type: ", "%#x ", table->entry[i].type);
            cprintf(RED, "(%s)", stmp_get_logical_drive_type_string(table->entry[i].type));
            cprintf_field(" Tag: ", "%#x ", table->entry[i].tag);
            cprintf(RED, "(%s)", stmp_get_logical_drive_tag_string(table->entry[i].tag));
            unsigned long long size = table->entry[i].size;
            cprintf_field(" Size: ", "%.3f %s", get_size_natural(size), get_size_suffix(size));
            cprintf(OFF, "\n");
        }

        for(int i = 0; i < table->header.count; i++)
        {
            uint8_t drive = table->entry[i].drive_no;
            cprintf(BLUE, "Drive ");
            cprintf(YELLOW, "%02x\n", drive);
            struct stmp_logical_drive_info_t info;
            ret = stmp_get_logical_drive_info(g_dev_fd, drive, &info);
            if(ret)
                continue;
            if(info.has.sector_size)
            {
                cprintf_field("  Sector size:", " %llu ", (unsigned long long)info.sector_size);
                cprintf(RED, "(%.3f %s)\n", get_size_natural(info.sector_size), get_size_suffix(info.sector_size));
            }
            if(info.has.erase_size)
            {
                cprintf_field("  Erase size:", " %llu ", (unsigned long long)info.erase_size);
                cprintf(RED, "(%.3f %s)\n", get_size_natural(info.erase_size), get_size_suffix(info.erase_size));
            }
            if(info.has.size)
            {
                cprintf_field("  Drive size:", " %llu ", (unsigned long long)info.size);
                cprintf(RED, "(%.3f %s)\n", get_size_natural(info.size), get_size_suffix(info.size));
            }
            if(info.has.sector_count)
                cprintf_field("  Sector count:", " %lu\n", (unsigned long)info.sector_count);
            if(info.has.type)
            {
                cprintf_field("  Type:", " %u ", info.type);
                cprintf(RED, "(%s)\n", stmp_get_logical_drive_type_string(info.type));
            }
            if(info.has.tag)
            {
                cprintf_field("  Tag:", " %u ", info.tag);
                cprintf(RED, "(%s)\n", stmp_get_logical_drive_tag_string(info.tag));
            }
            if(info.has.component_version)
            {
                cprintf_field("  Component version:", " ");
                print_ver(&info.component_version);
            }
            if(info.has.project_version)
            {
                cprintf_field("  Project version:", " ");
                print_ver(&info.project_version);
            }
            if(info.has.write_protected)
                cprintf_field("  Write protected:", " %u\n", info.write_protected);
            if(info.has.serial)
            {
                cprintf_field("  Serial:", " ");
                print_hex(info.serial, info.serial_len);
                free(info.serial);
            }
            if(info.has.change)
                cprintf_field("  Change:", " %u\n", info.change);
            if(info.has.present)
                cprintf_field("  Present:", " %u\n", info.present);
        }
        free(table);
    }
    else
        cprintf(GREY, "Cannot get logical table: %d\n", ret);

    return 0;
}

struct rw_fw_context_t
{
    int tot_size;
    int cur_size;
    int last_percent;
    FILE *f;
    bool read;
};

int rw_fw(void *user, void *buf, size_t size)
{
    struct rw_fw_context_t *ctx = user;
    int this_percent = (ctx->cur_size * 100LLU) / ctx->tot_size;
    if(this_percent != ctx->last_percent && (this_percent % 5) == 0)
    {
        cprintf(RED, "%d%%", this_percent);
        cprintf(YELLOW, "...");
        fflush(stdout);
    }
    ctx->last_percent = this_percent;
    int ret = -1;
    if(ctx->read)
        ret = fread(buf, size, 1, ctx->f);
    else
        ret = fwrite(buf, size, 1, ctx->f);
    ctx->cur_size += size;
    if(ret != 1)
        return -1;
    else
        return size;
}

void rw_finish(struct rw_fw_context_t *ctx)
{
    if(ctx->last_percent == 100)
        return;
    cprintf(RED, "100%%\n");
}

void do_extract(const char *file)
{
    FILE *f = fopen(file, "wb");
    if(f == NULL)
    {
        cprintf(GREY, "Cannot open output file: %m\n");
        return;
    }
    int ret = stmp_read_firmware(g_dev_fd, NULL, NULL);
    if(ret < 0)
    {
        cprintf(GREY, "Cannot get firmware size: %d\n", ret);
        return;
    }
    struct rw_fw_context_t ctx;
    ctx.tot_size = ret;
    ctx.cur_size = 0;
    ctx.f = f;
    ctx.last_percent = -1;
    ctx.read = false;
    ret = stmp_read_firmware(g_dev_fd, &ctx, &rw_fw);
    if(ret < 0)
        cprintf(GREY, "Cannot read firmware: %d\n", ret);
    rw_finish(&ctx);
    fclose(f);
}

void do_write(const char *file, int want_a_brick)
{
    if(!want_a_brick)
    {
        cprintf(GREY, "Writing a new firmware is a dangerous operation that should be attempted\n");
        cprintf(GREY, "if you know what you are doing. If you do, please add the --yes-i-want-a-brick\n");
        cprintf(GREY, "option on the command line and do not complain if you end up with a brick ;)\n");
        return;
    }
    FILE *f = fopen(file, "rb");
    if(f == NULL)
    {
        cprintf(GREY, "Cannot open output file: %m\n");
        return;
    }
    struct rw_fw_context_t ctx;
    fseek(f, 0, SEEK_END);
    ctx.tot_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    ctx.cur_size = 0;
    ctx.f = f;
    ctx.last_percent = -1;
    ctx.read = true;
    int ret = stmp_write_firmware(g_dev_fd, &ctx, &rw_fw);
    if(ret < 0)
        cprintf(GREY, "Cannot write firmware: %d\n", ret);
    rw_finish(&ctx);
    fclose(f);
}

static void usage(void)
{
    printf("Usage: scsitool [options] <dev>\n");
    printf("Options:\n");
    printf("  -f/--force              Force to continue on errors\n");
    printf("  -?/--help               Display this message\n");
    printf("  -d/--debug              Display debug messages\n");
    printf("  -c/--no-color           Disable color output\n");
    printf("  -x/--extract-fw <file>  Extract firmware to file\n");
    printf("  -w/--write-fw <file>    Write firmware to device\n");
    printf("  -i/--info               Display device information\n");
    printf("  --yes-i-want-a-brick    Allow the tool to turn your device into a brick\n");
    exit(1);
}

static int g_yes_i_want_a_brick = 0;

static void scsi_printf(void *user, const char *fmt, ...)
{
    (void)user;
    if(!g_debug)
        return;
    va_list args;
    va_start(args, fmt);
    color(GREY);
    vprintf(fmt, args);
    va_end(args);
}

int main(int argc, char **argv)
{
    if(argc == 1)
        usage();
    const char *extract_fw = NULL;
    const char *write_fw = NULL;
    bool info = false;
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {"no-color", no_argument, 0, 'c'},
            {"force", no_argument, 0, 'f'},
            {"extract-fw", required_argument, 0, 'x'},
            {"write-fw", required_argument, 0, 'w'},
            {"info", no_argument, 0, 'i'},
            {"yes-i-want-a-brick", no_argument, &g_yes_i_want_a_brick, 1},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?dcfx:iw:", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case 0:
                continue;
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
            case 'x':
                extract_fw = optarg;
                break;
            case 'w':
                write_fw = optarg;
                break;
            case 'i':
                info = true;
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

    int ret = 0;
    rb_scsi_device_t scsi_dev = rb_scsi_open(argv[optind], g_debug ? RB_SCSI_DEBUG : 0, NULL, scsi_printf);
    if(scsi_dev == 0)
    {
        cprintf(GREY, "Cannot open device: %m\n");
        ret = 1;
        goto Lend;
    }
    g_dev_fd = stmp_open(scsi_dev, g_debug ? STMP_DEBUG : 0, NULL, scsi_printf);
    if(g_dev_fd == 0)
    {
        cprintf(GREY, "Cannot open stmp device: %m\n");
        ret = 2;
        goto Lend;
    }

    if(extract_fw)
        do_extract(extract_fw);
    if(info)
        do_info();
    if(write_fw)
        do_write(write_fw, g_yes_i_want_a_brick);

    stmp_close(g_dev_fd);
    rb_scsi_close(scsi_dev);
Lend:
    color(OFF);

    return ret;
}
