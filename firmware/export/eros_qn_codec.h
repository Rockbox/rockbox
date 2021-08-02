/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 *
 * Copyright (c) 2021 Andrew Ryabinin, Dana Conrad
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

#ifndef _EROS_QN_CODEC_H
#define _EROS_QN_CODEC_H

#define PCM5102A_VOLUME_MIN -740
#define PCM5102A_VOLUME_MAX 0

/* a small DC offset appears to prevent play/pause clicking */
#define PCM_DC_OFFSET_VALUE -1

AUDIOHW_SETTING(VOLUME, "dB", 0, 2, PCM5102A_VOLUME_MIN/10, PCM5102A_VOLUME_MAX/10, 0)

/* this just calls audiohw_set_volume() with the last (locally) known volume,
 * used for switching to/from fixed line out volume. */
void pcm5102_set_outputs(void);

#endif
