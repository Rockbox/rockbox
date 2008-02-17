/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef ATA_TARGET_H
#define ATA_TARGET_H

/* Plain C read & write loops */
#define PREFER_C_READING
#define PREFER_C_WRITING
#if 0
#if !defined(BOOTLOADER)
#define ATA_OPTIMIZED_WRITING
void copy_write_sectors(const unsigned char* buf, int wordcount);
#endif
#endif

#define ATA_DATA        ATA_DRIVE_DATA
#define ATA_ERROR       ATA_DRIVE_FEATURES
#define ATA_NSECTOR     ATA_DRIVE_SECTOR_COUNT
#define ATA_SECTOR      ATA_DRIVE_SECTOR_NUM
#define ATA_LCYL        ATA_DRIVE_CYL_LOW
#define ATA_HCYL        ATA_DRIVE_CYL_HIGH
#define ATA_SELECT      ATA_DRIVE_CYL_HEAD
#define ATA_COMMAND     ATA_DRIVE_COMMAND
#define ATA_CONTROL     ATA_DRIVE_CONTROL

#define STATUS_BSY      0x80
#define STATUS_RDY      0x40
#define STATUS_DF       0x20
#define STATUS_DRQ      0x08
#define STATUS_ERR      0x01
#define ERROR_ABRT      0x04

#define WRITE_PATTERN1 0xa5
#define WRITE_PATTERN2 0x5a
#define WRITE_PATTERN3 0xaa
#define WRITE_PATTERN4 0x55

#define READ_PATTERN1 0xa5
#define READ_PATTERN2 0x5a
#define READ_PATTERN3 0xaa
#define READ_PATTERN4 0x55

#define READ_PATTERN1_MASK 0xff
#define READ_PATTERN2_MASK 0xff
#define READ_PATTERN3_MASK 0xff
#define READ_PATTERN4_MASK 0xff

#define SET_REG(reg,val) reg = (val)
#define SET_16BITREG(reg,val) reg = (val)

void ata_reset(void);
void ata_device_init(void);
bool ata_is_coldstart(void);

#endif
