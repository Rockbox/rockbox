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
#include "stdio.h"
#include "audio.h"
#include "sound.h"
#include "audiohw.h"
#include "cscodec.h"
#include "nwzlinux_codec.h"
#include "stdlib.h"

/* This driver handle the Sony linux audio drivers: despite using many differents
 * codecs, it appears that they all share a common interface and common controls. */

/* This is the alsa mixer interface exposed by Sony:
Simple mixer control 'Playback',0
  Capabilities: volume
  Playback channels: Front Left - Front Right
  Capture channels: Front Left - Front Right
  Limits: 0 - 100
  Front Left: 10 [10%]
  Front Right: 10 [10%]
Simple mixer control 'Playback Src',0
  Capabilities: enum
  Items: 'None' 'Music' 'Video' 'Tv' 'Fm' 'Line' 'Mic'
  Item0: 'Music'
Simple mixer control 'Capture Src',0
  Capabilities: enum
  Items: 'None' 'Line' 'Fm' 'Mic'
  Item0: 'None'
Simple mixer control 'CODEC Acoustic',0
  Capabilities: pswitch pswitch-joined
  Playback channels: Mono
  Mono: Playback [off]
Simple mixer control 'CODEC Cue/Rev',0
  Capabilities: pswitch pswitch-joined
  Playback channels: Mono
  Mono: Playback [off]
Simple mixer control 'CODEC Fade In',0
  Capabilities: pswitch pswitch-joined
  Playback channels: Mono
  Mono: Playback [off]
Simple mixer control 'CODEC Mute',0
  Capabilities: pswitch pswitch-joined
  Playback channels: Mono
  Mono: Playback [on]
Simple mixer control 'CODEC Power',0
  Capabilities: pswitch pswitch-joined
  Playback channels: Mono
  Mono: Playback [off]
Simple mixer control 'CODEC Stanby',0
  Capabilities: pswitch pswitch-joined
  Playback channels: Mono
  Mono: Playback [off]
Simple mixer control 'Output',0
  Capabilities: enum
  Items: 'Headphone' 'LineVariable' 'LineFixed' 'Speaker'
  Item0: 'Headphone'
*/

void audiohw_preinit(void)
{
    /* turn on codec */
    system("amixer cset name='CODEC Power Switch' on");
    /* unmute */
    system("amixer cset name='CODEC Mute Switch' off");
    /* not sure exactly what it means */
    system("amixer cset name='Playback Src Switch' Music");
    /* use headphone output */
    system("amixer cset name='Output Switch' Headphone");
    /* not sure what it does but without it, sound is distorted */
    system("amixer cset name='CODEC Acoustic Switch' on");
    system("ls -R /proc/asound > /contents/asound.txt");
}

void audiohw_postinit(void)
{
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    /* FIXME hack, turn that into proper code */
    char buffer[128];
    /* the driver expects percent, convert from centibel in range -100..0 */
    vol_l = vol_l / 10 + 100;
    vol_r = vol_r / 10 + 100;
    _logf("set vol: %d %d", vol_l, vol_r);
    snprintf(buffer, sizeof(buffer), "amixer cset name='Playback Volume' %d,%d", vol_l, vol_r);
    system(buffer);
}

void audiohw_close(void)
{
}

void audiohw_set_frequency(int fsel)
{
    (void) fsel;
}
