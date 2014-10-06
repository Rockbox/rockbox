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

#ifndef XRICK_CONFIG_H
#define XRICK_CONFIG_H

/* graphics (choose one) */
#define GFXST
#undef GFXPC

/* sound support */
#define ENABLE_SOUND

/* cheats support */
#define ENABLE_CHEATS

/* development tools */
#undef ENABLE_DEVTOOLS

/* Print debug info to screen */
#undef ENABLE_SYSPRINTF_TO_SCREEN

/* enable/disable subsystem debug */
#undef DEBUG_MEMORY
#undef DEBUG_ENTS
#undef DEBUG_SCROLLER
#undef DEBUG_MAPS
#undef DEBUG_JOYSTICK
#undef DEBUG_EVENTS
#undef DEBUG_AUDIO
#undef DEBUG_AUDIO2
#undef DEBUG_VIDEO
#undef DEBUG_VIDEO2

#endif /* ndef XRICK_CONFIG_H */

/* eof */

