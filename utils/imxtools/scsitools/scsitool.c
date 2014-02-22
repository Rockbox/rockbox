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
#ifndef _WIN32
#include <scsi/scsi.h>
#endif
#include <scsi/sg_lib.h>
#include <scsi/sg_pt.h>
#include "misc.h"
#include "stmp_scsi.h"

/* the windows port doesn't have scsi.h and GOOD */
#ifndef GOOD
#define GOOD                 0x00
#endif

bool g_debug = false;
bool g_force = false;
int g_dev_fd = 0;

#define let_the_force_flow(x) do { if(!g_force) return x; } while(0)
#define continue_the_force(x) if(x) let_the_force_flow(x)

#define check_field(v_exp, v_have, str_ok, str_bad) \
    if((v_exp) != (v_have)) \
    { cprintf(RED, str_bad); let_the_force_flow(__LINE__); } \
    else { cprintf(RED, str_ok); }

#define errorf(...) do { cprintf(GREY, __VA_ARGS__); return __LINE__; } while(0)

#if 0
void *buffer_alloc(int sz)
{
#ifdef SG_LIB_MINGW
    unsigned psz = getpagesize();
#else
    unsigned psz = sysconf(_SC_PAGESIZE); /* was getpagesize() */
#endif
    void *buffer = malloc(sz + psz);
    return (void *)(((ptrdiff_t)(buffer + psz - 1)) & ~(psz - 1));
}
#else
void *buffer_alloc(int sz)
{
    return malloc(sz);
}
#endif

static uint16_t fix_endian16be(uint16_t w)
{
    return w << 8 | w >> 8;
}

static uint32_t fix_endian32be(uint32_t w)
{
    return __builtin_bswap32(w);
}

static uint64_t fix_endian64be(uint64_t w)
{
    return __builtin_bswap64(w);
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
}

/* Do read */
#define DO_READ     (1 << 1)
/* Do write */
#define DO_WRITE    (1 << 2)

/* returns <0 on error and status otherwise */
int do_scsi(uint8_t *cdb, int cdb_size, unsigned flags, void *sense, int *sense_size, void *buffer, int *buf_size)
{
    char error[256];
    struct sg_pt_base *obj = construct_scsi_pt_obj();
    if(obj == NULL)
    {
        cprintf(GREY, "construct_scsi_pt_obj failed\n");
        return 1;
    }
    set_scsi_pt_cdb(obj, cdb, cdb_size);
    if(sense)
        set_scsi_pt_sense(obj, sense, *sense_size);
    if(flags & DO_READ)
        set_scsi_pt_data_in(obj, buffer, *buf_size);
    if(flags & DO_WRITE)
        set_scsi_pt_data_out(obj, buffer, *buf_size);
    int ret = do_scsi_pt(obj, g_dev_fd, 10, 0);
    switch(get_scsi_pt_result_category(obj))
    {
        case SCSI_PT_RESULT_SENSE:
        case SCSI_PT_RESULT_GOOD:
            ret = get_scsi_pt_status_response(obj);
            break;
        case SCSI_PT_RESULT_STATUS:
            cprintf(GREY, "Status error: %d (", get_scsi_pt_status_response(obj));
            sg_print_scsi_status(get_scsi_pt_status_response(obj));
            printf(")\n");
            break;
        case SCSI_PT_RESULT_TRANSPORT_ERR:
            cprintf(GREY, "Transport error: %s\n", get_scsi_pt_transport_err_str(obj, 256, error));
            ret = -2;
            break;
        case SCSI_PT_RESULT_OS_ERR:
            cprintf(GREY, "OS error: %s\n", get_scsi_pt_os_err_str(obj, 256, error));
            ret = -3;
            break;
        default:
            cprintf(GREY, "Unknown error\n");
            break;
    }

    if(sense)
        *sense_size = get_scsi_pt_sense_len(obj);
    if(flags & (DO_WRITE | DO_READ))
        *buf_size -= get_scsi_pt_resid(obj);
    
    destruct_scsi_pt_obj(obj);
    return ret;
}

int do_sense_analysis(int status, uint8_t *sense, int sense_size)
{
    if(status != GOOD && g_debug)
    {
        cprintf_field("Status:", " "); fflush(stdout);
        sg_print_scsi_status(status);
        cprintf_field("\nSense:", " "); fflush(stdout);
        sg_print_sense(NULL, sense, sense_size, 0);
    }
    if(status == GOOD)
        return 0;
    return status;
}

int stmp_inquiry(uint8_t *dev_type, char vendor[9], char product[17])
{
    unsigned char buffer[36];
    uint8_t cdb[10];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = 0x12;
    cdb[4] = sizeof(buffer);

    uint8_t sense[32];
    int sense_size = sizeof(sense);

    int buf_sz = sizeof(buffer);
    int ret = do_scsi(cdb, sizeof(cdb), DO_READ, sense, &sense_size, buffer, &buf_sz);
    if(ret < 0)
        return ret;
    ret = do_sense_analysis(ret, sense, sense_size);
    if(ret)
        return ret;
    if(buf_sz != sizeof(buffer))
        return -1;
    *dev_type = buffer[0];
    memcpy(vendor, buffer + 8, 8);
    vendor[8] = 0;
    memcpy(product, buffer + 16, 16);
    product[16] = 0;
    return 0;
}

