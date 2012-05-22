/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#include "pinctrl-imx233.h"

static int input_source = AUDIO_SRC_PLAYBACK;
static unsigned input_flags = 0;
static int output_source = AUDIO_SRC_PLAYBACK;
static bool initialized = false;

static void init(void)
{
    /* HP gate */
    imx233_pinctrl_acquire_pin(1, 30, "hp gate");
    imx233_set_pin_function(1, 30, PINCTRL_FUNCTION_GPIO);
    imx233_enable_gpio_output(1, 30, true);
    imx233_set_gpio_output(1, 30, false);
    /* SPKR gate */
    imx233_pinctrl_acquire_pin(1, 22, "spkr gate");
    imx233_set_pin_function(1, 22, PINCTRL_FUNCTION_GPIO);
    imx233_enable_gpio_output(1, 22, true);
    imx233_set_gpio_output(1, 22, false);
    
    initialized = true;
}

static void select_audio_path(void)
{
    if(!initialized)
        init();
    /* route audio to HP */
    imx233_set_gpio_output(1, 30, true);

    if(input_source == AUDIO_SRC_PLAYBACK)
        imx233_audioout_select_hp_input(false);
    else
        imx233_audioout_select_hp_input(true);
}

void audio_input_mux(int source, unsigned flags)
{
    (void) source;
    (void) flags;
    input_source = source;
    input_flags = flags;
    select_audio_path();
}
  
void audio_set_output_source(int source)
{
    (void) source;
    output_source = source;
    select_audio_path();
}

