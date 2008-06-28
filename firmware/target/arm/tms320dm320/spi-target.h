/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Catalin Patulea
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
 
#ifndef SPI_TARGET_H
#define SPI_TARGET_H

#include <inttypes.h>
#include <stdbool.h>

enum SPI_target {
#ifndef CREATIVE_ZVx
    SPI_target_TSC2100 = 0,
    SPI_target_RX5X348AB,
    SPI_target_BACKLIGHT,
#else
    SPI_target_LTV250QV = 0,
#endif
    SPI_MAX_TARGETS,
};

void spi_init(void);
int spi_block_transfer(enum SPI_target target,
                       const uint8_t *tx_bytes, unsigned int tx_size,
                             uint8_t *rx_bytes, unsigned int rx_size);

#endif