static int stmp_get_protocol_version(struct scsi_stmp_protocol_version_t *ver)
{
    uint8_t cdb[16];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_STMP_READ;
    cdb[1] = SCSI_STMP_CMD_GET_PROTOCOL_VERSION;

    uint8_t sense[32];
    int sense_size = sizeof(sense);

    int buf_sz = sizeof(struct scsi_stmp_protocol_version_t);
    int ret = do_scsi(cdb, sizeof(cdb), DO_READ, sense, &sense_size, ver, &buf_sz);
    if(ret < 0)
        return ret;
    ret = do_sense_analysis(ret, sense, sense_size);
    if(ret)
        return ret;
    if(buf_sz != sizeof(struct scsi_stmp_protocol_version_t))
        return -1;
    return 0;
}

static int stmp_get_chip_major_rev_id(struct scsi_stmp_chip_major_rev_id_t *ver)
{
    uint8_t cdb[16];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_STMP_READ;
    cdb[1] = SCSI_STMP_CMD_GET_CHIP_MAJOR_REV_ID;

    uint8_t sense[32];
    int sense_size = sizeof(sense);

    int buf_sz = sizeof(struct scsi_stmp_chip_major_rev_id_t);
    int ret = do_scsi(cdb, sizeof(cdb), DO_READ, sense, &sense_size, ver, &buf_sz);
    if(ret < 0)
        return ret;
    ret = do_sense_analysis(ret, sense, sense_size);
    if(ret)
        return ret;
    if(buf_sz != sizeof(struct scsi_stmp_chip_major_rev_id_t))
        return -1;
    ver->rev = fix_endian16be(ver->rev);
    return 0;
}

static int stmp_get_rom_rev_id(struct scsi_stmp_rom_rev_id_t *ver)
{
    uint8_t cdb[16];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_STMP_READ;
    cdb[1] = SCSI_STMP_CMD_GET_ROM_REV_ID;

    uint8_t sense[32];
    int sense_size = sizeof(sense);

    int buf_sz = sizeof(struct scsi_stmp_rom_rev_id_t);
    int ret = do_scsi(cdb, sizeof(cdb), DO_READ, sense, &sense_size, ver, &buf_sz);
    if(ret < 0)
        return ret;
    ret = do_sense_analysis(ret, sense, sense_size);
    if(ret)
        return ret;
    if(buf_sz != sizeof(struct scsi_stmp_rom_rev_id_t))
        return -1;
    ver->rev = fix_endian16be(ver->rev);
    return 0;
}

static int stmp_get_logical_media_info(uint8_t info, void *data, int *len)
{
    uint8_t cdb[16];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_STMP_READ;
    cdb[1] = SCSI_STMP_CMD_GET_LOGICAL_MEDIA_INFO;
    cdb[2] = info;

    uint8_t sense[32];
    int sense_size = sizeof(sense);

    int ret = do_scsi(cdb, sizeof(cdb), DO_READ, sense, &sense_size, data, len);
    if(ret < 0)
        return ret;
    return do_sense_analysis(ret, sense, sense_size);
}

static int stmp_get_logical_table(struct scsi_stmp_logical_table_t *table, int entry_count)
{
    uint8_t cdb[16];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_STMP_READ;
    cdb[1] = SCSI_STMP_CMD_GET_LOGICAL_TABLE;
    cdb[2] = entry_count;

    uint8_t sense[32];
    int sense_size = sizeof(sense);

    int buf_sz = sizeof(struct scsi_stmp_logical_table_t) +
        entry_count * sizeof(struct scsi_stmp_logical_table_entry_t);
    int ret = do_scsi(cdb, sizeof(cdb), DO_READ, sense, &sense_size, table, &buf_sz);
    if(ret < 0)
        return ret;
    ret = do_sense_analysis(ret, sense, sense_size);
    if(ret)
        return ret;
    if((buf_sz - sizeof(struct scsi_stmp_logical_table_t)) % sizeof(struct scsi_stmp_logical_table_entry_t))
        return -1;
    table->count = fix_endian16be(table->count);
    struct scsi_stmp_logical_table_entry_t *entry = (void *)(table + 1);
    for(int i = 0; i < entry_count; i++)
        entry[i].size = fix_endian64be(entry[i].size);
    return 0;
}

