/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Module wrapper for SI4709 FM Radio Chip, using /dev/si470x (si4709.ko) of Samsung YP-R0
 *
 * Copyright (c) 2012 Lorenzo Miori
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

#ifndef __RADIO_YPR0_H__
#define __RADIO_YPR0_H__

#include "si4709.h"
#include "stdint.h"
#include "rds.h"
#include "si4700.h"

void radiodev_open(void);
void radiodev_close(void);
void si4709_write_reg(int addr, uint16_t value);
uint16_t si4709_read_reg(int addr);

#endif /*__RADIO-YPR0_H__*/