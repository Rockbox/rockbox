/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 Amaury Pouly
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
#ifndef __NWZ_TS_H__
#define __NWZ_TS_H__

#define NWZ_TS_NAME "icx_touch_screen"

/* How touchscreen works:
 *
 * The touchscreen uses mostly the standard linux protocol, reporting ABS_X,
 * ABS_Y, ABS_PRESSURE, TOOL_WIDTH and BTN_TOUCH with SYN to synchronize. The
 * only nonstandard part is the use of REL_RX and REL_RY to report "flick"
 * detection by the hardware. */

#endif /* __NWZ_TS_H__ */


