/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
// #define LOGF_ENABLE

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "audio.h"
#include "audiohw.h"
#include "system.h"
#include "panic.h"
#include "sysfs.h"
#include "alsa-controls.h"
#include "pcm.h"
#include "pcm-internal.h"
#include "pcm-alsa.h"
#include "settings.h"

#include "logf.h"

#define EROSQ_ANALOG_PLAYBACK   "plughw:0,0"
#define EROSQ_ASOUND_BT_CONF    "/tmp/asound-bt.conf"

static volatile int erosq_bt_route_active;
static volatile int erosq_bt_port_latched;

static int hw_init = 0;

static long int vol_l_hw = 255;
static long int vol_r_hw = 255;
static long int last_ps = -1;

static int muted = -1;
static int erosq_bt_apply_attempts;

static char erosq_bt_peer[18];
static char erosq_bt_pcm_dev[40];

#define EROSQ_ROUTE_LOG      "/tmp/rb_route.log"
#define EROSQ_ROUTE_LOG_SD   "/mnt/sd_0/.rockbox/rb_route.log"

extern int hwver;

static void erosq_route_log_sd(const char *msg, long want, long got)
{
    FILE *f = fopen(EROSQ_ROUTE_LOG_SD, "a");
    if (!f)
        return;
    fprintf(f, "%s want=%ld got=%ld muted=%d bt=%d\n",
            msg, want, got, muted, erosq_bt_route_active);
    fclose(f);
}

static void erosq_route_log(const char *msg, long want, long got)
{
    FILE *f = fopen(EROSQ_ROUTE_LOG, "a");
    if (!f)
        return;
    fprintf(f, "%s want=%ld got=%ld muted=%d bt=%d last_ps=%ld pcm=%s\n",
            msg, want, got, muted, erosq_bt_route_active, last_ps,
            erosq_bt_pcm_dev[0] ? erosq_bt_pcm_dev : "-");
    fclose(f);
}

static long erosq_read_output_port(void)
{
    long verify = -1;

    if (!hw_init)
        return verify;
    if (alsa_controls_find("Output Port Switch") >= 0)
        alsa_controls_get("Output Port Switch", SND_CTL_ELEM_TYPE_INTEGER, 1, &verify);
    return verify;
}

static void erosq_bdaddr_to_pcm_id(const char *bdaddr, char *id, size_t id_len)
{
    size_t i = 0;
    size_t o = 0;

    for (; bdaddr[i] && o + 1 < id_len; i++) {
        if (bdaddr[i] == ':')
            id[o++] = '_';
        else
            id[o++] = (char)toupper((unsigned char)bdaddr[i]);
    }
    id[o] = '\0';
}

static void erosq_build_bt_pcm_name(const char *bdaddr, char *pcm, size_t pcm_len)
{
    char id[20];

    erosq_bdaddr_to_pcm_id(bdaddr, id, sizeof(id));
    snprintf(pcm, pcm_len, "pcm.%s", id);
}

static bool erosq_write_asound_bluetooth(const char *bdaddr)
{
    FILE *f = fopen(EROSQ_ASOUND_BT_CONF, "w");
    char id[20];
    long n;

    if (!f)
        return false;

    erosq_bdaddr_to_pcm_id(bdaddr, id, sizeof(id));
    n = fprintf(f,
                "# bluetooth playback (Rockbox)\n"
                "pcm.%s {\n"
                "    type bluetooth\n"
                "    bdaddr %s\n"
                "    profile a2dp\n"
                "}\n",
                id, bdaddr);
    if (n < 20 || fflush(f) != 0) {
        fclose(f);
        return false;
    }
    fclose(f);
    setenv("ALSA_CONFIG_PATH", EROSQ_ASOUND_BT_CONF, 1);
    return true;
}

static void erosq_clear_asound_bluetooth(void)
{
    unsetenv("ALSA_CONFIG_PATH");
}

static bool erosq_switch_to_analog_pcm(void)
{
    erosq_clear_asound_bluetooth();
    pcm_alsa_release_playback_for_mixer();
    pcm_alsa_set_playback_device(EROSQ_ANALOG_PLAYBACK);
    erosq_bt_pcm_dev[0] = '\0';
    if (!pcm_alsa_reopen_playback_safe())
        return false;
    pcm_playback_invalidate_config();
    pcm_alsa_reconfigure_playback();
    pcm_apply_settings();
    return true;
}

