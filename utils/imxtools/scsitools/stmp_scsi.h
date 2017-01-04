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
#ifndef __STMP_SCSI__
#define __STMP_SCSI__

#include <stdint.h>
#include <stdbool.h>
#include "rbscsi.h"

/**
 * Low-Level SCSI stuff
 */

#define SCSI_STMP_READ                          0xc0
#define SCSI_STMP_WRITE                         0xc1
/** STMP: Command */
#define SCSI_STMP_CMD_GET_PROTOCOL_VERSION      0x0
#define SCSI_STMP_CMD_GET_LOGICAL_MEDIA_INFO    0x2
#define SCSI_STMP_CMD_GET_LOGICAL_TABLE         0x5
#define SCSI_STMP_CMD_ALLOCATE_LOGICAL_MEDIA    0x6
#define SCSI_STMP_CMD_ERASE LOGICAL MEDIA       0x7
#define SCSI_STMP_CMD_GET_LOGICAL_DRIVE_INFO    0x12
#define SCSI_STMP_CMD_READ_LOGICAL_DRIVE_SECTOR 0x13
#define SCSI_STMP_CMD_SET_LOGICAL_DRIVE_INFO    0x20
#define SCSI_STMP_CMD_WRITE_LOGICAL_DRIVE_SECTOR    0x23
#define SCSI_STMP_CMD_ERASE_LOGICAL_DRIVE       0x2f
#define SCSI_STMP_CMD_GET_CHIP_MAJOR_REV_ID     0x30
#define SCSI_STMP_CMD_CHIP_RESET                0x31
#define SCSI_STMP_CMD_GET_CHIP_SERIAL_NUMBER    0x32
#define SCSI_STMP_CMD_GET_ROM_REV_ID            0x37
#define SCSI_STMP_CMD_GET_JANUS_STATUS          0x40
#define SCSI_STMP_CMD_INITIALIZE_STATUS         0x41
#define SCSI_STMP_CMD_RESET_TO_RECOVERY         0x42
#define SCSI_STMP_CMD_INITIALIZE_DATA_STORE     0x43
#define SCSI_STMP_CMD_RESET_TO_UPDATER          0x44
#define SCSI_STMP_CMD_GET_DEVICE_INFO           0x45

struct scsi_stmp_protocol_version_t
{
    uint8_t major;
    uint8_t minor;
} __attribute__((packed));

struct scsi_stmp_rom_rev_id_t
{
    uint16_t rev; /* big-endian */
} __attribute__((packed));

struct scsi_stmp_chip_major_rev_id_t
{
    uint16_t rev; /* big-endian */
} __attribute__((packed));

struct scsi_stmp_logical_table_entry_t
{
    uint8_t drive_no;
    uint8_t type;
    uint8_t tag;
    uint64_t size; /* big-endian */
} __attribute__((packed));

#define SCSI_STMP_DRIVE_TYPE_USER   0
#define SCSI_STMP_DRIVE_TYPE_SYSTEM 1
#define SCSI_STMP_DRIVE_TYPE_JANUS  2

#define SCSI_STMP_DRIVE_TAG_USER_STORAGE    0xa
#define SCSI_STMP_DRIVE_TAG_SYSTEM_BOOT     0x50

struct scsi_stmp_logical_table_header_t
{
    uint16_t count; /* big-endian */
} __attribute__((packed));

#define SCSI_STMP_MEDIA_INFO_NR_DRIVES          0 /** Number of drives (obsolete) */
#define SCSI_STMP_MEDIA_INFO_SIZE               1 /** Total size (bytes) */
#define SCSI_STMP_MEDIA_INFO_ALLOC_UNIT_SIZE    2 /** Allocation unit size (bytes) */
#define SCSI_STMP_MEDIA_INFO_IS_INITIALISED     3 /** Is initialised ? */
#define SCSI_STMP_MEDIA_INFO_STATE              4 /** Media state */
#define SCSI_STMP_MEDIA_INFO_IS_WRITE_PROTECTED 5 /** Is write protected ? */
#define SCSI_STMP_MEDIA_INFO_TYPE               6 /** Physical media type */
#define SCSI_STMP_MEDIA_INFO_SERIAL_NUMBER_SIZE 7 /** Serial number size (bytes) */
#define SCSI_STMP_MEDIA_INFO_SERIAL_NUMBER      8 /** Serial number */
#define SCSI_STMP_MEDIA_INFO_IS_SYSTEM_MEDIA    9 /** Is system media ? */
#define SCSI_STMP_MEDIA_INFO_IS_MEDIA_PRESENT   10 /** Is media present ? */
#define SCSI_STMP_MEDIA_INFO_PAGE_SIZE          11 /** Page size (bytes) */
#define SCSI_STMP_MEDIA_INFO_VENDOR             12 /** Vendor ID */
#define SCSI_STMP_MEDIA_INFO_NAND_ID            13 /** Full NAND ID */
#define SCSI_STMP_MEDIA_INFO_NR_DEVICES         14 /** Number of physical devices */

