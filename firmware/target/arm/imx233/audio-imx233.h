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
#ifndef __audio_imx233__
#define __audio_imx233__

#include "audio.h"
#include "audio-target.h"

/* target can override those functions to provide hooks to the audio code
 * alternativelly, the default implementation provide support for GPIO
 * controlled gates using the following defines
 * NOTE by default gates are enabled by setting GPIO to 1 (except if inverted)
 *
 * IMX233_AUDIO_HP_GATE_BANK (gpio bank)
 * IMX233_AUDIO_HP_GATE_PIN (gpio pin)
 * IMX233_AUDIO_HP_GATE_INVERTED (define if inverted)
 *
 * IMX233_AUDIO_SPKR_GATE_BANK (gpio bank)
 * IMX233_AUDIO_SPKR_GATE_PIN (gpio pin)
 * IMX233_AUDIO_SPKR_GATE_INVERTED (define if inverted)
 */
// do some initialisation related to next functions
void imx233_audio_preinit(void);
void imx233_audio_postinit(void);
// enable/disable the HP audio gate (typically using a GPIO)
void imx233_audio_enable_hp(bool en);
// enable/disable the speaker audio gate (typically using a GPIO)
void imx233_audio_enable_spkr(bool en);

#endif /* __audio_imx233__ */