/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Maurus Cuelenaere
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

#include "ata.h"

/* DMA optimized reading and writing */
#define ATA_OPTIMIZED_READING
#define ATA_OPTIMIZED_WRITING
/*
#include "dma-target.h"
#define copy_read_sectors dma_ata_read
#define copy_write_sectors dma_ata_write
*/
void copy_read_sectors(const unsigned char* buf, int wordcount);
void copy_write_sectors(const unsigned char* buf, int wordcount);

/* Nasty hack, but Creative is nasty... */
#define ata_read_sectors _ata_read_sectors
#define ata_write_sectors _ata_write_sectors
extern int _ata_read_sectors(IF_MD2(int drive,) unsigned long start, int count, void* buf);
extern int _ata_write_sectors(IF_MD2(int drive,) unsigned long start, int count, const void* buf);

/* General purpose memory region #1 */
#define ATA_IOBASE      0x50FEE000
#define ATA_DATA        (*((volatile unsigned short*)(ATA_IOBASE)))
#define ATA_ERROR       (*((volatile unsigned char*)(ATA_IOBASE+0x2)))
#define ATA_NSECTOR     (*((volatile unsigned char*)(ATA_IOBASE+0x4)))
#define ATA_SECTOR      (*((volatile unsigned char*)(ATA_IOBASE+0x6)))
#define ATA_LCYL        (*((volatile unsigned char*)(ATA_IOBASE+0x8)))
#define ATA_HCYL        (*((volatile unsigned char*)(ATA_IOBASE+0xA)))
#define ATA_SELECT      (*((volatile unsigned char*)(ATA_IOBASE+0xC)))
#define ATA_COMMAND     (*((volatile unsigned char*)(ATA_IOBASE+0xE)))
#define ATA_CONTROL     (*((volatile unsigned char*)(ATA_IOBASE+0x800C)))

void ata_reset(void);
void ata_device_init(void);
bool ata_is_coldstart(void);
void ide_power_enable(bool on);
#ifdef BOOTLOADER
int load_minifs_file(char* filename, unsigned char* location);
#endif

#endif
