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

struct stmp_device_t
{
    rb_scsi_device_t dev;
    unsigned flags;
    void *user;
    stmp_printf_t printf;
};

static inline int little_endian(void)
{
    static int g_endian = -1;
    if(g_endian == -1)
    {
        int i = 1;
        g_endian = (int)*((unsigned char *)&i) == 1;
    }
    return g_endian;
}

uint16_t stmp_fix_endian16be(uint16_t w)
{
    return little_endian() ? w << 8 | w >> 8 : w;
}

uint32_t stmp_fix_endian32be(uint32_t w)
{
    return !little_endian() ? w :
        (uint32_t)stmp_fix_endian16be(w) << 16 | stmp_fix_endian16be(w >> 16);
}

uint64_t stmp_fix_endian64be(uint64_t w)
{
    return !little_endian() ? w :
        (uint64_t)stmp_fix_endian32be(w) << 32 | stmp_fix_endian32be(w >> 32);
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

static void misc_std_printf(void *user, const char *fmt, ...)
{
    (void) user;
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

#define stmp_printf(dev, ...) \
    dev->printf(dev->user, __VA_ARGS__)

#define stmp_debugf(dev, ...) \
    do{ if(dev->flags & STMP_DEBUG) stmp_printf(dev, __VA_ARGS__); }while(0)

stmp_device_t stmp_open(rb_scsi_device_t rdev, unsigned flags, void *user, stmp_printf_t printf)
{
    if(printf == NULL)
        printf = misc_std_printf;
    stmp_device_t sdev = malloc(sizeof(struct stmp_device_t));
    memset(sdev, 0, sizeof(struct stmp_device_t));
    sdev->dev = rdev;
    sdev->flags = flags;
    sdev->user = user;
    sdev->printf = printf;
    return sdev;
}

void stmp_close(stmp_device_t dev)
{
    free(dev);
}

/* returns <0 on error and status otherwise */
int stmp_scsi(stmp_device_t dev, uint8_t *cdb, int cdb_size, unsigned flags,
    void *sense, int *sense_size, void *buffer, int *buf_size)
{
    struct rb_scsi_raw_cmd_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.dir = RB_SCSI_NONE;
    if(flags & STMP_READ)
        cmd.dir = RB_SCSI_READ;
    if(flags & STMP_WRITE)
        cmd.dir = RB_SCSI_WRITE;
    cmd.cdb = cdb;
    cmd.cdb_len = cdb_size;
    cmd.sense = sense;
    cmd.sense_len = *sense_size;
    cmd.buf = buffer;
    cmd.buf_len = *buf_size;
    cmd.tmo = 1;
    int ret = rb_scsi_raw_xfer(dev->dev, &cmd);
    *sense_size = cmd.sense_len;
    *buf_size = cmd.buf_len;
    return ret == RB_SCSI_OK || ret == RB_SCSI_SENSE ? cmd.status : -ret;
}

int stmp_sense_analysis(stmp_device_t dev, int status, uint8_t *sense, int sense_size)
{
    if(status != 0 && (dev->flags & STMP_DEBUG))
    {
        stmp_printf(dev, "Status: %d\n", status);
        stmp_printf(dev, "Sense:");
        for(int i = 0; i < sense_size; i++)
            stmp_printf(dev, " %02x", sense[i]);
        stmp_printf(dev, "\n");
    }
    return status;
}

static int stmp_scsi_read_cmd(stmp_device_t dev, uint8_t cmd, uint8_t subcmd,
    uint8_t subsubcmd, void *buf, int *len)
{
    uint8_t cdb[16];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_STMP_READ;
    cdb[1] = cmd;
    cdb[2] = subcmd;
    cdb[3] = subsubcmd;

    uint8_t sense[32];
    int sense_size = sizeof(sense);

    int ret = stmp_scsi(dev, cdb, sizeof(cdb), STMP_READ, sense, &sense_size, buf, len);
    if(ret < 0)
        return ret;
    ret = stmp_sense_analysis(dev, ret, sense, sense_size);
    if(ret)
        return ret;
    return 0;
}

int stmp_scsi_inquiry(stmp_device_t dev, uint8_t *dev_type, char vendor[9], char product[17])
{
    unsigned char buffer[36];
    uint8_t cdb[10];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = 0x12;
    cdb[4] = sizeof(buffer);

    uint8_t sense[32];
    int sense_size = sizeof(sense);

    int buf_sz = sizeof(buffer);
    int ret = stmp_scsi(dev, cdb, sizeof(cdb), STMP_READ, sense, &sense_size, buffer, &buf_sz);
    if(ret < 0)
        return ret;
    ret = stmp_sense_analysis(dev, ret, sense, sense_size);
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

int stmp_scsi_get_protocol_version(stmp_device_t dev, void *buf, int *len)
{
    return stmp_scsi_read_cmd(dev, SCSI_STMP_CMD_GET_PROTOCOL_VERSION, 0, 0, buf, len);
}

int stmp_get_protocol_version(stmp_device_t dev, struct scsi_stmp_protocol_version_t *ver)
{
    int len = sizeof(*ver);
    int ret = stmp_scsi_get_protocol_version(dev, ver, &len);
    if(ret || len != sizeof(*ver))
        return -1;
    return 0;
}

int stmp_scsi_get_chip_major_rev_id(stmp_device_t dev, void *buf, int *len)
{
    return stmp_scsi_read_cmd(dev, SCSI_STMP_CMD_GET_CHIP_MAJOR_REV_ID, 0, 0, buf, len);
}

int stmp_get_chip_major_rev_id(stmp_device_t dev, uint16_t *ver)
{
    int len = sizeof(*ver);
    int ret = stmp_scsi_get_chip_major_rev_id(dev, ver, &len);
    if(ret || len != sizeof(*ver))
        return -1;
    *ver = stmp_fix_endian16be(*ver);
    return 0;
}

int stmp_scsi_get_rom_rev_id(stmp_device_t dev, void *buf, int *len)
{
    return stmp_scsi_read_cmd(dev, SCSI_STMP_CMD_GET_ROM_REV_ID, 0, 0, buf, len);
}

int stmp_get_rom_rev_id(stmp_device_t dev, uint16_t *ver)
{
    int len = sizeof(*ver);
    int ret = stmp_scsi_get_rom_rev_id(dev, ver, &len);
    if(ret || len != sizeof(*ver))
        return -1;
    *ver = stmp_fix_endian16be(*ver);
    return 0;
}

int stmp_scsi_get_logical_media_info(stmp_device_t dev, uint8_t info, void *data, int *len)
{
    return stmp_scsi_read_cmd(dev, SCSI_STMP_CMD_GET_LOGICAL_MEDIA_INFO, info, 0, data, len);
}

int stmp_scsi_get_logical_table(stmp_device_t dev, int entry_count, void *buf, int *len)
{
    return stmp_scsi_read_cmd(dev, SCSI_STMP_CMD_GET_LOGICAL_TABLE, entry_count, 0, buf, len);
}

int stmp_get_logical_media_table(stmp_device_t dev, struct stmp_logical_media_table_t **table)
{
    struct scsi_stmp_logical_table_header_t header;
    int len = sizeof(header);
    int ret = stmp_scsi_get_logical_table(dev, 0, &header, &len);
    if(ret || len != sizeof(header))
        return -1;
    header.count = stmp_fix_endian16be(header.count);
    int sz = sizeof(header) + header.count * sizeof(struct scsi_stmp_logical_table_entry_t);
    len = sz;
    *table = malloc(sz);
    ret = stmp_scsi_get_logical_table(dev, header.count, &(*table)->header, &len);
    if(ret || len != sz)
        return -1;
    (*table)->header.count = stmp_fix_endian16be((*table)->header.count);
    for(unsigned i = 0; i < (*table)->header.count; i++)
        (*table)->entry[i].size = stmp_fix_endian64be((*table)->entry[i].size);
    return 0;
}

int stmp_scsi_get_logical_drive_info(stmp_device_t dev, uint8_t drive, uint8_t info, void *data, int *len)
{
    return stmp_scsi_read_cmd(dev, SCSI_STMP_CMD_GET_LOGICAL_DRIVE_INFO, drive, info, data, len);
}

int stmp_scsi_get_device_info(stmp_device_t dev, uint8_t info, void *data, int *len)
{
    return stmp_scsi_read_cmd(dev, SCSI_STMP_CMD_GET_DEVICE_INFO, info, 0, data, len);
}

int stmp_scsi_get_serial_number(stmp_device_t dev, uint8_t info, void *data, int *len)
{
    return stmp_scsi_read_cmd(dev, SCSI_STMP_CMD_GET_CHIP_SERIAL_NUMBER, info, 0, data, len);
}

int stmp_scsi_read_logical_drive_sectors(stmp_device_t dev, uint8_t drive, uint64_t address,
    uint32_t count, void *buffer, int *buffer_size)
{
    uint8_t cdb[16];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_STMP_READ;
    cdb[1] = SCSI_STMP_CMD_READ_LOGICAL_DRIVE_SECTOR;
    cdb[2] = drive;
    address = stmp_fix_endian64be(address);
    memcpy(&cdb[3], &address, sizeof(address));
    count = stmp_fix_endian32be(count);
    memcpy(&cdb[11], &count, sizeof(count));

    uint8_t sense[32];
    int sense_size = sizeof(sense);

    int ret = stmp_scsi(dev, cdb, sizeof(cdb), STMP_READ, sense, &sense_size, buffer, buffer_size);
    if(ret < 0)
        return ret;
    return stmp_sense_analysis(dev, ret, sense, sense_size);
}

int stmp_scsi_write_logical_drive_sectors(stmp_device_t dev, uint8_t drive, uint64_t address,
    uint32_t count, void *buffer, int *buffer_size)
{
    uint8_t cdb[16];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_STMP_WRITE;
    cdb[1] = SCSI_STMP_CMD_WRITE_LOGICAL_DRIVE_SECTOR;
    cdb[2] = drive;
    address = stmp_fix_endian64be(address);
    memcpy(&cdb[3], &address, sizeof(address));
    count = stmp_fix_endian32be(count);
    memcpy(&cdb[11], &count, sizeof(count));

    uint8_t sense[32];
    int sense_size = sizeof(sense);

    int ret = stmp_scsi(dev, cdb, sizeof(cdb), STMP_WRITE, sense, &sense_size, buffer, buffer_size);
    if(ret < 0)
        return ret;
    return stmp_sense_analysis(dev, ret, sense, sense_size);
}

int stmp_read_logical_drive_sectors(stmp_device_t dev, uint8_t drive, uint64_t address,
    uint32_t count, void *buffer, int buffer_size)
{
    int len = buffer_size;
    int ret = stmp_scsi_read_logical_drive_sectors(dev, drive, address, count, buffer, &len);
    if(ret || len != buffer_size)
        return -1;
    return 0;
}

int stmp_write_logical_drive_sectors(stmp_device_t dev, uint8_t drive, uint64_t address,
    uint32_t count, void *buffer, int buffer_size)
{
    int len = buffer_size;
    int ret = stmp_scsi_write_logical_drive_sectors(dev, drive, address, count, buffer, &len);
    if(ret || len != buffer_size)
        return -1;
    return 0;
}

#define check_len(l) if(len != l) return -1;
#define fixn(n, p) *(uint##n##_t *)(p) = stmp_fix_endian##n##be(*(uint##n##_t *)(p))
#define fix16(p) fixn(16, p)
#define fix32(p) fixn(32, p)
#define fix64(p) fixn(64, p)

int stmp_fix_logical_media_info(uint8_t info, void *data, int len)
{
    switch(info)
    {
        case SCSI_STMP_MEDIA_INFO_NR_DRIVES:
            check_len(2);
            fix16(data);
            return 0;
        case SCSI_STMP_MEDIA_INFO_TYPE:
        case SCSI_STMP_MEDIA_INFO_SERIAL_NUMBER_SIZE:
        case SCSI_STMP_MEDIA_INFO_VENDOR:
        case SCSI_STMP_MEDIA_INFO_ALLOC_UNIT_SIZE:
        case SCSI_STMP_MEDIA_INFO_PAGE_SIZE:
        case SCSI_STMP_MEDIA_INFO_NR_DEVICES:
            check_len(4);
            fix32(data);
            return 0;
        case SCSI_STMP_MEDIA_INFO_SIZE:
        case SCSI_STMP_MEDIA_INFO_NAND_ID:
            check_len(8);
            fix64(data);
            return 0;
        case SCSI_STMP_MEDIA_INFO_IS_INITIALISED:
        case SCSI_STMP_MEDIA_INFO_STATE:
        case SCSI_STMP_MEDIA_INFO_IS_WRITE_PROTECTED:
        case SCSI_STMP_MEDIA_INFO_IS_SYSTEM_MEDIA:
        case SCSI_STMP_MEDIA_INFO_IS_MEDIA_PRESENT:
            check_len(1);
            return 0;
        case SCSI_STMP_MEDIA_INFO_SERIAL_NUMBER:
            return 0;
        default:
            return -1;
    }
}

static void stmp_fix_version(struct scsi_stmp_logical_drive_info_version_t *w)
{
    w->major = stmp_fix_endian16be(w->major);
    w->minor = stmp_fix_endian16be(w->minor);
    w->revision = stmp_fix_endian16be(w->revision);
}

int stmp_fix_logical_drive_info(uint8_t info, void *data, int len)
{
    switch(info)
    {
        case SCSI_STMP_DRIVE_INFO_SERIAL_NUMBER_SIZE:
            check_len(2);
            fix16(data);
            return 0;
        case SCSI_STMP_DRIVE_INFO_SECTOR_SIZE:
        case SCSI_STMP_DRIVE_INFO_ERASE_SIZE:
        case SCSI_STMP_DRIVE_INFO_SIZE_MEGA:
        case SCSI_STMP_DRIVE_INFO_TYPE:
        case SCSI_STMP_DRIVE_INFO_SECTOR_ALLOCATION:
            check_len(4);
            fix32(data);
            return 0;
        case SCSI_STMP_DRIVE_INFO_SIZE:
        case SCSI_STMP_DRIVE_INFO_SECTOR_COUNT:
            check_len(8);
            fix64(data);
            return 0;
        case SCSI_STMP_DRIVE_INFO_TAG:
        case SCSI_STMP_DRIVE_INFO_IS_WRITE_PROTETED:
        case SCSI_STMP_DRIVE_INFO_MEDIA_PRESENT:
        case SCSI_STMP_DRIVE_INFO_MEDIA_CHANGE:
            check_len(1);
            return 0;
        case SCSI_STMP_DRIVE_INFO_COMPONENT_VERSION:
        case SCSI_STMP_DRIVE_INFO_PROJECT_VERSION:
            check_len(6)
            stmp_fix_version(data);
            return 0;
        case SCSI_STMP_DRIVE_INFO_SERIAL_NUMBER:
            return 0;
        default:
            return -1;
    }
}

int stmp_fix_device_info(uint8_t info, void *data, int len)
{
    switch(info)
    {
        case 0: case 1:
            check_len(4);
            fix32(data);
            return 0;
        default:
            return -1;
    }
}
#undef fix64
#undef fix32
#undef fix16
#undef fixn
#undef checl_len

const char *stmp_get_logical_media_type_string(uint32_t type)
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

const char *stmp_get_logical_media_vendor_string(uint32_t type)
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

const char *stmp_get_logical_drive_type_string(uint32_t type)
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

const char *stmp_get_logical_drive_tag_string(uint32_t type)
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

const char *stmp_get_logical_media_state_string(uint8_t state)
{
    switch(state)
    {
        case SCSI_STMP_MEDIA_STATE_UNKNOWN: return "Unknown";
        case SCSI_STMP_MEDIA_STATE_ERASED: return "Erased";
        case SCSI_STMP_MEDIA_STATE_ALLOCATED: return "Allocated";
        default: return "?";
    }
}

int stmp_get_device_serial(stmp_device_t dev, uint8_t **buffer, int *len)
{
    *len = 2;
    uint16_t len16;
    int ret = stmp_scsi_get_serial_number(dev, 0, &len16, len);
    if(!ret && *len == 2)
    {
        len16 = stmp_fix_endian16be(len16);
        *len = len16;
        *buffer = malloc(*len);
        ret = stmp_scsi_get_serial_number(dev, 1, *buffer, len);
    }
    else
        ret = -1;
    return ret;
}

#define ARRAYLEN(x) (int)(sizeof(x)/sizeof((x)[0]))

int stmp_get_logical_media_info(stmp_device_t dev, struct stmp_logical_media_info_t *info)
{
    memset(info, 0, sizeof(struct stmp_logical_media_info_t));
    int len, ret;
#define entry(name, def) \
    len = sizeof(info->name); \
    ret = stmp_scsi_get_logical_media_info(dev, SCSI_STMP_MEDIA_INFO_##def, &info->name, &len); \
    if(!ret) \
        ret = stmp_fix_logical_media_info(SCSI_STMP_MEDIA_INFO_##def, &info->name, len); \
    if(!ret) \
        info->has.name = true;

    entry(nr_drives, NR_DRIVES);
    entry(size, SIZE);
    entry(alloc_size, ALLOC_UNIT_SIZE);
    entry(initialised, IS_INITIALISED);
    entry(state, STATE);
    entry(write_protected, IS_WRITE_PROTECTED);
    entry(type, TYPE);
    entry(serial_len, SERIAL_NUMBER_SIZE);
    entry(system, IS_SYSTEM_MEDIA);
    entry(present, IS_MEDIA_PRESENT);
    entry(page_size, PAGE_SIZE);
    entry(vendor, VENDOR);
    entry(nand_id, NAND_ID);
    entry(nr_devices, NR_DEVICES);
#undef entry
    if(info->has.serial_len)
    {
        info->serial = malloc(info->serial_len);
        int len = info->serial_len;
        ret = stmp_scsi_get_logical_media_info(dev, SCSI_STMP_MEDIA_INFO_SERIAL_NUMBER,
            info->serial, &len);
        if(ret || len != (int)info->serial_len)
            free(info->serial);
        else
            info->has.serial = true;
    }
    return 0;
}

int stmp_get_logical_drive_info(stmp_device_t dev, uint8_t drive, struct stmp_logical_drive_info_t *info)
{
    memset(info, 0, sizeof(struct stmp_logical_drive_info_t));
    int len, ret;
#define entry(name, def) \
    len = sizeof(info->name); \
    ret = stmp_scsi_get_logical_drive_info(dev, drive, SCSI_STMP_DRIVE_INFO_##def, &info->name, &len); \
    if(!ret) \
        ret = stmp_fix_logical_drive_info(SCSI_STMP_DRIVE_INFO_##def, &info->name, len); \
    if(!ret) \
        info->has.name = true;

    entry(sector_size, SECTOR_SIZE);
    entry(erase_size, ERASE_SIZE);
    entry(size, SIZE);
    entry(sector_count, SECTOR_COUNT);
    entry(type, TYPE);
    entry(tag, TAG);
    entry(component_version, COMPONENT_VERSION);
    entry(project_version, PROJECT_VERSION);
    entry(write_protected, IS_WRITE_PROTECTED);
    entry(serial_len, SERIAL_NUMBER_SIZE);
    entry(present, MEDIA_PRESENT);
    entry(change, MEDIA_CHANGE);
    entry(sector_alloc, SECTOR_ALLOCATION);
#undef entry
    if(info->has.serial_len)
    {
        info->serial = malloc(info->serial_len);
        int len = info->serial_len;
        ret = stmp_scsi_get_logical_media_info(dev, SCSI_STMP_DRIVE_INFO_SERIAL_NUMBER,
            info->serial, &len);
        if(ret || len != (int)info->serial_len)
            free(info->serial);
        else
            info->has.serial = true;
    }
    return 0;
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

void do_extract(const char *file)
{
    FILE *f = NULL;
    cprintf(BLUE, "Extracting firmware...\n");

    struct stmp_logical_media_table_t *table = NULL;
    int ret = stmp_get_logical_media_table(g_dev_fd, &table);
    if(ret)
    {
        cprintf(GREY, "Cannot get logical table: %d\n", ret);
        goto Lend;
    }
    int entry = 0;
    while(entry < table->header.count)
        if(table->entry[entry].type == SCSI_STMP_DRIVE_TYPE_SYSTEM &&
                table->entry[entry].tag == SCSI_STMP_DRIVE_TAG_SYSTEM_BOOT)
            break;
        else
            entry++;
    if(entry == table->header.count)
    {
        cprintf(GREY, "Cannot find firmware partition\n");
        goto Lend;
    }
    uint8_t drive_no = table->entry[entry].drive_no;
    uint64_t drive_sz = table->entry[entry].size;
    if(g_debug)
    {
        cprintf(RED, "* ");
        cprintf_field("Drive: ", "%#x\n", drive_no);
        cprintf(RED, "* ");
        cprintf_field("Size: ", "%#llx\n", (unsigned long long)drive_sz);
    }
    struct stmp_logical_drive_info_t info;
    ret = stmp_get_logical_drive_info(g_dev_fd, drive_no, &info);
    if(ret || !info.has.sector_size)
    {
        cprintf(GREY, "Cannot get sector size\n");
        goto Lend;
    }
    unsigned sector_size = info.sector_size;
    if(g_debug)
    {
        cprintf(RED, "* ");
        cprintf_field("Sector size: ", "%lu\n", (unsigned long)sector_size);
    }
    uint8_t *sector = malloc(sector_size);
    ret = stmp_read_logical_drive_sectors(g_dev_fd, drive_no, 0, 1, sector, sector_size);
    if(ret)
    {
        cprintf(GREY, "Cannot read first sector: %d\n", ret);
        goto Lend;
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
        ret = stmp_read_logical_drive_sectors(g_dev_fd, drive_no, sec, 1, sector, sector_size);
        if(ret)
        {
            cprintf(GREY, "Cannot read sector %d: %d\n", sec, ret);
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
    free(table);
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

    struct stmp_logical_media_table_t *table = NULL;
    int ret = stmp_get_logical_media_table(g_dev_fd, &table);
    if(ret)
    {
        cprintf(GREY, "Cannot get logical table: %d\n", ret);
        goto Lend;
    }
    int entry = 0;
    while(entry < table->header.count)
        if(table->entry[entry].type == SCSI_STMP_DRIVE_TYPE_SYSTEM &&
                table->entry[entry].tag == SCSI_STMP_DRIVE_TAG_SYSTEM_BOOT)
            break;
        else
            entry++;
    if(entry == table->header.count)
    {
        cprintf(GREY, "Cannot find firmware partition\n");
        goto Lend;
    }
    uint8_t drive_no = table->entry[entry].drive_no;
    uint64_t drive_sz = table->entry[entry].size;
    if(g_debug)
    {
        cprintf(RED, "* ");
        cprintf_field("Drive: ", "%#x\n", drive_no);
        cprintf(RED, "* ");
        cprintf_field("Size: ", "%#llx\n", (unsigned long long)drive_sz);
    }
    struct stmp_logical_drive_info_t info;
    ret = stmp_get_logical_drive_info(g_dev_fd, drive_no, &info);
    if(ret || !info.has.sector_size)
    {
        cprintf(GREY, "Cannot get sector size\n");
        goto Lend;
    }
    unsigned sector_size = info.sector_size;
    uint8_t *sector = malloc(sector_size);

    /* sanity check by reading first sector */
    ret = stmp_read_logical_drive_sectors(g_dev_fd, drive_no, 0, 1, sector, sector_size);
    if(ret)
    {
        cprintf(GREY, "Cannot read first sector: %d\n", ret);
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
        ret = stmp_write_logical_drive_sectors(g_dev_fd, drive_no, sec, 1, sector, sector_size);
        if(ret)
        {
            cprintf(GREY, "Cannot write sector %d: %d\n", sec, ret);
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
