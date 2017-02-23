/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2016 by Amaury Pouly
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

#ifndef __NWZLINUX_CODEC_H__
#define __NWZLINUX_CODEC_H__

#define AUDIOHW_CAPS 0

/* Volume ranges from 0 to 31 (despite the ALSA control claiming 0..100 range).
 * It is not really in dB, but it's not really in percent either */
AUDIOHW_SETTING(VOLUME,       "dB", 0,  1, 0,  31, 0)

#endif /* __NWZLINUX_CODEC_H__ */