#define SCSI_STMP_MEDIA_STATE_UNKNOWN   0
#define SCSI_STMP_MEDIA_STATE_ERASED    1
#define SCSI_STMP_MEDIA_STATE_ALLOCATED 2

#define SCSI_STMP_MEDIA_TYPE_NAND   0
#define SCSI_STMP_MEDIA_TYPE_SDMMC  1
#define SCSI_STMP_MEDIA_TYPE_HDD    2
#define SCSI_STMP_MEDIA_TYPE_RAM    3
#define SCSI_STMP_MEDIA_TYPE_iNAND  4

#define SCSI_STMP_MEDIA_VENDOR_SAMSUNG  0xEC
#define SCSI_STMP_MEDIA_VENDOR_STMICRO  0x20
#define SCSI_STMP_MEDIA_VENDOR_HYNIX    0xAD
#define SCSI_STMP_MEDIA_VENDOR_MICRON   0x2C
#define SCSI_STMP_MEDIA_VENDOR_TOSHIBA  0x98
#define SCSI_STMP_MEDIA_VENDOR_RENESAS  0x07
#define SCSI_STMP_MEDIA_VENDOR_SANDISK  0x45
#define SCSI_STMP_MEDIA_VENDOR_INTEL    0x89

struct scsi_stmp_logical_media_info_type_t
{
    uint8_t type;
}  __attribute__((packed));

struct scsi_stmp_logical_media_info_manufacturer_t
{
    uint32_t type; /* big-endian */
}  __attribute__((packed));

#define SCSI_STMP_DRIVE_INFO_SECTOR_SIZE        0 /** Sector Size (bytes) */
#define SCSI_STMP_DRIVE_INFO_ERASE_SIZE         1 /** Erase Size (bytes) */
#define SCSI_STMP_DRIVE_INFO_SIZE               2 /** Total Size (bytes) */
#define SCSI_STMP_DRIVE_INFO_SIZE_MEGA          3 /** Total Size (mega-bytes) */
#define SCSI_STMP_DRIVE_INFO_SECTOR_COUNT       4 /** Sector Count */
#define SCSI_STMP_DRIVE_INFO_TYPE               5 /** Drive Type */
#define SCSI_STMP_DRIVE_INFO_TAG                6 /** Drive Tag */
#define SCSI_STMP_DRIVE_INFO_COMPONENT_VERSION  7 /** Component Version */
#define SCSI_STMP_DRIVE_INFO_PROJECT_VERSION    8 /** Project Version */
#define SCSI_STMP_DRIVE_INFO_IS_WRITE_PROTETED  9 /** Is Write Protected */
#define SCSI_STMP_DRIVE_INFO_SERIAL_NUMBER_SIZE 10 /** Serial Number Size */
#define SCSI_STMP_DRIVE_INFO_SERIAL_NUMBER      11 /** Serial Number */
#define SCSI_STMP_DRIVE_INFO_MEDIA_PRESENT      12 /** Is Media Present */
#define SCSI_STMP_DRIVE_INFO_MEDIA_CHANGE       13 /** Media Change */
#define SCSI_STMP_DRIVE_INFO_SECTOR_ALLOCATION  14 /** Sector Allocation */

#define SCSI_STMP_DRIVE_TYPE_DATA       0
#define SCSI_STMP_DRIVE_TYPE_SYSTEM     1
#define SCSI_STMP_DRIVE_TYPE_HIDDEN     2
#define SCSI_STMP_DRIVE_TYPE_UNKNOWN    3

#define SCSI_STMP_DRIVE_TAG_STMPSYS_S       0x00 /** Player drive */
#define SCSI_STMP_DRIVE_TAG_HOSTLINK_S      0x01 /** USB MSC/MTP drive */
#define SCSI_STMP_DRIVE_TAG_RESOURCE_BIN    0x02 /** Resource drive */
#define SCSI_STMP_DRIVE_TAG_EXTRA_S         0x03 /** Extra system drive */
#define SCSI_STMP_DRIVE_TAG_RESOURCE1_BIN   0x04 /** Extra resource drive */
#define SCSI_STMP_DRIVE_TAG_OTGHOST_S       0x05 /** OTG drive */
#define SCSI_STMP_DRIVE_TAG_HOSTRSC_BIN     0x06 /** USB MSC/MTP resource drive */
#define SCSI_STMP_DRIVE_TAG_DATA            0x0a /** Data drive */
#define SCSI_STMP_DRIVE_TAG_HIDDEN          0x0b /** Hidden data drive */
#define SCSI_STMP_DRIVE_TAG_BOOTMANAGER_S   0x50 /** Boot manager drive */
#define SCSI_STMP_DRIVE_TAG_UPDATER_S       0xff /** Recovery drive */

