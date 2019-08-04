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
#ifdef HAVE_SW_TONE_CONTROLS
#include "tone_controls.h"
#endif
#include "channel_mode.h"
#include "dsp_misc.h"
#include "platform.h"
#include "dsp_core.h"
#include "dsp_sample_io.h"

/** Functions exported by audiohw.h but implemented in DSP **/

void audiohw_set_channel(int value)
{
    channel_mode_set_config(value);
}

void audiohw_set_stereo_width(int value)
{
    channel_mode_custom_set_width(value);
}

void audiohw_set_swap_channels(int value)
{
    dsp_sample_io_set_swap_channels(value);
}

#ifdef HAVE_SW_TONE_CONTROLS
void audiohw_set_bass(int value)
{
    tone_set_bass(value*10);
}

void audiohw_set_treble(int value)
{
    tone_set_treble(value*10);
}
#endif /* HAVE_SW_TONE_CONTROLS */

#ifndef AUDIOHW_HAVE_PRESCALER
void audiohw_set_prescaler(int value)
{
#ifdef HAVE_SW_TONE_CONTROLS
    tone_set_prescale(value);
#endif
    /* FIXME: Should PGA be used if HW tone controls but no HW prescaler?
       Callback-based implementation would have had no prescaling at all
       so just do nothing for now, changing nothing. */
    (void)value;    
}
#endif /* AUDIOHW_HAVE_PRESCALER */

#ifdef HAVE_PITCHCONTROL
void audiohw_set_pitch(int32_t value)
{
    dsp_set_pitch(value);
}

int32_t audiohw_get_pitch(void)
{
    return dsp_get_pitch();
}
#endif /* HAVE_PITCHCONTROL */
