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

#define MPEG_SWAP_CHUNKSIZE  0x2000
#define MPEG_HIGH_WATER  2 /* We leave 2 bytes empty because otherwise we
                              wouldn't be able to see the difference between
                              an empty buffer and a full one. */
#define MPEG_LOW_WATER  0x60000
#define MPEG_RECORDING_LOW_WATER  0x80000
#define MPEG_LOW_WATER_CHUNKSIZE  0x40000
#define MPEG_LOW_WATER_SWAP_CHUNKSIZE  0x10000
#define MPEG_PLAY_PENDING_THRESHOLD 0x10000
#define MPEG_PLAY_PENDING_SWAPSIZE 0x10000

#define MPEG_MAX_PRERECORD_SECONDS 30

/* For ID3 info and VBR header */
#define MPEG_RESERVED_HEADER_SPACE (4096 + 1500)

struct mpeg_debug
{
        int mp3buflen;
        int mp3buf_write;
        int mp3buf_swapwrite;
        int mp3buf_read;

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

void mpeg_init(void);
void mpeg_play(int offset);
void mpeg_stop(void);
void mpeg_pause(void);
void mpeg_resume(void);
void mpeg_next(void);
void mpeg_prev(void);
void mpeg_ff_rewind(int newtime);
void mpeg_flush_and_reload_tracks(void);
struct mp3entry* mpeg_current_track(void);
struct mp3entry* mpeg_next_track(void);
bool mpeg_has_changed_track(void);
int mpeg_status(void);
#if (CONFIG_HWCODEC == MAS3587F) || defined(SIMULATOR)
void mpeg_init_recording(void);
void mpeg_init_playback(void);
void mpeg_record(const char *filename);
void mpeg_new_file(const char *filename);
void mpeg_set_recording_options(int frequency, int quality,
                                int source, int channel_mode,
                                bool editable, int prerecord_time);
void mpeg_set_recording_gain(int left, int right, bool use_mic);
unsigned long mpeg_recorded_time(void);
unsigned long mpeg_num_recorded_bytes(void);
void mpeg_pause_recording(void);
void mpeg_resume_recording(void);
#endif
void mpeg_get_debugdata(struct mpeg_debug *dbgdata);
void mpeg_set_buffer_margin(int seconds);
unsigned int mpeg_error(void);
void mpeg_error_clear(void);
int mpeg_get_file_pos(void);
unsigned long mpeg_get_last_header(void);

/* in order to keep the recording here, I have to expose this */
void rec_tick(void);
void playback_tick(void); /* FixMe: get rid of this, use mp3_get_playtime() */
void mpeg_id3_options(bool _v1first);

#define MPEG_STATUS_PLAY 1
#define MPEG_STATUS_PAUSE 2
#define MPEG_STATUS_RECORD 4
#define MPEG_STATUS_PRERECORD 8
#define MPEG_STATUS_ERROR 16

#define MPEGERR_DISK_FULL 1

#endif
