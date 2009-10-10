/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Rob Purchase
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
#ifndef PCF50635_H
#define PCF50635_H

#include "pcf5063x.h"

void pcf50635_init(void);
int pcf50635_write_multiple(int address, const unsigned char* buf, int count);
int pcf50635_write(int address, unsigned char val);
int pcf50635_read_multiple(int address, unsigned char* buf, int count);
int pcf50635_read(int address);

void pcf50635_read_adc(int adc, short* res1, short* res2);

#endif /* PCF50635_H */
