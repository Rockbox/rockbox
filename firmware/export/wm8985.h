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

#ifdef COWON_D2
/* FIXME: somehow something was out of sync in the .lang, settings and caps. Keep the
 * cutoffs disabled until someone with the device works it out. */
#define AUDIOHW_CAPS (BASS_CAP | TREBLE_CAP | LINEOUT_CAP | \
                      LIN_GAIN_CAP | MIC_GAIN_CAP)
#else
#define AUDIOHW_CAPS (BASS_CAP | TREBLE_CAP | BASS_CUTOFF_CAP | \
                      TREBLE_CUTOFF_CAP | LINEOUT_CAP | LIN_GAIN_CAP | \
                      MIC_GAIN_CAP)
AUDIOHW_SETTING(BASS_CUTOFF,   "",  0,  1,   1,   4,   1)
AUDIOHW_SETTING(TREBLE_CUTOFF, "",  0,  1,   1,   4,   1)
#endif

AUDIOHW_SETTING(VOLUME,      "dB",  0,  1, -90,   6, -25)
AUDIOHW_SETTING(BASS,        "dB",  0,  1, -12,  12,   0)
AUDIOHW_SETTING(TREBLE,      "dB",  0,  1, -12,  12,   0)
#ifdef HAVE_RECORDING
AUDIOHW_SETTING(LEFT_GAIN,   "dB",  1,  1,-128,  96,   0)
AUDIOHW_SETTING(RIGHT_GAIN,  "dB",  1,  1,-128,  96,   0)
AUDIOHW_SETTING(MIC_GAIN,    "dB",  1,  1,-128, 108,  16)
#endif /* HAVE_RECORDING */

void audiohw_set_aux_vol(int vol_l, int vol_r);

#endif /* _WM8985_H */
