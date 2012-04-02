/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#ifndef __SAFE_READ__
#define __SAFE_READ__

#include "system.h"

/* Try to read an X-bit unsigned integer. If the address is not readable,
 * returns false. Otherwise returns true and store the result in *value
 * if value is not NULL */
bool safe_read8(uint8_t *addr, uint8_t *value);
bool safe_read16(uint16_t *addr, uint16_t *value);
bool safe_read32(uint32_t *addr, uint32_t *value);

#endif /* __SAFE_READ__ */