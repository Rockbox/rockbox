/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
 
#include "config.h"
#include "jz4740.h"
#include "ata.h"

#define NAND_CMD_READ1_00                 0x00
#define NAND_CMD_READ1_01                 0x01
#define NAND_CMD_READ2                    0x50
#define NAND_CMD_READ_ID1                 0x90
#define NAND_CMD_READ_ID2                 0x91
#define NAND_CMD_RESET                    0xFF
#define NAND_CMD_PAGE_PROGRAM_START       0x80
#define NAND_CMD_PAGE_PROGRAM_STOP        0x10
#define NAND_CMD_BLOCK_ERASE_START        0x60
#define NAND_CMD_BLOCK_ERASE_CONFIRM      0xD0
#define NAND_CMD_READ_STATUS              0x70

#define NANDFLASH_CLE           0x00008000 //PA[15]
#define NANDFLASH_ALE           0x00010000 //PA[16]

#define NANDFLASH_BASE 0xB8000000
#define REG_NAND_DATA  (*((volatile unsigned char *) NANDFLASH_BASE))
#define REG_NAND_CMD   (*((volatile unsigned char *) (NANDFLASH_BASE + NANDFLASH_CLE)))
#define REG_NAND_ADDR  (*((volatile unsigned char *) (NANDFLASH_BASE + NANDFLASH_ALE)))

#define JZ_NAND_SET_CLE    (NANDFLASH_BASE |=  NANDFLASH_CLE)
#define JZ_NAND_CLR_CLE    (NANDFLASH_BASE &= ~NANDFLASH_CLE)
#define JZ_NAND_SET_ALE    (NANDFLASH_BASE |=  NANDFLASH_ALE)
#define JZ_NAND_CLR_ALE    (NANDFLASH_BASE &= ~NANDFLASH_ALE)

#define JZ_NAND_SELECT     (REG_EMC_NFCSR |=  EMC_NFCSR_NFCE1 )
#define JZ_NAND_DESELECT   (REG_EMC_NFCSR &= ~(EMC_NFCSR_NFCE1))

int ata_read_sectors(IF_MV2(int drive,) unsigned long start, int count, void* buf)
{
    (void)start;
    (void)count;
    (void)buf;
    return 0;
}

int ata_write_sectors(IF_MV2(int drive,) unsigned long start, int count, const void* buf)
{
    (void)start;
    (void)count;
    (void)buf;
    return 0;
}
