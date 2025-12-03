/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Thomas Martitz
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
#ifndef _SDL_CODEC_H
#define _SDL_CODEC_H

#if (defined(HAVE_SDL_AUDIO) \
    && !(CONFIG_PLATFORM & PLATFORM_MAEMO5)) \
    || (CONFIG_PLATFORM & PLATFORM_CTRU)
AUDIOHW_SETTING(VOLUME,      "dB",   0,  1, -80,   0,   0)
#else
#define AUDIOHW_CAPS    (MONO_VOL_CAP)
AUDIOHW_SETTING(VOLUME,      "dB",   0,  1, -99,   0,   0)
#endif /* CONFIG_PLATFORM & PLATFORM_SDL */

#endif /* _SDL_CODEC_H */
