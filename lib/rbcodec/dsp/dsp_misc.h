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
 * Copyright (C) 2005 Magnus Holmgren
 * Copyright (C) 2007 Thom Johansen
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
#ifndef DSP_MISC_H
#define DSP_MISC_H

/* Set the tri-pdf dithered output */
void dsp_dither_enable(bool enable); /* in dsp_sample_output.c */

/* Structure used with REPLAYGAIN_SET_GAINS message */
#define REPLAYGAIN_SET_GAINS (DSP_PROC_SETTING+DSP_PROC_MISC_HANDLER)
struct dsp_replay_gains
{
    long track_gain;
    long album_gain;
    long track_peak;
    long album_peak;
};

int get_replaygain_mode(bool have_track_gain, bool have_album_gain);
void dsp_set_replaygain(void);

#ifdef HAVE_PITCHSCREEN
void sound_set_pitch(int32_t ratio);
int32_t sound_get_pitch(void);
#endif /* HAVE_PITCHSCREEN */

/* Callback for firmware layers to interface */
int dsp_callback(int msg, intptr_t param);

#endif /* DSP_MISC_H */
