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

struct scsi_stmp_logical_table_t
{
    uint16_t count; /* big-endian */
} __attribute__((packed));

#define SCSI_STMP_MEDIA_INFO_NR_DRIVES          0
#define SCSI_STMP_MEDIA_INFO_SIZE               1 /* in bytes */
#define SCSI_STMP_MEDIA_INFO_ALLOC_UNIT_SIZE    2 /* in bytes */
#define SCSI_STMP_MEDIA_INFO_IS_INITIALISED     3
#define SCSI_STMP_MEDIA_INFO_STATE              4
#define SCSI_STMP_MEDIA_INFO_IS_WRITE_PROTECTED 5
#define SCSI_STMP_MEDIA_INFO_TYPE               6
#define SCSI_STMP_MEDIA_INFO_SERIAL_NUMBER_SIZE 7 /* in bytes */
#define SCSI_STMP_MEDIA_INFO_SERIAL_NUMBER      8
#define SCSI_STMP_MEDIA_INFO_IS_SYSTEM_MEDIA    9
#define SCSI_STMP_MEDIA_INFO_IS_MEDIA_PRESENT   10
#define SCSI_STMP_MEDIA_INFO_VENDOR             12

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
#define SCSI_STMP_DRIVe_INFO_ERASE_SIZE         1 /** Erase Size (bytes) */
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

#endif /* __STMP_SCSI__ */
