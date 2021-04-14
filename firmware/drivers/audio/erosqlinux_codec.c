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
 * Copyright (c) 2020 Solomon Peachy
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
#include "system.h"
#include "panic.h"
#include "sysfs.h"
#include "alsa-controls.h"
#include "pcm-alsa.h"
#include "settings.h"

#include "logf.h"

/*
 PCM device hw:0,0

   ACCESS:  MMAP_INTERLEAVED RW_INTERLEAVED
   FORMAT:  S16_LE S24_LE
   SUBFORMAT:  STD
   SAMPLE_BITS: [16 32]
   FRAME_BITS: [16 64]
   CHANNELS: [1 2]
   RATE: [8000 192000]
   PERIOD_TIME: (2666 8192000]
   PERIOD_SIZE: [512 65536]
   PERIOD_BYTES: [4096 131072]
   PERIODS: [4 128]
   BUFFER_TIME: (10666 32768000]
   BUFFER_SIZE: [2048 262144]
   BUFFER_BYTES: [4096 524288]
   TICK_TIME: ALL

 Mixer controls:

     numid=1,iface=MIXER,name='Output Port Switch'
      ; type=INTEGER,access=rw------,values=1,min=0,max=5,step=0
      : values=4
*/

static int hw_init = 0;

static long int vol_l_hw = 255;
static long int vol_r_hw = 255;
static long int last_ps = -1;

static int muted = -1;

void audiohw_mute(int mute)
{
    logf("mute %d", mute);

    if (hw_init < 0 || muted == mute)
       return;

    muted = mute;

    if(mute)
    {
        long int ps0 = 0;
        alsa_controls_set_ints("Output Port Switch", 1, &ps0);
    }
    else
    {
        last_ps = 0;
        erosq_get_outputs();
    }
}

int erosq_get_outputs(void) {
    long int ps = 0; // Muted, if nothing is plugged in!

    int status = 0;

    if (!hw_init) return ps;

    const char * const sysfs_lo_switch = "/sys/class/switch/lineout/state";
    const char * const sysfs_hs_switch = "/sys/class/switch/headset/state";

    sysfs_get_int(sysfs_lo_switch, &status);
    if (status) ps = 1; // lineout

    sysfs_get_int(sysfs_hs_switch, &status);
    if (status) ps = 2; // headset

    erosq_set_output(ps);

    return ps;
}

void erosq_set_output(int ps)
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
    audiohw_mute(false);  /* No need to stay muted */
}

void audiohw_postinit(void)
{
    logf("hw postinit");
}

void audiohw_close(void)
{
    logf("hw close");
    hw_init = 0;
    muted = -1;
    alsa_controls_close();
}

void audiohw_set_frequency(int fsel)
{
    (void)fsel;
}

/* min/max for pcm volume */
const int min_pcm = -740;
const int max_pcm = 0;

void audiohw_set_volume(int vol_l, int vol_r)
{
    logf("hw vol %d %d", vol_l, vol_r);

    long l,r;

    vol_l_hw = vol_l;
    vol_r_hw = vol_r;

    if (lineout_inserted()) {
        /* On the EROS Q/K hardware, full scale line out is _very_ hot
           at ~5.8Vpp. As the hardware provides no way to reduce
           output gain, we have to back off on the PCM signal
           to avoid blowing out the signal.
        */
        l = r = global_settings.volume_limit * 10;
    } else {
        l = vol_l_hw;
        r = vol_r_hw;
    }

    int sw_volume_l = l <= min_pcm ? min_pcm : MIN(l, max_pcm);
    int sw_volume_r = r <= min_pcm ? min_pcm : MIN(r, max_pcm);
    pcm_set_mixer_volume(sw_volume_l / 20, sw_volume_r / 20);
}

void audiohw_set_lineout_volume(int vol_l, int vol_r)
{
    long l,r;

    logf("lo vol %d %d", vol_l, vol_r);

    (void)vol_l;
    (void)vol_r;

    if (lineout_inserted()) {
        l = r = global_settings.volume_limit * 10;
    } else {
        l = vol_l_hw;
        r = vol_r_hw;
    }

    int sw_volume_l = l <= min_pcm ? min_pcm : MIN(l, max_pcm);
    int sw_volume_r = r <= min_pcm ? min_pcm : MIN(r, max_pcm);
    pcm_set_mixer_volume(sw_volume_l / 20, sw_volume_r / 20);
}
