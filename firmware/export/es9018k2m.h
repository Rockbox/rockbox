/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 *
 * Copyright (c) 2023 Dana Conrad
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

#ifndef _ES9018K2M_H
#define _ES9018K2M_H

//======================================================================================
// ES9018K2M support stuff
// Implement audiohw_* functions in audiohw-*.c. These functions are utilities which
// may be used there.

// AUDIOHW_SETTING(VOLUME, *) not set here, probably best to put it in device-specific *_codec.h
#ifdef AUDIOHW_HAVE_SHORT_ROLL_OFF
AUDIOHW_SETTING(FILTER_ROLL_OFF, "", 0, 1, 0, 3, 0)
#endif

#ifndef ES9018K2M_VOLUME_MIN
# define ES9018K2M_VOLUME_MIN	-1270
#endif

#ifndef ES9018K2M_VOLUME_MAX
# define ES9018K2M_VOLUME_MAX	0
#endif

#define ES9018K2M_REG0_SYSTEM_SETTINGS      0
#define ES9018K2M_REG1_INPUT_CONFIG         1
#define ES9018K2M_REG4_AUTOMUTE_TIME        4
#define ES9018K2M_REG5_AUTOMUTE_LEVEL       5
#define ES9018K2M_REG6_DEEMPHASIS           6
#define ES9018K2M_REG7_GENERAL_SETTINGS     7
#define ES9018K2M_REG8_GPIO_CONFIG          8
#define ES9018K2M_REG10_MASTER_MODE_CTRL    10
#define ES9018K2M_REG11_CHANNEL_MAPPING     11
#define ES9018K2M_REG12_DPLL_SETTINGS       12
#define ES9018K2M_REG13_THD_COMP            13
#define ES9018K2M_REG14_SOFTSTART_SETTINGS  14
#define ES9018K2M_REG15_VOLUME_L            15
#define ES9018K2M_REG16_VOLUME_R            16
#define ES9018K2M_REG21_GPIO_INPUT_SELECT   21

/* writes volume levels to DAC over I2C, asynchronously */
void es9018k2m_set_volume_async(int vol_l, int vol_r);

/* write filter roll-off setting to DAC over I2C, synchronously */
void es9018k2m_set_filter_roll_off(int value);

/* writes a single register */
/* returns I2C_STATUS_OK upon success, I2C_STATUS_* errors upon error */
int es9018k2m_write_reg(uint8_t reg, uint8_t val);

/* reads a single register */
/* returns register value, or -1 upon error */
int es9018k2m_read_reg(uint8_t reg);

#endif