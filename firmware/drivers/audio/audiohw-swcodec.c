/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Thom Johansen
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
#include "config.h"
#include "system.h"
#include "sound.h"
#include "dsp_misc.h"

/* Linking audio hardware calls to SWCODEC DSP emulation */

static audiohw_swcodec_cb_type callback = NULL;

void audiohw_swcodec_set_callback(audiohw_swcodec_cb_type func)
{
    callback = func;
}

/** Functions exported by audiohw.h **/

void audiohw_set_channel(int value)
{
    callback(DSP_CALLBACK_SET_CHANNEL_CONFIG, value);
}

void audiohw_set_stereo_width(int value)
{
    callback(DSP_CALLBACK_SET_STEREO_WIDTH, value);
}

#ifdef HAVE_SW_TONE_CONTROLS
void audiohw_set_bass(int value)
{
    callback(DSP_CALLBACK_SET_BASS, value);
}

void audiohw_set_treble(int value)
{
    callback(DSP_CALLBACK_SET_TREBLE, value);
}
#endif /* HAVE_SW_TONE_CONTROLS */

#ifndef AUDIOHW_HAVE_PRESCALER
void audiohw_set_prescaler(int value)
{
    callback(DSP_CALLBACK_SET_PRESCALE, value);
}
#endif /* AUDIOHW_HAVE_PRESCALER */

#ifdef HAVE_PITCHCONTROL
void audiohw_set_pitch(int32_t value)
{
    callback(DSP_CALLBACK_SET_PITCH, value);
}

int32_t audiohw_get_pitch(void)
{
    return callback(DSP_CALLBACK_GET_PITCH, 0);
}
#endif /* HAVE_PITCHCONTROL */
