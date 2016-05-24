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
#ifndef __audioin_imx233__
#define __audioin_imx233__

#include "config.h"
#include "cpu.h"
#include "system.h"

#define AUDIOIN_SELECT_MICROPHONE   0
#define AUDIOIN_SELECT_LINE1        1
#define AUDIOIN_SELECT_HEADPHONE    2
#define AUDIOIN_SELECT_LINE2        3

struct imx233_audioin_info_t
{
    // NOTE there is a convention here: adc -> adcvol -> adcmute
    int freq; // in mHz
    int muxselect[2];
    bool adc;
    int adcvol[2]; // in tenth-dB, l/r
    bool adcmute[2]; // l/r
    bool mux;
    int muxvol[2]; // in tenth-db, l/r
    bool muxmute[2]; // l/r
    bool mic;
    int micvol[2]; // in tenth-db, l/r
    int micmute[2]; // l/r
};

void imx233_audioin_preinit(void);
void imx233_audioin_postinit(void);
void imx233_audioin_open(void);
void imx233_audioin_close(void);
/* use AUDIONIN_SELECT_* values */
void imx233_audioin_select_mux_input(bool right, int select);
/* volume in half dB */
void imx233_audioin_set_vol(bool right, int vol, int select);
/* frequency index, NOT the frequency itself */
void imx233_audioin_set_freq(int fsel);
/* enable microphone */
void imx233_audioin_enable_mic(bool enable);

struct imx233_audioin_info_t imx233_audioin_get_info(void);

#endif /* __audioin_imx233__ */
