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

/* target can control those functions by using the following defines
 * NOTE by default gates are enabled by setting GPIO to 1 (except if inverted)
 *
 * IMX233_AUDIO_HP_GATE_BANK (gpio bank)
 * IMX233_AUDIO_HP_GATE_PIN (gpio pin)
 * IMX233_AUDIO_HP_GATE_INVERTED (define if inverted)
 *
 * IMX233_AUDIO_SPKR_GATE_BANK (gpio bank)
 * IMX233_AUDIO_SPKR_GATE_PIN (gpio pin)
 * IMX233_AUDIO_SPKR_GATE_INVERTED (define if inverted)
 *
 * target can set those to control microphone parameters
 * NOTE by default, mic select is 1, mic bias is 0 and mic resistor is 2KOhm
 * IMX233_AUDIO_MIC_SELECT (mic bias pin: 0=lradc0, 1=lradc1)
 * IMX233_AUDIO_MIC_BIAS (mic bias, 0=1.21V, 1=1.46, ..., 7=2.96V (0.25mV inc)
 * IMX233_AUDIO_MIC_RESISTOR (mic resistor: 2KOhm, 4KOhm, 8KOhm)
 */
// do some initialisation related to next functions
void imx233_audio_preinit(void);
void imx233_audio_postinit(void);
// enable/disable the HP audio gate (typically using a GPIO)
void imx233_audio_enable_hp(bool en);
// enable/disable the speaker audio gate (typically using a GPIO)
void imx233_audio_enable_spkr(bool en);

#endif /* __audio_imx233__ */
