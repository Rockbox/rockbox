/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: ata-target.h 11655 2006-12-03 22:13:44Z amiconn $
 *
 * Copyright (C) 2007 by Dave Chapman
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

/* Plain C read & write loops */

#define ATA_IOBASE      0x02400000

#define ATA_DATA        (*((volatile unsigned short*)(ATA_IOBASE)))
#define ATA_ERROR       (*((volatile unsigned char*)(ATA_IOBASE + 0x080)))
#define ATA_NSECTOR     (*((volatile unsigned char*)(ATA_IOBASE + 0x100)))
#define ATA_SECTOR      (*((volatile unsigned char*)(ATA_IOBASE + 0x180)))
#define ATA_LCYL        (*((volatile unsigned char*)(ATA_IOBASE + 0x200)))
#define ATA_HCYL        (*((volatile unsigned char*)(ATA_IOBASE + 0x280)))
#define ATA_SELECT      (*((volatile unsigned char*)(ATA_IOBASE + 0x300)))
#define ATA_CONTROL     (*((volatile unsigned char*)(ATA_IOBASE + 0x340)))
#define ATA_COMMAND     (*((volatile unsigned char*)(ATA_IOBASE + 0x380)))

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
void ata_enable(bool on);
bool ata_is_coldstart(void);
void ata_device_init(void);
