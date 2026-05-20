/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2016 Amaury Pouly
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
#ifndef __PCM_ALSA_RB_H__
#define __PCM_ALSA_RB_H__

#include <config.h>
#include <limits.h>

#if defined(HAVE_ALSA_32BIT)

/* Set the PCM volume in dB: each sample with have this volume applied digitally
 * before being sent to ALSA. Effective range -79 dB to 0 dB */
void pcm_set_mixer_volume(int vol_db_l, int vol_db_r);
#endif

/* These two should be invoked in your audiohw_preinit() call! */
void pcm_alsa_set_playback_device(const char *device);
#if defined(HAVE_RECORDING)
void pcm_alsa_set_capture_device(const char *device);
#endif

/* Re-open the current playback device (after changing playback_dev). */
void pcm_alsa_reopen_playback(void);
/* Like reopen_playback but returns false instead of panicking if open fails. */
bool pcm_alsa_reopen_playback_safe(void);

/* Drop and close PCM so mixer port switch can reach the driver (Eros Q). */
void pcm_alsa_release_playback_for_mixer(void);
/* Re-apply hw/sw params after switching playback_dev (e.g. bluetooth PCM). */
void pcm_alsa_reconfigure_playback(void);

unsigned int pcm_alsa_get_rate(void);
unsigned int pcm_alsa_get_xruns(void);

#endif /* __PCM_ALSA_RB_H__ */