static int stmp_get_logical_drive_info(uint8_t drive, uint8_t info, void *data, int *len)
{
    uint8_t cdb[16];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_STMP_READ;
    cdb[1] = SCSI_STMP_CMD_GET_LOGICAL_DRIVE_INFO;
    cdb[2] = drive;
    cdb[3] = info;

    uint8_t sense[32];
    int sense_size = sizeof(sense);

    int ret = do_scsi(cdb, sizeof(cdb), DO_READ, sense, &sense_size, data, len);
    if(ret < 0)
        return ret;
    return do_sense_analysis(ret, sense, sense_size);
}

static int stmp_get_device_info(uint8_t info, void *data, int *len)
{
    uint8_t cdb[16];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_STMP_READ;
    cdb[1] = SCSI_STMP_CMD_GET_DEVICE_INFO;
    cdb[2] = info;

    uint8_t sense[32];
    int sense_size = sizeof(sense);

    int ret = do_scsi(cdb, sizeof(cdb), DO_READ, sense, &sense_size, data, len);
    if(ret < 0)
        return ret;
    return do_sense_analysis(ret, sense, sense_size);
}

static int stmp_get_serial_number(uint8_t info, void *data, int *len)
{
    uint8_t cdb[16];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_STMP_READ;
    cdb[1] = SCSI_STMP_CMD_GET_CHIP_SERIAL_NUMBER;
    cdb[2] = info;

    uint8_t sense[32];
    int sense_size = sizeof(sense);

    int ret = do_scsi(cdb, sizeof(cdb), DO_READ, sense, &sense_size, data, len);
    if(ret < 0)
        return ret;
    return do_sense_analysis(ret, sense, sense_size);
}

static int stmp_read_logical_drive_sectors(uint8_t drive, uint64_t address,
    uint32_t count, void *buffer, int *buffer_size)
{
    uint8_t cdb[16];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_STMP_READ;
    cdb[1] = SCSI_STMP_CMD_READ_LOGICAL_DRIVE_SECTOR;
    cdb[2] = drive;
    address = fix_endian64be(address);
    memcpy(&cdb[3], &address, sizeof(address));
    count = fix_endian32be(count);
    memcpy(&cdb[11], &count, sizeof(count));

    uint8_t sense[32];
    int sense_size = sizeof(sense);

    int ret = do_scsi(cdb, sizeof(cdb), DO_READ, sense, &sense_size, buffer, buffer_size);
    if(ret < 0)
        return ret;
    return do_sense_analysis(ret, sense, sense_size);
}

static int stmp_write_logical_drive_sectors(uint8_t drive, uint64_t address,
    uint32_t count, void *buffer, int *buffer_size)
{
    uint8_t cdb[16];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_STMP_WRITE;
    cdb[1] = SCSI_STMP_CMD_WRITE_LOGICAL_DRIVE_SECTOR;
    cdb[2] = drive;
    address = fix_endian64be(address);
    memcpy(&cdb[3], &address, sizeof(address));
    count = fix_endian32be(count);
    memcpy(&cdb[11], &count, sizeof(count));

    uint8_t sense[32];
    int sense_size = sizeof(sense);

    int ret = do_scsi(cdb, sizeof(cdb), DO_WRITE, sense, &sense_size, buffer, buffer_size);
    if(ret < 0)
        return ret;
    return do_sense_analysis(ret, sense, sense_size);
}

static const char *stmp_get_logical_media_type_string(uint32_t type)
{
    switch(type)
    {
        case SCSI_STMP_MEDIA_TYPE_NAND: return "NAND";
        case SCSI_STMP_MEDIA_TYPE_SDMMC: return "SD/MMC";
        case SCSI_STMP_MEDIA_TYPE_HDD: return "HDD";
        case SCSI_STMP_MEDIA_TYPE_RAM: return "RAM";
        case SCSI_STMP_MEDIA_TYPE_iNAND: return "iNAND";
        default: return "?";
    }
}

static const char *stmp_get_logical_media_vendor_string(uint32_t type)
{
    switch(type)
    {
        case SCSI_STMP_MEDIA_VENDOR_SAMSUNG: return "Samsung";
        case SCSI_STMP_MEDIA_VENDOR_STMICRO: return "ST Micro";
        case SCSI_STMP_MEDIA_VENDOR_HYNIX: return "Hynix";
        case SCSI_STMP_MEDIA_VENDOR_MICRON: return "Micron";
        case SCSI_STMP_MEDIA_VENDOR_TOSHIBA: return "Toshiba";
        case SCSI_STMP_MEDIA_VENDOR_RENESAS: return "Renesas";
        case SCSI_STMP_MEDIA_VENDOR_INTEL: return "Intel";
        case SCSI_STMP_MEDIA_VENDOR_SANDISK: return "Sandisk";
        default: return "?";
    }
}

static const char *stmp_get_logical_drive_type_string(uint32_t type)
{
    switch(type)
    {
        case SCSI_STMP_DRIVE_TYPE_DATA: return "Data";
        case SCSI_STMP_DRIVE_TYPE_SYSTEM: return "System";
        case SCSI_STMP_DRIVE_TYPE_HIDDEN: return "Hidden";
        case SCSI_STMP_DRIVE_TYPE_UNKNOWN: return "Unknown";
        default: return "?";
    }
}