static bool erosq_switch_to_bluetooth_pcm(void)
{
    if (!erosq_bt_peer[0])
        return false;

    erosq_build_bt_pcm_name(erosq_bt_peer, erosq_bt_pcm_dev, sizeof(erosq_bt_pcm_dev));
    if (!erosq_write_asound_bluetooth(erosq_bt_peer)) {
        erosq_route_log("asound write fail", 0, 0);
        return false;
    }

    pcm_alsa_release_playback_for_mixer();
    pcm_alsa_set_playback_device(erosq_bt_pcm_dev);
    if (pcm_alsa_reopen_playback_safe()) {
        pcm_playback_invalidate_config();
        pcm_alsa_reconfigure_playback();
        pcm_apply_settings();
        audiohw_set_volume(vol_l_hw, vol_r_hw);
        erosq_bt_port_latched = 1;
        erosq_route_log("bt_pcm ok", 0, 0);
        erosq_route_log_sd("bt_pcm", 0, 0);
        return true;
    }

    pcm_alsa_set_playback_device(EROSQ_ANALOG_PLAYBACK);
    pcm_alsa_reopen_playback_safe();
    erosq_route_log("bt_pcm fail", 0, 0);
    return false;
}

static bool erosq_try_bt_audio_path(void)
{
    if (!erosq_bt_route_active)
        return false;

    if (erosq_switch_to_bluetooth_pcm())
        return true;

    return false;
}

bool erosq_is_bluetooth_route_active(void)
{
    return erosq_bt_route_active != 0;
}

void erosq_set_bluetooth_peer(const char *bdaddr)
{
    erosq_bt_peer[0] = '\0';
    if (!bdaddr || strlen(bdaddr) < 17)
        return;
    snprintf(erosq_bt_peer, sizeof(erosq_bt_peer), "%.17s", bdaddr);
}

void audiohw_mute(int mute)
{
    logf("mute %d", mute);

    if (hw_init < 0 || muted == mute)
       return;

    muted = mute;

    if (mute) {
        if (!erosq_bt_route_active) {
            long int ps0 = 0;
            alsa_controls_set_ints("Output Port Switch", 1, &ps0);
            last_ps = 0;
        }
    } else if (erosq_bt_route_active) {
        erosq_try_bt_audio_path();
    } else {
        last_ps = 0;
        erosq_get_outputs();
    }
}

int erosq_get_outputs(void)
{
    long int ps = 0;
    int status = 0;

    if (!hw_init)
        return ps;

    if (erosq_bt_route_active)
        return EROSQ_OUTPUT_BLUETOOTH;

    const char * const sysfs_lo_switch = "/sys/class/switch/lineout/state";
    const char * const sysfs_hs_switch = "/sys/class/switch/headset/state";

    sysfs_get_int(sysfs_lo_switch, &status);
    if (status)
        ps = 1;

    sysfs_get_int(sysfs_hs_switch, &status);
    if (status)
        ps = 2;

    erosq_set_output(ps);
    return ps;
}

void erosq_set_bluetooth_route(int on)
{
    erosq_bt_route_active = on ? 1 : 0;
    if (!hw_init) {
        erosq_route_log(on ? "bt_route(on) deferred" : "bt_route(off) deferred",
                        on ? EROSQ_OUTPUT_BLUETOOTH : 0, erosq_read_output_port());
        return;
    }

    last_ps = -1;
    erosq_bt_port_latched = 0;
    if (!on) {
        erosq_bt_apply_attempts = 0;
        erosq_switch_to_analog_pcm();
        erosq_get_outputs();
        erosq_route_log_sd("bt_route off", 0, erosq_read_output_port());
        return;
    }

    erosq_bt_apply_attempts = 0;
    unlink(EROSQ_ROUTE_LOG);
    unlink(EROSQ_ROUTE_LOG_SD);
    erosq_try_bt_audio_path();
    erosq_route_log_sd("bt_route on", EROSQ_OUTPUT_BLUETOOTH, erosq_read_output_port());
}

