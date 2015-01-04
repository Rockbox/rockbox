/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Bluetooth hardware abstraction layer
 *
 * Copyright (C) 2015 Lorenzo Miori
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
#ifndef __BLUETOOTH_H__
#define __BLUETOOTH_H__

#include "config.h"


/**
 * 
 * 
 * Bluetooth HAL (Hardware Abstraction Layer)
 * Management of basic controller parameters (power, reset, awake)
 * 
 * 
 */

// TODO rename all the function like bt_hal_XXX or so, to distinguish
// a more high-level bluetooth_init...which will aggregate every XXX_init()

/** Enumeration of possible bluetooth hardware states **/
enum
{
    BLUETOOTH_POWER_ON,
    BLUETOOTH_POWER_OFF,

    BLUETOOTH_AWAKE,

    BLUETOOTH_RESET_ON,
    BLUETOOTH_RESET_OFF,

    __BLUETOOTH_STATE_LAST
};

/* Initialize any target-specific hardware */
void bluetooth_init(void) INIT_ATTR;

/* Toggle soft-reset on the controller */
void bluetooth_reset(bool asserted);

/* Toggle power on the controller
 * "Power" refers to internal LDOs, baseband frequency
 * generators, etc.. i.e. more electronically related.
 */
void bluetooth_power(bool asserted);

/* Toggle Bluetooth controller awake state
 * This refers to a generic mechanism that allow the host
 * to tell the controller to remain awake; otherwise
 * the controller is allowed to put itself into a sleep mode
 */
void bluetooth_awake(bool asserted);

bool bluetooth_is_powered(void);

bool bluetooth_is_reset(void);

bool bluetooth_is_awake(void);

/* Debug screen for Bluetooth */
bool bluetooth_dbg_screen(void);

#endif /* __BLUETOOTH_H__ */
