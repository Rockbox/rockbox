/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2009 by Michael Sevakis
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

/* timer is based on ipg_clk */
#define TIMER_FREQ (66000000)

bool _timer_set(long cycles, bool set);
bool _timer_start(void);
void _timer_stop(void);

#define __TIMER_SET(cycles, set) \
    _timer_set(cycles, set)

#define __TIMER_START() \
    _timer_start()

#define __TIMER_STOP(...) \
    _timer_stop()

#endif /* TIMER_TARGET_H */
