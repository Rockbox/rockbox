/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _MPEG_H_
#define _MPEG_H_

#include <stdbool.h>
#include "id3.h"
#include "events.h"

#define MPEG_SWAP_CHUNKSIZE  0x2000
#define MPEG_HIGH_WATER  2 /* We leave 2 bytes empty because otherwise we
                              wouldn't be able to see the difference between
                              an empty buffer and a full one. */
#define MPEG_LOW_WATER  0x60000
#define MPEG_RECORDING_LOW_WATER  0x80000
#define MPEG_LOW_WATER_CHUNKSIZE  0x40000
#define MPEG_LOW_WATER_SWAP_CHUNKSIZE  0x10000
#ifdef HAVE_MMC
#define MPEG_PLAY_PENDING_THRESHOLD 0x20000
#define MPEG_PLAY_PENDING_SWAPSIZE 0x20000
#else
#define MPEG_PLAY_PENDING_THRESHOLD 0x10000
#define MPEG_PLAY_PENDING_SWAPSIZE 0x10000
#endif

#define MPEG_MAX_PRERECORD_SECONDS 30

/* For ID3 info and VBR header */
#define MPEG_RESERVED_HEADER_SPACE (4096 + 576)

#if (CONFIG_CODEC == MAS3587F) || defined(SIMULATOR)

#if CONFIG_TUNER & S1A0903X01
int mpeg_get_mas_pllfreq(void);
#endif

#endif
unsigned long mpeg_get_last_header(void);

/* in order to keep the recording here, I have to expose this */
void rec_tick(void);
void playback_tick(void); /* FixMe: get rid of this, use mp3_get_playtime() */

void audio_set_track_changed_event(void (*handler)(struct mp3entry *id3));
void audio_set_track_buffer_event(void (*handler)(struct mp3entry *id3));
void audio_set_track_unbuffer_event(void (*handler)(struct mp3entry *id3));
void audio_set_cuesheet_callback(bool (*handler)(const char *filename));

#endif
