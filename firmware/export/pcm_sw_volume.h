/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 by Michael Sevakis
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
#ifndef PCM_SW_VOLUME_H
#define PCM_SW_VOLUME_H

/***
 ** Note: Only PCM drivers that are themselves buffered should use the
 ** PCM_SW_VOLUME_UNBUFFERED configuration. This may be part of the platform,
 ** the library or a hardware necessity. Normally, it shouldn't be used and
 ** only the port developer can properly decide.
 **/
#ifdef WANT_SWVOL

#include <audiohw.h>
#include <limits.h>
#include <stddef.h>

#define PCM_MUTE_LEVEL INT_MIN

#ifdef AUDIOHW_HAVE_PRESCALER
/* Set the prescaler value for all PCM playback */
void pcm_set_prescaler(int prescale);
#endif /* AUDIOHW_HAVE_PRESCALER */

/* Set the per-channel volume cut/gain for all PCM playback */
void pcm_set_master_volume(int vol_l, int vol_r);

void pcm_play_dma_start_int_swvol(const void *addr, size_t size);
void pcm_play_dma_stop_int_swvol(void);
enum pcm_dma_status pcm_play_dma_status_callback_int_swvol(enum pcm_dma_status status);
bool pcm_play_dma_complete_callback_swvol(enum pcm_dma_status status,
                                          const void **addr, size_t *size);
#endif /* WANT_SWVOL */

#endif /* PCM_SW_VOLUME_H */
