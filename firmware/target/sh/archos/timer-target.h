/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2005 Jens Arnold
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
#ifndef TIMER_TARGET_H
#define TIMER_TARGET_H

#include "config.h"

bool __timer_set(long cycles, bool start);
bool __timer_start(int int_prio);
void __timer_stop(void);

#define TIMER_FREQ CPU_FREQ

#define __TIMER_SET(cycles, set) \
    __timer_set(cycles, set)

#define __TIMER_START(int_prio) \
    __timer_start(int_prio)

#define __TIMER_STOP(...) \
    __timer_stop()

#endif /* TIMER_TARGET_H */
