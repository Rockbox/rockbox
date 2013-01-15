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
#define SCSI_STMP_CMD_GET_PROTOCOL_VERSION      0
#define SCSI_STMP_CMD_GET_LOGICAL_MEDIA_INFO    2
#define SCSI_STMP_CMD_GET_LOGICAL_TABLE         5
#define SCSI_STMP_CMD_GET_LOGICAL_DRIVE_INFO    0x12
#define SCSI_STMP_CMD_GET_CHIP_MAJOR_REV_ID     0x30
#define SCSI_STMP_CMD_GET_ROM_REV_ID            0x37

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

struct scsi_stmp_logical_table_t
{
    uint16_t count; /* big-endian */
} __attribute__((packed));

#define SCSI_STMP_MEDIA_INFO_TYPE   6
#define SCSI_STMP_MEDIA_INFO_VENDOR 12

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

#define SCSI_STMP_DRIVE_INFO_SIZE   2
#define SCSI_STMP_DRIVE_INFO_TYPE   5

struct scsi_stmp_logical_drive_info_size_t
{
    uint64_t size; /* big-endian */
}  __attribute__((packed));

struct scsi_stmp_logical_drive_info_type_t
{
    uint8_t type;
}  __attribute__((packed));

#endif /* __STMP_SCSI__ */
