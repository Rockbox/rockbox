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
 * Copyright (c) 2018 Roman Stolyarov
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
#include "audio.h"
#include "audiohw.h"
#include "system.h"
#include "kernel.h"
#include "panic.h"
#include "sysfs.h"
#include "alsa-controls.h"
#include "pcm-alsa.h"

static int fd_hw;

static long int vol_l_hw = 255;
static long int vol_r_hw = 255;
static long int last_ps = 0;

static void hw_open(void)
{
    fd_hw = open("/dev/snd/controlC0", O_RDWR);
    if(fd_hw < 0)
        panicf("Cannot open '/dev/snd/controlC0'");
}

static void hw_close(void)
{
    close(fd_hw);
}

void audiohw_mute(int mute)
{
    if(mute)
    {
#if defined(XDUOO_X3II)
        alsa_controls_set_bool("AK4490 Soft Mute", true);
#endif
#if defined(XDUOO_X20)
        long int ps0 = (last_ps > 1) ? 1 : 2;
        alsa_controls_set_ints("Output Port Switch", 1, &ps0);
#endif
    }
    else
    {
#if defined(XDUOO_X3II)
        alsa_controls_set_bool("AK4490 Soft Mute", false);
#endif
#if defined(XDUOO_X20)
        alsa_controls_set_ints("Output Port Switch", 1, &last_ps);
#endif
    }
}

void audiohw_set_output(void)
{
    long int ps = 2; // headset

    int status = 0;

    const char * const sysfs_lo_switch = "/sys/class/switch/lineout/state";
    const char * const sysfs_hs_switch = "/sys/class/switch/headset/state";
#if defined(XDUOO_X20)
    const char * const sysfs_bal_switch = "/sys/class/switch/balance/state";
#endif

    sysfs_get_int(sysfs_lo_switch, &status);
    if (status) ps = 1; // lineout

    sysfs_get_int(sysfs_hs_switch, &status);
    if (status) ps = 2; // headset

#if defined(XDUOO_X20)
    sysfs_get_int(sysfs_bal_switch, &status);
    if (status) ps = 3; // balance
#endif

    if (last_ps != ps)
    {
        /* Output port switch */
        last_ps = ps;
        alsa_controls_set_ints("Output Port Switch", 1, &last_ps);
    }
}

void audiohw_preinit(void)
{
    alsa_controls_init();
    hw_open();
}

void audiohw_postinit(void)
{
    audiohw_set_output();
}

void audiohw_close(void)
{
    hw_close();
    alsa_controls_close();
}

void audiohw_set_frequency(int fsel)
{
    (void)fsel;
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    vol_l_hw = -vol_l/5;
    vol_r_hw = -vol_r/5;
 
    alsa_controls_set_ints("Left Playback Volume", 1, &vol_l_hw);
    alsa_controls_set_ints("Right Playback Volume", 1, &vol_r_hw);
}

void audiohw_set_filter_roll_off(int value)
{
    /* 0 = Sharp; 
       1 = Slow;
       2 = Short Sharp
       3 = Short Slow */
#if defined(XDUOO_X3II)
    long int value_hw = value;
    alsa_controls_set_ints("AK4490 Digital Filter", 1, &value_hw);
#elif defined(XDUOO_X20)
    long int value_hw = value;
    alsa_controls_set_ints("ES9018_K2M Digital Filter", 1, &value_hw);
#else
    (void)value;
#endif
}
