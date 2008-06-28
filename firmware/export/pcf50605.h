/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Dave Chapman
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

#ifndef PCF50605_H
#define PCF50605_H

extern unsigned char pcf50605_wakeup_flags;

int pcf50605_read(int address);
int pcf50605_read_multiple(int address, unsigned char* buf, int count);
int pcf50605_write(int address, unsigned char val);
int pcf50605_write_multiple(int address, const unsigned char* buf, int count);
int pcf50605_a2d_read(int channel);
bool pcf50605_charger_inserted(void);
void pcf50605_standby_mode(void);
void pcf50605_init(void);

#endif
