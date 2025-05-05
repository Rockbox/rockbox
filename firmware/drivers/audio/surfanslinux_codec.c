/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 *
 * Copyright (c) 2018 Marcin Bukat
 * Copyright (c) 2025 Solomon Peachy
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

//#define LOGF_ENABLE

#include "config.h"
#include "audio.h"
#include "audiohw.h"
#include "button.h"
#include "system.h"
#include "kernel.h"
#include "panic.h"
#include "sysfs.h"
#include "alsa-controls.h"
#include "pcm-alsa.h"

#include "logf.h"

/*

f28:

**** List of PLAYBACK Hardware Devices ****
card 0: tyinx1 [tyin_x1], device 0: tyin_x1-es9018_k2m-i2s es9018_k2m-hifi-0 []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 0: tyinx1 [tyin_x1], device 1: tyin_x1-es9018_k2m-pcm es9018_k2m-hifi-1 []
  Subdevices: 1/1
  Subdevice #0: subdevice #0
card 1: saspdif [sa_spdif], device 0: SA SPDIF Dummy spdif dump dai-0 []
  Subdevices: 1/1
  Subdevice #0: subdevice #0

Mixer controls:

numid=3,iface=MIXER,name='ES9018_K2M Digital Filter'
  ; type=INTEGER,access=rw------,values=1,min=0,max=4,step=0
  : values=0
numid=5,iface=MIXER,name='Hardware Mute'
  ; type=BOOLEAN,access=rw------,values=1
  : values=off
numid=1,iface=MIXER,name='Left Playback Volume'
  ; type=INTEGER,access=rw------,values=1,min=0,max=255,step=0
  : values=0
numid=4,iface=MIXER,name='Output Port Switch'
  ; type=INTEGER,access=rw------,values=1,min=0,max=5,step=0
  : values=0
numid=2,iface=MIXER,name='Right Playback Volume'
  ; type=INTEGER,access=rw------,values=1,min=0,max=255,step=0
  : values=0
numid=6,iface=MIXER,name='isDSD'
  ; type=BOOLEAN,access=rw------,values=1
  : values=off

*/

static int hw_init = 0;

static long int vol_l_hw = 255;
static long int vol_r_hw = 255;
static long int last_ps = -1;

static int muted = -1;

void audiohw_mute(int mute)
{
    if (hw_init < 0 || muted == mute)
       return;

    muted = mute;

    alsa_controls_set_bool("Hardware Mute", !!mute);
}

int surfans_get_outputs(void){
    long int ps = 0; // Muted, if nothing is plugged in!

    int status = 0;

    if (!hw_init) return ps;

    const char * const sysfs_hs_switch = "/sys/class/switch/headset/state";
    const char * const sysfs_bal_switch = "/sys/class/switch/balance/state";

    sysfs_get_int(sysfs_hs_switch, &status);
    if (status) ps = 2; // headset

    sysfs_get_int(sysfs_bal_switch, &status);
    if (status) ps = 3; // balanced output

    surfans_set_output(ps);

    return ps;
}

void surfans_set_output(int ps)
{
    if (!hw_init || muted) return;

    if (last_ps != ps)
    {
        logf("set out %d/%d", ps, last_ps);
        /* Output port switch */
        last_ps = ps;
        alsa_controls_set_ints("Output Port Switch", 1, &last_ps);
        audiohw_set_volume(vol_l_hw, vol_r_hw);
    }
}

void audiohw_preinit(void)
{
    logf("hw preinit");
    alsa_controls_init("default");
    hw_init = 1;

    audiohw_mute(false);  /* No need ? */
    alsa_controls_set_bool("isDSD", 0);
}

void audiohw_postinit(void)
{
    logf("hw postinit");
}

void audiohw_close(void)
{
    logf("hw close");
    hw_init = 0;
    alsa_controls_close();
}

void audiohw_set_frequency(int fsel)
{
    (void)fsel;
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    logf("hw vol %d %d", vol_l, vol_r);

    long l,r;

    vol_l_hw = vol_l;
    vol_r_hw = vol_r;

    l = -vol_l/5;
    r = -vol_r/5;

    if (!hw_init)
       return;

    alsa_controls_set_ints("Left Playback Volume", 1, &l);
    alsa_controls_set_ints("Right Playback Volume", 1, &r);
}

void audiohw_set_filter_roll_off(int value)
{
    logf("rolloff %d", value);
    /* 0 = Sharp;
       1 = Slow;
       2 = Short Sharp
       3 = Short Slow
       4 = Super Slow */
#if defined(XDUOO_X20)
    long int value_hw = value;
    alsa_controls_set_ints("ES9018_K2M Digital Filter", 1, &value_hw);
#else
    (void)value;
#endif
}
