/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Miika Pekkarinen
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

#ifndef _EEPROM_24CXX_H
#define _EEPROM_24CXX_H

#ifdef IRIVER_H300_SERIES
#define EEPROM_ADDR   0xA2
#define EEPROM_SIZE   256
#else
#define EEPROM_ADDR   0xA0
#define EEPROM_SIZE   128
#endif

void eeprom_24cxx_init(void);
int eeprom_24cxx_read(unsigned char address, void *dest, int length);
int eeprom_24cxx_write(unsigned char address, const void *src, int length);

#endif

