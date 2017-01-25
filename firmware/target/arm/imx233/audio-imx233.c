/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#include "audiohw.h"
#include "audio-imx233.h"
#include "pinctrl-imx233.h"

void imx233_audio_preinit(void)
{
#ifdef IMX233_AUDIO_HP_GATE_BANK
    imx233_pinctrl_acquire(IMX233_AUDIO_HP_GATE_BANK, IMX233_AUDIO_HP_GATE_PIN, "hp_gate");
    imx233_pinctrl_set_function(IMX233_AUDIO_HP_GATE_BANK, IMX233_AUDIO_HP_GATE_PIN, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(IMX233_AUDIO_HP_GATE_BANK, IMX233_AUDIO_HP_GATE_PIN, true);
    imx233_audio_enable_hp(false);
#endif
#ifdef IMX233_AUDIO_SPKR_GATE_BANK
    imx233_pinctrl_acquire(IMX233_AUDIO_SPKR_GATE_BANK, IMX233_AUDIO_SPKR_GATE_PIN, "spkr_gate");
    imx233_pinctrl_set_function(IMX233_AUDIO_SPKR_GATE_BANK, IMX233_AUDIO_SPKR_GATE_PIN, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(IMX233_AUDIO_SPKR_GATE_BANK, IMX233_AUDIO_SPKR_GATE_PIN, true);
    imx233_audio_enable_spkr(false);
#endif
}

void imx233_audio_postinit(void)
{
    imx233_audio_enable_hp(true);
}
// enable/disable the HP audio gate (typically using a GPIO)
void imx233_audio_enable_hp(bool en)
{
#ifdef IMX233_AUDIO_HP_GATE_BANK
# ifdef IMX233_AUDIO_HP_GATE_INVERTED
    en = !en;
# endif
    imx233_pinctrl_set_gpio(IMX233_AUDIO_HP_GATE_BANK, IMX233_AUDIO_HP_GATE_PIN, en);
#else
    (void) en;
#endif
}
// enable/disable the speaker audio gate (typically using a GPIO)
void imx233_audio_enable_spkr(bool en)
{
#ifdef IMX233_AUDIO_SPKR_GATE_BANK
# ifdef IMX233_AUDIO_SPKR_GATE_INVERTED
    en = !en;
# endif
    imx233_pinctrl_set_gpio(IMX233_AUDIO_SPKR_GATE_BANK, IMX233_AUDIO_SPKR_GATE_PIN, en);
#else
    (void) en;
#endif
}

static int input_source = AUDIO_SRC_PLAYBACK;
static unsigned input_flags = 0;
static int output_source = AUDIO_SRC_PLAYBACK;

static void select_audio_path(void)
{
    const bool recording = input_flags & SRCF_RECORDING;
    (void) recording; // it is not always used

    switch(input_source)
    {
        default:
            /* playback and no recording */
            input_source = AUDIO_SRC_PLAYBACK;
            /* fallthrough */
        case AUDIO_SRC_PLAYBACK:
            audiohw_set_monitor(false);
#if defined(HAVE_RECORDING)
            audiohw_disable_recording();
#endif
            break;

#if defined(HAVE_RECORDING) && (INPUT_SRC_CAPS & SRC_CAP_MIC)
        /* recording only */
        case AUDIO_SRC_MIC:
            audiohw_set_monitor(false);
            audiohw_enable_recording(true);
            break;
#endif

#if (INPUT_SRC_CAPS & SRC_CAP_FMRADIO)
        /* recording and playback */
        case AUDIO_SRC_FMRADIO:
            audiohw_set_monitor(true);
#if defined(HAVE_RECORDING)
            if(recording)
                audiohw_enable_recording(false);
            else
                audiohw_disable_recording();
#endif
            break;
#endif /* (INPUT_SRC_CAPS & SRC_CAP_FMRADIO) */
    }
}

void audio_input_mux(int source, unsigned flags)
{
    input_source = source;
    input_flags = flags;
    select_audio_path();
}

void audio_set_output_source(int source)
{
    output_source = source;
    select_audio_path();
}
