/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jens Arnold
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

#include "config.h"
#include "cpu.h"
#include "system.h"
#include "backlight.h"
#include "backlight-target.h"

bool _backlight_init(void)
{
    and_l(~0x00000008, &GPIO1_OUT);
    or_l(0x00000008, &GPIO1_ENABLE);
    or_l(0x00000008, &GPIO1_FUNCTION);
    return true; /* Backlight always ON after boot. */
}

void _backlight_on(void)
{
    and_l(~0x00000008, &GPIO1_OUT);
}

void _backlight_off(void)
{
    or_l(0x00000008, &GPIO1_OUT);
}
