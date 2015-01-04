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
#include "hwcompat.h"


/** Enumeration of possible bluetooth hardware states **/
enum
{
    BLUETOOTH_ON,
    BLUETOOTH_OFF,
    
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
void bluetooth_wake(bool asserted);

#endif /* __BLUETOOTH_H__ */
