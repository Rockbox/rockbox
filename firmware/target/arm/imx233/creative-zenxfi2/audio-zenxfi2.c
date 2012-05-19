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

static int input_source = AUDIO_SRC_PLAYBACK;
static unsigned input_flags = 0;
static int output_source = AUDIO_SRC_PLAYBACK;

static void select_audio_path(void)
{
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

