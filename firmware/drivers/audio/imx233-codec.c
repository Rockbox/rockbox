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

const struct sound_settings_info audiohw_settings[] =
{
    /* i.MX233 has half dB steps */
    [SOUND_VOLUME]        = {"dB", 0,  5, VOLUME_MIN / 10,   VOLUME_MAX / 10, -25},
    /* HAVE_SW_TONE_CONTROLS */
    [SOUND_BASS]          = {"dB", 0,  1, -24,  24,   0},
    [SOUND_TREBLE]        = {"dB", 0,  1, -24,  24,   0},
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
#ifdef HAVE_RECORDING
    [SOUND_LEFT_GAIN]     = {"dB", 1,  1,   0,  31,  23},
    [SOUND_RIGHT_GAIN]    = {"dB", 1,  1,   0,  31,  23},
    [SOUND_MIC_GAIN]      = {"dB", 1,  1,   0,   1,   0},
#endif
    [SOUND_DEPTH_3D]      = {"%",   0,  1,  0,  15,   0},
};

int tenthdb2master(int tdb)
{
    /* Just go from tenth of dB to half to dB */
    return tdb / 5;
}

void audiohw_preinit(void)
{
    imx233_audioout_preinit();
    imx233_audioin_preinit();
}

void audiohw_postinit(void)
{
    imx233_audioout_postinit();
    imx233_audioin_postinit();
}

void audiohw_close(void)
{
    imx233_audioout_close();
    imx233_audioin_close();
}

void audiohw_set_headphone_vol(int vol_l, int vol_r)
{
    imx233_audioout_set_hp_vol(vol_l, vol_r);
}

void audiohw_set_frequency(int fsel)
{
    imx233_audioout_set_freq(fsel);
}

void audiohw_set_recvol(int left, int right, int type)
{
    (void) left;
    (void) right;
    (void) type;
}

void audiohw_set_depth_3d(int val)
{
    (void) val;
}
