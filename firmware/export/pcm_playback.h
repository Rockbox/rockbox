/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef PCM_PLAYBACK_H
#define PCM_PLAYBACK_H

#include <sys/types.h>

/* Typedef for registered callback (play and record) */
typedef void (*pcm_more_callback_type)(unsigned char **start,
                                       size_t *size);

void pcm_init(void);

/* set the pcm frequency - use values in hw_sampr_list
 * use -1 for the default frequency
 */
void pcm_set_frequency(unsigned int frequency);
/* apply settings to hardware immediately */
void pcm_apply_settings(bool reset);

/* This is for playing "raw" PCM data */
void pcm_play_data(pcm_more_callback_type get_more,
        unsigned char* start, size_t size);

void pcm_calculate_peaks(int *left, int *right);
size_t pcm_get_bytes_waiting(void);

void pcm_play_stop(void);
void pcm_mute(bool mute);
void pcm_play_pause(bool play);
bool pcm_is_paused(void);
bool pcm_is_playing(void);

#endif /* PCM_PLAYBACK_H */
