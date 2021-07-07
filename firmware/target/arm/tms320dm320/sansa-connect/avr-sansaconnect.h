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

/* HDQ (bq27000) RAM registers */
#define HDQ_REG_CTRL            0x00
#define HDQ_REG_MODE            0x01
#define HDQ_REG_AR              0x02
#define HDQ_REG_ARTTE           0x04
#define HDQ_REG_TEMP            0x06
#define HDQ_REG_VOLT            0x08
#define HDQ_REG_FLAGS           0x0A
#define HDQ_REG_RSOC            0x0B
#define HDQ_REG_NAC             0x0C
#define HDQ_REG_CACD            0x0E
#define HDQ_REG_CACT            0x10
#define HDQ_REG_LMD             0x12
#define HDQ_REG_AI              0x14
#define HDQ_REG_TTE             0x16
#define HDQ_REG_TTF             0x18
#define HDQ_REG_SI              0x1A
#define HDQ_REG_STTE            0x1C
#define HDQ_REG_MLI             0x1E
#define HDQ_REG_MLTTE           0x20
#define HDQ_REG_SAE             0x22
#define HDQ_REG_AP              0x24
#define HDQ_REG_TTECP           0x26
#define HDQ_REG_CYCL            0x28
#define HDQ_REG_CYCT            0x2A
#define HDQ_REG_CSOC            0x2C

void avr_hid_init(void);

int avr_hid_hdq_read_byte(uint8_t address);
int avr_hid_hdq_read_short(uint8_t address);

void avr_hid_enable_charger(void);

void avr_hid_wifi_pd(int high);

void avr_hid_lcm_sleep(void);
void avr_hid_lcm_wake(void);
void avr_hid_lcm_power_on(void);
void avr_hid_lcm_power_off(void);
void avr_hid_reset_codec(void);
void avr_hid_power_off(void);

#endif /* _AVR_SANSACONNECT_H_ */
