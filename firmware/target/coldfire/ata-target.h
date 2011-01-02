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

#define ATA_IOBASE      0x20000000
#define ATA_DATA        (*((volatile unsigned short*)(ATA_IOBASE + 0x20)))
#define ATA_CONTROL     (*((volatile unsigned short*)(ATA_IOBASE + 0x1c)))

#define ATA_ERROR       (*((volatile unsigned short*)(ATA_IOBASE + 0x22)))
#define ATA_NSECTOR     (*((volatile unsigned short*)(ATA_IOBASE + 0x24)))
#define ATA_SECTOR      (*((volatile unsigned short*)(ATA_IOBASE + 0x26)))
#define ATA_LCYL        (*((volatile unsigned short*)(ATA_IOBASE + 0x28)))
#define ATA_HCYL        (*((volatile unsigned short*)(ATA_IOBASE + 0x2a)))
#define ATA_SELECT      (*((volatile unsigned short*)(ATA_IOBASE + 0x2c)))
#define ATA_COMMAND     (*((volatile unsigned short*)(ATA_IOBASE + 0x2e)))

#define ATA_OUT8(reg,val) reg = ((val) << 8)
#define ATA_OUT16(reg,val) reg = swap16(val)
#define ATA_IN8(reg) ((reg) >> 8)
#define ATA_IN16(reg) (swap16(reg))

void ata_reset(void);
void ata_enable(bool on);
void ata_device_init(void);
bool ata_is_coldstart(void);

void copy_read_sectors(unsigned char* buf, int wordcount);
void copy_write_sectors(const unsigned char* buf, int wordcount);
#endif
