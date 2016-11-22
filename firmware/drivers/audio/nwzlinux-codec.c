/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (c) 2016 Amaury Pouly
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

#include "logf.h"
#include "system.h"
#include "kernel.h"
#include "string.h"
#include "audio.h"
#include "sound.h"
#include "audiohw.h"
#include "cscodec.h"
#include "nwzlinux_codec.h"
#include "stdlib.h"

/* This driver handle the Sony linux audio drivers: despite using many differents
 * codecs, it appears that they all share a common interface and common controls. */

void audiohw_preinit(void)
{
    /* we need to enable the codec */
    system("amixer cset name='CODEC Power Switch' on");
}

void audiohw_postinit(void)
{
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    (void) vol_l;
    (void) vol_r;
}

void audiohw_close(void)
{
    /* disable codec */
    system("amixer cset name='CODEC Power Switch' off");
}

void audiohw_set_frequency(int fsel)
{
    (void) fsel;
}
