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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#define _BSD_SOURCE
#include <endian.h>
#include "stmp_scsi.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static int g_endian = -1;

struct stmp_device_t
{
    rb_scsi_device_t dev;
    unsigned flags;
    void *user;
    stmp_printf_t printf;
};

static inline int little_endian(void)
{
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

int stmp_read_firmware(stmp_device_t dev, void *user, stmp_fw_rw_fn_t fn)
{
    /* read logicial table */
    struct stmp_logical_media_table_t *table = NULL;
    int ret = stmp_get_logical_media_table(dev, &table);
    if(ret)
    {
        stmp_printf(dev, "Cannot get logical table: %d\n", ret);
        return -1;
    }
    /* locate firmware partition */
    int entry = 0;
    while(entry < table->header.count)
        if(table->entry[entry].type == SCSI_STMP_DRIVE_TYPE_SYSTEM &&
                table->entry[entry].tag == SCSI_STMP_DRIVE_TAG_SYSTEM_BOOT)
            break;
        else
            entry++;
    if(entry == table->header.count)
    {
        stmp_printf(dev, "Cannot find firmware partition\n");
        goto Lerr;
    }
    uint8_t drive_no = table->entry[entry].drive_no;
    uint64_t drive_sz = table->entry[entry].size;
    stmp_debugf(dev, "Firmware drive: %#x\n", drive_no);
    stmp_debugf(dev, "Firmware max size: %#llx\n", (unsigned long long)drive_sz);
    /* get drive info */
    struct stmp_logical_drive_info_t info;
    ret = stmp_get_logical_drive_info(dev, drive_no, &info);
    if(ret || !info.has.sector_size)
    {
        stmp_printf(dev, "Cannot get sector size\n");
        goto Lerr;
    }
    unsigned sector_size = info.sector_size;
    stmp_debugf(dev, "Firmware sector size: %lu\n", (unsigned long)sector_size);
    /* allocate a buffer for one sector */
    uint8_t *sector = malloc(sector_size);
    /* read the first sector to check it is correct and get the total size */
    ret = stmp_read_logical_drive_sectors(dev, drive_no, 0, 1, sector, sector_size);
    if(ret)
    {
        stmp_printf(dev, "Cannot read first sector: %d\n", ret);
        goto Lerr;
    }
    uint32_t sig = *(uint32_t *)(sector + 0x14);
    if(sig != 0x504d5453)
    {
        stmp_printf(dev, "There is something wrong: the first sector doesn't have the STMP signature.\n");
        goto Lerr;
    }
    uint32_t fw_size = *(uint32_t *)(sector + 0x1c) * 16; /* see SB file format */
    stmp_debugf(dev, "Firmware size: %#x\n", fw_size);
    /* if fn is NULL, just return the size immediately */
    if(fn == NULL)
        return fw_size;
    /* read all sectors one by one */
    for(int sec = 0; sec * sector_size < fw_size; sec++)
    {
        ret = stmp_read_logical_drive_sectors(dev, drive_no, sec, 1, sector, sector_size);
        if(ret)
        {
            stmp_printf(dev, "Cannot read sector %d: %d\n", sec, ret);
            goto Lerr;
        }
        int xfer_len = MIN(sector_size, fw_size - sec * sector_size);
        ret = fn(user, sector, xfer_len);
        if(ret != xfer_len)
        {
            stmp_printf(dev, "User write failed: %d\n", ret);
            goto Lerr;
        }
    }
    ret = fw_size;
Lend:
    free(table);
    return ret;
Lerr:
    ret = -1;
    goto Lend;
}

int stmp_write_firmware(stmp_device_t dev, void *user, stmp_fw_rw_fn_t fn)
{
    /* read logicial table */
    struct stmp_logical_media_table_t *table = NULL;
    int ret = stmp_get_logical_media_table(dev, &table);
    if(ret)
    {
        stmp_printf(dev, "Cannot get logical table: %d\n", ret);
        return -1;
    }
    /* locate firmware partition */
    int entry = 0;
    while(entry < table->header.count)
        if(table->entry[entry].type == SCSI_STMP_DRIVE_TYPE_SYSTEM &&
                table->entry[entry].tag == SCSI_STMP_DRIVE_TAG_SYSTEM_BOOT)
            break;
        else
            entry++;
    if(entry == table->header.count)
    {
        stmp_printf(dev, "Cannot find firmware partition\n");
        goto Lerr;
    }
    uint8_t drive_no = table->entry[entry].drive_no;
    uint64_t drive_sz = table->entry[entry].size;
    stmp_debugf(dev, "Firmware drive: %#x\n", drive_no);
    stmp_debugf(dev, "Firmware max size: %#llx\n", (unsigned long long)drive_sz);
    /* get drive info */
    struct stmp_logical_drive_info_t info;
    ret = stmp_get_logical_drive_info(dev, drive_no, &info);
    if(ret || !info.has.sector_size)
    {
        stmp_printf(dev, "Cannot get sector size\n");
        goto Lerr;
    }
    unsigned sector_size = info.sector_size;
    stmp_debugf(dev, "Firmware sector size: %lu\n", (unsigned long)sector_size);
    /* allocate a buffer for one sector */
    uint8_t *sector = malloc(sector_size);
    /* read the first sector to check it is correct and get the total size */
    ret = fn(user, sector, sector_size);
    /* the whole file could be smaller than one sector, but it must be greater
     * then the header size */
    if(ret < 0x20)
    {
        stmp_printf(dev, "User read failed: %d\n", ret);
        goto Lerr;
    }
    uint32_t sig = *(uint32_t *)(sector + 0x14);
    if(sig != 0x504d5453)
    {
        stmp_printf(dev, "There is something wrong: the first sector doesn't have the STMP signature.\n");
        goto Lerr;
    }
    uint32_t fw_size = *(uint32_t *)(sector + 0x1c) * 16; /* see SB file format */
    stmp_debugf(dev, "Firmware size: %#x\n", fw_size);
    /* write all sectors one by one */
    for(int sec = 0; sec * sector_size < fw_size; sec++)
    {
        int xfer_len = MIN(sector_size, fw_size - sec * sector_size);
        /* avoid rereading the first sector */
        if(sec != 0)
            ret = fn(user, sector, xfer_len);
        if(ret != xfer_len)
        {
            stmp_printf(dev, "User read failed: %d\n", ret);
            goto Lerr;
        }
        if(ret < (int)sector_size)
            memset(sector + ret, 0, sector_size - ret);
        ret = stmp_write_logical_drive_sectors(dev, drive_no, sec, 1, sector, sector_size);
        if(ret)
        {
            stmp_printf(dev, "Cannot write sector %d: %d\n", sec, ret);
            goto Lerr;
        }
    }
    ret = fw_size;
Lend:
    free(table);
    return ret;
Lerr:
    ret = -1;
    goto Lend;
}
