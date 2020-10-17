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
#include "button.h"
#include "system.h"
#include "kernel.h"
#include "panic.h"
#include "sysfs.h"
#include "alsa-controls.h"
#include "pcm-alsa.h"
#include "pcm_sw_volume.h"

#include "logf.h"

static int fd_hw = -1;

static long int vol_l_hw = 255;
static long int vol_r_hw = 255;
static long int last_ps = -1;

static void hw_open(void)
{
    fd_hw = open("/dev/snd/controlC0", O_RDWR);
    if(fd_hw < 0)
        panicf("Cannot open '/dev/snd/controlC0'");
}

static void hw_close(void)
{
    close(fd_hw);
    fd_hw = -1;
}

static int muted = -1;

void audiohw_mute(int mute)
{
    logf("mute %d", mute);

    if (fd_hw < 0 || muted == mute)
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

    if (fd_hw < 0) return ps;

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
    if (fd_hw < 0 || muted) return;

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
    alsa_controls_init();
    hw_open();
    audiohw_mute(false);  /* No need to stay muted */
}

void audiohw_postinit(void)
{
    logf("hw postinit");
}

void audiohw_close(void)
{
    logf("hw close");
    hw_close();
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

    if (lineout_inserted()) {
        l = 0;
        r = 0;
    } else {
        l = vol_l_hw;
        r = vol_r_hw;
    }

    /* SW volume for <= 1.0 gain, HW at unity, < -740 == MUTE */
    int sw_volume_l = l <= -740 ? PCM_MUTE_LEVEL : MIN(l, 0);
    int sw_volume_r = r <= -740 ? PCM_MUTE_LEVEL : MIN(r, 0);
    pcm_set_master_volume(sw_volume_l, sw_volume_r);
}

void audiohw_set_lineout_volume(int vol_l, int vol_r)
{
    long l,r;

    logf("lo vol %d %d", vol_l, vol_r);

    (void)vol_l;
    (void)vol_r;

    if (lineout_inserted()) {
        l = 0;
        r = 0;
    } else {
        l = vol_l_hw;
        r = vol_r_hw;
    }

    int sw_volume_l = l <= -740 ? PCM_MUTE_LEVEL : MIN(l, 0);
    int sw_volume_r = r <= -740 ? PCM_MUTE_LEVEL : MIN(r, 0);
    pcm_set_master_volume(sw_volume_l, sw_volume_r);
}
