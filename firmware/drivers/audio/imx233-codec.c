/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "audiohw.h"
#include "audio.h"
#include "audioout-imx233.h"
#include "audioin-imx233.h"
#include "audio-imx233.h"

void audiohw_preinit(void)
{
    imx233_audioout_preinit();
    imx233_audioin_preinit();
    imx233_audio_preinit();
}

void audiohw_postinit(void)
{
    imx233_audioout_postinit();
    imx233_audioin_postinit();
    imx233_audio_postinit();
}

void audiohw_close(void)
{
    imx233_audioout_close();
    imx233_audioin_close();
}

/* volume in centibels */
void audiohw_set_volume(int vol_l, int vol_r)
{
    /* convert to half-dB */
    imx233_audioout_set_hp_vol(vol_l / 5, vol_r / 5);
}

void audiohw_set_frequency(int fsel)
{
    imx233_audioout_set_freq(fsel);
}

void audiohw_enable_recording(bool source_mic)
{
    imx233_audioin_open();
    /* if source is microhpone we need to power the microphone too */
    imx233_audioin_enable_mic(source_mic);
    int src = source_mic ? AUDIOIN_SELECT_MICROPHONE : AUDIOIN_SELECT_LINE1;
    imx233_audioin_select_mux_input(false, src);
    imx233_audioin_select_mux_input(true, src);
}

void audiohw_disable_recording(void)
{
    imx233_audioin_close();
}

/* volume in decibels */
void audiohw_set_recvol(int left, int right, int type)
{
    left *= 2; /* convert to half-dB */
    right *= 2;
    if(type == AUDIO_GAIN_LINEIN)
    {
        imx233_audioin_set_vol(false, left, AUDIOIN_SELECT_LINE1);
        imx233_audioin_set_vol(true, right, AUDIOIN_SELECT_LINE1);
        imx233_audioin_set_vol(false, left, AUDIOIN_SELECT_LINE2);
        imx233_audioin_set_vol(true, right, AUDIOIN_SELECT_LINE2);
        imx233_audioin_set_vol(false, left, AUDIOIN_SELECT_HEADPHONE);
        imx233_audioin_set_vol(true, right, AUDIOIN_SELECT_HEADPHONE);
    }
    else
        imx233_audioin_set_vol(false, left, AUDIOIN_SELECT_MICROPHONE);
}

void audiohw_set_depth_3d(int val)
{
    imx233_audioout_set_3d_effect(val);
}

void audiohw_set_monitor(bool enable)
{
    imx233_audioout_select_hp_input(enable);
}

#ifdef HAVE_SPEAKER
void audiohw_enable_speaker(bool en)
{
    imx233_audioout_enable_spkr(en);
    imx233_audio_enable_spkr(en);
}
#endif
