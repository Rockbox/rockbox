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
#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>

#ifdef SIMULATOR
#define audio_play(x) sim_audio_play(x)
#endif

#define AUDIO_STATUS_PLAY 1
#define AUDIO_STATUS_PAUSE 2
#define AUDIO_STATUS_RECORD 4
#define AUDIO_STATUS_PRERECORD 8
#define AUDIO_STATUS_ERROR 16

#define AUDIOERR_DISK_FULL 1

struct audio_debug
{
        int audiobuflen;
        int audiobuf_write;
        int audiobuf_swapwrite;
        int audiobuf_read;

        int last_dma_chunk_size;

        bool dma_on;
        bool playing;
        bool play_pending;
        bool is_playing;
        bool filling;
        bool dma_underrun;

        int unplayed_space;
        int playable_space;
        int unswapped_space;

        int low_watermark_level;
        int lowest_watermark_level;
};

void audio_init(void);
void audio_play(int offset);
void audio_stop(void);
void audio_pause(void);
void audio_resume(void);
void audio_next(void);
void audio_prev(void);
int audio_status(void);
void audio_ff_rewind(int newtime);
void audio_flush_and_reload_tracks(void);
struct mp3entry* audio_current_track(void);
struct mp3entry* audio_next_track(void);
bool audio_has_changed_track(void);
void audio_get_debugdata(struct audio_debug *dbgdata);
void audio_set_crossfade(int type);
void audio_set_buffer_margin(int seconds);
unsigned int audio_error(void);
void audio_error_clear(void);
int audio_get_file_pos(void);
void audio_beep(int duration);
void audio_init_playback(void);

#endif