static const char *stmp_get_logical_drive_tag_string(uint8_t type)
{
    switch(type)
    {
        case SCSI_STMP_DRIVE_TAG_STMPSYS_S: return "System";
        case SCSI_STMP_DRIVE_TAG_HOSTLINK_S: return "Hostlink";
        case SCSI_STMP_DRIVE_TAG_RESOURCE_BIN: return "Resource";
        case SCSI_STMP_DRIVE_TAG_EXTRA_S: return "Extra";
        case SCSI_STMP_DRIVE_TAG_RESOURCE1_BIN: return "Resource1";
        case SCSI_STMP_DRIVE_TAG_OTGHOST_S: return "OTG";
        case SCSI_STMP_DRIVE_TAG_HOSTRSC_BIN: return "Host Resource";
        case SCSI_STMP_DRIVE_TAG_DATA: return "Data";
        case SCSI_STMP_DRIVE_TAG_HIDDEN: return "Hidden";
        case SCSI_STMP_DRIVE_TAG_BOOTMANAGER_S: return "Boot";
        case SCSI_STMP_DRIVE_TAG_UPDATER_S: return "Updater";
        default: return "?";
    }
}

static const char *stmp_get_logical_media_state_string(uint8_t state)
{
    switch(state)
    {
        case SCSI_STMP_MEDIA_STATE_UNKNOWN: return "Unknown";
        case SCSI_STMP_MEDIA_STATE_ERASED: return "Erased";
        case SCSI_STMP_MEDIA_STATE_ALLOCATED: return "Allocated";
        default: return "?";
    }
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

static int do_info(void)
{
    cprintf(BLUE, "Information\n");

    uint8_t dev_type;
    char vendor[9];
    char product[17];
    int ret = stmp_inquiry(&dev_type, vendor, product);
    if(ret)
    {
        cprintf(GREY, "Cannot get inquiry data: %d\n", ret);
    }
    else
    {
        cprintf_field("  Vendor: ", "%s\n", vendor);
        cprintf_field("  Product: ", "%s\n", product);
    }

    struct scsi_stmp_protocol_version_t ver;
    ret = stmp_get_protocol_version(&ver);
    if(ret)
        cprintf(GREY, "Cannot get protocol version: %d\n", ret);
    else
        cprintf_field("  Protocol: ", "%x.%x\n", ver.major, ver.minor);

    do
    {
        union
        {
            uint8_t u8;
            uint16_t u16;
            uint32_t u32;
            uint64_t u64;
            uint8_t buf[1024];
        }u;

        cprintf(GREEN, "  Device\n");
        int len = 4;
        ret = stmp_get_device_info(0, &u.u32, &len);
        if(!ret && len == 4)
        {
            u.u32 = fix_endian32be(u.u32);
            cprintf_field("    Info 0: ", "%lu\n", (unsigned long)u.u32);
        }

        len = 4;
        ret = stmp_get_device_info(1, &u.u32, &len);
        if(!ret && len == 4)
        {
            u.u32 = fix_endian32be(u.u32);
            cprintf_field("    Info 1: ", "%lu\n", (unsigned long)u.u32);
        }

        len = 2;
        ret = stmp_get_serial_number(0, &u.u16, &len);
        if(!ret && len == 2)
        {
            u.u16 = fix_endian16be(u.u16);
            len = MIN(u.u16, sizeof(u.buf));
            ret = stmp_get_serial_number(1, u.buf, &len);
            cprintf_field("    Serial Number:", " ");
            print_hex(u.buf, len);
            cprintf(OFF, "\n");
        }

        len = 2;
        ret = stmp_get_logical_media_info(SCSI_STMP_MEDIA_INFO_NR_DRIVES, &u.u16, &len);
        if(!ret && len == 2)
        {
            u.u16 = fix_endian16be(u.u16);
            cprintf_field("  Number of Drives: ", "%d\n", u.u16);
        }

        len = 4;
        ret = stmp_get_logical_media_info(SCSI_STMP_MEDIA_INFO_TYPE, &u.u32, &len);
        if(!ret && len == 4)
        {
            u.u32 = fix_endian32be(u.u32);
            cprintf_field("  Media Type: ", "%#x", u.u32);
            cprintf(RED, " (%s)\n", stmp_get_logical_media_type_string(u.u32));
        }

        len = 1;
        ret = stmp_get_logical_media_info(SCSI_STMP_MEDIA_INFO_IS_INITIALISED, &u.u8, &len);
        if(!ret && len == 1)
            cprintf_field("  Is Initialised: ", "%d\n", u.u8);

        len = 1;
        ret = stmp_get_logical_media_info(SCSI_STMP_MEDIA_INFO_STATE, &u.u8, &len);
        if(!ret && len == 1)
            cprintf_field("  State: ", "%s\n", stmp_get_logical_media_state_string(u.u8));

        len = 1;
        ret = stmp_get_logical_media_info(SCSI_STMP_MEDIA_INFO_IS_WRITE_PROTECTED, &u.u8, &len);
        if(!ret && len == 1)
            cprintf_field("  Is Write Protected: ", "%#x\n", u.u8);

        len = 8;
        ret = stmp_get_logical_media_info(SCSI_STMP_MEDIA_INFO_SIZE, &u.u64, &len);
        if(!ret && len == 8)
        {
            u.u64 = fix_endian64be(u.u64);
            cprintf_field("  Media Size: ", "%llu B (%.3f %s)\n", (unsigned long long)u.u64, 
                get_size_natural(u.u64), get_size_suffix(u.u64));
        }

        int serial_number_size = 0;
        len = 4;
        ret = stmp_get_logical_media_info(SCSI_STMP_MEDIA_INFO_SERIAL_NUMBER_SIZE, &u.u32, &len);
        if(!ret && len == 4)
        {
            u.u32 = fix_endian32be(u.u32);
            cprintf_field("  Serial Number Size: ", "%d\n", u.u32);
            serial_number_size = u.u32;
        }

        len = serial_number_size;
        ret = stmp_get_logical_media_info(SCSI_STMP_MEDIA_INFO_SERIAL_NUMBER, &u.buf, &len);
        if(!ret && len != 0)
        {
            cprintf(GREEN, "  Serial Number:");
            print_hex(u.buf, len);
        }

        len = 1;
        ret = stmp_get_logical_media_info(SCSI_STMP_MEDIA_INFO_IS_SYSTEM_MEDIA, &u.u8, &len);
        if(!ret && len == 1)
            cprintf_field("  Is System Media: ", "%d\n", u.u8);

        len = 1;
        ret = stmp_get_logical_media_info(SCSI_STMP_MEDIA_INFO_IS_MEDIA_PRESENT, &u.u8, &len);
        if(!ret && len == 1)
            cprintf_field("  Is Media Present: ", "%d\n", u.u8);

        len = 4;
        ret = stmp_get_logical_media_info(SCSI_STMP_MEDIA_INFO_VENDOR, &u.u32, &len);
        if(!ret && len == 4)
        {
            u.u32 = fix_endian32be(u.u32);
            cprintf_field("  Media Vendor: ", "%#x", u.u32);
            cprintf(RED, " (%s)\n", stmp_get_logical_media_vendor_string(u.u32));
        }

        len = 8;
        ret = stmp_get_logical_media_info(13, &u.u64, &len);
        if(!ret && len == 8)
        {
            u.u64 = fix_endian64be(u.u64);
            cprintf_field("  Logical Media Info (13): ", "%#llx\n", (unsigned long long)u.u64);
        }

        len = 4;
        ret = stmp_get_logical_media_info(11, &u.u32, &len);
        if(!ret && len == 4)
        {
            u.u32 = fix_endian32be(u.u32);
            cprintf_field("  Logical Media Info (11): ", "%#x\n", u.u32);
        }

        len = 4;
        ret = stmp_get_logical_media_info(14, &u.u32, &len);
        if(!ret && len == 4)
        {
            u.u32 = fix_endian32be(u.u32);
            cprintf_field("  Logical Media Info (14): ", "%#x\n", u.u32);
        }

        len = 4;
        ret = stmp_get_logical_media_info(SCSI_STMP_MEDIA_INFO_ALLOC_UNIT_SIZE, &u.u32, &len);
        if(!ret && len == 4)
        {
            u.u32 = fix_endian32be(u.u32);
            cprintf_field("  Allocation Unit Size: ", "%d B\n", u.u32);
        }
    }while(0);
    
    struct scsi_stmp_chip_major_rev_id_t chip_rev;
    ret = stmp_get_chip_major_rev_id(&chip_rev);
    if(ret)
        cprintf(GREY, "Cannot get chip major revision id: %d\n", ret);
    else
        cprintf_field("  Chip Major Rev ID: ", "%x\n", chip_rev.rev);
    
    struct scsi_stmp_rom_rev_id_t rom_rev;
    ret = stmp_get_rom_rev_id(&rom_rev);
    if(ret)
        cprintf(GREY, "Cannot get rom revision id: %d\n", ret);
    else
        cprintf_field("  ROM Rev ID: ", "%x\n", rom_rev.rev);

    struct
    {
        struct scsi_stmp_logical_table_t header;
        struct scsi_stmp_logical_table_entry_t entry[20];
    }__attribute__((packed)) table;

    ret = stmp_get_logical_table(&table.header, sizeof(table.entry) / sizeof(table.entry[0]));
    if(ret)
        cprintf(GREY, "Cannot get logical table: %d\n", ret);
    else
    {
        cprintf_field("  Logical Table: ", "%d entries\n", table.header.count);
        for(int i = 0; i < table.header.count; i++)
        {
            cprintf(BLUE, "    Drive ");
            cprintf_field("No: ", "%2x", table.entry[i].drive_no);
            cprintf_field(" Type: ", "%#x ", table.entry[i].type);
            cprintf(RED, "(%s)", stmp_get_logical_drive_type_string(table.entry[i].type));
            cprintf_field(" Tag: ", "%#x ", table.entry[i].tag);
            cprintf(RED, "(%s)", stmp_get_logical_drive_tag_string(table.entry[i].tag));
            unsigned long long size = table.entry[i].size;
            int order = 0;
            while(size >= 1024)
            {
                size /= 1024;
                order++;
            }
            static const char *suffix[] = {"B", "KiB", "MiB", "GiB", "TiB"};
            cprintf_field(" Size: ", "%llu %s", size, suffix[order]);
            cprintf(OFF, "\n");
        }

        for(int i = 0; i < table.header.count; i++)
        {
            union
            {
                uint8_t u8;
                uint16_t u16;
                uint32_t u32;
                uint64_t u64;
                uint8_t buf[52];
            }u;
            uint8_t drive = table.entry[i].drive_no;
            cprintf_field("  Drive ", "%02x\n", drive);
            
            int len = 4;
            ret = stmp_get_logical_drive_info(drive, SCSI_STMP_DRIVE_INFO_SECTOR_SIZE, &u.u32, &len);
            if(!ret && len == 4)
            {
                u.u32 = fix_endian32be(u.u32);
                cprintf_field("    Sector Size: ", "%lu B (%.3f %s)\n", (unsigned long)u.u32,
                    get_size_natural(u.u32), get_size_suffix(u.u32));
            }

            len = 4;
            ret = stmp_get_logical_drive_info(drive, SCSI_STMP_DRIVe_INFO_ERASE_SIZE, &u.u32, &len);
            if(!ret && len == 4)
            {
                u.u32 = fix_endian32be(u.u32);
                cprintf_field("    Erase Size: ", "%lu B (%.3f %s)\n", (unsigned long)u.u32,
                    get_size_natural(u.u32), get_size_suffix(u.u32));
            }

            len = 8;
            ret = stmp_get_logical_drive_info(drive, SCSI_STMP_DRIVE_INFO_SIZE, &u.u64, &len);
            if(!ret && len == 8)
            {
                u.u64 = fix_endian64be(u.u64);
                cprintf_field("    Total Size: ", "%llu B (%.3f %s)\n",
                    (unsigned long long)u.u64, get_size_natural(u.u32),
                    get_size_suffix(u.u32));
            }

            len = 4;
            ret = stmp_get_logical_drive_info(drive, SCSI_STMP_DRIVE_INFO_SIZE_MEGA, &u.u32, &len);
            if(!ret && len == 4)
            {
                u.u32 = fix_endian32be(u.u32);
                cprintf_field("    Total Size (MB): ", "%lu MB\n", (unsigned long)u.u32);
            }

            len = 8;
            ret = stmp_get_logical_drive_info(drive, SCSI_STMP_DRIVE_INFO_SECTOR_COUNT, &u.u64, &len);
            if(!ret && len == 8)
            {
                u.u64 = fix_endian64be(u.u64);
                cprintf_field("    Sector Count: ", "%llu\n", (unsigned long long)u.u64);
            }

            len = 4;
            ret = stmp_get_logical_drive_info(drive,SCSI_STMP_DRIVE_INFO_TYPE, &u.u32, &len);
            if(!ret && len == 4)
            {
                u.u32 = fix_endian32be(u.u32);
                cprintf_field("    Type: ", "%#x", u.u32);
                cprintf(RED, " (%s)\n", stmp_get_logical_drive_type_string(u.u32));
            }

            len = 1;
            ret = stmp_get_logical_drive_info(drive, SCSI_STMP_DRIVE_INFO_TAG, &u.u8, &len);
            if(!ret && len == 1)
            {
                cprintf_field("    Tag: ", "%#x", u.u8);
                cprintf(RED, " (%s)\n", stmp_get_logical_drive_tag_string(u.u8));
            }

            len = 52;
            ret = stmp_get_logical_drive_info(drive, SCSI_STMP_DRIVE_INFO_COMPONENT_VERSION, &u.buf, &len);
            if(!ret && len != 0)
            {
                cprintf(GREEN, "    Component Version:");
                print_hex(u.buf, len);
            }

            len = 52;
            ret = stmp_get_logical_drive_info(drive, SCSI_STMP_DRIVE_INFO_PROJECT_VERSION, &u.buf, &len);
            if(!ret && len != 0)
            {
                cprintf(GREEN, "    Project Version:");
                print_hex(u.buf, len);
            }

            len = 1;
            ret = stmp_get_logical_drive_info(drive, SCSI_STMP_DRIVE_INFO_IS_WRITE_PROTETED, &u.u8, &len);
            if(!ret && len == 1)
            {
                cprintf_field("    Is Writed Protected: ", "%d\n", u.u8);
            }

            len = 2;
            int serial_number_size = 0;
            ret = stmp_get_logical_drive_info(drive, SCSI_STMP_DRIVE_INFO_SERIAL_NUMBER_SIZE, &u.u16, &len);
            if(!ret && len == 2)
            {
                u.u16 = fix_endian16be(u.u16);
                cprintf_field("    Serial Number Size: ", "%d\n", u.u16);
                serial_number_size = u.u16;
            }

            len = serial_number_size;
            ret = stmp_get_logical_drive_info(drive, SCSI_STMP_DRIVE_INFO_SERIAL_NUMBER, &u.buf, &len);
            if(!ret && len != 0)
            {
                cprintf(GREEN, "    Serial Number:");
                print_hex(u.buf, len);
            }

            len = 1;
            ret = stmp_get_logical_drive_info(drive, SCSI_STMP_DRIVE_INFO_MEDIA_PRESENT, &u.u8, &len);
            if(!ret && len == 1)
            {
                cprintf_field("    Is Media Present: ", "%d\n", u.u8);
            }

            len = 1;
            ret = stmp_get_logical_drive_info(drive, SCSI_STMP_DRIVE_INFO_MEDIA_CHANGE, &u.u8, &len);
            if(!ret && len == 1)
            {
                cprintf_field("    Media Change: ", "%d\n", u.u8);
            }

            len = 4;
            ret = stmp_get_logical_drive_info(drive, SCSI_STMP_DRIVE_INFO_SECTOR_ALLOCATION, &u.u32, &len);
            if(!ret && len == 4)
            {
                u.u32 = fix_endian32be(u.u32);
                cprintf_field("    Sector Allocation: ", "%lu\n", (unsigned long)u.u32);
            }
        }
    }

    return 0;
}

void do_extract(const char *file)
{
    FILE *f = NULL;
    cprintf(BLUE, "Extracting firmware...\n");

    struct
    {
        struct scsi_stmp_logical_table_t header;
        struct scsi_stmp_logical_table_entry_t entry[20];
    }__attribute__((packed)) table;

    int ret = stmp_get_logical_table(&table.header, sizeof(table.entry) / sizeof(table.entry[0]));
    if(ret)
    {
        cprintf(GREY, "Cannot get logical table: %d\n", ret);
        goto Lend;
    }
    int entry = 0;
    while(entry < table.header.count)
        if(table.entry[entry].type == SCSI_STMP_DRIVE_TYPE_SYSTEM &&
                table.entry[entry].tag == SCSI_STMP_DRIVE_TAG_SYSTEM_BOOT)
            break;
        else
            entry++;
    if(entry == table.header.count)
    {
        cprintf(GREY, "Cannot find firmware partition\n");
        goto Lend;
    }
    uint8_t drive_no = table.entry[entry].drive_no;
    uint64_t drive_sz = table.entry[entry].size;
    if(g_debug)
    {
        cprintf(RED, "* ");
        cprintf_field("Drive: ", "%#x\n", drive_no);
        cprintf(RED, "* ");
        cprintf_field("Size: ", "%#llx\n", (unsigned long long)drive_sz);
    }
    int len = 4;
    uint32_t sector_size;
    ret = stmp_get_logical_drive_info(drive_no, SCSI_STMP_DRIVE_INFO_SECTOR_SIZE, &sector_size, &len);
    if(ret || len != 4)
    {
        cprintf(GREY, "Cannot get sector size\n");
        goto Lend;
    }
    sector_size = fix_endian32be(sector_size);
    if(g_debug)
    {
        cprintf(RED, "* ");
        cprintf_field("Sector size: ", "%lu\n", (unsigned long)sector_size);
    }
    uint8_t *sector = malloc(sector_size);
    len = sector_size;
    ret = stmp_read_logical_drive_sectors(drive_no, 0, 1, sector, &len);
    if(ret || len != (int)sector_size)
    {
        cprintf(GREY, "Cannot read first sector\n");
        return;
    }
    uint32_t fw_size = *(uint32_t *)(sector + 0x1c) * 16;
    if(g_debug)
    {
        cprintf(RED, "* ");
        cprintf_field("Firmware size: ", "%#x\n", fw_size);
    }

    f = fopen(file, "wb");
    if(f == NULL)
    {
        cprintf(GREY, "Cannot open '%s' for writing: %m\n", file);
        goto Lend;
    }

    for(int sec = 0; sec * sector_size < fw_size; sec++)
    {
        ret = stmp_read_logical_drive_sectors(drive_no, sec, 1, sector, &len);
        if(ret || len != (int)sector_size)
        {
            cprintf(GREY, "Cannot read sector %d\n", sec);
            goto Lend;
        }
        if(fwrite(sector, sector_size, 1, f) != 1)
        {
            cprintf(GREY, "Write failed: %m\n");
            goto Lend;
        }
    }
    cprintf(BLUE, "Done\n");
Lend:
    if(f)
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
    FILE *f = NULL;
    cprintf(BLUE, "Writing firmware...\n");

    struct
    {
        struct scsi_stmp_logical_table_t header;
        struct scsi_stmp_logical_table_entry_t entry[20];
    }__attribute__((packed)) table;

    int ret = stmp_get_logical_table(&table.header, sizeof(table.entry) / sizeof(table.entry[0]));
    if(ret)
    {
        cprintf(GREY, "Cannot get logical table: %d\n", ret);
        goto Lend;
    }
    int entry = 0;
    while(entry < table.header.count)
        if(table.entry[entry].type == SCSI_STMP_DRIVE_TYPE_SYSTEM &&
                table.entry[entry].tag == SCSI_STMP_DRIVE_TAG_SYSTEM_BOOT)
            break;
        else
            entry++;
    if(entry == table.header.count)
    {
        cprintf(GREY, "Cannot find firmware partition\n");
        goto Lend;
    }
    uint8_t drive_no = table.entry[entry].drive_no;
    uint64_t drive_sz = table.entry[entry].size;
    if(g_debug)
    {
        cprintf(RED, "* ");
        cprintf_field("Drive: ", "%#x\n", drive_no);
        cprintf(RED, "* ");
        cprintf_field("Size: ", "%#llx\n", (unsigned long long)drive_sz);
    }
    int len = 4;
    uint32_t sector_size;
    ret = stmp_get_logical_drive_info(drive_no, SCSI_STMP_DRIVE_INFO_SECTOR_SIZE, &sector_size, &len);
    if(ret || len != 4)
    {
        cprintf(GREY, "Cannot get sector size\n");
        goto Lend;
    }
    sector_size = fix_endian32be(sector_size);
    if(g_debug)
    {
        cprintf(RED, "* ");
        cprintf_field("Sector size: ", "%lu\n", (unsigned long)sector_size);
    }
    uint8_t *sector = malloc(sector_size);

    /* sanity check by reading first sector */
    len = sector_size;
    ret = stmp_read_logical_drive_sectors(drive_no, 0, 1, sector, &len);
    if(ret || len != (int)sector_size)
    {
        cprintf(GREY, "Cannot read first sector\n");
        return;
    }
    uint32_t sig = *(uint32_t *)(sector + 0x14);
    if(sig != 0x504d5453)
    {
        cprintf(GREY, "There is something wrong: the first sector doesn't have the STMP signature. Bailing out...\n");
        return;
    }

    f = fopen(file, "rb");
    if(f == NULL)
    {
        cprintf(GREY, "Cannot open '%s' for writing: %m\n", file);
        goto Lend;
    }
    fseek(f, 0, SEEK_END);
    int fw_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if(g_debug)
    {
        cprintf(RED, "* ");
        cprintf_field("Firmware size: ", "%#x\n", fw_size);
    }
    /* sanity check size */
    if((uint64_t)fw_size > drive_sz)
    {
        cprintf(GREY, "You cannot write a firmware greater than the partition size.\n");
        goto Lend;
    }

    int percent = -1;
    for(int off = 0; off < fw_size; off += sector_size)
    {
        int sec = off / sector_size;
        int this_percent = (sec * 100) / (fw_size / sector_size);
        if(this_percent != percent && (this_percent % 5) == 0)
        {
            cprintf(RED, "%d%%", this_percent);
            cprintf(YELLOW, "...");
            fflush(stdout);
        }
        percent = this_percent;
        int xfer_len = MIN(fw_size - off, (int)sector_size);
        if(fread(sector, xfer_len, 1, f) != 1)
        {
            cprintf(GREY, "Read failed: %m\n");
            goto Lend;
        }
        /* NOTE transfer a whole sector even if incomplete, the device won't access
         * partial sectors */
        if(xfer_len < (int)sector_size)
            memset(sector + xfer_len, 0, sector_size - xfer_len);
        len = sector_size;
        ret = stmp_write_logical_drive_sectors(drive_no, sec, 1, sector, &len);
        if(ret || len != (int)sector_size)
        {
            cprintf(GREY, "Cannot write sector %d\n", sec);
            goto Lend;
        }
    }
    cprintf(BLUE, "Done\n");
Lend:
    if(f)
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
    g_dev_fd = scsi_pt_open_device(argv[optind], false, true);
    if(g_dev_fd < 0)
    {
        cprintf(GREY, "Cannot open device: %m\n");
        ret = 1;
        goto Lend;
    }

    if(extract_fw)
        do_extract(extract_fw);
    if(info)
        do_info();
    if(write_fw)
        do_write(write_fw, g_yes_i_want_a_brick);

    scsi_pt_close_device(g_dev_fd);
Lend:
    color(OFF);

    return ret;
}

