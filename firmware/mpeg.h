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
#define MPEG_LOW_WATER  0x40000
#define MPEG_LOW_WATER_CHUNKSIZE  0x40000

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
        int unswapped_space;

        int lowest_watermark_level;
};

void mpeg_init(int volume, int bass, int treble, int balance,
               int loudness, int bass_boost, int avc);
void mpeg_play(int offset);
void mpeg_stop(void);
void mpeg_pause(void);
void mpeg_resume(void);
void mpeg_next(void);
void mpeg_prev(void);
void mpeg_ff_rewind(int change);
void mpeg_flush_and_reload_tracks(void);
void mpeg_sound_set(int setting, int value);
int mpeg_sound_min(int setting);
int mpeg_sound_max(int setting);
int mpeg_sound_default(int setting);
void mpeg_sound_channel_config(int configuration);
int mpeg_val2phys(int setting, int value);
int mpeg_phys2val(int setting, int value);
char *mpeg_sound_unit(int setting);
int mpeg_sound_numdecimals(int setting);
struct mp3entry* mpeg_current_track(void);
bool mpeg_has_changed_track(void);
int mpeg_status(void);
#if defined(HAVE_MAS3587F) || defined(SIMULATOR)
void mpeg_set_pitch(int percent);
void mpeg_init_recording(void);
void mpeg_init_playback(void);
void mpeg_record(char *filename);
void mpeg_set_recording_options(int frequency, int quality,
                                int source, int channel_mode);
void mpeg_set_recording_gain(int left, int right, int mic);
#endif
void mpeg_get_debugdata(struct mpeg_debug *dbgdata);

#define SOUND_VOLUME 0
#define SOUND_BASS 1
#define SOUND_TREBLE 2
#define SOUND_BALANCE 3
#define SOUND_LOUDNESS 4
#define SOUND_SUPERBASS 5
#define SOUND_AVC 6
#define SOUND_CHANNELS 7
#define SOUND_LEFT_GAIN 8
#define SOUND_RIGHT_GAIN 9
#define SOUND_MIC_GAIN 10
#define SOUND_NUMSETTINGS 11

#define MPEG_SOUND_STEREO 0
#define MPEG_SOUND_MONO 1
#define MPEG_SOUND_MONO_LEFT 2
#define MPEG_SOUND_MONO_RIGHT 3

#define MPEG_STATUS_PLAY 1
#define MPEG_STATUS_PAUSE 2
#define MPEG_STATUS_RECORD 4

#endif
