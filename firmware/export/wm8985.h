/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Dave Chapman
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

#ifndef _WM8985_H
#define _WM8985_H

/* volume/balance/treble/bass interdependency */
#define VOLUME_MIN -570
#define VOLUME_MAX  60

#define AUDIOHW_CAPS (BASS_CAP | TREBLE_CAP | BASS_CUTOFF_CAP | TREBLE_CUTOFF_CAP)

extern int tenthdb2master(int db);

extern void audiohw_set_headphone_vol(int vol_l, int vol_r);
extern void audiohw_set_lineout_vol(int vol_l, int vol_r);

#endif /* _WM8985_H */
