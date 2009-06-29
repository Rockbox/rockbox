/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2006 Thom Johansen
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

/* FIXME : this header is PP specific */
#ifndef CPU_PP
#error "PP specific header"
#endif

bool __timer_set(long cycles, bool start);
bool __timer_start(IF_COP_VOID(int core));
void __timer_stop(void);

/* Portalplayer chips use a microsecond timer. */
#define TIMER_FREQ 1000000

#define __TIMER_SET(cycles, set) \
    __timer_set(cycles, set)

#if NUM_CORES > 1
#define __TIMER_START(core) \
    __timer_start(core)
#else
#define __TIMER_START() \
    __timer_start()
#endif

#define __TIMER_STOP(...) \
    __timer_stop()

#endif /* TIMER_TARGET_H */
