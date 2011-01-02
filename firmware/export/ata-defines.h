/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: ata.h 28951 2011-01-02 23:02:55Z theseven $
 *
 * Copyright (C) 2011 by Michael Sparmann
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
#ifndef __ATA_DEFINES_H__
#define __ATA_DEFINES_H__

#ifndef ATA_OUT8
#define ATA_OUT8(reg, data) (reg) = (data)
#endif
#ifndef ATA_OUT16
#define ATA_OUT16(reg, data) (reg) = (data)
#endif
#ifndef ATA_IN8
#define ATA_IN8(reg) (reg)
#endif
#ifndef ATA_IN16
#define ATA_IN16(reg) (reg)
#endif
#ifndef ATA_SWAP_IDENTIFY
#define ATA_SWAP_IDENTIFY(word) (word)
#endif

#define STATUS_BSY      0x80
#define STATUS_RDY      0x40
#define STATUS_DRQ      0x08
#define STATUS_ERR      0x01
#define STATUS_DF       0x20
#define ERROR_IDNF      0x10
#define ERROR_ABRT      0x04

#define TEST_PATTERN1   0xa5
#define TEST_PATTERN2   0x5a
#define TEST_PATTERN3   0xaa
#define TEST_PATTERN4   0x55

#define ATA_FEATURE     ATA_ERROR

#define ATA_STATUS      ATA_COMMAND
#define ATA_ALT_STATUS  ATA_CONTROL

#endif
