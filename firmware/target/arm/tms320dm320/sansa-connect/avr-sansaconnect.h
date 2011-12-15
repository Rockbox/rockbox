/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id: $
*
* Copyright (C) 2011 by Tomasz Moń
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

#ifndef _AVR_SANSACONNECT_H_
#define _AVR_SANSACONNECT_H_

#include "config.h"

void avr_hid_sync(void);
void avr_hid_init(void);

void avr_hid_enable_charger(void);

void avr_hid_lcm_sleep(void);
void avr_hid_lcm_wake(void);
void avr_hid_lcm_power_on(void);
void avr_hid_lcm_power_off(void);
void avr_hid_reset_codec(void);
void avr_hid_power_off(void);

#endif /* _AVR_SANSACONNECT_H_ */
