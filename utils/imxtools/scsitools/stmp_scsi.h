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

#endif /* __STMP_SCSI__ */