void erosq_apply_bluetooth_route(void)
{
    static unsigned int last_try_tick;

    if (!erosq_bt_route_active || !hw_init || erosq_bt_port_latched)
        return;
    if (erosq_bt_apply_attempts >= 8)
        return;
    if (TIME_AFTER(last_try_tick + HZ / 2, current_tick))
        return;
    last_try_tick = current_tick;
    erosq_bt_apply_attempts++;

    erosq_try_bt_audio_path();
}

bool erosq_bluetooth_route_applied(void)
{
    return erosq_bt_port_latched != 0;
}

void erosq_notify_pcm_running(void)
{
    (void)erosq_bt_route_active;
    (void)erosq_bt_port_latched;
}

void erosq_set_output(int ps)
{
    if (!hw_init)
        return;
    if (muted && ps != 0)
        return;
    if (erosq_bt_route_active)
        return;

    if (last_ps != ps) {
        logf("set out %d/%ld", ps, last_ps);
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

    if (alsa_controls_find("Left Playback Volume") == -1)
        hwver = 1;
    else if (hwver == 1)
        hwver = 23;

    audiohw_mute(false);
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

const int min_pcm = -740;
const int max_pcm = 0;

static void audiohw_set_volume_v1(int vol_l, int vol_r)
{
    long l,r;

    vol_l_hw = vol_l;
    vol_r_hw = vol_r;

    logf("set_volume_v1 %d, %d", vol_l, vol_r);

    if (lineout_inserted() && !headphones_inserted()) {
        l = r = global_settings.volume_limit * 10;
    } else {
        l = vol_l_hw;
        r = vol_r_hw;
    }

    int sw_volume_l = l <= min_pcm ? min_pcm : MIN(l, max_pcm);
    int sw_volume_r = r <= min_pcm ? min_pcm : MIN(r, max_pcm);
    pcm_set_mixer_volume(sw_volume_l, sw_volume_r);
}

static void audiohw_set_volume_v2(int vol_l, int vol_r)
{
    long l,r;

    vol_l_hw = vol_l;
    vol_r_hw = vol_r;

    logf("set_volume_v2 %d, %d", vol_l, vol_r);

    if (lineout_inserted() && !headphones_inserted()) {
        l = -1 * (global_settings.volume_limit * 10) / 5;
        r = l;
    } else {
        l = -1 * vol_l_hw / 5;
        r = -1 * vol_r_hw / 5;
    }

    if (!hw_init)
       return;

    if (erosq_bt_route_active) {
        int sw_volume_l = vol_l_hw <= min_pcm ? min_pcm : MIN(vol_l_hw, max_pcm);
        int sw_volume_r = vol_r_hw <= min_pcm ? min_pcm : MIN(vol_r_hw, max_pcm);
        pcm_set_mixer_volume(sw_volume_l, sw_volume_r);
        return;
    }

    alsa_controls_set_ints("Left Playback Volume", 1, &l);
    alsa_controls_set_ints("Right Playback Volume", 1, &r);

    pcm_set_mixer_volume(global_settings.volume_limit, global_settings.volume_limit);
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    if (hwver >= 2) {
        audiohw_set_volume_v2(vol_l, vol_r);
    } else {
        audiohw_set_volume_v1(vol_l, vol_r);
    }
}

void audiohw_set_lineout_volume(int vol_l, int vol_r)
{
    long l,r;

    (void)vol_l;
    (void)vol_r;

    if (lineout_inserted() && !headphones_inserted()) {
        l = global_settings.volume_limit * 10;
        r = l;
        logf("lo vol %ld %ld", l, r);

        if (hw_init){
            if (hwver >= 2) {
                l /= 5;
                l = l * -1;
                r /= 5;
                r = r * -1;
                alsa_controls_set_ints("Left Playback Volume", 1, &l);
                alsa_controls_set_ints("Right Playback Volume", 1, &r);
            } else {
                int sw_volume_l = l <= min_pcm ? min_pcm : MIN(l, max_pcm);
                int sw_volume_r = r <= min_pcm ? min_pcm : MIN(r, max_pcm);
                pcm_set_mixer_volume(sw_volume_l, sw_volume_r);
            }
        }
    }
}
