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
#ifndef __audioout_imx233__
#define __audioout_imx233__

#include "config.h"
#include "cpu.h"
#include "system.h"

/* target-defined output stage coupling method
 * its setting is IMX233_AUDIO_COUPLING_MODE and must be set for every target
 * Use ACM_CAP if output stage (i.e. headphones) have output capacitors,
 * ACM_CAPLESS (DC-coupled) otherwise.
 */
#define ACM_CAPLESS    0
#define ACM_CAP        1

struct imx233_audioout_info_t
{
    // NOTE there is a convention here: dac -> dacvol -> dacmute
    int freq; // in mHz
    bool hpselect;
    bool dac;
    int dacvol[2]; // in tenth-dB, l/r
    bool dacmute[2]; // l/r
    bool hp;
    int hpvol[2]; // in tenth-db, l/r
    bool hpmute[2]; // l/r
    bool spkr;
    int spkrvol[2]; // in tenth-db, l/r
    int spkrmute[2]; // l/r
    int ss3d; // in tenth-db
    bool capless;
};

void imx233_audioout_preinit(void);
void imx233_audioout_postinit(void);
void imx233_audioout_close(void);
/* volume in half dB */
void imx233_audioout_set_hp_vol(int vol_l, int vol_r);
/* frequency index, NOT the frequency itself */
void imx233_audioout_set_freq(int fsel);
/* select between DAC and Line1 */
void imx233_audioout_select_hp_input(bool line1);
/* value is uses register encoding: 0=Off, 1=3dB, 2=4.5dB, 3=6dB */
void imx233_audioout_set_3d_effect(int val);
/* enable/disable speaker amplifier */
void imx233_audioout_enable_spkr(bool en);

struct imx233_audioout_info_t imx233_audioout_get_info(void);

#endif /* __audioout_imx233__ */
