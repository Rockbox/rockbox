/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: jackpot.c 14034 2007-07-28 05:42:55Z kevin $
 *
 * Copyright (C) 2007 Copyright Kévin Ferrare
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

#ifndef _DIGITAL_CLOCK_
#define _DIGITAL_CLOCK_
#include "plugin.h"
#include "clock.h"
#include "clock_counter.h"
#include "clock_settings.h"

void digital_clock_draw(struct screen* display, struct time* time,
                        struct clock_settings* settings,
                        struct counter* counter, int skin);

#endif /* _DIGITAL_CLOCK_ */
