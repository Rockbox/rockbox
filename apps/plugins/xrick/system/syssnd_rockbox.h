 /***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Port of xrick, a Rick Dangerous clone, to Rockbox.
 * See http://www.bigorno.net/xrick/
 *
 * Copyright (C) 2008-2014 Pierluigi Vicinanza
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

#ifndef _SYSSND_ROCKBOX_H
#define _SYSSND_ROCKBOX_H

#include "xrick/config.h"

#ifdef ENABLE_SOUND

#include "xrick/system/system.h"

typedef struct {
    sound_t *sound;
    U8 *buf;
    U32 len;
    S8 loop;
} channel_t;

extern void syssnd_load(sound_t *);
extern void syssnd_unload(sound_t *);

#endif /* ENABLE_SOUND */

#endif /* ndef _SYSSND_ROCKBOX_H */

/* eof */