struct scsi_stmp_logical_drive_info_sector_t
{
    uint32_t size; /* big-endian */
}  __attribute__((packed));

struct scsi_stmp_logical_drive_info_count_t
{
    uint64_t count; /* big-endian */
}  __attribute__((packed));

struct scsi_stmp_logical_drive_info_size_t
{
    uint64_t size; /* big-endian */
}  __attribute__((packed));

struct scsi_stmp_logical_drive_info_type_t
{
    uint8_t type;
}  __attribute__((packed));

struct scsi_stmp_logical_drive_info_version_t
{
    uint16_t major;
    uint16_t minor;
    uint16_t revision;
}  __attribute__((packed));

struct stmp_device_t;
typedef struct stmp_device_t *stmp_device_t;

typedef void (*stmp_printf_t)(void *user, const char *fmt, ...);

/* open flags */
#define STMP_DEBUG  (1 << 0)
/* scsi flags */
#define STMP_READ   (1 << 1)
#define STMP_WRITE  (1 << 2)

uint16_t stmp_fix_endian16be(uint16_t w);
uint32_t stmp_fix_endian32be(uint32_t w);
uint64_t stmp_fix_endian64be(uint64_t w);
/* returns NULL on error */
stmp_device_t stmp_open(rb_scsi_device_t dev, unsigned flags, void *user, stmp_printf_t printf);
void stmp_close(stmp_device_t dev);
/* returns <0 on error and status otherwise */
int stmp_scsi(stmp_device_t dev, uint8_t *cdb, int cdb_size, unsigned flags,
    void *sense, int *sense_size, void *buffer, int *buf_size);
/* returns != 0 on error */
int stmp_sense_analysis(stmp_device_t dev, int status, uint8_t *sense, int sense_size);
void stmp_printf(rb_scsi_device_t dev, const char *fmt, ...);
void stmp_debugf(rb_scsi_device_t dev, const char *fmt, ...);

/**
 * Mid-level API
 */

/* returns !=0 on error */
int stmp_scsi_inquiry(stmp_device_t dev, uint8_t *dev_type, char vendor[9], char product[17]);

int stmp_scsi_get_protocol_version(stmp_device_t dev, void *buf, int *len);
int stmp_scsi_get_chip_major_rev_id(stmp_device_t dev, void *buf, int *len);
int stmp_scsi_get_rom_rev_id(stmp_device_t dev, void *buf, int *len);
int stmp_scsi_read_logical_drive_sectors(stmp_device_t dev, uint8_t drive, uint64_t address,
    uint32_t count, void *buffer, int *buffer_size);
int stmp_scsi_write_logical_drive_sectors(stmp_device_t dev, uint8_t drive, uint64_t address,
    uint32_t count, void *buffer, int *buffer_size);
/* the following functions *DO NOT* fix the endianness of the structures */
int stmp_scsi_get_logical_table(stmp_device_t dev, int entry_count, void *buf, int *len);
int stmp_scsi_get_serial_number(stmp_device_t dev, uint8_t info, void *data, int *len);
int stmp_scsi_get_logical_media_info(stmp_device_t dev, uint8_t info, void *data, int *len);
int stmp_scsi_get_logical_drive_info(stmp_device_t dev, uint8_t drive, uint8_t info, void *data, int *len);
int stmp_scsi_get_device_info(stmp_device_t dev, uint8_t info, void *data, int *len);
/* these helper functions fix the endianness for the previous calls, or returns != 0
 * if they don't know about this info or the size doesn't match */
int stmp_fix_logical_media_info(uint8_t info, void *data, int len);
int stmp_fix_logical_drive_info(uint8_t info, void *data, int len);
int stmp_fix_device_info(uint8_t info, void *data, int len);

/**
 * High-Level API
 */

struct stmp_logical_media_table_t
{
    struct scsi_stmp_logical_table_header_t header;
    struct scsi_stmp_logical_table_entry_t entry[];
}__attribute__((packed)) table;

struct stmp_logical_media_info_t
{
    struct
    {
        bool nr_drives;
        bool size;
        bool alloc_size;
        bool initialised;
        bool state;
        bool write_protected;
        bool type;
        bool serial_len;
        bool serial;
        bool system;
        bool present;
        bool page_size;
        bool vendor;
        bool nand_id;
        bool nr_devices;
    }has;
    uint16_t nr_drives;
    uint64_t size;
    uint32_t alloc_size;
    uint8_t initialised;
    uint8_t state;
    uint8_t write_protected;
    uint8_t type;
    uint32_t serial_len;
    uint8_t *serial; /* must be released with free() */
    uint8_t system;
    uint8_t present;
    uint32_t page_size;
    uint32_t vendor;
    uint8_t nand_id[6];
    uint32_t nr_devices;
};

