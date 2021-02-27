/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __FIIOM3K_CODEC__
#define __FIIOM3K_CODEC__

#define AUDIOHW_CAPS (FILTER_ROLL_OFF_CAP)

/* TODO: this needs to be adjusted to make use of SW volume control */
AUDIOHW_SETTING(VOLUME, "dB", 0, 1, -13, 3, 0)
AUDIOHW_SETTING(FILTER_ROLL_OFF, "", 0, 1, 0, 3, 0)

#endif /* __FIIOM3K_CODEC__ */
