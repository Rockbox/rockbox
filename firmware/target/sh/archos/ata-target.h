/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Jens Arnold
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

/* asm optimised read & write loops */
#define ATA_OPTIMIZED_READING
#define ATA_OPTIMIZED_WRITING

#define SWAP_WORDS

#define ATA_IOBASE      0x06100100
#define ATA_DATA        (*((volatile unsigned short*)0x06104100))
#define ATA_CONTROL     (*ata_control)

#define ATA_ERROR       (*((volatile unsigned char*)ATA_IOBASE + 1))
#define ATA_NSECTOR     (*((volatile unsigned char*)ATA_IOBASE + 2))
#define ATA_SECTOR      (*((volatile unsigned char*)ATA_IOBASE + 3))
#define ATA_LCYL        (*((volatile unsigned char*)ATA_IOBASE + 4))
#define ATA_HCYL        (*((volatile unsigned char*)ATA_IOBASE + 5))
#define ATA_SELECT      (*((volatile unsigned char*)ATA_IOBASE + 6))
#define ATA_COMMAND     (*((volatile unsigned char*)ATA_IOBASE + 7))

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

extern volatile unsigned char* ata_control;

void ata_reset(void);
void ata_enable(bool on);
void ata_device_init(void);
bool ata_is_coldstart(void);

void copy_read_sectors(unsigned char* buf, int wordcount);
void copy_write_sectors(const unsigned char* buf, int wordcount);
#endif
