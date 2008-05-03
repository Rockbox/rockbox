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

#define STATUS_BSY      0x8000
#define STATUS_RDY      0x4000
#define STATUS_DF       0x2000
#define STATUS_DRQ      0x0800
#define STATUS_ERR      0x0100

#define ERROR_ABRT      0x0400

#define WRITE_PATTERN1 0xa5
#define WRITE_PATTERN2 0x5a
#define WRITE_PATTERN3 0xaa
#define WRITE_PATTERN4 0x55

#define READ_PATTERN1 0xa500
#define READ_PATTERN2 0x5a00
#define READ_PATTERN3 0xaa00
#define READ_PATTERN4 0x5500

#define READ_PATTERN1_MASK 0xff00
#define READ_PATTERN2_MASK 0xff00
#define READ_PATTERN3_MASK 0xff00
#define READ_PATTERN4_MASK 0xff00

#define SET_REG(reg,val) reg = ((val) << 8)
#define SET_16BITREG(reg,val) reg = (val)

void ata_reset(void);
void ata_enable(bool on);
void ata_device_init(void);
bool ata_is_coldstart(void);

void copy_read_sectors(unsigned char* buf, int wordcount);
void copy_write_sectors(const unsigned char* buf, int wordcount);
#endif
