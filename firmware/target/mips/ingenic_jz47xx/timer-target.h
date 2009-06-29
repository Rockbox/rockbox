/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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

#ifndef __TIMER_H_
#define __TIMER_H_

#include "config.h"

#define TIMER_FREQ (CFG_EXTAL) /* For full precision! */

bool __timer_set(long cycles, bool set);
bool __timer_start(void);
void __timer_stop(void);

#define __TIMER_SET(cycles, set) \
    __timer_set(cycles, set)

#define __TIMER_START(int_prio) \
    __timer_start()

#define __TIMER_STOP(...) \
    __timer_stop()

#endif /* __TIMER_H_ */