#define SCSI_STMP_DRIVE_INFO_SECTOR_SIZE        0 /** Sector Size (bytes) */
#define SCSI_STMP_DRIVE_INFO_ERASE_SIZE         1 /** Erase Size (bytes) */
#define SCSI_STMP_DRIVE_INFO_SIZE               2 /** Total Size (bytes) */
#define SCSI_STMP_DRIVE_INFO_SIZE_MEGA          3 /** Total Size (mega-bytes) */
#define SCSI_STMP_DRIVE_INFO_SECTOR_COUNT       4 /** Sector Count */
#define SCSI_STMP_DRIVE_INFO_TYPE               5 /** Drive Type */
#define SCSI_STMP_DRIVE_INFO_TAG                6 /** Drive Tag */
#define SCSI_STMP_DRIVE_INFO_COMPONENT_VERSION  7 /** Component Version */
#define SCSI_STMP_DRIVE_INFO_PROJECT_VERSION    8 /** Project Version */
#define SCSI_STMP_DRIVE_INFO_IS_WRITE_PROTECTED 9 /** Is Write Protected */
#define SCSI_STMP_DRIVE_INFO_SERIAL_NUMBER_SIZE 10 /** Serial Number Size */
#define SCSI_STMP_DRIVE_INFO_SERIAL_NUMBER      11 /** Serial Number */
#define SCSI_STMP_DRIVE_INFO_MEDIA_PRESENT      12 /** Is Media Present */
#define SCSI_STMP_DRIVE_INFO_MEDIA_CHANGE       13 /** Media Change */
#define SCSI_STMP_DRIVE_INFO_SECTOR_ALLOCATION  14 /** Sector Allocation */

struct stmp_logical_drive_info_t
{
    struct
    {
        bool sector_size;
        bool erase_size;
        bool size;
        bool sector_count;
        bool type;
        bool tag;
        bool component_version;
        bool project_version;
        bool write_protected;
        bool serial_len;
        bool serial;
        bool present;
        bool change;
        bool sector_alloc;
    }has;
    uint32_t sector_size;
    uint32_t erase_size;
    uint64_t size;
    uint64_t sector_count;
    uint32_t type;
    uint8_t tag;
    struct scsi_stmp_logical_drive_info_version_t component_version;
    struct scsi_stmp_logical_drive_info_version_t project_version;
    uint8_t write_protected;
    uint32_t serial_len;
    uint8_t *serial;
    uint8_t present;
    uint8_t change;
    uint32_t sector_alloc;
};

int stmp_get_protocol_version(stmp_device_t dev, struct scsi_stmp_protocol_version_t *ver);
int stmp_get_chip_major_rev_id(stmp_device_t dev, uint16_t *ver);
int stmp_get_rom_rev_id(stmp_device_t dev, uint16_t *ver);
/* return 0 on success, buffers must be released with free() */
int stmp_get_device_serial(stmp_device_t dev, uint8_t **buffer, int *len);
int stmp_get_logical_media_info(stmp_device_t dev, struct stmp_logical_media_info_t *info);
int stmp_get_logical_media_table(stmp_device_t dev, struct stmp_logical_media_table_t **table);
int stmp_get_logical_drive_info(stmp_device_t dev, uint8_t drive, struct stmp_logical_drive_info_t *info);
int stmp_read_logical_drive_sectors(stmp_device_t dev, uint8_t drive, uint64_t address,
    uint32_t count, void *buffer, int buffer_size);
int stmp_write_logical_drive_sectors(stmp_device_t dev, uint8_t drive, uint64_t address,
    uint32_t count, void *buffer, int buffer_size);
/* return <0 on error, or firmware size in bytes otherwise,
 * if not NULL, the read/write function will be called as many times as needed to provide
 * the entire firmware, it should return number of bytes read/written on success or -1 on error
 * in all cases, the total size of the firmware is based on the header
 * if NULL for read, return firmware size */
typedef int (*stmp_fw_rw_fn_t)(void *user, void *buf, size_t size);
int stmp_read_firmware(stmp_device_t dev, void *user, stmp_fw_rw_fn_t fn);
int stmp_write_firmware(stmp_device_t dev, void *user, stmp_fw_rw_fn_t fn);
/* string helpers */
const char *stmp_get_logical_media_type_string(uint32_t type);
const char *stmp_get_logical_media_vendor_string(uint32_t type);
const char *stmp_get_logical_drive_type_string(uint32_t type);
const char *stmp_get_logical_drive_tag_string(uint32_t type);
const char *stmp_get_logical_media_state_string(uint8_t state);

#endif /* __STMP_SCSI__ */
