/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Miika Pekkarinen
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
#ifndef DSP_REPLAYGAIN_H
#define DSP_REPLAYGAIN_H

/* Structure used with DSP_SET_REPLAY_GAIN message */
struct dsp_replay_gains
{
    long track_gain;
    long album_gain;
    long track_peak;
    long album_peak;
};

/** DSP Interface **/
int get_replaygain_mode(bool have_track_gain, bool have_album_gain);
void dsp_set_replaygain(void);

extern const struct dsp_proc_db_entry dsp_replaygain_proc_db_entry;

#endif /* DSP_REPLAYGAIN_H */
