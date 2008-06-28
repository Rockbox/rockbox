/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
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
#ifndef PCF50606_H
#define PCF50606_H

#include "pcf5060x.h"
#include "system.h"

/* The following functions may either be implemented by an optimised driver
   in the target tree, or by the generic drivers/pcf50606.c */

void pcf50606_init(void);
int pcf50606_write_multiple(int address, const unsigned char* buf, int count);
int pcf50606_write(int address, unsigned char val);
int pcf50606_read_multiple(int address, unsigned char* buf, int count);
int pcf50606_read(int address);
void pcf50606_set_usb_charging(bool on);
bool pcf50606_usb_charging_enabled(void);

/* internal low level calls used by the eeprom driver for h300 */
void pcf50606_i2c_init(void);
void pcf50606_i2c_recalc_delay(int cpu_clock);
void pcf50606_i2c_start(void);
void pcf50606_i2c_stop(void);
void pcf50606_i2c_ack(bool ack);
bool pcf50606_i2c_getack(void);
#if defined(IRIVER_H300_SERIES)
/* USB charging support */
void pcf50606_i2c_outb(unsigned char byte);
unsigned char pcf50606_i2c_inb(bool ack);
#endif

#if (defined(IAUDIO_X5) || defined(IAUDIO_M5)) && !defined (SIMULATOR)
void pcf50606_reset_timeout(void);
#endif

#endif /* PCF50606_H */
