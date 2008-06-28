/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
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

#ifndef ATA_TARGET_H
#define ATA_TARGET_H

/* Plain C read & write loops */
#define PREFER_C_READING
#define PREFER_C_WRITING

#define ATA_IOBASE      0x50000000
#define REGISTER_OFFSET (ATA_IOBASE+0x00400000) /* A21 = High */
#define CONTROL_OFFSET  (ATA_IOBASE+0x00800000) /* A22 = High */
#define IDE_SHIFT 17
#define ATA_DATA        (*((volatile unsigned short*)(REGISTER_OFFSET + (0x00 << IDE_SHIFT))))
#define ATA_ERROR       (*((volatile unsigned char*)(REGISTER_OFFSET + (0x01 << IDE_SHIFT))))
#define ATA_NSECTOR     (*((volatile unsigned char*)(REGISTER_OFFSET + (0x02 << IDE_SHIFT))))
#define ATA_SECTOR      (*((volatile unsigned char*)(REGISTER_OFFSET + (0x03 << IDE_SHIFT))))
#define ATA_LCYL        (*((volatile unsigned char*)(REGISTER_OFFSET + (0x04 << IDE_SHIFT))))
#define ATA_HCYL        (*((volatile unsigned char*)(REGISTER_OFFSET + (0x05 << IDE_SHIFT))))
#define ATA_SELECT      (*((volatile unsigned char*)(REGISTER_OFFSET + (0x06 << IDE_SHIFT))))
#define ATA_COMMAND     (*((volatile unsigned char*)(REGISTER_OFFSET + (0x07 << IDE_SHIFT))))
#define ATA_CONTROL     (*((volatile unsigned char*)(CONTROL_OFFSET  + (0x06 << IDE_SHIFT))))

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
