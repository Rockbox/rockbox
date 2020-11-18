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

#if defined(SONY_NWZ_LINUX) || defined(HAVE_FIIO_LINUX_CODEC)
/* Set the PCM volume in dB: each sample with have this volume applied digitally
 * before being sent to ALSA. Volume must satisfy -43 <= dB <= 0 */
void pcm_set_mixer_volume(int vol_db_l, int vol_db_r);
#endif

/* These two should be invoked in your audiohw_preinit() call! */
void pcm_alsa_set_playback_device(const char *device);
#if defined(HAVE_RECORDING)
void pcm_alsa_set_capture_device(const char *device);
#endif

unsigned int pcm_alsa_get_rate(void);
unsigned int pcm_alsa_get_xruns(void);

#endif /* __PCM_ALSA_RB_H__ */
