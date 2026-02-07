/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#ifndef __ECHOPLAYER_CODEC_H__
#define __ECHOPLAYER_CODEC_H__

#include <stdbool.h>

/* -79 to 0 dB in 0.5 dB steps; software volume control
 * is used because the hardware volume controls "click"
 * when changing the volume */
AUDIOHW_SETTING(VOLUME, "dB", 1, 5, -790, 0, -200);

void audiohw_mute(bool mute);

#endif /* __ECHOPLAYER_CODEC_H__ */
