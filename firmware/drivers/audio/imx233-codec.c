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

void audiohw_set_volume(int vol_l, int vol_r)
{
    imx233_audioout_set_hp_vol(vol_l / 5, vol_r / 5);
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
