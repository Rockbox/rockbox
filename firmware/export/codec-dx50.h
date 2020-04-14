/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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


#ifndef _CODEC_DX50_H_
#define _CODEC_DX50_H_


#define AUDIOHW_CAPS (MONO_VOL_CAP | FILTER_ROLL_OFF_CAP)


/*
    http://www.wolfsonmicro.com/media/76425/WM8740.pdf

    0.5 * ( x - 255 ) = ydB     1 <= x <= 255
    mute                        x = 0

    x = 255 -> 0dB
    .
    .
    .
    x = 2 -> -126.5dB
    x = 1 -> -127dB
    x = 0 -> -128dB

    See audiohw.h, sound.c.
*/
AUDIOHW_SETTING(VOLUME, "dB", 0, 1, -128, 0, -30)


/* 1: slow roll off, 0: sharp roll off, sharp roll off default */
AUDIOHW_SETTING(FILTER_ROLL_OFF, "", 0, 1, 0, 1, 0)


#endif
