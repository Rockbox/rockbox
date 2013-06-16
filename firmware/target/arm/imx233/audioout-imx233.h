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

#include "regs/regs-audioout.h"

void imx233_audioout_preinit(void);
void imx233_audioout_postinit(void);
void imx233_audioout_close(void);
/* volume in half dB */
void imx233_audioout_set_hp_vol(int vol_l, int vol_r);
/* frequency index, NOT the frequency itself */
void imx233_audioout_set_freq(int fsel);
/* select between DAC and Line1 */
void imx233_audioout_select_hp_input(bool line1);
/* value in 1.5dB steps, from 0dB to 6dB */
void imx233_audioout_set_3d_effect(int val);

#endif /* __audioout_imx233__ */
