/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: ata-target.h 25525 2010-04-07 20:01:21Z torne $
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
#ifndef ATA_TARGET_H
#define ATA_TARGET_H

#include "inttypes.h"
#include "s5l8702.h"

#ifdef BOOTLOADER
#define ATA_DRIVER_CLOSE
#endif

#define ATA_SWAP_IDENTIFY(word) (swap16(word))

void ata_reset(void);
void ata_device_init(void);
bool ata_is_coldstart(void);
uint16_t ata_read_cbr(uint32_t volatile* reg);
void ata_write_cbr(uint32_t volatile* reg, uint16_t data);

#define ATA_OUT8(reg, data) ata_write_cbr(reg, data)
#define ATA_OUT16(reg, data) ata_write_cbr(reg, data)
#define ATA_IN8(reg) ata_read_cbr(reg)
#define ATA_IN16(reg) ata_read_cbr(reg)

#define ATA_SET_DEVICE_FEATURES
void ata_set_pio_timings(int mode);

#endif
